package raven_odin

// One distinct type per kind of handle: a Window cannot be passed where a
// Widget is expected. The C API accepts any widget for any widget function, so
// there is no type per widget class (Button, Label, ...).
Window :: distinct u32
Widget :: distinct u32
Timer  :: distinct u32

// R_INVALID_ID of the C API. An untyped constant: it converts to any handle.
INVALID :: 0xFFFFFFFF
