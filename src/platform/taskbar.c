/**
 * Raven - platform/taskbar.c
 * What the system shows of the application outside of its windows
 * (raven_shell.h, TASKBAR AND DOCK): a progress bar and a badge on its icon,
 * a request for attention, the menu of the icon in the Dock and the jump list
 * of Windows. Each platform does what its system knows (rvn_platform_t);
 * here are the checks, and the menu of the icon that follows its Menu.
 */

#include "platform.h"
#include "../widget/widget.h"

static struct {
	/** The Menu given to Rtaskbar_set_menu, and what it showed when the platform was last given it. */
	Rid menu;
	uint32_t menu_hash;
	int menu_sent;
} g_taskbar;

int Rwindow_set_taskbar_progress(Rid id, RTaskbarProgress state, float value) {
	rvn_window_t* window;
	RVN_API_GUARD(R_FALSE);
	window = rvn_window_check(id, __func__);
	if (!window) return R_FALSE;
	if (state < R_TASKBAR_PROGRESS_NONE || state > R_TASKBAR_PROGRESS_INDETERMINATE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwindow_set_taskbar_progress: invalid state");
		return R_FALSE;
	}
	if (!g_rvn.platform->window_set_progress) return R_FALSE;
	if (value != value) value = 0.0f;
	return g_rvn.platform->window_set_progress(window->platform, (int)(state - R_TASKBAR_PROGRESS_NONE), rvn_clampf(value, 0.0f, 1.0f));
}

int Rwindow_request_attention(Rid id, int critical) {
	rvn_window_t* window;
	RVN_API_GUARD(R_FALSE);
	window = rvn_window_check(id, __func__);
	if (!window) return R_FALSE;
	if (!g_rvn.platform->window_request_attention) return R_FALSE;
	return g_rvn.platform->window_request_attention(window->platform, critical ? R_TRUE : R_FALSE);
}

int Rtaskbar_set_badge(const char* text) {
	RVN_API_GUARD(R_FALSE);
	if (!g_rvn.platform->app_set_badge) return R_FALSE;
	return g_rvn.platform->app_set_badge(text && text[0] ? text : NULL);
}

/* Gives the platform the menu of the icon when it changed since it was last given. */
static void rvn_taskbar_sync_menu(int force) {
	rvn_widget_t* menu = rvn_widget_get(g_taskbar.menu);
	uint32_t hash;
	if (!g_rvn.platform->app_set_icon_menu) return;
	if (menu && (menu->cls->type != R_WIDGET_MENU || (menu->flags & RVN_WF_DEAD))) menu = NULL;
	if (!menu) {
		/* (deleted by the application: the system forgets it too) */
		g_taskbar.menu = R_INVALID_ID;
		if (g_taskbar.menu_sent) g_rvn.platform->app_set_icon_menu(NULL, 0);
		g_taskbar.menu_sent = R_FALSE;
		return;
	}
	hash = rvn_menu_signature(menu, 2166136261u);
	if (!force && g_taskbar.menu_sent && hash == g_taskbar.menu_hash) return;
	{
		uint32_t count;
		rvn_pmenu_t* items = rvn_menu_native_items(menu, &count);
		g_taskbar.menu_sent = g_rvn.platform->app_set_icon_menu(items, count);
		rvn_menu_native_free(items, count);
	}
	g_taskbar.menu_hash = hash;
}

int Rtaskbar_set_menu(Rid id) {
	RVN_API_GUARD(R_FALSE);
	if (id != R_INVALID_ID) {
		rvn_widget_t* menu = rvn_widget_check(id, R_WIDGET_MENU, __func__);
		if (!menu) return R_FALSE;
		if (menu->parent) {
			rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtaskbar_set_menu: the menu belongs to a menu bar or another menu");
			return R_FALSE;
		}
	}
	if (!g_rvn.platform->app_set_icon_menu) return R_FALSE;
	g_taskbar.menu = id;
	rvn_taskbar_sync_menu(R_TRUE);
	return id == R_INVALID_ID ? R_TRUE : g_taskbar.menu_sent;
}

void rvn_taskbar_update(void) {
	if (g_taskbar.menu != R_INVALID_ID || g_taskbar.menu_sent) rvn_taskbar_sync_menu(R_FALSE);
}

int Rtaskbar_set_jump_list(const RJumpTask* tasks, int count) {
	int i;
	RVN_API_GUARD(R_FALSE);
	if (count < 0 || (count > 0 && !tasks)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtaskbar_set_jump_list: no tasks");
		return R_FALSE;
	}
	for (i = 0; i < count; i++) {
		if (!tasks[i].title || !tasks[i].title[0]) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtaskbar_set_jump_list: task %d has no title", i);
			return R_FALSE;
		}
	}
	if (!g_rvn.platform->app_set_jump_list) return R_FALSE;
	return g_rvn.platform->app_set_jump_list(tasks, count);
}

void rvn_taskbar_shutdown(void) {
	memset(&g_taskbar, 0, sizeof(g_taskbar));
}
