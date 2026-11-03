/**
 * Raven - platform/web/web_platform.c
 * A browser: the one window is the canvas of the page (Emscripten, HTML5 API).
 *
 * The canvas is sized by the page (CSS); the window follows it, in physical pixels, scaled by the device
 * pixel ratio. The page owns the loop: Rrun hands the tick of the application to the animation frames
 * (platform->run), and the events arrive as callbacks between two frames. There is no thread to wait for:
 * pump and wake do nothing.
 *
 *   pointer   mouse and touch events of the canvas (touch becomes RVN_PEVENT_TOUCH, which the core turns
 *             into taps, scrolling and so on: widget/touch.c)
 *   keyboard  keys of the page (code), characters (key); Ctrl+V waits for the paste event of the browser,
 *             which is the only place a page may read the clipboard from
 *   clipboard the text of the last paste or copy (the browser does not allow a synchronous read)
 *
 * Not yet: the soft keyboard of a phone (needs a hidden text field), IME composition, drag and drop of files,
 * cursors beyond the CSS ones, accessibility.
 */

#include "../platform.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <math.h>

#define RVN_WEB_CANVAS "#canvas"

struct rvn_pwindow_t {
	void* user;
	int width, height;
	float scale;
	int focused;
};

static struct {
	rvn_pevent_handler_t handler;
	rvn_pwindow_t* window;
	uint32_t modifiers;
	int mouse_inside;
	char* clipboard;
	/** Ctrl+V pressed: the key goes to the core when the browser delivers the text. */
	int paste_pending;
	uint32_t paste_modifiers;
	/** The hidden text field has the focus: the browser delivers the typed text (and the input method's) to it. */
	int text_active;
	int (*tick)(void* user);
	void* tick_user;
} g_web;

/* ---------------------------------------------------------------------------
 *  JavaScript
 * ------------------------------------------------------------------------- */

EM_JS(void, rvn_web_js_cursor, (const char* css), {
	var canvas = document.querySelector('#canvas');
	if (canvas) canvas.style.cursor = UTF8ToString(css);
})

EM_JS(void, rvn_web_js_copy, (const char* text), {
	var value = UTF8ToString(text);
	if (navigator.clipboard && navigator.clipboard.writeText) navigator.clipboard.writeText(value).catch(function () {});
})

EM_JS(void, rvn_web_js_title, (const char* title), {
	document.title = UTF8ToString(title);
})

/* The page side that cannot be done with the HTML5 API: the paste event, no context menu, no touch panning of the page. */
EM_JS(void, rvn_web_js_install, (void), {
	var canvas = document.querySelector('#canvas');
	if (canvas) {
		canvas.addEventListener('contextmenu', function (e) { e.preventDefault(); });
		canvas.style.touchAction = 'none';
		canvas.style.outline = 'none';
	}
	document.addEventListener('paste', function (e) {
		var text = (e.clipboardData && e.clipboardData.getData('text/plain')) || "";
		var bytes = new TextEncoder().encode(text);
		var memory = _malloc(bytes.length + 1);
		HEAPU8.set(bytes, memory);
		HEAPU8[memory + bytes.length] = 0;
		_rvn_web_paste(memory);
		_free(memory);
		e.preventDefault();
	});
})

/*
 * The text field that takes the keyboard of a phone or tablet (and the input method of a desktop): hidden, but
 * focusable, placed over the widget so that the browser does not scroll the page to it. The text comes from
 * its beforeinput event (and the end of a composition); Raven keeps its own copy of the text.
 */
EM_JS(void, rvn_web_js_text_input, (int active, double x, double y, double width, double height), {
	var field = Module.ravenTextField;
	if (!field) {
		field = document.createElement('textarea');
		field.setAttribute('autocapitalize', 'off');
		field.setAttribute('autocomplete', 'off');
		field.setAttribute('autocorrect', 'off');
		field.setAttribute('spellcheck', 'false');
		field.setAttribute('aria-hidden', 'true');
		field.tabIndex = -1;
		field.style.cssText = 'position:fixed;opacity:0;pointer-events:none;border:0;padding:0;margin:0;outline:none;resize:none;overflow:hidden;font-size:16px;background:transparent;color:transparent;caret-color:transparent;z-index:-1';
		document.body.appendChild(field);
		var send = function (text) {
			if (!text) return;
			var bytes = new TextEncoder().encode(text);
			var memory = _malloc(bytes.length + 1);
			HEAPU8.set(bytes, memory);
			HEAPU8[memory + bytes.length] = 0;
			_rvn_web_text(memory);
			_free(memory);
		};
		var press = function (name) {
			var bytes = new TextEncoder().encode(name);
			var memory = _malloc(bytes.length + 1);
			HEAPU8.set(bytes, memory);
			HEAPU8[memory + bytes.length] = 0;
			_rvn_web_synth_key(memory);
			_free(memory);
		};
		field.addEventListener('beforeinput', function (e) {
			if (e.isComposing) return;
			if (e.inputType === 'insertText' || e.inputType === 'insertReplacementText') { send(e.data); e.preventDefault(); }
			else if (e.inputType === 'insertLineBreak' || e.inputType === 'insertParagraph') { press('Enter'); e.preventDefault(); }
			else if (e.inputType === 'deleteContentBackward') { press('Backspace'); e.preventDefault(); }
			else if (e.inputType === 'deleteContentForward') { press('Delete'); e.preventDefault(); }
		});
		field.addEventListener('compositionend', function (e) { send(e.data); field.value = ""; });
		field.addEventListener('input', function (e) { if (!e.isComposing) field.value = ""; });
		Module.ravenTextField = field;
	}
	if (active) {
		field.style.left = x + 'px';
		field.style.top = y + 'px';
		field.style.width = Math.max(width, 1) + 'px';
		field.style.height = Math.max(height, 1) + 'px';
		if (document.activeElement !== field) field.focus({ preventScroll: true });
	} else if (document.activeElement === field) {
		field.blur();
	}
})

/* A touch screen (the main pointer is a finger): a phone gets larger controls than a tablet, a mouse none. */
EM_JS(double, rvn_web_js_touch_scale, (void), {
	if (!window.matchMedia || !window.matchMedia('(pointer: coarse)').matches) return 1.0;
	return Math.min(screen.width, screen.height) < 700 ? 1.35 : 1.2;
})

EM_JS(double, rvn_web_js_density, (void), {
	return window.devicePixelRatio || 1.0;
})

/* ---------------------------------------------------------------------------
 *  Size
 * ------------------------------------------------------------------------- */

static void rvn_web_send(const rvn_pevent_t* event) {
	if (g_web.handler) g_web.handler(event);
}

static void rvn_web_pevent(rvn_pevent_t* event, rvn_pevent_type_t type) {
	memset(event, 0, sizeof(*event));
	event->type = type;
	event->user = g_web.window ? g_web.window->user : NULL;
	event->modifiers = g_web.modifiers;
}

/* The size of the canvas in physical pixels, from its CSS size: R_TRUE when it changed. */
static int rvn_web_measure(int* width, int* height, float* scale) {
	double css_width = 0.0, css_height = 0.0;
	double density = rvn_web_js_density();
	emscripten_get_element_css_size(RVN_WEB_CANVAS, &css_width, &css_height);
	if (density < 1.0) density = 1.0;
	*scale = (float)density;
	*width = (int)lround(css_width * density);
	*height = (int)lround(css_height * density);
	return *width > 0 && *height > 0;
}

/* Checked once per frame (the browser tells nothing when the page is resized or zoomed): the drawing size of the canvas follows. */
static void rvn_web_sync_size(void) {
	rvn_pwindow_t* window = g_web.window;
	int width, height;
	float scale;
	if (!window || !rvn_web_measure(&width, &height, &scale)) return;
	if (width == window->width && height == window->height && fabsf(scale - window->scale) < 0.001f) return;
	window->width = width;
	window->height = height;
	window->scale = scale;
	emscripten_set_canvas_element_size(RVN_WEB_CANVAS, width, height);
	{
		rvn_pevent_t event;
		rvn_web_pevent(&event, RVN_PEVENT_RESIZE);
		event.width = width;
		event.height = height;
		event.scale = scale;
		rvn_web_send(&event);
	}
}

/* ---------------------------------------------------------------------------
 *  Pointer
 * ------------------------------------------------------------------------- */

static uint32_t rvn_web_modifiers(const EmscriptenMouseEvent* event) {
	return (event->shiftKey ? R_MOD_SHIFT : 0) | (event->ctrlKey ? R_MOD_CTRL : 0) |
		(event->altKey ? R_MOD_ALT : 0) | (event->metaKey ? R_MOD_SUPER : 0);
}

static void rvn_web_pointer_position(const EmscriptenMouseEvent* mouse, rvn_pevent_t* event) {
	float scale = g_web.window ? g_web.window->scale : 1.0f;
	/* (target is the canvas for the mouse events registered on it; clientX/Y for those of the page) */
	event->x = (int)lroundf((float)mouse->targetX * scale);
	event->y = (int)lroundf((float)mouse->targetY * scale);
}

static EM_BOOL rvn_web_on_mouse(int type, const EmscriptenMouseEvent* mouse, void* user) {
	rvn_pevent_t event;
	RVN_UNUSED(user);
	if (!g_web.window) return EM_FALSE;
	g_web.modifiers = rvn_web_modifiers(mouse);
	switch (type) {
		case EMSCRIPTEN_EVENT_MOUSEMOVE:
			rvn_web_pevent(&event, RVN_PEVENT_MOUSE_MOVE);
			rvn_web_pointer_position(mouse, &event);
			g_web.mouse_inside = R_TRUE;
			break;
		case EMSCRIPTEN_EVENT_MOUSEDOWN:
		case EMSCRIPTEN_EVENT_MOUSEUP:
			rvn_web_pevent(&event, RVN_PEVENT_MOUSE_BUTTON);
			rvn_web_pointer_position(mouse, &event);
			event.pressed = type == EMSCRIPTEN_EVENT_MOUSEDOWN;
			/* DOM: 0 left, 1 middle, 2 right, 3 back, 4 forward */
			event.button = mouse->button == 0 ? R_MOUSE_LEFT : mouse->button == 1 ? R_MOUSE_MIDDLE :
				mouse->button == 2 ? R_MOUSE_RIGHT : mouse->button == 3 ? R_MOUSE_X1 : R_MOUSE_X2;
			break;
		case EMSCRIPTEN_EVENT_MOUSELEAVE:
			rvn_web_pevent(&event, RVN_PEVENT_MOUSE_LEAVE);
			g_web.mouse_inside = R_FALSE;
			break;
		default:
			return EM_FALSE;
	}
	rvn_web_send(&event);
	return EM_TRUE;
}

static EM_BOOL rvn_web_on_wheel(int type, const EmscriptenWheelEvent* wheel, void* user) {
	rvn_pevent_t event;
	double unit;
	RVN_UNUSED(type);
	RVN_UNUSED(user);
	if (!g_web.window) return EM_FALSE;
	g_web.modifiers = rvn_web_modifiers(&wheel->mouse);
	rvn_web_pevent(&event, RVN_PEVENT_MOUSE_WHEEL);
	/* pixels (a notch is about 100), lines or pages; Raven counts notches, up and left positive */
	unit = wheel->deltaMode == DOM_DELTA_PIXEL ? 100.0 : wheel->deltaMode == DOM_DELTA_LINE ? 3.0 : 0.1;
	event.wheel_x = (float)(-wheel->deltaX / unit);
	event.wheel_y = (float)(-wheel->deltaY / unit);
	rvn_web_send(&event);
	return EM_TRUE;
}

static EM_BOOL rvn_web_on_touch(int type, const EmscriptenTouchEvent* touch, void* user) {
	rvn_pevent_t event;
	float scale = g_web.window ? g_web.window->scale : 1.0f;
	int i;
	RVN_UNUSED(user);
	if (!g_web.window) return EM_FALSE;
	g_web.modifiers = (touch->shiftKey ? R_MOD_SHIFT : 0) | (touch->ctrlKey ? R_MOD_CTRL : 0) |
		(touch->altKey ? R_MOD_ALT : 0) | (touch->metaKey ? R_MOD_SUPER : 0);
	for (i = 0; i < touch->numTouches; i++) {
		const EmscriptenTouchPoint* point = &touch->touches[i];
		if (!point->isChanged) continue;
		rvn_web_pevent(&event, RVN_PEVENT_TOUCH);
		event.touch_id = (int)point->identifier;
		event.fx = (float)point->targetX * scale;
		event.fy = (float)point->targetY * scale;
		event.pressure = 1.0f;
		switch (type) {
			case EMSCRIPTEN_EVENT_TOUCHSTART: event.phase = R_TOUCH_DOWN; break;
			case EMSCRIPTEN_EVENT_TOUCHMOVE: event.phase = R_TOUCH_MOVE; break;
			case EMSCRIPTEN_EVENT_TOUCHEND: event.phase = R_TOUCH_UP; break;
			default: event.phase = R_TOUCH_CANCEL; break;
		}
		rvn_web_send(&event);
	}
	return EM_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Keyboard
 * ------------------------------------------------------------------------- */

typedef struct rvn_web_key_t {
	const char* code;
	RKey key;
} rvn_web_key_t;

static const rvn_web_key_t g_web_keys[] = {
	{ "Space", R_KEY_SPACE }, { "Quote", R_KEY_APOSTROPHE }, { "Comma", R_KEY_COMMA }, { "Minus", R_KEY_MINUS },
	{ "Period", R_KEY_PERIOD }, { "Slash", R_KEY_SLASH }, { "Semicolon", R_KEY_SEMICOLON }, { "Equal", R_KEY_EQUAL },
	{ "BracketLeft", R_KEY_LEFT_BRACKET }, { "Backslash", R_KEY_BACKSLASH }, { "BracketRight", R_KEY_RIGHT_BRACKET },
	{ "Backquote", R_KEY_GRAVE }, { "Escape", R_KEY_ESCAPE }, { "Enter", R_KEY_ENTER }, { "Tab", R_KEY_TAB },
	{ "Backspace", R_KEY_BACKSPACE }, { "Insert", R_KEY_INSERT }, { "Delete", R_KEY_DELETE },
	{ "ArrowRight", R_KEY_RIGHT }, { "ArrowLeft", R_KEY_LEFT }, { "ArrowDown", R_KEY_DOWN }, { "ArrowUp", R_KEY_UP },
	{ "PageUp", R_KEY_PAGE_UP }, { "PageDown", R_KEY_PAGE_DOWN }, { "Home", R_KEY_HOME }, { "End", R_KEY_END },
	{ "CapsLock", R_KEY_CAPS_LOCK }, { "PrintScreen", R_KEY_PRINT_SCREEN }, { "Pause", R_KEY_PAUSE },
	{ "NumpadDecimal", R_KEY_KP_DECIMAL }, { "NumpadDivide", R_KEY_KP_DIVIDE }, { "NumpadMultiply", R_KEY_KP_MULTIPLY },
	{ "NumpadSubtract", R_KEY_KP_SUBTRACT }, { "NumpadAdd", R_KEY_KP_ADD }, { "NumpadEnter", R_KEY_KP_ENTER },
	{ "ShiftLeft", R_KEY_LEFT_SHIFT }, { "ControlLeft", R_KEY_LEFT_CTRL }, { "AltLeft", R_KEY_LEFT_ALT },
	{ "MetaLeft", R_KEY_LEFT_SUPER }, { "ShiftRight", R_KEY_RIGHT_SHIFT }, { "ControlRight", R_KEY_RIGHT_CTRL },
	{ "AltRight", R_KEY_RIGHT_ALT }, { "MetaRight", R_KEY_RIGHT_SUPER }, { "ContextMenu", R_KEY_MENU }
};

static RKey rvn_web_key(const char* code) {
	size_t i;
	int number;
	if (strncmp(code, "Key", 3) == 0 && code[3] >= 'A' && code[3] <= 'Z' && !code[4]) return (RKey)(R_KEY_A + (code[3] - 'A'));
	if (strncmp(code, "Digit", 5) == 0 && code[5] >= '0' && code[5] <= '9' && !code[6]) return (RKey)(R_KEY_0 + (code[5] - '0'));
	if (strncmp(code, "Numpad", 6) == 0 && code[6] >= '0' && code[6] <= '9' && !code[7]) return (RKey)(R_KEY_KP_0 + (code[6] - '0'));
	if (code[0] == 'F' && (number = atoi(code + 1)) >= 1 && number <= 12) return (RKey)(R_KEY_F1 + (number - 1));
	for (i = 0; i < RVN_COUNT_OF(g_web_keys); i++) {
		if (strcmp(g_web_keys[i].code, code) == 0) return g_web_keys[i].key;
	}
	return R_KEY_UNKNOWN;
}

/* The key of a key event is a single character when it types one ("a", "é", "€"; "Enter", "Shift" are names): its code point, else 0. */
static uint32_t rvn_web_character(const char* key) {
	const unsigned char* s = (const unsigned char*)key;
	uint32_t cp;
	int length;
	if (!s[0]) return 0;
	if (s[0] < 0x80) { cp = s[0]; length = 1; }
	else if ((s[0] & 0xE0) == 0xC0) { cp = s[0] & 0x1F; length = 2; }
	else if ((s[0] & 0xF0) == 0xE0) { cp = s[0] & 0x0F; length = 3; }
	else if ((s[0] & 0xF8) == 0xF0) { cp = s[0] & 0x07; length = 4; }
	else return 0;
	{
		int i;
		for (i = 1; i < length; i++) {
			if ((s[i] & 0xC0) != 0x80) return 0;
			cp = (cp << 6) | (s[i] & 0x3F);
		}
	}
	/* more than one character: a name */
	if (s[length]) return 0;
	return cp >= 0x20 && cp != 0x7F ? cp : 0;
}

/* Keys the browser keeps: reload, the developer tools, full screen, zoom, closing and opening tabs. */
static int rvn_web_browser_key(const EmscriptenKeyboardEvent* event) {
	int command = event->ctrlKey || event->metaKey;
	if (strcmp(event->code, "F5") == 0 || strcmp(event->code, "F11") == 0 || strcmp(event->code, "F12") == 0) return R_TRUE;
	if (command && strncmp(event->code, "Key", 3) == 0 && event->code[3] && strchr("RLTWNQ", event->code[3])) return R_TRUE;
	if (command && (strcmp(event->code, "Minus") == 0 || strcmp(event->code, "Equal") == 0 || strcmp(event->code, "Digit0") == 0)) return R_TRUE;
	return R_FALSE;
}

static EM_BOOL rvn_web_on_key(int type, const EmscriptenKeyboardEvent* key, void* user) {
	rvn_pevent_t event;
	int pressed = type == EMSCRIPTEN_EVENT_KEYDOWN;
	uint32_t modifiers = (key->shiftKey ? R_MOD_SHIFT : 0) | (key->ctrlKey ? R_MOD_CTRL : 0) |
		(key->altKey ? R_MOD_ALT : 0) | (key->metaKey ? R_MOD_SUPER : 0);
	RVN_UNUSED(user);
	if (!g_web.window) return EM_FALSE;
	g_web.modifiers = modifiers;
	if (type != EMSCRIPTEN_EVENT_KEYDOWN && type != EMSCRIPTEN_EVENT_KEYUP) return EM_FALSE;
	if (rvn_web_browser_key(key)) return EM_FALSE;
	/* paste: the page may read the clipboard only in the paste event, which follows this key; the key waits for it */
	if ((key->ctrlKey || key->metaKey) && strcmp(key->code, "KeyV") == 0) {
		if (pressed) {
			g_web.paste_pending = R_TRUE;
			g_web.paste_modifiers = modifiers;
		}
		return EM_FALSE;
	}
	/* an input method is working (code 229): the text comes from the field when it is done */
	if (key->keyCode == 229 || strcmp(key->key, "Process") == 0 || strcmp(key->key, "Dead") == 0) return EM_FALSE;
	rvn_web_pevent(&event, RVN_PEVENT_KEY);
	event.key = rvn_web_key(key->code);
	event.pressed = pressed;
	event.repeat = key->repeat ? 1 : 0;
	rvn_web_send(&event);
	if (pressed && !key->ctrlKey && !key->altKey && !key->metaKey) {
		uint32_t codepoint = rvn_web_character(key->key);
		if (codepoint) {
			/* while the hidden field has the focus the character comes from it (accents, input methods): the key is left alone */
			if (g_web.text_active) return EM_FALSE;
			rvn_web_pevent(&event, RVN_PEVENT_TEXT);
			event.codepoint = codepoint;
			rvn_web_send(&event);
		}
	}
	return EM_TRUE;
}

/* The text of a paste (rvn_web_js_install): what the clipboard returns from now on, and the Ctrl+V that was waiting. */
EMSCRIPTEN_KEEPALIVE void rvn_web_paste(const char* text) {
	rvn_pevent_t event;
	rvn_str_set(&g_web.clipboard, text);
	if (!g_web.paste_pending || !g_web.window) return;
	g_web.paste_pending = R_FALSE;
	g_web.modifiers = g_web.paste_modifiers;
	rvn_web_pevent(&event, RVN_PEVENT_KEY);
	event.key = R_KEY_V;
	event.pressed = R_TRUE;
	rvn_web_send(&event);
	event.pressed = R_FALSE;
	rvn_web_send(&event);
}

/* Text typed into the hidden field: a string of characters (what a soft keyboard or an input method commits). */
EMSCRIPTEN_KEEPALIVE void rvn_web_text(const char* text) {
	rvn_pevent_t event;
	const unsigned char* s = (const unsigned char*)text;
	if (!g_web.window) return;
	while (*s) {
		uint32_t cp;
		int length, i;
		if (s[0] < 0x80) { cp = s[0]; length = 1; }
		else if ((s[0] & 0xE0) == 0xC0) { cp = s[0] & 0x1F; length = 2; }
		else if ((s[0] & 0xF0) == 0xE0) { cp = s[0] & 0x0F; length = 3; }
		else if ((s[0] & 0xF8) == 0xF0) { cp = s[0] & 0x07; length = 4; }
		else { s++; continue; }
		for (i = 1; i < length && (s[i] & 0xC0) == 0x80; i++) cp = (cp << 6) | (s[i] & 0x3F);
		s += i;
		if (cp < 0x20 || cp == 0x7F) continue;
		rvn_web_pevent(&event, RVN_PEVENT_TEXT);
		event.codepoint = cp;
		rvn_web_send(&event);
	}
}

/* A key the field cannot type (Backspace and Enter of a soft keyboard): pressed and released by name (a code of key events). */
EMSCRIPTEN_KEEPALIVE void rvn_web_synth_key(const char* code) {
	rvn_pevent_t event;
	if (!g_web.window) return;
	rvn_web_pevent(&event, RVN_PEVENT_KEY);
	event.key = rvn_web_key(code);
	event.pressed = R_TRUE;
	rvn_web_send(&event);
	event.pressed = R_FALSE;
	rvn_web_send(&event);
}

static EM_BOOL rvn_web_on_focus(int type, const EmscriptenFocusEvent* focus, void* user) {
	rvn_pevent_t event;
	RVN_UNUSED(focus);
	RVN_UNUSED(user);
	if (!g_web.window) return EM_FALSE;
	g_web.window->focused = type == EMSCRIPTEN_EVENT_FOCUS;
	rvn_web_pevent(&event, RVN_PEVENT_FOCUS);
	event.focused = g_web.window->focused;
	rvn_web_send(&event);
	return EM_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Platform
 * ------------------------------------------------------------------------- */

static float rvn_web_default_ui_scale(void) {
	return (float)rvn_web_js_touch_scale();
}

static int rvn_web_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	RVN_UNUSED(regions);
	memset(&g_web, 0, sizeof(g_web));
	g_web.handler = handler;
	rvn_web_js_install();
	emscripten_set_mousedown_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_mouse);
	emscripten_set_mouseleave_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_mouse);
	/* the movement and the release go on while the button is down outside the canvas */
	emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, rvn_web_on_mouse);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, rvn_web_on_mouse);
	emscripten_set_wheel_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_wheel);
	emscripten_set_touchstart_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_touch);
	emscripten_set_touchmove_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_touch);
	emscripten_set_touchend_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_touch);
	emscripten_set_touchcancel_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, rvn_web_on_touch);
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, rvn_web_on_key);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, rvn_web_on_key);
	emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, rvn_web_on_focus);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, rvn_web_on_focus);
	return R_TRUE;
}

static void rvn_web_shutdown(void) {
	emscripten_set_mousedown_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_mouseleave_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_wheel_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_touchstart_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_touchmove_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_touchend_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_touchcancel_callback(RVN_WEB_CANVAS, NULL, EM_TRUE, NULL);
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, NULL);
	rvn_free(g_web.clipboard);
	memset(&g_web, 0, sizeof(g_web));
}

static void* rvn_web_native_display(void) {
	return (void*)RVN_WEB_CANVAS;
}

static rvn_pwindow_t* rvn_web_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window;
	int width, height;
	float scale;
	if (g_web.window) return NULL;
	window = (rvn_pwindow_t*)rvn_calloc(sizeof(*window));
	window->user = desc->user;
	window->focused = R_TRUE;
	if (rvn_web_measure(&width, &height, &scale)) {
		window->width = width;
		window->height = height;
		window->scale = scale;
	} else {
		/* the page gave the canvas no size (hidden, or not laid out yet): the size asked for */
		window->width = desc->width;
		window->height = desc->height;
		window->scale = 1.0f;
	}
	emscripten_set_canvas_element_size(RVN_WEB_CANVAS, window->width, window->height);
	if (desc->title) rvn_web_js_title(desc->title);
	g_web.window = window;
	return window;
}

static void rvn_web_window_destroy(rvn_pwindow_t* window) {
	if (g_web.window == window) g_web.window = NULL;
	rvn_free(window);
}

static void rvn_web_window_set_title(rvn_pwindow_t* window, const char* title) {
	RVN_UNUSED(window);
	rvn_web_js_title(title ? title : "");
}

static void rvn_web_window_set_size(rvn_pwindow_t* window, int width, int height) {
	/* the page decides the size of the canvas */
	RVN_UNUSED(window);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_web_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_web_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	RVN_UNUSED(window);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
}

static void rvn_web_window_set_position(rvn_pwindow_t* window, int x, int y) {
	RVN_UNUSED(window);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
}

static void rvn_web_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	RVN_UNUSED(window);
	*x = 0;
	*y = 0;
}

static float rvn_web_window_get_scale(rvn_pwindow_t* window) {
	return window->scale;
}

static void rvn_web_window_show(rvn_pwindow_t* window, int visible) {
	RVN_UNUSED(window);
	RVN_UNUSED(visible);
}

static void rvn_web_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	RVN_UNUSED(window);
	RVN_UNUSED(state);
}

static rvn_pwindow_state_t rvn_web_window_get_state(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
	return RVN_PWINDOW_NORMAL;
}

static void rvn_web_window_begin_move(rvn_pwindow_t* window) {
	RVN_UNUSED(window);
}

static void rvn_web_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	RVN_UNUSED(window);
	RVN_UNUSED(edge);
}

static void rvn_web_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	const char* css;
	RVN_UNUSED(window);
	switch (cursor) {
		case R_CURSOR_IBEAM: css = "text"; break;
		case R_CURSOR_HAND: css = "pointer"; break;
		case R_CURSOR_CROSSHAIR: css = "crosshair"; break;
		case R_CURSOR_RESIZE_H: css = "ew-resize"; break;
		case R_CURSOR_RESIZE_V: css = "ns-resize"; break;
		case R_CURSOR_RESIZE_NWSE: css = "nwse-resize"; break;
		case R_CURSOR_RESIZE_NESW: css = "nesw-resize"; break;
		case R_CURSOR_MOVE: css = "move"; break;
		case R_CURSOR_NOT_ALLOWED: css = "not-allowed"; break;
		case R_CURSOR_NONE: css = "none"; break;
		default: css = "default"; break;
	}
	rvn_web_js_cursor(css);
}

static void rvn_web_window_text_input(rvn_pwindow_t* window, int active, int x, int y, int width, int height) {
	float scale = window->scale > 0.0f ? window->scale : 1.0f;
	g_web.text_active = active;
	/* (css pixels: the window is in physical ones) */
	rvn_web_js_text_input(active, x / scale, y / scale, width / scale, height / scale);
}

static void rvn_web_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	RVN_UNUSED(window);
	memset(native, 0, sizeof(*native));
	native->system = R_WINDOW_SYSTEM_WEB;
	native->display = (void*)RVN_WEB_CANVAS;
	native->window = 1;
}

static rvn_pwindow_t* rvn_web_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	RVN_UNUSED(parent);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(user);
	return NULL;
}

static void rvn_web_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	RVN_UNUSED(child);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(visible);
}

static void rvn_web_frame(void) {
	rvn_web_sync_size();
	if (!g_web.tick(g_web.tick_user)) emscripten_cancel_main_loop();
}

static int rvn_web_run(int (*tick)(void* user), void* user) {
	g_web.tick = tick;
	g_web.tick_user = user;
	emscripten_set_main_loop(rvn_web_frame, 0, 0);
	return R_TRUE;
}

static void rvn_web_pump(double timeout_seconds) {
	/* the events are callbacks of the browser: there is nothing to wait for or to fetch */
	RVN_UNUSED(timeout_seconds);
}

static void rvn_web_wake(void) {
}

static void rvn_web_clipboard_set(const char* text) {
	rvn_str_set(&g_web.clipboard, text);
	rvn_web_js_copy(text ? text : "");
}

static char* rvn_web_clipboard_get(void) {
	return rvn_strdup(g_web.clipboard);
}

const rvn_platform_t* rvn_platform_web(void) {
	static const rvn_platform_t platform = {
		.name = "web",
		.system = R_WINDOW_SYSTEM_WEB,
		.init = rvn_web_init,
		.shutdown = rvn_web_shutdown,
		.native_display = rvn_web_native_display,
		.window_create = rvn_web_window_create,
		.window_destroy = rvn_web_window_destroy,
		.window_set_title = rvn_web_window_set_title,
		.window_set_size = rvn_web_window_set_size,
		.window_get_size = rvn_web_window_get_size,
		.window_set_min_size = rvn_web_window_set_min_size,
		.window_set_position = rvn_web_window_set_position,
		.window_get_position = rvn_web_window_get_position,
		.window_get_scale = rvn_web_window_get_scale,
		.window_show = rvn_web_window_show,
		.window_set_state = rvn_web_window_set_state,
		.window_get_state = rvn_web_window_get_state,
		.window_begin_move = rvn_web_window_begin_move,
		.window_begin_resize = rvn_web_window_begin_resize,
		.window_set_cursor = rvn_web_window_set_cursor,
		.window_text_input = rvn_web_window_text_input,
		.window_get_native = rvn_web_window_get_native,
		.child_create = rvn_web_child_create,
		.child_set_rect = rvn_web_child_set_rect,
		.single_window = R_TRUE,
		.single_thread_render = R_TRUE,
		.default_ui_scale = rvn_web_default_ui_scale,
		.run = rvn_web_run,
		.pump = rvn_web_pump,
		.wake = rvn_web_wake,
		.clipboard_set = rvn_web_clipboard_set,
		.clipboard_get = rvn_web_clipboard_get
	};
	return &platform;
}
