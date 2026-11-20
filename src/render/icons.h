/**
 * Raven - render/icons.h
 * The icons shipped with Raven (Lucide, third_party/lucide): their shapes are
 * embedded by tools/embed_icons.py (src/resources/icons.c), and each becomes
 * an SVG texture drawn in currentColor the first time it is asked for.
 */

#ifndef RVN_ICONS_H
#define RVN_ICONS_H

#include <stddef.h>

typedef struct rvn_icon_source_t {
	const char* name;
	/** The shapes inside the <svg> element, which is the same for every icon. */
	const char* shapes;
} rvn_icon_source_t;

/* src/resources/icons.c, sorted by name */
extern const rvn_icon_source_t rvn_icon_sources[];
extern const int rvn_icon_source_count;

/** Index of an icon by its name, with or without the "raven:" of XML; -1 when there is none. */
int rvn_icon_find(const char* name);
/** R_TRUE when `path` names a built-in icon in XML ("raven:search"). */
int rvn_icon_is_reference(const char* path);
/** The whole SVG file of an icon, null-terminated, to release with rvn_free; its length in `size`. */
char* rvn_icon_svg(int index, size_t* size);

#endif /* RVN_ICONS_H */
