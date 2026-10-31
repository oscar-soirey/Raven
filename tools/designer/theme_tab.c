/**
 * Raven Designer - theme_tab.c
 * The Theme tab: the editor of themes (tools/theme_editor) built into the
 * designer. Its source is compiled here a second time, without its main()
 * and its scripted runs (TE_EMBEDDED): te_embed() makes it a panel of the
 * designer instead of a window (editor.h).
 */

#define TE_EMBEDDED 1
#include "../theme_editor/main.c"
