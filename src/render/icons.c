/**
 * Raven - render/icons.c
 * The icons shipped with Raven: Ricon_get and the names of XML ("raven:search").
 *
 * An icon is the SVG of Lucide (24 x 24, a stroke of 2 in currentColor),
 * made a texture on first use and kept for the life of the library: an SVG
 * painted only in currentColor is drawn in white and tinted at draw time with
 * the text color of the widget that shows it (svg_decode.c), so one texture
 * serves every theme, state and window. It is drawn again at the density it is
 * shown with, as every SVG (Rtexture_create_from_svg with a scale of 0).
 */

#include "renderer.h"
#include "icons.h"
#include "../core/context.h"

#define RVN_ICON_PREFIX "raven:"

/* What Lucide puts around the shapes of every icon (tools/embed_icons.py checks it). */
static const char g_icon_head[] =
	"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" "
	"stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">";
static const char g_icon_tail[] = "</svg>";

int rvn_icon_is_reference(const char* path) {
	return path && strncmp(path, RVN_ICON_PREFIX, sizeof(RVN_ICON_PREFIX) - 1) == 0;
}

int rvn_icon_find(const char* name) {
	int low = 0, high = rvn_icon_source_count - 1;
	if (!name) return -1;
	if (rvn_icon_is_reference(name)) name += sizeof(RVN_ICON_PREFIX) - 1;
	while (low <= high) {
		int middle = (low + high) / 2;
		int order = strcmp(name, rvn_icon_sources[middle].name);
		if (order == 0) return middle;
		if (order < 0) high = middle - 1;
		else low = middle + 1;
	}
	return -1;
}

char* rvn_icon_svg(int index, size_t* size) {
	size_t head = sizeof(g_icon_head) - 1, tail = sizeof(g_icon_tail) - 1, shapes;
	char* svg;
	if (index < 0 || index >= rvn_icon_source_count) return NULL;
	shapes = strlen(rvn_icon_sources[index].shapes);
	svg = (char*)rvn_alloc(head + shapes + tail + 1);
	memcpy(svg, g_icon_head, head);
	memcpy(svg + head, rvn_icon_sources[index].shapes, shapes);
	memcpy(svg + head + shapes, g_icon_tail, tail + 1);
	if (size) *size = head + shapes + tail;
	return svg;
}

Rid Ricon_get(const char* name) {
	int index;
	Rid* icon;
	RVN_API_GUARD(R_INVALID_ID);
	index = rvn_icon_find(name);
	if (index < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Ricon_get: no icon is named '%s'", name ? name : "(null)");
		return R_INVALID_ID;
	}
	if (!g_rvn.icons) g_rvn.icons = (Rid*)rvn_calloc(sizeof(Rid) * (size_t)rvn_icon_source_count);
	icon = &g_rvn.icons[index];
	/* (made again if the application destroyed it) */
	if (!rvn_texture_get(*icon)) {
		size_t size = 0;
		char* svg = rvn_icon_svg(index, &size);
		*icon = Rtexture_create_from_svg(svg, size, 0.0f);
		rvn_free(svg);
	}
	return *icon;
}

int Ricon_get_count(void) {
	return rvn_icon_source_count;
}

const char* Ricon_get_name(int index) {
	return index >= 0 && index < rvn_icon_source_count ? rvn_icon_sources[index].name : NULL;
}
