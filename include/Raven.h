/**
 * Raven
 *
 * Lightweight, multithreaded, retained-mode GUI library in C with a Slate-like
 * layout model, a Vulkan renderer, native windowing and XML descriptions.
 *
 * This single header includes the whole API:
 *
 *     #include <Raven.h>
 *
 *     int main(void) {
 *         Rinit(R_BACKEND_VULKAN);
 *         Rid window = Rwindow_create("Hello", 640, 480, R_WINDOW_DEFAULT);
 *         Rwindow_set_content(window, Rlabel_create("Hello, world"));
 *         while (Rwait_frame(-1.0)) {}
 *         Rshutdown();
 *         return 0;
 *     }
 *
 * The headers can also be included one by one:
 *
 *     raven/raven_core.h         lifecycle, the loop, memory, threads, actions, errors
 *     raven/raven_window.h       windows, input events, drag and drop
 *     raven/raven_touch.h        fingers and pens: settings, events, gestures
 *     raven/raven_style.h        themes, colors, metrics, materials
 *     raven/raven_resources.h    fonts, textures
 *     raven/raven_widget.h       widget tree, slots, common properties, transitions
 *     raven/raven_layout.h       panels
 *     raven/raven_controls.h     labels, buttons, inputs, lists, menus...
 *     raven/raven_text_editor.h  multi-line text editor and its languages
 *     raven/raven_shell.h        header bar, docked panels, island
 *     raven/raven_tools.h        color picker, timeline, node graph
 *     raven/raven_dialogs.h      dialogs, message boxes, file dialogs, file browser
 *     raven/raven_viewport.h     content rendered by the application
 *     raven/raven_draw.h         custom widgets: painting, springs
 *     raven/raven_xml.h          interfaces in XML, declared tags, documents
 *     raven/raven_vulkan.h       sharing the Vulkan device of the library
 *     raven/raven_backend.h      writing a rendering backend
 */

#ifndef RAVEN_H
#define RAVEN_H

#include "raven/raven_core.h"
#include "raven/raven_window.h"
#include "raven/raven_touch.h"
#include "raven/raven_style.h"
#include "raven/raven_resources.h"
#include "raven/raven_widget.h"
#include "raven/raven_layout.h"
#include "raven/raven_controls.h"
#include "raven/raven_text_editor.h"
#include "raven/raven_shell.h"
#include "raven/raven_tools.h"
#include "raven/raven_dialogs.h"
#include "raven/raven_viewport.h"
#include "raven/raven_draw.h"
#include "raven/raven_shader.h"
#include "raven/raven_xml.h"
#include "raven/raven_locale.h"
#include "raven/raven_accessibility.h"
#include "raven/raven_vulkan.h"
#include "raven/raven_backend.h"
#include "raven/raven_host.h"

#endif /* RAVEN_H */
