package raven_odin

// One distinct type per kind of handle: a Window cannot be passed where a
// Widget is expected. The C API accepts any widget for any widget function, so
// there is no type per widget class (Button, Label, ...).
Window :: distinct u32
Widget :: distinct u32
Timer  :: distinct u32

// The other objects of the C API that have a handle of their own. A tab, a label or a tree view is a
// Widget; these are not.
Texture   :: distinct u32
Font      :: distinct u32
Shader    :: distinct u32 // a shader, and a built-in effect
Xml_Node  :: distinct u32 // a node of an XML document (an Xml_Document is declared in window.odin)
Tree_Item :: distinct u32
Menu      :: distinct u32
Menu_Item :: distinct u32
Tray      :: distinct u32

// R_INVALID_ID of the C API. An untyped constant: it converts to any handle.
INVALID :: 0xFFFFFFFF
