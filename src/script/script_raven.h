/**
 * Raven - script/script_raven.h
 * Raven Script in the library: what the XML loader asks of it.
 */

#ifndef RVN_SCRIPT_RAVEN_H
#define RVN_SCRIPT_RAVEN_H

#include "../core/base.h"
#include "../xml/xml.h"

/** The scripts of a document being built. */
typedef struct rvn_script_document_t rvn_script_document_t;

/**
 * Before the widgets of a document are built: takes its <Script> elements
 * out of the tree (they are not widgets), compiles them with the files they
 * name (Src) and installs their functions, so that the actions of the
 * widgets find them. `path` names the document in the diagnostics (NULL:
 * the file being loaded, or "<xml>"). Never NULL (a document without
 * scripts still checks its actions against the scripts loaded before).
 * Calls nest (a document built while another is).
 */
rvn_script_document_t* rvn_script_document_begin(rvn_xml_document_t* document, const char* path);

/**
 * After the widgets were built: the initializers of the globals run, with
 * `built` (a window or a widget, R_INVALID_ID after a failure) as the
 * place where ui.* look for widgets by name. Releases `scripts`.
 */
void rvn_script_document_end(rvn_script_document_t* scripts, Rid built);

/**
 * While a document is built: the action attribute of an element names a
 * function; reports RS103 when it is a script function whose parameters do
 * not fit the event of the widget (specification 12.5).
 */
void rvn_script_check_action(const rvn_xml_node_t* node, const char* attribute, const char* action);

/** The file of the document being loaded (the diagnostics name it). Returns the previous one. */
const char* rvn_script_set_file(const char* path);

#endif /* RVN_SCRIPT_RAVEN_H */
