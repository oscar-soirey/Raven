/**
 * Raven - widget/actions.c
 * The actions Raven knows by itself: an interface written in XML quits,
 * closes its window, shows a panel, turns a page or changes its theme
 * without a line of C (OnClicked="app.quit", OnClicked="show:details").
 *
 * They are looked at when no function is bound to the name (Raction_bind
 * wins), before the default handler. Raction_set_builtins(R_FALSE) turns
 * them off: a design tool that shows interfaces without running them.
 *
 * Built on the public API only: what an application could write itself.
 */

#include <Raven.h>

#include <stdlib.h>
#include <string.h>

/* The window an action is about: the sender, or the window of the sender. */
static Rid rvn_action_window(Rid sender) {
	if (sender == R_INVALID_ID) return R_INVALID_ID;
	if (Rwindow_is_valid(sender)) return sender;
	if (Rwidget_is_valid(sender)) return Rwidget_get_window(sender);
	return R_INVALID_ID;
}

/* "show:details" -> the widget named "details" in the window of the sender. */
static Rid rvn_action_target(Rid sender, const char* name, size_t length) {
	char buffer[128];
	Rid window = rvn_action_window(sender);
	if (window == R_INVALID_ID || length == 0 || length >= sizeof(buffer)) return R_INVALID_ID;
	memcpy(buffer, name, length);
	buffer[length] = '\0';
	return Rwidget_find(window, buffer);
}

static int rvn_prefix(const char* name, const char* prefix, const char** rest) {
	size_t length = strlen(prefix);
	if (strncmp(name, prefix, length) != 0) return 0;
	*rest = name + length;
	return 1;
}

static int rvn_theme_is_dark(void) {
	float r, g, b, a;
	Rtheme_get_color(R_COLOR_WINDOW, &r, &g, &b, &a);
	return 0.2126f * r + 0.7152f * g + 0.0722f * b < 0.5f;
}

/* "page:pages=2": the page (child) a Switcher, a TabView, a ComboBox or a ListView shows; "+1" and "-1" step. */
static int rvn_action_page(Rid sender, const char* rest) {
	const char* equal = strchr(rest, '=');
	Rid widget;
	int index, current = -1, count;
	RWidgetType type;
	if (!equal) return 1;
	widget = rvn_action_target(sender, rest, (size_t)(equal - rest));
	if (widget == R_INVALID_ID) return 1;
	type = Rwidget_get_type(widget);
	switch (type) {
		case R_WIDGET_SWITCHER: current = Rswitcher_get_index(widget); count = Rwidget_get_child_count(widget); break;
		case R_WIDGET_TAB_VIEW: current = Rtab_view_get_active(widget); count = Rtab_view_get_tab_count(widget); break;
		case R_WIDGET_COMBO_BOX: current = Rcombo_box_get_selected(widget); count = Rcombo_box_get_item_count(widget); break;
		case R_WIDGET_LIST_VIEW: current = Rlist_view_get_selected(widget); count = Rlist_view_get_item_count(widget); break;
		default: return 1;
	}
	if (equal[1] == '+' || equal[1] == '-') {
		index = current + atoi(equal + 1);
		/* (it goes round) */
		if (count > 0) index = ((index % count) + count) % count;
	} else {
		index = atoi(equal + 1);
	}
	if (count <= 0 || index < 0 || index >= count) return 1;
	switch (type) {
		case R_WIDGET_SWITCHER: Rswitcher_set_index(widget, index); break;
		case R_WIDGET_TAB_VIEW: Rtab_view_set_active(widget, index); break;
		case R_WIDGET_COMBO_BOX: Rcombo_box_set_selected(widget, index); break;
		default: Rlist_view_set_selected(widget, index); break;
	}
	return 1;
}

/* Called by rvn_fire_action when no function is bound to `name`. Returns 1 when it was one of them. */
int rvn_builtin_action(Rid sender, const char* name);
int rvn_builtin_action(Rid sender, const char* name) {
	const char* rest;
	Rid window = rvn_action_window(sender), widget, island;

	/* ---- the application ---- */
	if (strcmp(name, "app.quit") == 0) {
		Rquit(0);
		return 1;
	}

	/* ---- the window of the widget ---- */
	if (strncmp(name, "window.", 7) == 0) {
		if (window == R_INVALID_ID) return 1;
		if (strcmp(name, "window.close") == 0) Rwindow_close(window);
		else if (strcmp(name, "window.minimize") == 0) Rwindow_minimize(window);
		else if (strcmp(name, "window.maximize") == 0) {
			if (Rwindow_is_maximized(window)) Rwindow_restore(window);
			else Rwindow_maximize(window);
		} else if (strcmp(name, "window.restore") == 0) Rwindow_restore(window);
		else if (strcmp(name, "window.picture_in_picture") == 0) {
			Rwindow_set_picture_in_picture(window, !Rwindow_is_picture_in_picture(window));
		} else return 0;
		return 1;
	}
	if (strncmp(name, "island.", 7) == 0) {
		island = window != R_INVALID_ID ? Rwindow_get_island(window) : R_INVALID_ID;
		if (island == R_INVALID_ID) return strcmp(name, "island.expand") == 0 || strcmp(name, "island.fold") == 0 || strcmp(name, "island.toggle") == 0;
		if (strcmp(name, "island.expand") == 0) Risland_set_expanded(island, R_TRUE);
		else if (strcmp(name, "island.fold") == 0) Risland_set_expanded(island, R_FALSE);
		else if (strcmp(name, "island.toggle") == 0) Risland_set_expanded(island, !Risland_is_expanded(island));
		else return 0;
		return 1;
	}

	/* ---- the look ---- */
	if (strcmp(name, "theme.dark") == 0) {
		Rlook_set(R_LOOK_RAVEN_DARK);
		return 1;
	}
	if (strcmp(name, "theme.light") == 0) {
		Rlook_set(R_LOOK_RAVEN_LIGHT);
		return 1;
	}
	if (strcmp(name, "theme.toggle") == 0) {
		Rlook_set(rvn_theme_is_dark() ? R_LOOK_RAVEN_LIGHT : R_LOOK_RAVEN_DARK);
		return 1;
	}
	if (rvn_prefix(name, "theme:", &rest)) {
		Rtheme_use(rest);
		return 1;
	}
	if (rvn_prefix(name, "language:", &rest)) {
		Rlocale_set_language(rest);
		return 1;
	}

	/* ---- outside the application ---- */
	if (rvn_prefix(name, "open:", &rest)) {
		Ropen_url(rest);
		return 1;
	}
	if (rvn_prefix(name, "copy:", &rest)) {
		Rclipboard_set_text(rest);
		return 1;
	}

	/* ---- the widgets of the window, by their names ---- */
	if (rvn_prefix(name, "show:", &rest) || rvn_prefix(name, "hide:", &rest) || rvn_prefix(name, "toggle:", &rest)) {
		widget = rvn_action_target(sender, rest, strlen(rest));
		if (widget == R_INVALID_ID) return 1;
		if (name[0] == 's') Rwidget_set_visibility(widget, R_VISIBLE);
		else if (name[0] == 'h') Rwidget_set_visibility(widget, R_COLLAPSED);
		else Rwidget_set_visibility(widget, Rwidget_get_visibility(widget) == R_VISIBLE ? R_COLLAPSED : R_VISIBLE);
		return 1;
	}
	if (rvn_prefix(name, "enable:", &rest) || rvn_prefix(name, "disable:", &rest)) {
		widget = rvn_action_target(sender, rest, strlen(rest));
		if (widget != R_INVALID_ID) Rwidget_set_enabled(widget, name[0] == 'e');
		return 1;
	}
	if (rvn_prefix(name, "focus:", &rest)) {
		widget = rvn_action_target(sender, rest, strlen(rest));
		if (widget != R_INVALID_ID) Rwidget_focus(widget);
		return 1;
	}
	if (rvn_prefix(name, "page:", &rest)) return rvn_action_page(sender, rest);
	return 0;
}

int Raction_is_builtin(const char* name) {
	static const char* const names[] = {
		"app.quit", "window.close", "window.minimize", "window.maximize", "window.restore", "window.picture_in_picture", "island.expand", "island.fold",
		"island.toggle", "theme.dark", "theme.light", "theme.toggle"
	};
	static const char* const prefixes[] = {
		"theme:", "language:", "open:", "copy:", "show:", "hide:", "toggle:", "enable:", "disable:", "focus:", "page:"
	};
	size_t i;
	if (!name) return 0;
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) if (strcmp(name, names[i]) == 0) return 1;
	for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
		if (strncmp(name, prefixes[i], strlen(prefixes[i])) == 0 && name[strlen(prefixes[i])]) return 1;
	}
	return 0;
}
