/*
 * Raven - platform/android/java/raven/android/RavenAccessibility.java
 *
 * TalkBack (and the other accessibility services: Switch Access, Select to
 * Speak...) for Raven's window on Android: an AccessibilityNodeProvider whose
 * virtual views are the nodes of core/accessibility.h.
 *
 * NativeActivity draws into its own surface and takes the input queue, so
 * no view of its window draws or gets touches. This view fills the window,
 * above the NativeActivity's content view, only to carry the provider.
 *
 * Threads: accessibility services ask the provider on the UI thread and
 * expect an answer at once, while Raven lives on the android_main thread.
 * So native keeps the provider up to date with a copy of the tree (setTree,
 * after the frames that may have changed it, at most ten times a second),
 * and the provider answers from that copy. Actions (a double tap, a swipe on
 * a slider, a text set by a service) go back to native through nativeAction:
 * they are queued and done on android_main by the core
 * (rvn_a11y_activate / _adjust / _act / _set_value), and the next copy shows
 * what they changed. Accessibility focus (TalkBack's cursor) is the
 * provider's own: Raven does not know it.
 *
 * Explore by touch: with TalkBack on, the system turns a finger into hover
 * events; they reach native (the input queue), which finds the node under
 * the finger and calls hover(), sent as the view's hover events, as
 * ExploreByTouchHelper does.
 *
 * Native loads the class from the activity's class loader with
 * RavenTextView and registers nativeAction.
 */

package raven.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

public final class RavenAccessibility extends View {
	private static final String TAG = "Raven";

	/* the states of a node (RVN_A11Y_* of core/accessibility.h) */
	static final int FOCUSABLE = 1 << 0, FOCUSED = 1 << 1, DISABLED = 1 << 2, CHECKED = 1 << 3, CHECKABLE = 1 << 4,
		EXPANDED = 1 << 5, EXPANDABLE = 1 << 6, SELECTED = 1 << 7, SELECTABLE = 1 << 8, MULTILINE = 1 << 9,
		READONLY = 1 << 10, PASSWORD = 1 << 11, HAS_RANGE = 1 << 12, EDITABLE = 1 << 13, INVOKABLE = 1 << 14,
		OFFSCREEN = 1 << 15, MODAL = 1 << 16;
	/* how a touch screen reader takes it (RVN_A11Y_TOUCH_*) */
	static final int ELEMENT = 1 << 0, CLICKABLE = 1 << 1, ADJUSTABLE = 1 << 2, SET_TEXT = 1 << 3, HEADING = 1 << 4,
		COLLECTION = 1 << 5, COLLECTION_ITEM = 1 << 6, LIVE = 1 << 7, GROUP = 1 << 8;

	/* actions to native (RVN_ANDROID_AX_* of android_platform.c) */
	static final int ACT_ACTIVATE = 1, ACT_FOCUS = 2, ACT_SELECT = 3, ACT_EXPAND = 4, ACT_COLLAPSE = 5,
		ACT_ADJUST = 6, ACT_SET_TEXT = 7, ACT_SET_NUMBER = 8, ACT_REFRESH = 9;

	/* no virtual view (ExploreByTouchHelper.INVALID_ID) */
	static final int NONE = Integer.MIN_VALUE;

	/* the layout of setTree: per node, INTS ints and FLOATS floats, and STRINGS strings ending in 0 */
	static final int INTS = 4, FLOATS = 8, STRINGS = 4;

	/** Registered by native (RegisterNatives). Thread-safe: queues the action for the android_main thread. */
	private static native void nativeAction(int vid, int action, byte[] text, double number);

	private static final Handler MAIN = new Handler(Looper.getMainLooper());
	private static RavenAccessibility sView;
	/* read by native (isActive) on its own thread */
	private static volatile boolean sActive;

	static final class Node {
		int vid, parent, states, flags, index;
		float x, y, width, height, number, minimum, maximum, step;
		String name, value, description, className;
		final ArrayList<Integer> children = new ArrayList<Integer>();

		boolean sameContent(Node o) {
			return states == o.states && flags == o.flags && x == o.x && y == o.y && width == o.width && height == o.height
				&& number == o.number && minimum == o.minimum && maximum == o.maximum && name.equals(o.name)
				&& value.equals(o.value) && description.equals(o.description) && className.equals(o.className);
		}

		boolean sameText(Node o) {
			return name.equals(o.name) && value.equals(o.value) && description.equals(o.description);
		}
	}

	private final AccessibilityManager manager;
	private final AccessibilityManager.AccessibilityStateChangeListener listener;
	private SparseArray<Node> nodes = new SparseArray<Node>();
	private final ArrayList<Integer> tops = new ArrayList<Integer>();
	/* TalkBack's cursor, the node under the finger (explore by touch), a node whose click is told with the next tree */
	private int accessibilityFocus = NONE, hovered = NONE, clicked = NONE;
	private final Provider provider = new Provider();
	private final int[] location = new int[2];

	private RavenAccessibility(Activity activity) {
		super(activity);
		manager = (AccessibilityManager)activity.getSystemService(Context.ACCESSIBILITY_SERVICE);
		setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_YES);
		setFocusable(false);
		setWillNotDraw(true);
		listener = new AccessibilityManager.AccessibilityStateChangeListener() {
			@Override public void onAccessibilityStateChanged(boolean enabled) {
				sActive = enabled;
				/* (native makes a first copy of the tree now: no frame may come) */
				if (enabled) nativeAction(NONE, ACT_REFRESH, null, 0.0);
			}
		};
	}

	/* ------------------------------------------------------------------ called by native (any thread) */

	/** Adds the view to the activity's window, over all of it (on the UI thread). */
	public static void attach(final Activity activity) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) return;
				RavenAccessibility view = new RavenAccessibility(activity);
				activity.addContentView(view, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
				view.manager.addAccessibilityStateChangeListener(view.listener);
				sActive = view.manager.isEnabled();
				sView = view;
				Log.i(TAG, "accessibility view attached (services " + (sActive ? "on" : "off") + ")");
				if (sActive) nativeAction(NONE, ACT_REFRESH, null, 0.0);
			}
		});
	}

	public static void detach() {
		MAIN.post(new Runnable() {
			@Override public void run() {
				RavenAccessibility view = sView;
				sView = null;
				sActive = false;
				if (view == null) return;
				view.manager.removeAccessibilityStateChangeListener(view.listener);
				ViewParent parent = view.getParent();
				if (parent instanceof ViewGroup) ((ViewGroup)parent).removeView(view);
			}
		});
	}

	/** An accessibility service is on: native keeps the copy of the tree. */
	public static boolean isActive() {
		return sActive;
	}

	/** A new copy of the tree, in preorder (see INTS, FLOATS, STRINGS). */
	public static void setTree(final int[] ints, final float[] floats, final byte[] strings) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) sView.onTree(ints, floats, strings);
			}
		});
	}

	/** The finger of explore by touch is over this node now (NONE: over none, or lifted). */
	public static void hover(final int vid) {
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (sView != null) sView.onHover(vid);
			}
		});
	}

	/** Raccessibility_announce. */
	public static void announce(final byte[] text) {
		MAIN.post(new Runnable() {
			@SuppressWarnings("deprecation")
			@Override public void run() {
				if (sView != null && sView.manager.isEnabled()) sView.announceForAccessibility(new String(text, StandardCharsets.UTF_8));
			}
		});
	}

	/* ------------------------------------------------------------------ UI thread */

	@Override public AccessibilityNodeProvider getAccessibilityNodeProvider() {
		return provider;
	}

	private void onTree(int[] ints, float[] floats, byte[] strings) {
		int count = ints.length / INTS, s = 0;
		SparseArray<Node> next = new SparseArray<Node>(count);
		ArrayList<Integer> nextTops = new ArrayList<Integer>();
		for (int i = 0; i < count; i++) {
			Node n = new Node();
			n.vid = ints[i * INTS];
			n.parent = ints[i * INTS + 1];
			n.states = ints[i * INTS + 2];
			n.flags = ints[i * INTS + 3];
			n.x = floats[i * FLOATS];
			n.y = floats[i * FLOATS + 1];
			n.width = floats[i * FLOATS + 2];
			n.height = floats[i * FLOATS + 3];
			n.number = floats[i * FLOATS + 4];
			n.minimum = floats[i * FLOATS + 5];
			n.maximum = floats[i * FLOATS + 6];
			n.step = floats[i * FLOATS + 7];
			String[] texts = new String[STRINGS];
			for (int k = 0; k < STRINGS; k++) {
				int end = s;
				while (end < strings.length && strings[end] != 0) end++;
				texts[k] = new String(strings, s, end - s, StandardCharsets.UTF_8);
				s = end + 1;
			}
			n.name = texts[0];
			n.value = texts[1];
			n.description = texts[2];
			n.className = texts[3];
			Node parent = n.parent == NONE ? null : next.get(n.parent);
			if (parent != null) {
				n.index = parent.children.size();
				parent.children.add(n.vid);
			} else {
				n.parent = NONE;
				n.index = nextTops.size();
				nextTops.add(n.vid);
			}
			next.put(n.vid, n);
		}
		SparseArray<Node> previous = nodes;
		nodes = next;
		tops.clear();
		tops.addAll(nextTops);
		if (!manager.isEnabled()) return;

		/* what changed: the shape of the tree (the whole subtree of the view), or nodes; the input focus */
		boolean structure = previous.size() != next.size();
		for (int i = 0; i < next.size(); i++) {
			Node n = next.valueAt(i), o = previous.get(n.vid);
			if (o == null || o.parent != n.parent || o.index != n.index || o.children.size() != n.children.size()) {
				structure = true;
				continue;
			}
			if (!o.sameContent(n))
				send(n.vid, AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED, o.sameText(n) ? AccessibilityEvent.CONTENT_CHANGE_TYPE_UNDEFINED
					: AccessibilityEvent.CONTENT_CHANGE_TYPE_TEXT);
			if ((n.states & FOCUSED) != 0 && (o.states & FOCUSED) == 0) send(n.vid, AccessibilityEvent.TYPE_VIEW_FOCUSED, 0);
		}
		if (structure) {
			send(View.NO_ID, AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED, AccessibilityEvent.CONTENT_CHANGE_TYPE_SUBTREE);
			for (int i = 0; i < next.size(); i++) {
				Node n = next.valueAt(i), o = previous.get(n.vid);
				if ((n.states & FOCUSED) != 0 && (o == null || (o.states & FOCUSED) == 0)) send(n.vid, AccessibilityEvent.TYPE_VIEW_FOCUSED, 0);
			}
		}
		/* TalkBack's cursor on a node that is gone: it is lost */
		if (accessibilityFocus != NONE && next.get(accessibilityFocus) == null) {
			accessibilityFocus = NONE;
			invalidate();
		}
		if (hovered != NONE && next.get(hovered) == null) hovered = NONE;
		/* a click is told with the state it gave (TalkBack reads "checked") */
		if (clicked != NONE) {
			if (next.get(clicked) != null) send(clicked, AccessibilityEvent.TYPE_VIEW_CLICKED, 0);
			clicked = NONE;
		}
	}

	private void onHover(int vid) {
		if (vid != NONE && nodes.get(vid) == null) vid = NONE;
		if (vid == hovered) return;
		int previous = hovered;
		hovered = vid;
		if (vid != NONE) send(vid, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER, 0);
		if (previous != NONE) send(previous, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT, 0);
	}

	@SuppressWarnings("deprecation")
	private void send(int vid, int type, int changes) {
		if (!manager.isEnabled()) return;
		ViewParent parent = getParent();
		if (parent == null) return;
		AccessibilityEvent event = AccessibilityEvent.obtain(type);
		event.setPackageName(getContext().getPackageName());
		Node n = vid == View.NO_ID ? null : nodes.get(vid);
		if (n != null) {
			event.setSource(this, vid);
			event.setClassName(n.className);
			event.setEnabled((n.states & DISABLED) == 0);
			event.setChecked((n.states & CHECKED) != 0);
			event.setPassword((n.states & PASSWORD) != 0);
			CharSequence text = text(n);
			if (text != null) event.getText().add(text);
			event.setContentDescription(contentDescription(n));
		} else {
			event.setSource(this);
			event.setClassName(RavenAccessibility.class.getName());
		}
		if (type == AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED) event.setContentChangeTypes(changes);
		parent.requestSendAccessibilityEvent(this, event);
	}

	/* the text a node shows: what is typed in a field, else its name (a button, a label) */
	private static CharSequence text(Node n) {
		if ((n.flags & ELEMENT) == 0) return null;
		if ((n.states & EDITABLE) != 0) return (n.states & PASSWORD) != 0 || n.value.isEmpty() ? null : n.value;
		if (n.name.isEmpty()) return null;
		/* (before API 30 there is no state description: the value is read after the name) */
		if (!n.value.isEmpty() && Build.VERSION.SDK_INT < 30) return n.name + ", " + n.value;
		return n.name;
	}

	/* a field is named by its hint; an image, by its description */
	private static CharSequence contentDescription(Node n) {
		if ((n.flags & ELEMENT) == 0) return null;
		if ((n.states & EDITABLE) != 0 && Build.VERSION.SDK_INT < 26 && n.value.isEmpty() && !n.name.isEmpty()) return n.name;
		if (n.className.equals("android.widget.ImageView") && !n.name.isEmpty()) return n.name;
		return null;
	}

	/* ------------------------------------------------------------------ the provider */

	private final class Provider extends AccessibilityNodeProvider {
		@SuppressWarnings("deprecation")
		@Override public AccessibilityNodeInfo createAccessibilityNodeInfo(int vid) {
			RavenAccessibility host = RavenAccessibility.this;
			if (vid == View.NO_ID) {
				AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain(host);
				onInitializeAccessibilityNodeInfo(info);
				for (int top : tops) info.addChild(host, top);
				return info;
			}
			Node n = nodes.get(vid);
			if (n == null) return null;
			AccessibilityNodeInfo info = AccessibilityNodeInfo.obtain(host, vid);
			info.setPackageName(getContext().getPackageName());
			info.setClassName(n.className);
			if (n.parent == NONE) info.setParent(host);
			else info.setParent(host, n.parent);
			for (int child : n.children) info.addChild(host, child);

			Rect bounds = new Rect(Math.round(n.x), Math.round(n.y), Math.round(n.x + n.width), Math.round(n.y + n.height));
			boolean visible = (n.states & OFFSCREEN) == 0 && bounds.intersect(0, 0, getWidth(), getHeight()) && host.isShown();
			info.setBoundsInParent(bounds);
			getLocationOnScreen(location);
			bounds.offset(location[0], location[1]);
			info.setBoundsInScreen(bounds);
			info.setVisibleToUser(visible);

			boolean enabled = (n.states & DISABLED) == 0;
			info.setEnabled(enabled);
			info.setText(text(n));
			info.setContentDescription(contentDescription(n));
			boolean editable = (n.states & EDITABLE) != 0;
			if (editable && Build.VERSION.SDK_INT >= 26 && !n.name.isEmpty()) info.setHintText(n.name);
			else if (!editable && !n.description.isEmpty() && Build.VERSION.SDK_INT >= 26) info.setHintText(n.description);
			if (!editable && !n.value.isEmpty() && Build.VERSION.SDK_INT >= 30) info.setStateDescription(n.value);
			if (Build.VERSION.SDK_INT >= 28) {
				info.setHeading((n.flags & HEADING) != 0);
				/* (a dialog over the rest is a pane: TalkBack says when it comes) */
				if ((n.flags & GROUP) != 0 && (n.states & MODAL) != 0) info.setPaneTitle(n.name);
			}
			info.setEditable(editable && (n.flags & SET_TEXT) != 0);
			info.setPassword((n.states & PASSWORD) != 0);
			info.setMultiLine((n.states & MULTILINE) != 0);
			info.setCheckable((n.states & CHECKABLE) != 0);
			info.setChecked((n.states & CHECKED) != 0);
			info.setSelected((n.states & SELECTED) != 0);
			info.setFocusable(enabled && (n.states & FOCUSABLE) != 0);
			info.setFocused((n.states & FOCUSED) != 0);
			info.setAccessibilityFocused(vid == accessibilityFocus);
			info.setClickable((n.flags & CLICKABLE) != 0);
			info.setScrollable((n.flags & ADJUSTABLE) != 0);
			if ((n.flags & LIVE) != 0) info.setLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
			if ((n.states & HAS_RANGE) != 0)
				info.setRangeInfo(AccessibilityNodeInfo.RangeInfo.obtain(AccessibilityNodeInfo.RangeInfo.RANGE_TYPE_FLOAT, n.minimum, n.maximum, n.number));
			if ((n.flags & COLLECTION) != 0)
				info.setCollectionInfo(AccessibilityNodeInfo.CollectionInfo.obtain(n.children.size(), 1, false));
			if ((n.flags & COLLECTION_ITEM) != 0)
				info.setCollectionItemInfo(AccessibilityNodeInfo.CollectionItemInfo.obtain(n.index, 1, 0, 1, false, (n.states & SELECTED) != 0));

			info.addAction(vid == accessibilityFocus ? AccessibilityNodeInfo.AccessibilityAction.ACTION_CLEAR_ACCESSIBILITY_FOCUS
				: AccessibilityNodeInfo.AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS);
			if (!enabled) return info;
			if ((n.flags & CLICKABLE) != 0) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_CLICK);
			if ((n.states & FOCUSABLE) != 0 && (n.states & FOCUSED) == 0) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_FOCUS);
			if ((n.states & SELECTABLE) != 0 && (n.states & SELECTED) == 0) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SELECT);
			if ((n.states & EXPANDABLE) != 0)
				info.addAction((n.states & EXPANDED) != 0 ? AccessibilityNodeInfo.AccessibilityAction.ACTION_COLLAPSE
					: AccessibilityNodeInfo.AccessibilityAction.ACTION_EXPAND);
			if ((n.flags & ADJUSTABLE) != 0) {
				if (n.number < n.maximum) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_FORWARD);
				if (n.number > n.minimum) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_BACKWARD);
				if (Build.VERSION.SDK_INT >= 24) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_PROGRESS);
			}
			if ((n.flags & SET_TEXT) != 0) info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SET_TEXT);
			return info;
		}

		@Override public boolean performAction(int vid, int action, Bundle arguments) {
			RavenAccessibility host = RavenAccessibility.this;
			if (vid == View.NO_ID) return host.performAccessibilityAction(action, arguments);
			Node n = nodes.get(vid);
			if (n == null) return false;
			switch (action) {
			case AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS:
				if (accessibilityFocus == vid) return false;
				if (accessibilityFocus != NONE) send(accessibilityFocus, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED, 0);
				accessibilityFocus = vid;
				invalidate();
				send(vid, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED, 0);
				return true;
			case AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS:
				if (accessibilityFocus != vid) return false;
				accessibilityFocus = NONE;
				invalidate();
				send(vid, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED, 0);
				return true;
			default:
				break;
			}
			if ((n.states & DISABLED) != 0) return false;
			switch (action) {
			case AccessibilityNodeInfo.ACTION_CLICK:
				if ((n.flags & CLICKABLE) == 0) return false;
				clicked = vid;
				nativeAction(vid, ACT_ACTIVATE, null, 0.0);
				return true;
			case AccessibilityNodeInfo.ACTION_FOCUS:
				if ((n.states & FOCUSABLE) == 0) return false;
				nativeAction(vid, ACT_FOCUS, null, 0.0);
				return true;
			case AccessibilityNodeInfo.ACTION_SELECT:
				if ((n.states & SELECTABLE) == 0) return false;
				nativeAction(vid, ACT_SELECT, null, 0.0);
				return true;
			case AccessibilityNodeInfo.ACTION_EXPAND:
			case AccessibilityNodeInfo.ACTION_COLLAPSE:
				if ((n.states & EXPANDABLE) == 0) return false;
				nativeAction(vid, action == AccessibilityNodeInfo.ACTION_EXPAND ? ACT_EXPAND : ACT_COLLAPSE, null, 0.0);
				return true;
			case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD:
			case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD:
				if ((n.flags & ADJUSTABLE) == 0) return false;
				nativeAction(vid, ACT_ADJUST, null, action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD ? 1.0 : -1.0);
				return true;
			case AccessibilityNodeInfo.ACTION_SET_TEXT: {
				if ((n.flags & SET_TEXT) == 0) return false;
				CharSequence text = arguments != null ? arguments.getCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE) : null;
				nativeAction(vid, ACT_SET_TEXT, (text != null ? text.toString() : "").getBytes(StandardCharsets.UTF_8), 0.0);
				return true;
			}
			default:
				if (Build.VERSION.SDK_INT >= 24 && action == android.R.id.accessibilityActionSetProgress) {
					if ((n.flags & ADJUSTABLE) == 0 || arguments == null) return false;
					float value = arguments.getFloat(AccessibilityNodeInfo.ACTION_ARGUMENT_PROGRESS_VALUE, n.number);
					nativeAction(vid, ACT_SET_NUMBER, null, value);
					return true;
				}
				return false;
			}
		}

		@Override public AccessibilityNodeInfo findFocus(int focus) {
			if (focus == AccessibilityNodeInfo.FOCUS_ACCESSIBILITY)
				return accessibilityFocus != NONE ? createAccessibilityNodeInfo(accessibilityFocus) : null;
			for (int i = 0; i < nodes.size(); i++)
				if ((nodes.valueAt(i).states & FOCUSED) != 0) return createAccessibilityNodeInfo(nodes.valueAt(i).vid);
			return null;
		}
	}
}
