/**
 * Raven - core/accessibility_touch.c
 * The tree of core/accessibility.h as the screen readers of touch screens
 * take it: VoiceOver on iOS (platform/uikit: UIAccessibilityTraits) and
 * TalkBack on Android (platform/android: AccessibilityNodeInfo). Both read
 * the same nodes and act through the same calls; only the words differ, and
 * they are chosen here, once, so that the two platforms agree (and the
 * tests can check them without a device). UI thread.
 */

#include "context.h"
#include "accessibility.h"

/* Roles that only hold others: their children are the elements. */
static int rvn_a11y_touch_structural(RAccessibleRole role) {
	switch (role) {
		case R_ROLE_WINDOW: case R_ROLE_GROUP: case R_ROLE_PANE: case R_ROLE_LIST: case R_ROLE_TREE: case R_ROLE_TABLE:
		case R_ROLE_MENU_BAR: case R_ROLE_MENU: case R_ROLE_TAB_LIST: case R_ROLE_TAB_PANEL: case R_ROLE_TOOLBAR:
		case R_ROLE_DIALOG: case R_ROLE_SEPARATOR: case R_ROLE_SCROLL_BAR:
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static const char* rvn_a11y_touch_class(RAccessibleRole role, uint32_t states) {
	switch (role) {
		case R_ROLE_TEXT: case R_ROLE_HEADING: case R_ROLE_LINK: case R_ROLE_ALERT: case R_ROLE_STATUS_BAR:
			return "android.widget.TextView";
		case R_ROLE_BUTTON: case R_ROLE_MENU_ITEM: return "android.widget.Button";
		case R_ROLE_TOGGLE_BUTTON: return "android.widget.ToggleButton";
		case R_ROLE_CHECKBOX: return "android.widget.CheckBox";
		case R_ROLE_RADIO_BUTTON: return "android.widget.RadioButton";
		case R_ROLE_SLIDER: return "android.widget.SeekBar";
		case R_ROLE_PROGRESS_BAR: return "android.widget.ProgressBar";
		case R_ROLE_SPIN_BUTTON: return "android.widget.NumberPicker";
		case R_ROLE_TEXT_FIELD: return "android.widget.EditText";
		/* (a document that cannot be edited is a text) */
		case R_ROLE_DOCUMENT: return (states & RVN_A11Y_EDITABLE) && !(states & RVN_A11Y_READONLY) ? "android.widget.EditText" : "android.widget.TextView";
		case R_ROLE_COMBO_BOX: return "android.widget.Spinner";
		case R_ROLE_LIST: case R_ROLE_TREE: case R_ROLE_MENU: case R_ROLE_MENU_BAR: return "android.widget.ListView";
		case R_ROLE_TABLE: return "android.widget.GridView";
		case R_ROLE_TAB_LIST: return "android.widget.TabWidget";
		/* (the class TalkBack reads as "tab") */
		case R_ROLE_TAB: return "android.app.ActionBar$Tab";
		case R_ROLE_IMAGE: case R_ROLE_CANVAS: return "android.widget.ImageView";
		case R_ROLE_LIST_ITEM: case R_ROLE_TREE_ITEM: return "android.view.View";
		case R_ROLE_SCROLL_BAR: return "android.widget.AbsSeekBar";
		default: return "android.view.ViewGroup";
	}
}

void rvn_a11y_touch(const rvn_a11y_node_t* node, rvn_a11y_touch_t* touch) {
	RAccessibleRole role = node->role;
	uint32_t states = node->states;
	int named = node->name && node->name[0];
	int valued = node->value && node->value[0];
	touch->traits = 0;
	touch->flags = 0;
	touch->class_name = rvn_a11y_touch_class(role, states);

	/*
	 * An element: what does something (even a group: an expander, a custom
	 * widget with a press action), or what says something and is not only a
	 * holder of others.
	 */
	if ((states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE | RVN_A11Y_EDITABLE | RVN_A11Y_HAS_RANGE | RVN_A11Y_EXPANDABLE)) ||
		(!rvn_a11y_touch_structural(role) && (named || valued)))
		touch->flags |= RVN_A11Y_TOUCH_ELEMENT;
	/* a named group (or a dialog) that is not an element itself is read on the way in */
	else if ((role == R_ROLE_GROUP || role == R_ROLE_DIALOG || role == R_ROLE_PANE || role == R_ROLE_TOOLBAR) && named)
		touch->flags |= RVN_A11Y_TOUCH_GROUP;

	if (!(states & RVN_A11Y_DISABLED)) {
		if (states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE | RVN_A11Y_EXPANDABLE | RVN_A11Y_SELECTABLE))
			touch->flags |= RVN_A11Y_TOUCH_CLICKABLE;
		/* (a field is focused by a double tap: the keyboard comes) */
		if ((states & RVN_A11Y_EDITABLE) && (states & RVN_A11Y_FOCUSABLE) && !(states & RVN_A11Y_READONLY))
			touch->flags |= RVN_A11Y_TOUCH_CLICKABLE;
		if ((states & RVN_A11Y_HAS_RANGE) && !(states & RVN_A11Y_READONLY) && node->maximum > node->minimum)
			touch->flags |= RVN_A11Y_TOUCH_ADJUSTABLE;
		if ((states & RVN_A11Y_EDITABLE) && !(states & RVN_A11Y_READONLY)) touch->flags |= RVN_A11Y_TOUCH_SET_TEXT;
	}
	if (role == R_ROLE_HEADING) touch->flags |= RVN_A11Y_TOUCH_HEADING;
	if (role == R_ROLE_LIST || role == R_ROLE_TREE || role == R_ROLE_TABLE || role == R_ROLE_TAB_LIST || role == R_ROLE_MENU)
		touch->flags |= RVN_A11Y_TOUCH_COLLECTION;
	if (role == R_ROLE_LIST_ITEM || role == R_ROLE_TREE_ITEM || role == R_ROLE_TAB || (states & RVN_A11Y_TABLE_ROW))
		touch->flags |= RVN_A11Y_TOUCH_COLLECTION_ITEM;
	if (role == R_ROLE_ALERT || role == R_ROLE_STATUS_BAR) touch->flags |= RVN_A11Y_TOUCH_LIVE;

	/* VoiceOver */
	switch (role) {
		case R_ROLE_TEXT: case R_ROLE_ALERT: case R_ROLE_STATUS_BAR: case R_ROLE_LIST_ITEM: case R_ROLE_TREE_ITEM:
			touch->traits |= RVN_A11Y_TRAIT_STATIC_TEXT;
			break;
		case R_ROLE_HEADING: touch->traits |= RVN_A11Y_TRAIT_HEADER | RVN_A11Y_TRAIT_STATIC_TEXT; break;
		case R_ROLE_LINK: touch->traits |= RVN_A11Y_TRAIT_LINK; break;
		case R_ROLE_BUTTON: case R_ROLE_MENU_ITEM: case R_ROLE_COMBO_BOX: case R_ROLE_TAB: case R_ROLE_RADIO_BUTTON:
			touch->traits |= RVN_A11Y_TRAIT_BUTTON;
			break;
		case R_ROLE_TOGGLE_BUTTON: case R_ROLE_CHECKBOX: touch->traits |= RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_TOGGLE; break;
		case R_ROLE_PROGRESS_BAR: touch->traits |= RVN_A11Y_TRAIT_UPDATES_FREQUENTLY; break;
		case R_ROLE_IMAGE: case R_ROLE_CANVAS: touch->traits |= RVN_A11Y_TRAIT_IMAGE; break;
		case R_ROLE_TAB_LIST: touch->traits |= RVN_A11Y_TRAIT_TAB_BAR; break;
		default: break;
	}
	/* a custom widget that can be checked is a toggle; one that can be pressed, a button */
	if ((states & RVN_A11Y_CHECKABLE) && role != R_ROLE_RADIO_BUTTON && role != R_ROLE_MENU_ITEM)
		touch->traits |= RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_TOGGLE;
	if ((states & RVN_A11Y_INVOKABLE) && !(touch->traits & (RVN_A11Y_TRAIT_BUTTON | RVN_A11Y_TRAIT_LINK)) &&
		!(touch->flags & RVN_A11Y_TOUCH_COLLECTION_ITEM) && !(states & RVN_A11Y_EXPANDABLE))
		touch->traits |= RVN_A11Y_TRAIT_BUTTON;
	if (touch->flags & RVN_A11Y_TOUCH_ADJUSTABLE) touch->traits |= RVN_A11Y_TRAIT_ADJUSTABLE;
	/* a radio button or a checked menu item that is on is "selected" (a toggle says "on" itself) */
	if (states & RVN_A11Y_SELECTED) touch->traits |= RVN_A11Y_TRAIT_SELECTED;
	if ((states & RVN_A11Y_CHECKED) && !(touch->traits & RVN_A11Y_TRAIT_TOGGLE)) touch->traits |= RVN_A11Y_TRAIT_SELECTED;
	if (states & RVN_A11Y_DISABLED) touch->traits |= RVN_A11Y_TRAIT_NOT_ENABLED;
}

int rvn_a11y_touch_shown(const rvn_a11y_node_t* node) {
	return !(node->states & RVN_A11Y_OFFSCREEN) && node->width > 0.0f && node->height > 0.0f;
}

int rvn_a11y_activate(rvn_a11y_id_t id) {
	rvn_a11y_node_t node;
	uint32_t states;
	if (!rvn_a11y_describe(id, &node) || (node.states & RVN_A11Y_DISABLED)) return R_FALSE;
	states = node.states;
	if ((states & RVN_A11Y_CHECKABLE) && rvn_a11y_act(id, RVN_A11Y_TOGGLE)) return R_TRUE;
	if ((states & RVN_A11Y_INVOKABLE) && rvn_a11y_act(id, RVN_A11Y_PRESS)) return R_TRUE;
	if ((states & RVN_A11Y_SELECTABLE) && rvn_a11y_act(id, RVN_A11Y_SELECT)) return R_TRUE;
	if ((states & RVN_A11Y_EXPANDABLE) && rvn_a11y_act(id, (states & RVN_A11Y_EXPANDED) ? RVN_A11Y_COLLAPSE : RVN_A11Y_EXPAND)) return R_TRUE;
	if ((states & RVN_A11Y_FOCUSABLE) && !(states & RVN_A11Y_FOCUSED) && rvn_a11y_act(id, RVN_A11Y_FOCUS)) return R_TRUE;
	return R_FALSE;
}

int rvn_a11y_adjust(rvn_a11y_id_t id, int steps) {
	rvn_a11y_node_t node;
	double step, value;
	if (steps == 0 || !rvn_a11y_describe(id, &node)) return R_FALSE;
	if (!(node.states & RVN_A11Y_HAS_RANGE) || (node.states & (RVN_A11Y_READONLY | RVN_A11Y_DISABLED))) return R_FALSE;
	step = node.step > 0.0 ? node.step : (node.maximum > node.minimum ? (node.maximum - node.minimum) / 10.0 : 1.0);
	value = node.number + step * (double)steps;
	if (node.maximum > node.minimum) {
		if (value > node.maximum) value = node.maximum;
		if (value < node.minimum) value = node.minimum;
	}
	if (value == node.number) return R_FALSE;
	return rvn_a11y_set_number(id, value);
}
