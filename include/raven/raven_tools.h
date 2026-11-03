/**
 * Raven
 * raven_tools.h : the widgets of creative tools.
 *
 *   R_WIDGET_COLOR_PICKER   chooses a color
 *   R_WIDGET_TIMELINE       tracks of clips and keys along a time ruler (a sequencer)
 *   R_WIDGET_NODE_GRAPH     nodes linked by wires
 *   R_WIDGET_BLOCK_EDITOR   blocks that snap into each other (Scratch)
 *
 * Each of them holds a small model (a color; tracks, clips and keys; nodes,
 * pins and links) that the application fills and reads through the
 * functions below, or writes in XML. The things inside a timeline or a
 * graph are named by integers that are only meaningful for that widget, 0
 * meaning "none"; they are never reused while the widget lives.
 */

#ifndef RAVEN_TOOLS_H
#define RAVEN_TOOLS_H

#include "raven_widget.h"


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  COLOR PICKER
	 *
	 *  A square of saturation and value, a strip of hues, a strip of opacity
	 *  over a checkerboard, the color before and the color now side by side,
	 *  its hexadecimal code, its numbers (red green blue, or hue saturation
	 *  value), and a row of swatches. Everything is linked: dragging in the
	 *  square moves the numbers, typing a code moves the square.
	 *
	 *    <ColorPicker Name="tint" Color="#3B82F6" Alpha="true" OnChanged="tint.changed"/>
	 *
	 *  With Popup="true" the widget is a swatch the size of a button, which
	 *  opens the picker in a popup when clicked.
	 * ============================================================================ */

	RAVEN_API Rid Rcolor_picker_create(void);

	/** @brief Sets the color (0..1 each). The callback is not called. */
	RAVEN_API void Rcolor_picker_set_color(Rid picker, float r, float g, float b, float a);
	RAVEN_API void Rcolor_picker_get_color(Rid picker, float* r, float* g, float* b, float* a);
	/**
	 * @brief The same color as hue (0..360), saturation and value (0..1).
	 * The picker remembers the hue of a gray and the saturation of black,
	 * which red, green and blue cannot tell.
	 */
	RAVEN_API void Rcolor_picker_set_hsv(Rid picker, float hue, float saturation, float value);
	RAVEN_API void Rcolor_picker_get_hsv(Rid picker, float* hue, float* saturation, float* value);
	/**
	 * @brief Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
	 * '#' may be left out) or one of the color names of the XML attributes.
	 * A code without opacity leaves the opacity as it is.
	 * @return R_FALSE when the text is not a color (the color is unchanged).
	 */
	RAVEN_API int Rcolor_picker_set_hex(Rid picker, const char* text);
	/** @return "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes. */
	RAVEN_API const char* Rcolor_picker_get_hex(Rid picker);

	/** @brief R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque. */
	RAVEN_API void Rcolor_picker_set_alpha_enabled(Rid picker, int enabled);
	/**
	 * @brief The color shown next to the current one, to compare with what
	 * was there before: clicking it comes back to it. Rcolor_picker_set_color
	 * does not change it; a popup picker takes it when it opens.
	 */
	RAVEN_API void Rcolor_picker_set_previous_color(Rid picker, float r, float g, float b, float a);

	/** @brief Adds a color to the row of swatches: one click takes it. */
	RAVEN_API void Rcolor_picker_add_swatch(Rid picker, float r, float g, float b, float a);
	RAVEN_API void Rcolor_picker_clear_swatches(Rid picker);
	RAVEN_API int Rcolor_picker_get_swatch_count(Rid picker);

	/** @brief R_TRUE turns the widget into a swatch that opens the picker in a popup. */
	RAVEN_API void Rcolor_picker_set_popup(Rid picker, int popup);
	/** @brief R_TRUE hides the code and the numbers: only the square, the strips and the swatches. */
	RAVEN_API void Rcolor_picker_set_compact(Rid picker, int compact);

	/**
	 * @brief Called each time the user changes the color: while dragging,
	 * and when a code or a number is entered. The action of the widget
	 * (OnChanged in XML) fires too.
	 */
	typedef void (*RColorChangedCallback)(Rid picker, float r, float g, float b, float a, void* user_data);
	RAVEN_API void Rcolor_picker_set_changed_callback(Rid picker, RColorChangedCallback callback, void* user_data);

	/* ---- colors as numbers ---- */

	/** @brief Red, green, blue (0..1) to hue (0..360), saturation, value (0..1). */
	RAVEN_API void Rcolor_rgb_to_hsv(float r, float g, float b, float* hue, float* saturation, float* value);
	RAVEN_API void Rcolor_hsv_to_rgb(float hue, float saturation, float value, float* r, float* g, float* b);


	/* ============================================================================
	 *  TIMELINE
	 *
	 *  A sequencer: a ruler of time at the top, a playhead, and tracks one
	 *  under the other. A track holds clips (something that lasts: a start
	 *  and a length) and keys (a moment). The user scrubs the playhead on
	 *  the ruler, moves clips along their track or to another one, stretches
	 *  them by their ends, moves keys, selects by clicking or with a
	 *  rectangle, zooms with Ctrl + wheel, pans with the wheel, the middle
	 *  button or the scrollbars. Moves snap to the frames, to the playhead,
	 *  to the markers and to the ends of the other clips.
	 *
	 *    <Timeline Name="sequence" Duration="12" FrameRate="24">
	 *        <Track Name="Camera" Color="#0A84FF">
	 *            <Clip Start="0" Length="3.5" Label="Wide shot"/>
	 *            <Clip Start="4" Length="2" Label="Close up" Color="#FF9F0A"/>
	 *        </Track>
	 *        <Track Name="Opacity">
	 *            <Key Time="1"/> <Key Time="2.5"/>
	 *        </Track>
	 *        <Marker Time="6" Label="Drop"/>
	 *    </Timeline>
	 *
	 *  Times are in seconds. Tracks and items (clips, keys, markers) are
	 *  named by the integers the add functions return.
	 * ============================================================================ */

	/** What an item of a timeline is. */
	typedef enum RTimelineItem{
		R_TIMELINE_NONE = 0x0460,
		R_TIMELINE_CLIP,
		R_TIMELINE_KEY,
		R_TIMELINE_MARKER
	}RTimelineItem;

	RAVEN_API Rid Rtimeline_create(void);

	/* ---- time ---- */

	/** @brief Length of the sequence. The view and the playhead stay within 0 .. duration. */
	RAVEN_API void Rtimeline_set_duration(Rid timeline, double seconds);
	RAVEN_API double Rtimeline_get_duration(Rid timeline);
	/** @brief Moves the playhead. The time callback is not called. */
	RAVEN_API void Rtimeline_set_time(Rid timeline, double seconds);
	RAVEN_API double Rtimeline_get_time(Rid timeline);
	/**
	 * @brief Frames per second. With a frame rate the ruler counts in
	 * frames ("2:12" is second 2, frame 12) and everything snaps to frames;
	 * 0 (default) counts in seconds and snaps to the marks of the ruler.
	 */
	RAVEN_API void Rtimeline_set_frame_rate(Rid timeline, double frames_per_second);
	RAVEN_API double Rtimeline_get_frame_rate(Rid timeline);
	/** @brief R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move). */
	RAVEN_API void Rtimeline_set_snapping(Rid timeline, int enabled);
	/** @brief The part of the sequence that is shown. Zooming is showing less. */
	RAVEN_API void Rtimeline_set_view(Rid timeline, double start, double end);
	RAVEN_API void Rtimeline_get_view(Rid timeline, double* start, double* end);
	/**
	 * @brief Makes the playhead run by itself, at the speed of the clock,
	 * calling the time callback at each frame; at the end it stops, or
	 * starts again with Rtimeline_set_loop.
	 */
	RAVEN_API void Rtimeline_set_playing(Rid timeline, int playing);
	RAVEN_API int Rtimeline_is_playing(Rid timeline);
	RAVEN_API void Rtimeline_set_loop(Rid timeline, int loop);

	/* ---- sizes ---- */

	/*
	 * By default the sizes of the timeline follow the theme (its row height)
	 * and the text scale of the accessibility settings. These set them in
	 * logical units; 0 goes back to the automatic size.
	 */
	/** @brief Width of the column of the names of the tracks. 0 (default): fits the longest name. */
	RAVEN_API void Rtimeline_set_header_width(Rid timeline, float width);
	/** @return The width of the column of the names, as laid out now. */
	RAVEN_API float Rtimeline_get_header_width(Rid timeline);
	/** @brief Height of the ruler of time (and of the markers' flags band). 0 (default): automatic. */
	RAVEN_API void Rtimeline_set_ruler_height(Rid timeline, float height);
	RAVEN_API float Rtimeline_get_ruler_height(Rid timeline);
	/** @brief Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic. */
	RAVEN_API void Rtimeline_set_default_track_height(Rid timeline, float height);
	RAVEN_API float Rtimeline_get_default_track_height(Rid timeline);

	/* ---- tracks ---- */

	/** @return The track, or 0. Tracks are shown in the order they were added. */
	RAVEN_API int Rtimeline_add_track(Rid timeline, const char* name);
	/** @brief Removes a track and everything on it. */
	RAVEN_API void Rtimeline_remove_track(Rid timeline, int track);
	RAVEN_API int Rtimeline_get_track_count(Rid timeline);
	/** @return The index-th track from the top, or 0. */
	RAVEN_API int Rtimeline_get_track(Rid timeline, int index);
	RAVEN_API void Rtimeline_set_track_name(Rid timeline, int track, const char* name);
	RAVEN_API const char* Rtimeline_get_track_name(Rid timeline, int track);
	/** @brief Color of the clips and keys of the track that have none of their own. */
	RAVEN_API void Rtimeline_set_track_color(Rid timeline, int track, float r, float g, float b);
	/** @brief Height of the track in logical units; 0 = the default. */
	RAVEN_API void Rtimeline_set_track_height(Rid timeline, int track, float height);
	/** @brief A locked track shows its items but does not let the user change them. */
	RAVEN_API void Rtimeline_set_track_locked(Rid timeline, int track, int locked);
	/** @brief A muted track is drawn dimmed; what muting means is up to the application. */
	RAVEN_API void Rtimeline_set_track_muted(Rid timeline, int track, int muted);
	RAVEN_API int Rtimeline_is_track_muted(Rid timeline, int track);

	/* ---- items ---- */

	/** @return The clip, or 0 when the track does not exist. */
	RAVEN_API int Rtimeline_add_clip(Rid timeline, int track, double start, double length, const char* label);
	/** @return The key, or 0. */
	RAVEN_API int Rtimeline_add_key(Rid timeline, int track, double time);
	/** @brief A named moment of the whole sequence, shown on the ruler and across the tracks. */
	RAVEN_API int Rtimeline_add_marker(Rid timeline, double time, const char* label);
	RAVEN_API void Rtimeline_remove_item(Rid timeline, int item);
	/** @brief Removes every track, clip, key and marker. */
	RAVEN_API void Rtimeline_clear(Rid timeline);

	RAVEN_API RTimelineItem Rtimeline_get_item_type(Rid timeline, int item);
	/** @return The track of a clip or of a key; 0 for a marker. */
	RAVEN_API int Rtimeline_get_item_track(Rid timeline, int item);
	RAVEN_API double Rtimeline_get_item_start(Rid timeline, int item);
	/** @return The length of a clip; 0 for a key or a marker. */
	RAVEN_API double Rtimeline_get_item_length(Rid timeline, int item);
	/** @brief Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep). */
	RAVEN_API void Rtimeline_set_item(Rid timeline, int item, int track, double start, double length);
	RAVEN_API void Rtimeline_set_item_label(Rid timeline, int item, const char* label);
	RAVEN_API const char* Rtimeline_get_item_label(Rid timeline, int item);
	RAVEN_API void Rtimeline_set_item_color(Rid timeline, int item, float r, float g, float b);
	RAVEN_API int Rtimeline_get_item_count(Rid timeline);
	/** @return The index-th item, in the order they were added, or 0. */
	RAVEN_API int Rtimeline_get_item(Rid timeline, int index);

	/* ---- selection ---- */

	/** @brief Selects an item, alone or (add) with the ones already selected. */
	RAVEN_API void Rtimeline_select(Rid timeline, int item, int add);
	RAVEN_API void Rtimeline_clear_selection(Rid timeline);
	RAVEN_API int Rtimeline_is_selected(Rid timeline, int item);
	RAVEN_API int Rtimeline_get_selection_count(Rid timeline);
	RAVEN_API int Rtimeline_get_selected(Rid timeline, int index);

	/* ---- callbacks ---- */

	/** @brief The playhead moved: scrubbed by the user, or running by itself. */
	typedef void (*RTimelineTimeCallback)(Rid timeline, double seconds, void* user_data);
	RAVEN_API void Rtimeline_set_time_callback(Rid timeline, RTimelineTimeCallback callback, void* user_data);
	/** @brief Something happened to an item; which thing depends on the function the callback was given to. */
	typedef void (*RTimelineItemCallback)(Rid timeline, int item, void* user_data);
	/**
	 * @brief Called for each item the user moved, stretched or retimed, as
	 * it happens. The action of the widget (OnChanged in XML) fires once
	 * per gesture, when the button is released.
	 */
	RAVEN_API void Rtimeline_set_changed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
	/** @brief Called when the selection changed; `item` is the item just clicked, or 0. */
	RAVEN_API void Rtimeline_set_selection_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
	/** @brief Called when an item is double-clicked. */
	RAVEN_API void Rtimeline_set_activated_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
	/**
	 * @brief Called when the user deletes an item (the Delete key), before
	 * it is removed: its values can still be read. Without this callback the
	 * Delete key does nothing: the application decides what can be deleted.
	 */
	RAVEN_API void Rtimeline_set_removed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);


	/* ============================================================================
	 *  NODE GRAPH
	 *
	 *  Nodes on a canvas that pans (drag the background, or the middle
	 *  button) and zooms (the wheel). A node has a title, input pins on its
	 *  left, output pins on its right, and may hold any widget (a slider, a
	 *  color picker, a preview). The user links an output to an input by
	 *  dragging from one pin to the other; dragging a link away from its
	 *  input removes it. Links are wires: they hang between their pins,
	 *  swing when a node is moved and settle, and carry a slow flow from
	 *  their output to their input.
	 *
	 *    <NodeGraph Name="graph">
	 *        <Node Name="noise" Title="Noise" X="40" Y="60" Color="#FF9F0A">
	 *            <Input Name="scale" Label="Scale" Type="number"/>
	 *            <Output Name="value" Label="Value" Type="number"/>
	 *            <Slider Value="0.4"/>
	 *        </Node>
	 *        <Node Name="blend" Title="Blend" X="320" Y="90">
	 *            <Input Name="a" Label="A" Type="number"/>
	 *            <Output Name="out" Label="Result" Type="number"/>
	 *        </Node>
	 *        <Link From="noise.value" To="blend.a"/>
	 *    </NodeGraph>
	 *
	 *  Pins have a type, a free text ("number", "color", "mesh"...): an
	 *  output only links to an input of the same type, or to a pin without
	 *  a type. Each type has a color (Rnode_graph_set_type_color, or one
	 *  Raven picks from its name); a link shades from the color of one pin
	 *  to the color of the other. An input takes one link (a new one
	 *  replaces it); an output feeds as many as wanted.
	 *
	 *  Nodes, pins and links are named by the integers the functions
	 *  return, all different within one graph.
	 * ============================================================================ */

	RAVEN_API Rid Rnode_graph_create(void);

	/* ---- nodes ---- */

	/** @param x,y Top-left corner of the node, in the units of the canvas. */
	RAVEN_API int Rnode_graph_add_node(Rid graph, const char* title, float x, float y);
	/** @brief Removes a node, its pins, their links, and the widget it holds. */
	RAVEN_API void Rnode_graph_remove_node(Rid graph, int node);
	RAVEN_API void Rnode_graph_clear(Rid graph);
	RAVEN_API int Rnode_graph_get_node_count(Rid graph);
	RAVEN_API int Rnode_graph_get_node(Rid graph, int index);
	RAVEN_API void Rnode_graph_set_node_title(Rid graph, int node, const char* title);
	RAVEN_API const char* Rnode_graph_get_node_title(Rid graph, int node);
	RAVEN_API void Rnode_graph_set_node_position(Rid graph, int node, float x, float y);
	RAVEN_API void Rnode_graph_get_node_position(Rid graph, int node, float* x, float* y);
	/** @brief Size of a node on the canvas: Raven works it out from its title, its pins and its content. */
	RAVEN_API void Rnode_graph_get_node_size(Rid graph, int node, float* width, float* height);
	/** @brief Color of the header of the node; without one it takes the accent color of the theme. */
	RAVEN_API void Rnode_graph_set_node_color(Rid graph, int node, float r, float g, float b);
	/**
	 * @brief Puts a widget inside the node, under its pins: the graph owns
	 * it from now on. R_INVALID_ID removes (and deletes) the one it had.
	 */
	RAVEN_API void Rnode_graph_set_node_content(Rid graph, int node, Rid widget);
	RAVEN_API Rid Rnode_graph_get_node_content(Rid graph, int node);
	/**
	 * @brief Name of the node, the `Name` of its XML element: what
	 * <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
	 */
	RAVEN_API void Rnode_graph_set_node_name(Rid graph, int node, const char* name);
	RAVEN_API const char* Rnode_graph_get_node_name(Rid graph, int node);
	/** @return The node of that name, or 0. */
	RAVEN_API int Rnode_graph_find_node(Rid graph, const char* name);
	/** @brief Value the application attaches to a node. */
	RAVEN_API void Rnode_graph_set_node_user_data(Rid graph, int node, void* user_data);
	RAVEN_API void* Rnode_graph_get_node_user_data(Rid graph, int node);

	/* ---- pins ---- */

	/** @param type Free text, or NULL for a pin that links to any type. @return The pin, or 0. */
	RAVEN_API int Rnode_graph_add_input(Rid graph, int node, const char* label, const char* type);
	RAVEN_API int Rnode_graph_add_output(Rid graph, int node, const char* label, const char* type);
	RAVEN_API void Rnode_graph_remove_pin(Rid graph, int pin);
	RAVEN_API int Rnode_graph_get_pin_node(Rid graph, int pin);
	RAVEN_API int Rnode_graph_is_pin_output(Rid graph, int pin);
	RAVEN_API const char* Rnode_graph_get_pin_label(Rid graph, int pin);
	RAVEN_API const char* Rnode_graph_get_pin_type(Rid graph, int pin);
	/** @brief Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label. */
	RAVEN_API void Rnode_graph_set_pin_name(Rid graph, int pin, const char* name);
	/** @return The pin of a node with that name (or label), or 0. */
	RAVEN_API int Rnode_graph_find_pin(Rid graph, int node, const char* name);
	/** @brief Where a pin is on the canvas (the middle of its dot). */
	RAVEN_API void Rnode_graph_get_pin_position(Rid graph, int pin, float* x, float* y);
	/** @return The index-th input (or output) of a node, or 0. */
	RAVEN_API int Rnode_graph_get_input(Rid graph, int node, int index);
	RAVEN_API int Rnode_graph_get_output(Rid graph, int node, int index);
	/** @brief Color of the pins of a type and of the links between them. */
	RAVEN_API void Rnode_graph_set_type_color(Rid graph, const char* type, float r, float g, float b);

	/* ---- links ---- */

	/**
	 * @brief Links an output to an input, as the user does by dragging.
	 * The connect callback is not asked.
	 * @return The link; 0 when the pins cannot be linked (two inputs, the
	 * same node, different types).
	 */
	RAVEN_API int Rnode_graph_connect(Rid graph, int output_pin, int input_pin);
	RAVEN_API void Rnode_graph_disconnect(Rid graph, int link);
	RAVEN_API int Rnode_graph_get_link_count(Rid graph);
	RAVEN_API int Rnode_graph_get_link(Rid graph, int index);
	RAVEN_API int Rnode_graph_get_link_output(Rid graph, int link);
	RAVEN_API int Rnode_graph_get_link_input(Rid graph, int link);
	/** @return The link that arrives at an input pin, or 0. */
	RAVEN_API int Rnode_graph_get_input_link(Rid graph, int input_pin);

	/* ---- view and selection ---- */

	/** @brief What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit). */
	RAVEN_API void Rnode_graph_set_view(Rid graph, float center_x, float center_y, float zoom);
	RAVEN_API void Rnode_graph_get_view(Rid graph, float* center_x, float* center_y, float* zoom);
	/** @brief Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now. */
	RAVEN_API void Rnode_graph_to_canvas(Rid graph, float x, float y, float* canvas_x, float* canvas_y);
	RAVEN_API void Rnode_graph_from_canvas(Rid graph, float canvas_x, float canvas_y, float* x, float* y);
	/** @brief Moves and zooms so that every node shows. */
	RAVEN_API void Rnode_graph_frame_all(Rid graph);
	/** @brief R_FALSE hides the grid of dots (shown by default). */
	RAVEN_API void Rnode_graph_set_grid_visible(Rid graph, int visible);
	/**
	 * @brief The pulses of light that run along the links, from output to
	 * input. By default they run while the pointer is over the graph, and
	 * the window rests the rest of the time. R_TRUE makes them run all the
	 * time (the window then draws without pause); R_FALSE stops them.
	 */
	RAVEN_API void Rnode_graph_set_flow_visible(Rid graph, int visible);
	/** @brief R_TRUE: the user can look but not move, link or delete. */
	RAVEN_API void Rnode_graph_set_read_only(Rid graph, int read_only);

	RAVEN_API void Rnode_graph_select_node(Rid graph, int node, int add);
	RAVEN_API void Rnode_graph_clear_selection(Rid graph);
	RAVEN_API int Rnode_graph_is_node_selected(Rid graph, int node);
	RAVEN_API int Rnode_graph_get_selection_count(Rid graph);
	RAVEN_API int Rnode_graph_get_selected_node(Rid graph, int index);

	/* ---- callbacks ---- */

	/**
	 * @brief Asked when the user is about to link two pins that may be
	 * linked: return R_FALSE to refuse (a loop, a rule of the application).
	 */
	typedef int (*RNodeGraphConnectCallback)(Rid graph, int output_pin, int input_pin, void* user_data);
	RAVEN_API void Rnode_graph_set_connect_callback(Rid graph, RNodeGraphConnectCallback callback, void* user_data);
	/** @brief Something happened to a link or to a node; which thing depends on the function the callback was given to. */
	typedef void (*RNodeGraphItemCallback)(Rid graph, int item, void* user_data);
	/** @brief Called after the user made a link, and before a link the user removed goes (its pins can still be read). */
	RAVEN_API void Rnode_graph_set_linked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
	RAVEN_API void Rnode_graph_set_unlinked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
	/** @brief Called for each node the user moved, when the button is released. */
	RAVEN_API void Rnode_graph_set_moved_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
	/** @brief Called when the selection changed; `item` is the node just clicked, or 0. */
	RAVEN_API void Rnode_graph_set_selection_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
	/**
	 * @brief Called when the user deletes a node (the Delete key), before
	 * it is removed. Without this callback the Delete key only removes
	 * links: the application decides whether nodes can be deleted.
	 */
	RAVEN_API void Rnode_graph_set_removed_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
	/** @brief Called when the user double-clicks a node (to open what it stands for). */
	RAVEN_API void Rnode_graph_set_activated_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
	/**
	 * @brief Right click: `node` is the node under the pointer (selected first),
	 * or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
	 * Without this callback the right button does nothing.
	 */
	typedef void (*RNodeGraphContextCallback)(Rid graph, int node, float x, float y, void* user_data);
	RAVEN_API void Rnode_graph_set_context_callback(Rid graph, RNodeGraphContextCallback callback, void* user_data);

	/* ---- more for editors ---- */

	/**
	 * @brief R_TRUE: this input keeps every link made to it (a state that several
	 * transitions enter) instead of one. Pulling a wire from it starts a new one.
	 */
	RAVEN_API void Rnode_graph_set_pin_multiple(Rid graph, int pin, int multiple);
	/** @brief R_TRUE: the node is running (live debug) : a soft ring breathes around it. */
	RAVEN_API void Rnode_graph_set_node_active(Rid graph, int node, int active);
	/** @brief Color of that ring (the success color of the theme by default). */
	RAVEN_API void Rnode_graph_set_node_active_color(Rid graph, int node, float r, float g, float b);
	/** @brief R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph). */
	RAVEN_API void Rnode_graph_set_node_deletable(Rid graph, int node, int deletable);
	/** @brief A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it. */
	RAVEN_API void Rnode_graph_set_node_badge(Rid graph, int node, const char* text);

	/* ============================================================================
	 *  Block editor
	 *
	 *  Programs made of blocks that snap into each other, in the style of
	 *  Scratch: a palette on the left (one block of each kind, by category),
	 *  the scripts on a canvas that pans (drag the background, middle button)
	 *  and zooms (Ctrl + wheel).
	 *
	 *  A kind has a label: words and slots. %n a number, %s a text, %b a
	 *  condition, %r any value, %m a choice in a menu (its choices :
	 *  Rblock_editor_set_slot_options); {text} right after a slot is its default;
	 *  | separates the sections of a C block (each one is followed by a
	 *  mouth): "repeat %n{10}", "if %b then|else", "%n + %n".
	 *
	 *  Blocks are named by integers (> 0). A block hangs under another one
	 *  (R_BLOCK_NEXT), in a mouth of a C block (R_BLOCK_BODY, index = the
	 *  mouth) or in a value slot (R_BLOCK_INPUT, index = the slot), or is
	 *  the top of a script of its own.
	 *
	 *  The user drags a block with what is under it; the place it would go
	 *  opens and shows its shadow; dropped on the palette, it is deleted.
	 *  A left click selects a block (Ctrl / Shift : adds it), a left drag on
	 *  the canvas draws a selection rectangle, and dragging a block of the
	 *  selection moves the whole selection; Delete removes it. A right drag
	 *  (or middle, or Alt + left) pans, a right click opens the context menu.
	 *  A click on a script fires the clicked callback (to run it); the
	 *  application makes the running blocks glow.
	 * ============================================================================ */

	typedef enum RBlockShape {
		/** A step: snaps under another one, others snap under it. */
		R_BLOCK_STATEMENT = 0x0700,
		/** Starts a script ("when the game starts"): nothing goes above it. */
		R_BLOCK_HAT,
		/** Ends a script ("stop"): nothing goes under it. */
		R_BLOCK_CAP,
		/** Wraps stacks in its mouths ("repeat", "if ... else"). */
		R_BLOCK_C,
		/** A rounded value that goes into a slot. */
		R_BLOCK_REPORTER,
		/** A hexagonal condition that goes into a %b slot (or a %r one). */
		R_BLOCK_BOOLEAN
	} RBlockShape;

	typedef enum RBlockPlace {
		R_BLOCK_NEXT = 0x0710,
		R_BLOCK_BODY,
		R_BLOCK_INPUT
	} RBlockPlace;

	/** @brief A block changed (0: several, or one was deleted), or a script was clicked (its top block). */
	typedef void (*RBlockEditorCallback)(Rid editor, int block, void* user_data);
	/** @brief Right click: on a block (0 on the canvas), at x, y in the widget. */
	typedef void (*RBlockEditorContextCallback)(Rid editor, int block, float x, float y, void* user_data);

	RAVEN_API Rid Rblock_editor_create(void);
	/** @brief A kind of block, and its block in the palette. Returns its index, -1 on an error. */
	RAVEN_API int Rblock_editor_add_kind(Rid editor, const char* name, const char* label, RBlockShape shape, const char* category);
	/** @brief Color of the blocks of a category (Raven picks one from its name otherwise). */
	RAVEN_API void Rblock_editor_set_category_color(Rid editor, const char* category, float r, float g, float b);
	RAVEN_API void Rblock_editor_set_palette_visible(Rid editor, int visible);

	/** @brief A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error. */
	RAVEN_API int Rblock_editor_add_block(Rid editor, const char* kind, float x, float y);
	/** @brief Removes a block and everything that hangs under it. */
	RAVEN_API void Rblock_editor_remove_block(Rid editor, int block);
	RAVEN_API void Rblock_editor_clear(Rid editor);
	/** @brief Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there. */
	RAVEN_API int Rblock_editor_attach(Rid editor, int block, int target, RBlockPlace place, int index);
	/** @brief Takes a block (with what is under it) out: a script of its own at x, y. */
	RAVEN_API void Rblock_editor_detach(Rid editor, int block, float x, float y);

	RAVEN_API int Rblock_editor_get_script_count(Rid editor);
	/** @brief The top block of a script. */
	RAVEN_API int Rblock_editor_get_script(Rid editor, int index);
	RAVEN_API const char* Rblock_editor_get_kind(Rid editor, int block);
	/** @brief The block under it, 0 if none. */
	RAVEN_API int Rblock_editor_get_next(Rid editor, int block);
	RAVEN_API int Rblock_editor_get_mouth_count(Rid editor, int block);
	/** @brief The first block in a mouth of a C block, 0 if empty. */
	RAVEN_API int Rblock_editor_get_body(Rid editor, int block, int mouth);
	RAVEN_API int Rblock_editor_get_slot_count(Rid editor, int block);
	/** @brief The value block in a slot, 0 if the slot holds a typed value. */
	RAVEN_API int Rblock_editor_get_input(Rid editor, int block, int slot);
	/** @brief The typed value of a slot. */
	RAVEN_API const char* Rblock_editor_get_value(Rid editor, int block, int slot);
	RAVEN_API void Rblock_editor_set_value(Rid editor, int block, int slot, const char* value);
	/** @brief Where a block hangs: the block it hangs from (0: the top of a script). */
	RAVEN_API int Rblock_editor_get_parent(Rid editor, int block, RBlockPlace* place, int* index);
	RAVEN_API void Rblock_editor_get_position(Rid editor, int block, float* x, float* y);
	/** @brief R_TRUE: the block is running: a halo breathes around it. */
	RAVEN_API void Rblock_editor_set_block_glow(Rid editor, int block, int glow);

	/** @brief Every script as text (valid until the next call). */
	RAVEN_API const char* Rblock_editor_save(Rid editor);
	/** @brief Replaces the scripts with those of a text from Rblock_editor_save. */
	RAVEN_API int Rblock_editor_load(Rid editor, const char* text);

	RAVEN_API void Rblock_editor_set_changed_callback(Rid editor, RBlockEditorCallback callback, void* user_data);
	RAVEN_API void Rblock_editor_set_clicked_callback(Rid editor, RBlockEditorCallback callback, void* user_data);
	RAVEN_API void Rblock_editor_set_context_callback(Rid editor, RBlockEditorContextCallback callback, void* user_data);

	/** @brief The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in). */
	RAVEN_API void Rblock_editor_set_slot_options(Rid editor, const char* kind, int slot, const char* options);
	/** @brief R_FALSE: the kind is not in the palette (its blocks stay where they are). */
	RAVEN_API void Rblock_editor_set_kind_visible(Rid editor, const char* kind, int visible);
	/**
	 * @brief A new label for a kind (a function that takes one more input...).
	 * The blocks of that kind keep the values of the slots that are still there.
	 */
	RAVEN_API int Rblock_editor_set_kind_label(Rid editor, const char* kind, const char* label);
	/** @brief R_FALSE: the user cannot delete the block (the entry of a function). Not saved. */
	RAVEN_API void Rblock_editor_set_block_deletable(Rid editor, int block, int deletable);

	RAVEN_API int Rblock_editor_get_selection_count(Rid editor);
	RAVEN_API int Rblock_editor_get_selected(Rid editor, int index);
	/** @brief Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears). */
	RAVEN_API void Rblock_editor_select(Rid editor, int block, int add);

	/** @brief Pan (canvas units at the top left) and zoom. */
	RAVEN_API void Rblock_editor_get_view(Rid editor, float* x, float* y, float* zoom);
	RAVEN_API void Rblock_editor_set_view(Rid editor, float x, float y, float zoom);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_TOOLS_H */
