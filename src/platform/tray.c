/**
 * Raven - platform/tray.c
 * Icons beside the clock (raven_shell.h, TRAY ICON): the handles, the
 * callbacks, and the menu of each icon, given to the platform again whenever
 * its Menu changes. The platform draws the icon and shows the menu: the
 * notification area of Windows, a status item of macOS, a StatusNotifierItem
 * on Linux (platform/sni.c).
 */

#include "platform.h"
#include "../widget/widget.h"

typedef struct rvn_tray_t {
	Rid id;
	rvn_ptray_t* platform;
	/** The Menu of the icon, and what it showed when the platform was last given it. */
	Rid menu;
	uint32_t menu_hash;
	int menu_sent;
	RTrayClickedCallback callback;
	void* user_data;
} rvn_tray_t;

static struct {
	rvn_pool_t pool;
	int ready;
} g_tray;

static rvn_tray_t* rvn_tray_check(Rid id, const char* function) {
	rvn_tray_t* tray;
	if (!rvn_api_check(function)) return NULL;
	tray = g_tray.ready ? (rvn_tray_t*)rvn_pool_get(&g_tray.pool, id) : NULL;
	if (!tray) rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: invalid tray icon", function);
	return tray;
}

int Rtray_is_supported(void) {
	RVN_API_GUARD(R_FALSE);
	if (!g_rvn.platform->tray_create) return R_FALSE;
	return g_rvn.platform->tray_supported ? g_rvn.platform->tray_supported() : R_TRUE;
}

Rid Rtray_create(void) {
	rvn_tray_t* tray;
	RVN_API_GUARD(R_INVALID_ID);
	if (!g_rvn.platform->tray_create) return R_INVALID_ID;
	if (!g_tray.ready) {
		rvn_pool_init(&g_tray.pool, RVN_HANDLE_TRAY);
		g_tray.ready = R_TRUE;
	}
	tray = (rvn_tray_t*)rvn_calloc(sizeof(rvn_tray_t));
	tray->platform = g_rvn.platform->tray_create(tray);
	if (!tray->platform) {
		rvn_free(tray);
		return R_INVALID_ID;
	}
	tray->id = rvn_pool_add(&g_tray.pool, tray);
	if (tray->id == R_INVALID_ID) {
		g_rvn.platform->tray_destroy(tray->platform);
		rvn_free(tray);
		return R_INVALID_ID;
	}
	return tray->id;
}

static void rvn_tray_release(rvn_tray_t* tray) {
	g_rvn.platform->tray_destroy(tray->platform);
	rvn_pool_remove(&g_tray.pool, tray->id);
	rvn_free(tray);
}

void Rtray_destroy(Rid id) {
	rvn_tray_t* tray;
	if (id == R_INVALID_ID) return;
	tray = rvn_tray_check(id, __func__);
	if (tray) rvn_tray_release(tray);
}

int Rtray_is_valid(Rid id) {
	return g_rvn.initialized && g_tray.ready && rvn_pool_get(&g_tray.pool, id) != NULL;
}

void Rtray_set_icon(Rid id, const uint8_t* rgba, int width, int height) {
	rvn_tray_t* tray = rvn_tray_check(id, __func__);
	if (!tray) return;
	if (!rgba || width <= 0 || height <= 0 || width > 512 || height > 512) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtray_set_icon: invalid pixels (1 to 512 pixels square)");
		return;
	}
	if (g_rvn.platform->tray_set_icon) g_rvn.platform->tray_set_icon(tray->platform, rgba, width, height);
}

void Rtray_set_tooltip(Rid id, const char* text) {
	rvn_tray_t* tray = rvn_tray_check(id, __func__);
	if (tray && g_rvn.platform->tray_set_tooltip) g_rvn.platform->tray_set_tooltip(tray->platform, text ? text : "");
}

/* Gives the platform the menu of an icon when it changed since it was last given. */
static void rvn_tray_sync_menu(rvn_tray_t* tray, int force) {
	rvn_widget_t* menu = rvn_widget_get(tray->menu);
	uint32_t hash;
	if (!g_rvn.platform->tray_set_menu) return;
	if (menu && (menu->cls->type != R_WIDGET_MENU || (menu->flags & RVN_WF_DEAD))) menu = NULL;
	if (!menu) {
		/* (deleted by the application: the icon has no menu any more) */
		tray->menu = R_INVALID_ID;
		if (tray->menu_sent) g_rvn.platform->tray_set_menu(tray->platform, NULL, 0);
		tray->menu_sent = R_FALSE;
		return;
	}
	hash = rvn_menu_signature(menu, 2166136261u);
	if (!force && tray->menu_sent && hash == tray->menu_hash) return;
	{
		uint32_t count;
		rvn_pmenu_t* items = rvn_menu_native_items(menu, &count);
		g_rvn.platform->tray_set_menu(tray->platform, items, count);
		rvn_menu_native_free(items, count);
	}
	tray->menu_sent = R_TRUE;
	tray->menu_hash = hash;
}

void Rtray_set_menu(Rid id, Rid menu_id) {
	rvn_tray_t* tray = rvn_tray_check(id, __func__);
	if (!tray) return;
	if (menu_id != R_INVALID_ID) {
		rvn_widget_t* menu = rvn_widget_check(menu_id, R_WIDGET_MENU, __func__);
		if (!menu) return;
		if (menu->parent) {
			rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtray_set_menu: the menu belongs to a menu bar or another menu");
			return;
		}
	}
	tray->menu = menu_id;
	rvn_tray_sync_menu(tray, R_TRUE);
}

void Rtray_set_title(Rid id, const char* text) {
	rvn_tray_t* tray = rvn_tray_check(id, __func__);
	if (tray && g_rvn.platform->tray_set_title) g_rvn.platform->tray_set_title(tray->platform, text ? text : "");
}

int Rtray_get_rect(Rid id, int* x, int* y, int* width, int* height) {
	int bx = 0, by = 0, bw = 0, bh = 0, ok = R_FALSE;
	rvn_tray_t* tray = rvn_tray_check(id, __func__);
	if (tray && g_rvn.platform->tray_get_rect) ok = g_rvn.platform->tray_get_rect(tray->platform, &bx, &by, &bw, &bh) ? R_TRUE : R_FALSE;
	if (!ok) bx = by = bw = bh = 0;
	if (x) *x = bx;
	if (y) *y = by;
	if (width) *width = bw;
	if (height) *height = bh;
	return ok;
}

void Rtray_set_clicked_callback(Rid id, RTrayClickedCallback callback, void* user_data) {
	rvn_tray_t* tray = rvn_tray_check(id, __func__);
	if (!tray) return;
	tray->callback = callback;
	tray->user_data = user_data;
}

int rvn_tray_count(void) {
	return g_tray.ready ? (int)g_tray.pool.live : 0;
}

void rvn_tray_update(void) {
	uint32_t i;
	if (!g_tray.ready) return;
	for (i = 0; i < g_tray.pool.used; i++) {
		rvn_tray_t* tray = (rvn_tray_t*)rvn_pool_at(&g_tray.pool, i);
		if (tray && (tray->menu != R_INVALID_ID || tray->menu_sent)) rvn_tray_sync_menu(tray, R_FALSE);
	}
}

void rvn_tray_on_event(const rvn_pevent_t* event) {
	rvn_tray_t* tray = (rvn_tray_t*)event->user;
	uint32_t i;
	if (!g_tray.ready || !tray) return;
	/* (an icon deleted since the system sent it: nothing) */
	for (i = 0; i < g_tray.pool.used && rvn_pool_at(&g_tray.pool, i) != tray; i++) {}
	if (i == g_tray.pool.used || !tray->callback) return;
	tray->callback(tray->id, (RMouseButton)event->button, tray->user_data);
}

void rvn_tray_shutdown(void) {
	uint32_t i;
	if (!g_tray.ready) return;
	for (i = 0; i < g_tray.pool.used; i++) {
		rvn_tray_t* tray = (rvn_tray_t*)rvn_pool_at(&g_tray.pool, i);
		if (tray) rvn_tray_release(tray);
	}
	rvn_pool_free(&g_tray.pool);
	memset(&g_tray, 0, sizeof(g_tray));
}
