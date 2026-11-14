/*
 * Raven - platform/android/java/raven/android/RavenTextView.java
 *
 * The InputConnection of Raven's focused text input on Android, for the
 * on-screen keyboard: keyboard type and action key per field, suggestions,
 * autocorrection, composition, glide typing, any character.
 *
 * NativeActivity has no editor: this invisible 1x1 view is added to its
 * window, keeps the focus of the view hierarchy (touch and keys go to the
 * native input queue, so nothing takes it away) and answers the IME.
 *
 * Threads: the IME calls the connection on the UI thread and asks
 * synchronous questions (text before the cursor...), while Raven lives on the
 * android_main thread. The view keeps a mirror of the field (text, selection,
 * composing region) in an Editable; BaseInputConnection edits it.
 *  - IME edits change the mirror at once; at the end of each batch edit the
 *    new text and selection (UTF-8 byte offsets) go to native (nativeOp),
 *    which applies them to Raven on its own thread with
 *    Rwindow_replace_text_input / Rwindow_set_text_input_selection.
 *  - When Raven's field changes by itself (a tap moves the caret, a hardware
 *    key, the app), native pushes the new state (setState); when the focus
 *    moves to another field, setField (restartInput, new EditorInfo).
 *  - Each op has a number; a state pushed by native carries the number of the
 *    last op it had applied, and a state older than what the view sent is
 *    dropped (native's next comparison sends it again if it still differs).
 * Native loads the class from the activity's class loader and registers
 * nativeOp; without classes.dex the platform keeps its key-event path.
 */

package raven.android;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.SpannableStringBuilder;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import java.nio.charset.StandardCharsets;

public final class RavenTextView extends View {
	private static final String TAG = "Raven";

	/* what has Raven's focus (setField) */
	static final int KIND_NONE = 0;   /* no text field: no editor */
	static final int KIND_KEYS = 1;   /* a SpinBox, a TextEditor: key events (TYPE_NULL) as before */
	static final int KIND_TEXT = 2;   /* a TextInput: the full connection */

	/* RTextInputType - R_TEXT_INPUT_TYPE_TEXT */
	static final int TYPE_TEXT = 0, TYPE_EMAIL = 1, TYPE_NUMBER = 2, TYPE_DECIMAL = 3, TYPE_PHONE = 4, TYPE_URL = 5, TYPE_SEARCH = 6;
	/* RReturnKey - R_RETURN_KEY_DEFAULT */
	static final int RETURN_DEFAULT = 0, RETURN_DONE = 1, RETURN_GO = 2, RETURN_NEXT = 3, RETURN_SEARCH = 4, RETURN_SEND = 5;
	/* setField flags */
	static final int FLAG_PASSWORD = 1;

	/* ops to native */
	static final int OP_EDIT = 1;    /* text (UTF-8), a = selection start, b = selection end (bytes) */
	static final int OP_ACTION = 2;  /* the action key / Enter: a = EditorInfo action */
	static final int OP_KEY = 3;     /* a key the mirror does not handle: a = key code, b = meta state */

	/** Registered by native (RegisterNatives). Thread-safe: queues the op for the android_main thread. */
	private static native void nativeOp(int op, int gen, int seq, byte[] text, int a, int b);

	private static final Handler MAIN = new Handler(Looper.getMainLooper());
	private static RavenTextView sView;

	private final InputMethodManager imm;
	/* the mirror of the focused field; composing spans are BaseInputConnection's */
	private final SpannableStringBuilder editable = new SpannableStringBuilder();
	/* the field: a new gen for every field native announces */
	private int gen, kind, type, returnKey, flags, fieldId;
	/* number of the last op sent to native */
	private int seq;
	/* the text and selection native has (sent to it or received from it) */
	private String synced = "";
	private int syncedStart, syncedEnd;
	/* batch edits of the IME in progress */
	private int batch;
	/* a state pushed by native during a batch edit: applied at its end if still current */
	private String pendingText;
	private int pendingStart, pendingEnd, pendingAck;
	/* getExtractedText with GET_EXTRACTED_TEXT_MONITOR: the IME follows the text */
	private ExtractedTextRequest extractRequest;
	private String extractedSent;
	/* the IME calls of the current batch, logged (tag RavenIme) when the app is debuggable */
	private static final String IME_TAG = "RavenIme";
	private final StringBuilder calls = new StringBuilder();
	private boolean logCalls;

	private void call(String what) {
		if (!logCalls) return;
		if (calls.length() > 0) calls.append(", ");
		calls.append(what);
	}

	private RavenTextView(Context context) {
		super(context);
		imm = (InputMethodManager)context.getSystemService(Context.INPUT_METHOD_SERVICE);
		logCalls = (context.getApplicationInfo().flags & android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE) != 0;
		setFocusable(true);
		setFocusableInTouchMode(true);
		Selection.setSelection(editable, 0);
	}

	/* ------------------------------------------------------------------ called by native (any thread) */

	/** Adds the view to the activity's window (on the UI thread). */
	public static void attach(final Activity activity) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) return;
				RavenTextView view = new RavenTextView(activity);
				activity.addContentView(view, new ViewGroup.LayoutParams(1, 1));
				boolean focused = view.requestFocus();
				sView = view;
				Log.i(TAG, "text view attached (focus " + focused + ")");
			}
		});
	}

	public static void detach() {
		MAIN.post(new Runnable() {
			@Override public void run() {
				RavenTextView view = sView;
				sView = null;
				if (view == null) return;
				ViewParent parent = view.getParent();
				if (parent instanceof ViewGroup) ((ViewGroup)parent).removeView(view);
			}
		});
	}

	/** The focused field changed (or its kind). */
	public static void setField(final int gen, final int fieldId, final int kind, final int type, final int returnKey, final int flags,
			final byte[] text, final int start, final int end, final int ack) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) sView.onField(gen, fieldId, kind, type, returnKey, flags, text, start, end, ack);
			}
		});
	}

	/** Raven's field changed by itself (not by the IME). */
	public static void setState(final int gen, final byte[] text, final int start, final int end, final int ack) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) sView.onState(gen, text, start, end, ack);
			}
		});
	}

	public static void showKeyboard(final boolean show) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) sView.onShowKeyboard(show);
			}
		});
	}

	/* ------------------------------------------------------------------ UI thread */

	private void onField(int gen, int fieldId, int kind, int type, int returnKey, int flags, byte[] text, int start, int end, int ack) {
		this.gen = gen;
		this.fieldId = fieldId;
		this.kind = kind;
		this.type = type;
		this.returnKey = returnKey;
		this.flags = flags;
		/* (a batch of the old field's connection may never end) */
		batch = 0;
		calls.setLength(0);
		pendingText = null;
		extractRequest = null;
		extractedSent = null;
		String value = utf8(text);
		setMirror(value, utf16Index(text, start), utf16Index(text, end));
		Log.i(TAG, "text field " + gen + ": kind " + kind + " type " + type + " return " + returnKey + " flags " + flags
			+ " (" + value.length() + " chars, ack " + ack + "/" + seq + ")");
		imm.restartInput(this);
	}

	private void onState(int gen, byte[] text, int start, int end, int ack) {
		if (gen != this.gen || kind != KIND_TEXT) return;
		/* the view has sent edits native had not seen: this state is older than the mirror */
		if (ack != seq) return;
		String value = utf8(text);
		int s = utf16Index(text, start), e = utf16Index(text, end);
		if (batch > 0) {
			pendingText = value;
			pendingStart = s;
			pendingEnd = e;
			pendingAck = ack;
			return;
		}
		applyState(value, s, e);
	}

	private void applyState(String value, int s, int e) {
		boolean changed = !value.equals(editable.toString());
		setMirror(value, s, e);
		reportSelection();
		if (changed) reportExtracted();
	}

	private void onShowKeyboard(boolean show) {
		if (show && !hasFocus()) requestFocus();
		if (Build.VERSION.SDK_INT >= 30) {
			WindowInsetsController controller = getWindowInsetsController();
			if (controller != null) {
				if (show) controller.show(WindowInsets.Type.ime());
				else controller.hide(WindowInsets.Type.ime());
				return;
			}
		}
		if (show) imm.showSoftInput(this, 0);
		else imm.hideSoftInputFromWindow(getWindowToken(), 0);
	}

	/* the mirror takes Raven's state: no composing region, nothing sent back */
	private void setMirror(String value, int start, int end) {
		BaseInputConnection.removeComposingSpans(editable);
		if (!value.contentEquals(editable)) editable.replace(0, editable.length(), value);
		start = clamp(start, editable.length());
		end = clamp(end, editable.length());
		Selection.setSelection(editable, start, end);
		synced = value;
		syncedStart = start;
		syncedEnd = end;
	}

	private int selectionStart() {
		int a = Selection.getSelectionStart(editable), b = Selection.getSelectionEnd(editable);
		if (a < 0) return editable.length();
		return Math.min(a, b < 0 ? a : b);
	}

	private int selectionEnd() {
		int a = Selection.getSelectionStart(editable), b = Selection.getSelectionEnd(editable);
		if (a < 0) return editable.length();
		return Math.max(a, b < 0 ? a : b);
	}

	/* end of an IME edit: what changed goes to native and to the IME */
	private void flush() {
		if (calls.length() > 0) {
			Log.v(IME_TAG, calls + " -> \"" + editable + "\" selection " + selectionStart() + "-" + selectionEnd()
				+ " composing " + BaseInputConnection.getComposingSpanStart(editable) + "-" + BaseInputConnection.getComposingSpanEnd(editable));
			calls.setLength(0);
		}
		sendEdit();
		reportSelection();
		reportExtracted();
		if (pendingText != null) {
			String value = pendingText;
			pendingText = null;
			if (pendingAck == seq) applyState(value, pendingStart, pendingEnd);
		}
	}

	private void sendEdit() {
		if (kind != KIND_TEXT) return;
		String value = editable.toString();
		int s = selectionStart(), e = selectionEnd();
		if (value.equals(synced) && s == syncedStart && e == syncedEnd) return;
		synced = value;
		syncedStart = s;
		syncedEnd = e;
		seq++;
		nativeOp(OP_EDIT, gen, seq, value.getBytes(StandardCharsets.UTF_8), utf8Length(value, s), utf8Length(value, e));
	}

	private void sendOp(int op, int a, int b) {
		/* (the edits before it go first) */
		sendEdit();
		seq++;
		nativeOp(op, gen, seq, null, a, b);
	}

	private void reportSelection() {
		if (kind != KIND_TEXT) return;
		imm.updateSelection(this, selectionStart(), selectionEnd(),
			BaseInputConnection.getComposingSpanStart(editable), BaseInputConnection.getComposingSpanEnd(editable));
	}

	private void reportExtracted() {
		if (extractRequest == null || kind != KIND_TEXT) return;
		String value = editable.toString();
		if (value.equals(extractedSent)) return;
		extractedSent = value;
		imm.updateExtractedText(this, extractRequest.token, extracted());
	}

	private ExtractedText extracted() {
		ExtractedText text = new ExtractedText();
		text.text = editable.toString();
		text.startOffset = 0;
		text.partialStartOffset = -1;
		text.partialEndOffset = -1;
		text.selectionStart = selectionStart();
		text.selectionEnd = selectionEnd();
		text.flags = ExtractedText.FLAG_SINGLE_LINE;
		return text;
	}

	@Override public boolean onCheckIsTextEditor() {
		return kind != KIND_NONE;
	}

	@Override public InputConnection onCreateInputConnection(EditorInfo info) {
		if (kind == KIND_NONE) return null;
		info.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN;
		info.fieldId = fieldId;
		if (kind == KIND_KEYS) {
			/* SpinBox, TextEditor: as without this view, committed text comes back as key events */
			info.inputType = InputType.TYPE_NULL;
			return new BaseInputConnection(this, false);
		}
		info.inputType = inputType();
		info.imeOptions |= imeAction();
		info.initialSelStart = selectionStart();
		info.initialSelEnd = selectionEnd();
		info.initialCapsMode = BaseInputConnection.getComposingSpanStart(editable) < 0
			? android.text.TextUtils.getCapsMode(editable, info.initialSelStart, info.inputType) : 0;
		if (Build.VERSION.SDK_INT >= 30) info.setInitialSurroundingText(editable);
		Log.i(TAG, "input connection for field " + gen + ": inputType 0x" + Integer.toHexString(info.inputType)
			+ " imeOptions 0x" + Integer.toHexString(info.imeOptions));
		return new Connection(gen);
	}

	private int inputType() {
		boolean password = (flags & FLAG_PASSWORD) != 0;
		switch (type) {
		case TYPE_NUMBER:
			return InputType.TYPE_CLASS_NUMBER | (password ? InputType.TYPE_NUMBER_VARIATION_PASSWORD : 0);
		case TYPE_DECIMAL:
			return InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL;
		case TYPE_PHONE:
			return InputType.TYPE_CLASS_PHONE;
		case TYPE_EMAIL:
			return InputType.TYPE_CLASS_TEXT | (password ? InputType.TYPE_TEXT_VARIATION_PASSWORD : InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
		case TYPE_URL:
			return InputType.TYPE_CLASS_TEXT | (password ? InputType.TYPE_TEXT_VARIATION_PASSWORD : InputType.TYPE_TEXT_VARIATION_URI);
		default:
			if (password) return InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD;
			return InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_AUTO_CORRECT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
		}
	}

	private int imeAction() {
		switch (returnKey) {
		case RETURN_DONE: return EditorInfo.IME_ACTION_DONE;
		case RETURN_GO: return EditorInfo.IME_ACTION_GO;
		case RETURN_NEXT: return EditorInfo.IME_ACTION_NEXT;
		case RETURN_SEARCH: return EditorInfo.IME_ACTION_SEARCH;
		case RETURN_SEND: return EditorInfo.IME_ACTION_SEND;
		default: return type == TYPE_SEARCH ? EditorInfo.IME_ACTION_SEARCH : EditorInfo.IME_ACTION_DONE;
		}
	}

	/* ------------------------------------------------------------------ the connection */

	private final class Connection extends BaseInputConnection {
		/* the field it was made for: a connection of an older field edits nothing */
		private final int connectionGen;

		Connection(int gen) {
			super(RavenTextView.this, true);
			connectionGen = gen;
		}

		private boolean live() {
			return connectionGen == gen && kind == KIND_TEXT;
		}

		@Override public Editable getEditable() {
			return editable;
		}

		@Override public boolean beginBatchEdit() {
			if (!live()) return false;
			batch++;
			return true;
		}

		@Override public boolean endBatchEdit() {
			if (!live()) return false;
			if (batch > 0 && --batch == 0) flush();
			return batch > 0;
		}

		@Override public boolean commitText(CharSequence text, int newCursorPosition) {
			if (!live()) return false;
			call("commitText(\"" + text + "\", " + newCursorPosition + ")");
			beginBatchEdit();
			boolean done = super.commitText(text, newCursorPosition);
			endBatchEdit();
			return done;
		}

		@Override public boolean setComposingText(CharSequence text, int newCursorPosition) {
			if (!live()) return false;
			call("setComposingText(\"" + text + "\", " + newCursorPosition + ")");
			beginBatchEdit();
			boolean done = super.setComposingText(text, newCursorPosition);
			endBatchEdit();
			return done;
		}

		@Override public boolean setComposingRegion(int start, int end) {
			if (!live()) return false;
			call("setComposingRegion(" + start + ", " + end + ")");
			beginBatchEdit();
			boolean done = super.setComposingRegion(start, end);
			endBatchEdit();
			return done;
		}

		@Override public boolean finishComposingText() {
			if (!live()) return false;
			call("finishComposingText()");
			beginBatchEdit();
			boolean done = super.finishComposingText();
			endBatchEdit();
			return done;
		}

		@Override public boolean deleteSurroundingText(int before, int after) {
			if (!live()) return false;
			call("deleteSurroundingText(" + before + ", " + after + ")");
			beginBatchEdit();
			boolean done = super.deleteSurroundingText(before, after);
			endBatchEdit();
			return done;
		}

		@Override public boolean deleteSurroundingTextInCodePoints(int before, int after) {
			if (!live()) return false;
			call("deleteSurroundingTextInCodePoints(" + before + ", " + after + ")");
			beginBatchEdit();
			boolean done = super.deleteSurroundingTextInCodePoints(before, after);
			endBatchEdit();
			return done;
		}

		@Override public boolean setSelection(int start, int end) {
			if (!live()) return false;
			call("setSelection(" + start + ", " + end + ")");
			beginBatchEdit();
			boolean done = super.setSelection(start, end);
			endBatchEdit();
			return done;
		}

		@Override public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
			if (!live()) return null;
			if ((flags & GET_EXTRACTED_TEXT_MONITOR) != 0) {
				extractRequest = request;
				extractedSent = editable.toString();
			}
			return extracted();
		}

		@Override public boolean performEditorAction(int action) {
			if (!live()) return false;
			Log.i(TAG, "editor action " + action + " in field " + gen);
			sendOp(OP_ACTION, action, 0);
			return true;
		}

		@Override public boolean performContextMenuAction(int id) {
			if (!live()) return false;
			ClipboardManager clipboard = (ClipboardManager)getContext().getSystemService(Context.CLIPBOARD_SERVICE);
			int s = selectionStart(), e = selectionEnd();
			beginBatchEdit();
			if (id == android.R.id.selectAll) {
				Selection.setSelection(editable, 0, editable.length());
			} else if ((id == android.R.id.cut || id == android.R.id.copy) && s < e && clipboard != null) {
				clipboard.setPrimaryClip(ClipData.newPlainText(null, editable.subSequence(s, e).toString()));
				if (id == android.R.id.cut) editable.delete(s, e);
			} else if (id == android.R.id.paste && clipboard != null && clipboard.hasPrimaryClip()) {
				CharSequence text = clipboard.getPrimaryClip().getItemAt(0).coerceToText(getContext());
				if (text != null) super.commitText(text, 1);
			}
			endBatchEdit();
			return true;
		}

		/*
		 * Keys the IME sends (Backspace on an empty composition, Enter, digits
		 * of some pads): done on the mirror, so they stay in order with the
		 * edits around them. Others go to Raven as keys.
		 */
		@Override public boolean sendKeyEvent(KeyEvent event) {
			if (!live()) return false;
			int action = event.getAction();
			call("sendKeyEvent(" + action + ", " + event.getKeyCode() + ")");
			if (action == KeyEvent.ACTION_MULTIPLE && event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
				String characters = event.getCharacters();
				if (characters != null) commitText(characters, 1);
				return true;
			}
			if (action != KeyEvent.ACTION_DOWN) return true;
			int s = selectionStart(), e = selectionEnd(), length = editable.length();
			beginBatchEdit();
			switch (event.getKeyCode()) {
			case KeyEvent.KEYCODE_DEL:
				removeComposingSpans(editable);
				if (s < e) editable.delete(s, e);
				else if (s > 0) editable.delete(Character.offsetByCodePoints(editable, s, -1), s);
				break;
			case KeyEvent.KEYCODE_FORWARD_DEL:
				removeComposingSpans(editable);
				if (s < e) editable.delete(s, e);
				else if (s < length) editable.delete(s, Character.offsetByCodePoints(editable, s, 1));
				break;
			case KeyEvent.KEYCODE_DPAD_LEFT:
				Selection.setSelection(editable, s < e ? s : (s > 0 ? Character.offsetByCodePoints(editable, s, -1) : 0));
				break;
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				Selection.setSelection(editable, s < e ? e : (e < length ? Character.offsetByCodePoints(editable, e, 1) : length));
				break;
			case KeyEvent.KEYCODE_MOVE_HOME:
				Selection.setSelection(editable, 0);
				break;
			case KeyEvent.KEYCODE_MOVE_END:
				Selection.setSelection(editable, length);
				break;
			case KeyEvent.KEYCODE_ENTER:
			case KeyEvent.KEYCODE_NUMPAD_ENTER:
				sendOp(OP_ACTION, EditorInfo.IME_ACTION_UNSPECIFIED, 0);
				break;
			default: {
				int unicode = event.getUnicodeChar(event.getMetaState());
				if (unicode > 0 && (unicode & KeyCharacterMapCombining) == 0 && !Character.isISOControl(unicode))
					super.commitText(new String(Character.toChars(unicode)), 1);
				else sendOp(OP_KEY, event.getKeyCode(), event.getMetaState());
				break;
			}
			}
			endBatchEdit();
			return true;
		}

		@Override public boolean requestCursorUpdates(int cursorUpdateMode) {
			return false;
		}
	}

	/* KeyCharacterMap.COMBINING_ACCENT */
	private static final int KeyCharacterMapCombining = 0x80000000;

	/* ------------------------------------------------------------------ UTF-8 <-> UTF-16 */

	private static int clamp(int value, int max) {
		return value < 0 ? 0 : value > max ? max : value;
	}

	private static String utf8(byte[] bytes) {
		return bytes == null ? "" : new String(bytes, StandardCharsets.UTF_8);
	}

	/* bytes of the UTF-8 form of text[0, end) */
	static int utf8Length(CharSequence text, int end) {
		int bytes = 0;
		for (int i = 0; i < end; i++) {
			char c = text.charAt(i);
			if (c < 0x80) bytes += 1;
			else if (c < 0x800) bytes += 2;
			else if (Character.isHighSurrogate(c) && i + 1 < end && Character.isLowSurrogate(text.charAt(i + 1))) {
				bytes += 4;
				i++;
			} else bytes += 3;
		}
		return bytes;
	}

	/* UTF-16 index of a UTF-8 byte offset */
	static int utf16Index(byte[] bytes, int offset) {
		int i = 0, index = 0;
		if (bytes == null) return 0;
		while (i < offset && i < bytes.length) {
			int c = bytes[i] & 0xff;
			int length = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
			index += length == 4 ? 2 : 1;
			i += length;
		}
		return index;
	}
}
