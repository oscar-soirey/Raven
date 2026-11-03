/**
 * Raven - widgets/viewport.c
 * Viewport: an area of the layout showing content rendered by the
 * application (see raven_viewport.h for the three modes), and Custom: a widget
 * measured and painted by application callbacks.
 */

#include "common.h"
#include "../render/renderer.h"

/* ===========================================================================
 *  Viewport
 * ========================================================================= */

typedef struct rvn_viewport_t {
	rvn_widget_t base;
	/* mode 1: texture */
	Rid texture;
	RStretch stretch;
	int flip_y;
	/* mode 2: draw callback */
	RViewportDrawCallback draw_callback;
	void* draw_user;
	/* mode 3: native child window */
	rvn_pwindow_t* native;
	/** Frame at which the viewport was last painted, and where (pixels). */
	uint64_t native_frame;
	int native_rect[4];
	int native_shown;

	int realtime;
	int pixel_width, pixel_height;
	RViewportResizeCallback resize_callback;
	void* resize_user;
} rvn_viewport_t;

static void rvn_viewport_init(rvn_widget_t* widget) {
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	viewport->texture = R_INVALID_ID;
	viewport->stretch = R_STRETCH_FILL;
}

/* Removes the viewport from the native list of a window and destroys its child window. */
void rvn_viewport_native_release(rvn_widget_t* widget, rvn_window_t* window) {
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	uint32_t i;
	for (i = 0; window && i < window->native_viewport_count; i++) {
		if (window->native_viewports[i] != widget) continue;
		window->native_viewports[i] = window->native_viewports[--window->native_viewport_count];
		break;
	}
	if (viewport->native) {
		g_rvn.platform->window_destroy(viewport->native);
		viewport->native = NULL;
	}
	viewport->native_shown = R_FALSE;
}

/* Called on the UI thread after each paint: moves, shows or hides the child window. */
void rvn_viewport_native_sync(rvn_widget_t* widget, uint64_t frame) {
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	int visible = viewport->native_frame == frame && viewport->native_rect[2] > 0 && viewport->native_rect[3] > 0;
	if (!viewport->native) return;
	if (!visible && !viewport->native_shown) return;
	g_rvn.platform->child_set_rect(viewport->native, viewport->native_rect[0], viewport->native_rect[1],
		viewport->native_rect[2], viewport->native_rect[3], visible);
	viewport->native_shown = visible;
}

static void rvn_viewport_destroy(rvn_widget_t* widget) {
	/* the child window was released when the viewport left its window; this is a safety net */
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	if (viewport->native && g_rvn.platform) g_rvn.platform->window_destroy(viewport->native);
	viewport->native = NULL;
}

static rvn_vec2_t rvn_viewport_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	/* a viewport takes the size it is given; this is only a visible minimum */
	return rvn_vec2(64.0f, 64.0f);
}

static void rvn_viewport_arrange(rvn_widget_t* widget) {
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	float scale = widget->window ? widget->window->scale : 1.0f;
	int width = (int)floorf(widget->rect.w * scale + 0.5f), height = (int)floorf(widget->rect.h * scale + 0.5f);
	if (width == viewport->pixel_width && height == viewport->pixel_height) return;
	viewport->pixel_width = width;
	viewport->pixel_height = height;
	if (viewport->resize_callback && width > 0 && height > 0) {
		rvn_dispatch_begin();
		viewport->resize_callback(widget->id, width, height, viewport->resize_user);
		rvn_dispatch_end();
	}
}

static void rvn_viewport_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_box_t box = rvn_paint_snap(painter, rect);
	rvn_box_t visible = rvn_box_intersect(box, painter->clip);
	rvn_texture_t* texture = rvn_texture_get(viewport->texture);

	if (viewport->realtime && painter->window) rvn_atomic_add(&painter->window->realtime_viewports, 1);

	if (viewport->native) {
		/* the child window covers this area: nothing of ours would be seen */
		viewport->native_frame = painter->window ? painter->window->frame_index : 0;
		viewport->native_rect[0] = (int)box.x0;
		viewport->native_rect[1] = (int)box.y0;
		viewport->native_rect[2] = (int)(box.x1 - box.x0);
		viewport->native_rect[3] = (int)(box.y1 - box.y0);
		return;
	}

	if (texture && texture->created) {
		static const float full[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		rvn_rect_t target;
		float uv[4];
		rvn_image_fit(viewport->stretch, rvn_vec2((float)texture->width, (float)texture->height),
			rect.w, rect.h, full, &target, uv);
		if (viewport->flip_y) {
			float top = uv[1];
			uv[1] = uv[3];
			uv[3] = top;
		}
		if (viewport->stretch != R_STRETCH_FILL && viewport->stretch != R_STRETCH_COVER) {
			rvn_paint_rect(painter, rect, rvn_rgba8(0, 0, 0, 255));
		}
		rvn_paint_push_clip(painter, rect);
		rvn_paint_image(painter, target, viewport->texture, uv, 0xFFFFFFFFu, 0.0f);
		rvn_paint_pop_clip(painter);
	} else {
		rvn_paint_rect(painter, rect, rvn_rgba8(0, 0, 0, 255));
	}

	if (viewport->draw_callback && !rvn_box_empty(visible)) {
		RDrawCommand command;
		memset(&command, 0, sizeof(command));
		command.callback = viewport->draw_callback;
		command.user_data = viewport->draw_user;
		command.viewport = widget->id;
		command.texture = R_INVALID_ID;
		command.rect[0] = (int)box.x0;
		command.rect[1] = (int)box.y0;
		command.rect[2] = (int)(box.x1 - box.x0);
		command.rect[3] = (int)(box.y1 - box.y0);
		command.clip[0] = (int)visible.x0;
		command.clip[1] = (int)visible.y0;
		command.clip[2] = (int)(visible.x1 - visible.x0);
		command.clip[3] = (int)(visible.y1 - visible.y0);
		rvn_drawlist_add_callback(painter->list, &command);
	}
}

static void rvn_viewport_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, 0.0f);
}

static int rvn_viewport_event(rvn_widget_t* widget, const REvent* event) {
	/* The application receives the events through Rwidget_set_event_callback
	 * before this function. Here the viewport only keeps a drag alive when
	 * the cursor leaves it, and stops clicks from reaching what is behind. */
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			rvn_widget_capture_mouse(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_viewport_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_viewport_t* viewport = (rvn_viewport_t*)widget;
	if (RVN_ATTR("Stretch")) {
		if (!rvn_parse_stretch(value, &viewport->stretch)) return R_FALSE;
	} else if (RVN_ATTR("FlipY")) {
		if (!rvn_parse_bool(value, &viewport->flip_y)) return R_FALSE;
	} else if (RVN_ATTR("Realtime")) {
		if (!rvn_parse_bool(value, &viewport->realtime)) return R_FALSE;
	} else if (RVN_ATTR("Source")) {
		viewport->texture = rvn_xml_texture(value);
		if (viewport->texture == R_INVALID_ID) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_viewport = {
	.type = R_WIDGET_VIEWPORT,
	.name = "Viewport",
	.size = sizeof(rvn_viewport_t),
	/* painting updates the state of the native child window: UI thread only */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT | RVN_CF_SURFACE | RVN_CF_NO_MIRROR,
	.init = rvn_viewport_init,
	.destroy = rvn_viewport_destroy,
	.measure = rvn_viewport_measure,
	.arrange = rvn_viewport_arrange,
	.paint = rvn_viewport_paint,
	.paint_overlay = rvn_viewport_paint_overlay,
	.event = rvn_viewport_event,
	.set_attribute = rvn_viewport_set_attribute
};

void Rviewport_set_texture(Rid id, Rid texture) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIEWPORT);
	if (texture != R_INVALID_ID && !rvn_texture_get(texture)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rviewport_set_texture: invalid texture");
		return;
	}
	((rvn_viewport_t*)widget)->texture = texture;
	rvn_widget_invalidate_paint(widget);
}

void Rviewport_set_stretch(Rid id, RStretch stretch) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIEWPORT);
	if (stretch < R_STRETCH_FILL || stretch > R_STRETCH_NONE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rviewport_set_stretch: invalid stretch mode");
		return;
	}
	((rvn_viewport_t*)widget)->stretch = stretch;
	rvn_widget_invalidate_paint(widget);
}

void Rviewport_set_flip_y(Rid id, int flip) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIEWPORT);
	((rvn_viewport_t*)widget)->flip_y = flip != 0;
	rvn_widget_invalidate_paint(widget);
}

void Rviewport_set_draw_callback(Rid id, RViewportDrawCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIEWPORT);
	((rvn_viewport_t*)widget)->draw_callback = callback;
	((rvn_viewport_t*)widget)->draw_user = user_data;
	rvn_widget_invalidate_paint(widget);
}

int Rviewport_set_native(Rid id, int enabled) {
	rvn_viewport_t* viewport;
	rvn_window_t* window;
	rvn_vec2_t origin;
	RVN_WIDGET(widget, id, R_WIDGET_VIEWPORT, R_FALSE);
	viewport = (rvn_viewport_t*)widget;
	window = widget->window;

	if (!enabled) {
		rvn_viewport_native_release(widget, window);
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	if (viewport->native) return R_TRUE;
	if (!window || !window->platform) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rviewport_set_native: the viewport is not in a window yet");
		return R_FALSE;
	}
	if (window->needs_layout) rvn_window_layout(window);
	origin = rvn_widget_origin(widget);
	viewport->native = g_rvn.platform->child_create(window->platform,
		(int)(origin.x * window->scale), (int)(origin.y * window->scale),
		rvn_maxi(1, (int)(widget->rect.w * window->scale)), rvn_maxi(1, (int)(widget->rect.h * window->scale)), window);
	if (!viewport->native) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rviewport_set_native: the window system cannot create a child window");
		return R_FALSE;
	}
	RVN_ARRAY_RESERVE(window->native_viewports, window->native_viewport_cap, window->native_viewport_count + 1);
	window->native_viewports[window->native_viewport_count++] = widget;
	viewport->native_shown = R_TRUE;
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

int Rviewport_get_native_handle(Rid id, RNativeWindow* native) {
	RVN_WIDGET(widget, id, R_WIDGET_VIEWPORT, R_FALSE);
	if (!native) return R_FALSE;
	memset(native, 0, sizeof(*native));
	if (!((rvn_viewport_t*)widget)->native) return R_FALSE;
	g_rvn.platform->window_get_native(((rvn_viewport_t*)widget)->native, native);
	return native->system != R_WINDOW_SYSTEM_NONE;
}

void Rviewport_get_pixel_size(Rid id, int* width, int* height) {
	rvn_widget_t* widget;
	if (width) *width = 0;
	if (height) *height = 0;
	widget = rvn_widget_check(id, R_WIDGET_VIEWPORT, __func__);
	if (!widget) return;
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	if (width) *width = ((rvn_viewport_t*)widget)->pixel_width;
	if (height) *height = ((rvn_viewport_t*)widget)->pixel_height;
}

void Rviewport_set_resize_callback(Rid id, RViewportResizeCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIEWPORT);
	((rvn_viewport_t*)widget)->resize_callback = callback;
	((rvn_viewport_t*)widget)->resize_user = user_data;
}

void Rviewport_set_realtime(Rid id, int realtime) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_VIEWPORT);
	((rvn_viewport_t*)widget)->realtime = realtime != 0;
	rvn_widget_invalidate_paint(widget);
}

/* ===========================================================================
 *  Custom
 * ========================================================================= */

typedef struct rvn_custom_t {
	rvn_widget_t base;
	RPaintCallback paint_callback;
	void* paint_user;
	RMeasureCallback measure_callback;
	void* measure_user;
} rvn_custom_t;

static rvn_vec2_t rvn_custom_measure(rvn_widget_t* widget) {
	rvn_custom_t* custom = (rvn_custom_t*)widget;
	float width = 0.0f, height = 0.0f;
	uint32_t i;
	if (custom->measure_callback) custom->measure_callback(widget->id, &width, &height, custom->measure_user);
	/* a custom widget may also hold children: it is at least as large as they need */
	for (i = 0; i < widget->child_count; i++) {
		rvn_vec2_t child = rvn_widget_measure_slot(widget->children[i]);
		width = rvn_maxf(width, child.x);
		height = rvn_maxf(height, child.y);
	}
	return rvn_vec2(rvn_maxf(width, 0.0f), rvn_maxf(height, 0.0f));
}

static void rvn_custom_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_custom_t* custom = (rvn_custom_t*)widget;
	if (custom->paint_callback) {
		custom->paint_callback(widget->id, painter, widget->rect.w, widget->rect.h, custom->paint_user);
	}
}

const rvn_widget_class_t rvn_class_custom = {
	.type = R_WIDGET_CUSTOM,
	.name = "Custom",
	.size = sizeof(rvn_custom_t),
	.flags = RVN_CF_UI_PAINT,
	.measure = rvn_custom_measure,
	.paint = rvn_custom_paint
};

void Rcustom_set_paint_callback(Rid id, RPaintCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CUSTOM);
	((rvn_custom_t*)widget)->paint_callback = callback;
	((rvn_custom_t*)widget)->paint_user = user_data;
	rvn_widget_invalidate_paint(widget);
}

void Rcustom_set_measure_callback(Rid id, RMeasureCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_CUSTOM);
	((rvn_custom_t*)widget)->measure_callback = callback;
	((rvn_custom_t*)widget)->measure_user = user_data;
	rvn_widget_invalidate_layout(widget);
}
