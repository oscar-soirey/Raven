/**
 * Raven
 * raven_tools.h : the widgets of creative tools.
 *
 *   R_WIDGET_COLOR_PICKER   chooses a color
 *   R_WIDGET_TIMELINE       tracks of clips and keys along a time ruler (a sequencer)
 *   R_WIDGET_NODE_GRAPH     nodes linked by wires
 *   R_WIDGET_BLOCK_EDITOR   blocks that snap into each other (Scratch)
 *   R_WIDGET_DIFF_VIEW      the differences between two texts, or a patch
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
	 *  LOOKS OF THE TIMELINE AND OF THE NODE GRAPH
	 *
	 *  Both start from a look (Preset="..." in XML): AUTO follows the theme
	 *  of the window, its platform (Raven, Windows, macOS, GNOME) and whether
	 *  it is dark; the others are looks of their own. Then three levels,
	 *  the most specific last, change any part of it:
	 *
	 *  - the styles of the theme, for every graph or timeline (or a class):
	 *      <Style Widget="NodeGraph" Part="Link" Shape="Step" Size="2"/>
	 *      <Style Widget="Timeline" Part="Clip" State="Selected" BorderColor="#FFF"/>
	 *  - the attributes of the widget: a part, a dot, a property
	 *      <NodeGraph Preset="Blueprint" Link.Shape="Angled" Pin.Size="6" Grid.Shape="Lines">
	 *  - one item: a node, a type of pin, a pin, a track, a clip, a key
	 *      <Node Title="Noise" Color="#FF9F0A" HeaderStyle="Gradient" Icon="noise.svg">
	 *      <Type Name="exec" Color="#FFFFFF" Shape="Arrow"/>
	 *      <Track Name="Audio" ClipStyle="Outline" KeyShape="Circle">
	 *
	 *  Parts: NodeGraph: the widget itself (Body), Grid, Node, Header, Pin,
	 *  Link, Indicator (the badge); Timeline: Body, Ruler, Header (the column
	 *  of the names of the tracks), Track, Grid, Clip, Key, Marker,
	 *  Playhead. States: Hovered, Selected (nodes, links, clips, keys).
	 *  Properties: those of every style (Background, Foreground,
	 *  BorderColor, BorderWidth, CornerRadius, Size, Opacity, ShadowColor,
	 *  ShadowSize) and Shape, Glow (0..1, light around it), Gradient (the
	 *  color it fades to). Shapes: Circle, Ring, Square, Diamond, Triangle,
	 *  Arrow (pins, keys); Bezier, Straight, Step, Angled (links); Dots,
	 *  Lines, Cross, None (grids); Solid, Gradient, Stripe, Tint, Outline,
	 *  None (headers of nodes, clips).
	 * ============================================================================ */

	typedef enum RVisualPreset{
		/** The look of the theme of the window: its platform, dark or light. */
		R_PRESET_AUTO = 0x0510,
		/** Cards with soft shadows, few lines (follows dark or light). */
		R_PRESET_MODERN,
		/** Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows. */
		R_PRESET_BLUEPRINT,
		/** Compact nodes, flat headers in muted colors, thin links. */
		R_PRESET_BLENDER,
		/** Almost black, outlines and links of light. */
		R_PRESET_NEON
	}RVisualPreset;


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
	/**
	 * @brief A property of the look of a track: "ClipStyle" (Solid,
	 * Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
	 * "KeySize", "ClipRadius", "Background" (its lane), or a part and a
	 * property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
	 * the one of the timeline.
	 * @return R_FALSE when the name or the value is not one.
	 */
	RAVEN_API int Rtimeline_set_track_style(Rid timeline, int track, const char* property, const char* value);
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
	/**
	 * @brief A property of the look of one clip, key or marker: "Style"
	 * (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
	 * property of its part ("Background", "BorderColor", "Glow",
	 * "CornerRadius", "Foreground" for its label...).
	 */
	RAVEN_API int Rtimeline_set_item_style(Rid timeline, int item, const char* property, const char* value);
	/** @brief The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML). */
	RAVEN_API void Rtimeline_set_preset(Rid timeline, RVisualPreset preset);
	RAVEN_API RVisualPreset Rtimeline_get_preset(Rid timeline);
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
	 * @brief A property of the look of one node: "HeaderStyle" (Solid,
	 * Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
	 * "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
	 * "Icon" (an image file shown before its title), or a part and a property
	 * ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
	 * one of the graph.
	 * @return R_FALSE when the name or the value is not one.
	 */
	RAVEN_API int Rnode_graph_set_node_style(Rid graph, int node, const char* property, const char* value);
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
	/**
	 * @brief A property of the pins of a type and of their links: "Shape"
	 * (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
	 * "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
	 */
	RAVEN_API int Rnode_graph_set_type_style(Rid graph, const char* type, const char* property, const char* value);
	/** @brief The same for one pin ("Shape", "Size", "Glow", "Color"). */
	RAVEN_API int Rnode_graph_set_pin_style(Rid graph, int pin, const char* property, const char* value);

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
	/** @brief The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML). */
	RAVEN_API void Rnode_graph_set_preset(Rid graph, RVisualPreset preset);
	RAVEN_API RVisualPreset Rnode_graph_get_preset(Rid graph);

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
	/**
	 * @brief Something happened at a point of the canvas: `item` depends on the
	 * function the callback was given to; x, y are in canvas units (as node positions).
	 */
	typedef void (*RNodeGraphPointCallback)(Rid graph, int item, float x, float y, void* user_data);
	/**
	 * @brief Double click on the empty canvas (`item` 0): the application acts
	 * there (a search to add a node...). Without this callback it frames all nodes.
	 */
	RAVEN_API void Rnode_graph_set_canvas_activated_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data);
	/**
	 * @brief A wire the user pulled from a pin was released on nothing: `item` is
	 * that pin, x, y where it was dropped (to offer a node to connect it to). The
	 * wire draws back as usual.
	 */
	RAVEN_API void Rnode_graph_set_wire_dropped_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data);
	/**
	 * @brief A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
	 * the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
	 * the second may go). With this callback such a click neither detaches the wire
	 * of an input nor counts as a wire dropped on nothing.
	 */
	RAVEN_API void Rnode_graph_set_pin_clicked_callback(Rid graph, RNodeGraphPointCallback callback, void* user_data);
	/** @brief The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0. */
	RAVEN_API int Rnode_graph_get_node_at(Rid graph, float x, float y);
	/** @brief The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin). */
	RAVEN_API int Rnode_graph_get_pin_at(Rid graph, float x, float y);
	/** @brief The link under a point of the widget, or 0 (hidden links are never under it). */
	RAVEN_API int Rnode_graph_get_link_at(Rid graph, float x, float y);

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
	/** @brief Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph). */
	RAVEN_API void Rnode_graph_set_node_shown(Rid graph, int node);
	/** @brief The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output. */
	RAVEN_API void Rnode_graph_set_link_shown(Rid graph, int link);
	/** @brief An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it. */
	RAVEN_API void Rnode_graph_set_node_icon(Rid graph, int node, Rid texture);
	/** @brief A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it. */
	RAVEN_API void Rnode_graph_set_node_badge(Rid graph, int node, const char* text);
	/**
	 * @brief A short text in a chip of the pin's color outside the node, beside the pin
	 * (left of an input, right of an output): the name of what goes out or comes in
	 * there, as a net label of a schematic. NULL or "" removes it.
	 */
	RAVEN_API void Rnode_graph_set_pin_tag(Rid graph, int pin, const char* text);
	/**
	 * @brief R_TRUE: the link stays (its pins are linked, the application keeps
	 * it) but is not drawn and not picked: its two ends are shown otherwise (pin
	 * tags naming it, as the labels of a schematic stand for a wire).
	 */
	RAVEN_API void Rnode_graph_set_link_hidden(Rid graph, int link, int hidden);
	/**
	 * @brief How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
	 * to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
	 */
	RAVEN_API void Rnode_graph_set_pin_emphasis(Rid graph, int pin, int emphasis);
	/** @brief The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin. */
	RAVEN_API int Rnode_graph_get_tag_at(Rid graph, float x, float y);
	/**
	 * @brief R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
	 * thicker edge) — every tag of one net lit while the pointer is on one of them.
	 */
	RAVEN_API void Rnode_graph_set_pin_tag_lit(Rid graph, int pin, int lit);
	/**
	 * @brief Pin labels wider than `width` (logical units) are cut short with an
	 * ellipsis and the nodes are laid out with them so (compact ports); 0, the
	 * default, shows every label whole.
	 */
	RAVEN_API void Rnode_graph_set_pin_label_width(Rid graph, float width);
	RAVEN_API float Rnode_graph_get_pin_label_width(Rid graph);
	/** @brief Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. A negative component restores the look's color. */
	RAVEN_API void Rnode_graph_set_node_badge_color(Rid graph, int node, float r, float g, float b);

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

	/* ============================================================================
	 *  Diff view
	 *
	 *  The differences between two texts (two versions of a file, of a
	 *  setting, of a document), or a patch as `git diff` writes it: the lines
	 *  removed and the lines added, in one column (unified) or side by side
	 *  (split), with the line numbers of both sides, the words that changed
	 *  inside a changed line, and the unchanged lines folded away except a
	 *  few around each change ("42 unchanged lines": a click shows them).
	 *
	 *    <DiffView Mode="Split" LeftSource="old/main.c" RightSource="new/main.c" Context="3"/>
	 *
	 *  Only the rows on screen are drawn: a file of a hundred thousand lines
	 *  scrolls as one of ten. In split mode both sides are one list: they
	 *  scroll together, and the lines of a change face each other. The widget
	 *  is read-only: the text can be selected and copied (Ctrl+C / Cmd+C),
	 *  F7 / Shift+F7 (or Alt+Down / Alt+Up) go to the next / previous change.
	 *  With a language (Rdiff_view_set_language, or the extension of the
	 *  names), the code is colored as in the text editor.
	 *
	 *  The comparison (Myers' algorithm, linear in memory) finds the fewest
	 *  lines removed and added; lines found on one side only are set apart
	 *  first, so two texts with nothing in common cost no more than reading
	 *  them, and a comparison that would take too long (very many small
	 *  changes in very long texts) settles for a result that is a little
	 *  longer than the shortest one rather than freezing.
	 * ============================================================================ */

	typedef enum RDiffViewMode {
		/** One column: the removed lines, then the added lines, between the lines kept. */
		R_DIFF_VIEW_UNIFIED = 0x0C50,
		/** Two columns: the left text and the right text side by side, aligned. */
		R_DIFF_VIEW_SPLIT
	} RDiffViewMode;

	typedef enum RDiffSide {
		/** The text before (the old one, "a/", the lines removed). */
		R_DIFF_LEFT = 0x0C54,
		/** The text after (the new one, "b/", the lines added). */
		R_DIFF_RIGHT
	} RDiffSide;

	typedef enum RDiffRowKind {
		/** A line that is on both sides. */
		R_DIFF_ROW_SAME = 0x0C58,
		/** A line of the left text only. */
		R_DIFF_ROW_REMOVED,
		/** A line of the right text only. */
		R_DIFF_ROW_ADDED,
		/** Split mode: a removed line facing the added line that replaces it. */
		R_DIFF_ROW_CHANGED,
		/** Unchanged lines folded away ("12 unchanged lines"); Rdiff_view_expand shows them. */
		R_DIFF_ROW_FOLD,
		/** A patch: the header of a hunk ("@@ -12,7 +12,8 @@ main"). */
		R_DIFF_ROW_HUNK,
		/** A patch: the names of a file. */
		R_DIFF_ROW_FILE,
		/** A note attached to the line above it (Rdiff_view_add_annotation). */
		R_DIFF_ROW_NOTE
	} RDiffRowKind;

	/** How lines are compared (Rdiff_view_set_flags, Rdiff_unified). */
	typedef enum RDiffFlags {
		/** Spaces and tabs at the end of a line (and a carriage return) do not count. */
		R_DIFF_IGNORE_TRAILING_SPACE = (1 << 0),
		/** No space or tab counts, wherever it is. */
		R_DIFF_IGNORE_SPACE          = (1 << 1),
		/** Upper and lower case letters are the same (A to Z). */
		R_DIFF_IGNORE_CASE           = (1 << 2)
	} RDiffFlags;

	/**
	 * A line, or its number in the gutter, was clicked. `side` is the side
	 * of the line clicked (in unified mode: the left side for a removed line
	 * and for the left column of numbers, else the right side), `line` its
	 * number on that side, from 1.
	 */
	typedef void (*RDiffViewLineCallback)(Rid diff_view, RDiffSide side, int line, int in_gutter, void* user_data);

	/**
	 * @brief The differences between two texts as a unified diff (the format
	 * of `diff -u` and `git diff`), with `context` unchanged lines around each
	 * change (3 is usual). `left_name` and `right_name` follow `---` and `+++`
	 * (NULL: "a" and "b"). `flags`: RDiffFlags. It touches nothing else of
	 * the library: it may be called from any thread.
	 * @return The patch followed by a '\0' (an empty text when the texts are
	 * the same), to release with Rmemory_free; NULL when a text is NULL.
	 * `size` receives its length in bytes, and may be NULL.
	 */
	RAVEN_API char* Rdiff_unified(const char* left, const char* right, const char* left_name, const char* right_name, int context,
		uint32_t flags, size_t* size);
	/** @brief The number of changes (runs of lines removed and / or added) between two texts. Any thread. */
	RAVEN_API int Rdiff_count_changes(const char* left, const char* right, uint32_t flags);

	RAVEN_API Rid Rdiff_view_create(void);
	/** @brief The two texts to compare (NULL: empty). Lines end with "\n" or "\r\n". */
	RAVEN_API void Rdiff_view_set_texts(Rid diff_view, const char* left, const char* right);
	/** @brief Names shown over the two sides (file names, "Before" / "After"); NULL: none. Their extension chooses the colors of the code. */
	RAVEN_API void Rdiff_view_set_names(Rid diff_view, const char* left_name, const char* right_name);
	/**
	 * @brief Compares two files. Their paths become the names unless names
	 * were given. @return R_FALSE when a file cannot be read (it is compared
	 * as empty).
	 */
	RAVEN_API int Rdiff_view_load_files(Rid diff_view, const char* left_path, const char* right_path);
	/**
	 * @brief Shows a patch (unified diff: the output of `git diff`, `diff -u`)
	 * instead of two texts: each file with its name, each hunk with its header.
	 * @return R_FALSE when the text holds no hunk (nothing is shown).
	 */
	RAVEN_API int Rdiff_view_set_patch(Rid diff_view, const char* patch);
	RAVEN_API void Rdiff_view_set_mode(Rid diff_view, RDiffViewMode mode);
	RAVEN_API RDiffViewMode Rdiff_view_get_mode(Rid diff_view);
	/**
	 * @brief Unchanged lines shown around each change (3 by default); the
	 * others are folded. A negative number shows every line.
	 */
	RAVEN_API void Rdiff_view_set_context(Rid diff_view, int lines);
	RAVEN_API int Rdiff_view_get_context(Rid diff_view);
	/** @brief How lines are compared: RDiffFlags (0 by default). Not used by a patch. */
	RAVEN_API void Rdiff_view_set_flags(Rid diff_view, uint32_t flags);
	RAVEN_API uint32_t Rdiff_view_get_flags(Rid diff_view);
	/** @brief Marks the words that changed inside a changed line (default R_TRUE). */
	RAVEN_API void Rdiff_view_set_word_diff(Rid diff_view, int enabled);
	/** @brief Shows the numbers of the lines (default R_TRUE). */
	RAVEN_API void Rdiff_view_set_line_numbers(Rid diff_view, int visible);
	/**
	 * @brief Colors the code as the text editor does: a language name of
	 * Rlanguage_register ("C", "Python", "JSON"...), "None" for plain text,
	 * NULL (default) to guess it from the extension of the names.
	 */
	RAVEN_API void Rdiff_view_set_language(Rid diff_view, const char* language);

	/** @brief The number of changes: runs of lines removed and / or added. */
	RAVEN_API int Rdiff_view_get_change_count(Rid diff_view);
	/**
	 * @brief Where a change is: its first line and its number of lines on
	 * each side (a count of 0: nothing on that side; the line is then the one
	 * after which the other side's lines go). @return R_FALSE when there is
	 * no such change.
	 */
	RAVEN_API int Rdiff_view_get_change(Rid diff_view, int change, int* left_line, int* left_count, int* right_line, int* right_count);
	/** @brief Scrolls to a change (from 0) and marks it as the current one. */
	RAVEN_API void Rdiff_view_go_to_change(Rid diff_view, int change);
	/** @brief The current change, -1 before the first one is gone to. */
	RAVEN_API int Rdiff_view_get_current_change(Rid diff_view);
	/** @brief Goes to the next / previous change. @return Its index, -1 when there is none. */
	RAVEN_API int Rdiff_view_next_change(Rid diff_view);
	RAVEN_API int Rdiff_view_prev_change(Rid diff_view);

	/** @brief The rows shown, folds, hunk headers and notes included. */
	RAVEN_API int Rdiff_view_get_row_count(Rid diff_view);
	/**
	 * @brief What a row shows, and the numbers of its lines (0 for a side it
	 * has no line of). A fold gives the first line it hides on each side.
	 */
	RAVEN_API RDiffRowKind Rdiff_view_get_row(Rid diff_view, int row, int* left_line, int* right_line);
	/**
	 * @brief The text of a row on one side (in unified mode the side does not
	 * matter for a line); a fold, a hunk, a file or a note give their label.
	 * Valid until the next call. "" when there is none.
	 */
	RAVEN_API const char* Rdiff_view_get_row_text(Rid diff_view, int row, RDiffSide side);
	/** @brief Shows the lines of a fold. @return R_FALSE when the row is not a fold. */
	RAVEN_API int Rdiff_view_expand(Rid diff_view, int row);
	/** @brief The first row at the top of the view; Rdiff_view_scroll_to_row brings one there. */
	RAVEN_API int Rdiff_view_get_first_visible_row(Rid diff_view);
	RAVEN_API void Rdiff_view_scroll_to_row(Rid diff_view, int row);

	/**
	 * @brief Attaches a short note to a line (a review comment, an error): a
	 * mark in the gutter and the text in a row under the line, which is never
	 * folded away. Several notes may go to one line.
	 */
	RAVEN_API void Rdiff_view_add_annotation(Rid diff_view, RDiffSide side, int line, const char* text);
	RAVEN_API void Rdiff_view_clear_annotations(Rid diff_view);
	/** @brief The text selected by the user ("" when none), one line after another. */
	RAVEN_API const char* Rdiff_view_get_selected_text(Rid diff_view);
	/** @brief Called when a line or its number is clicked (the action of the widget is fired too). */
	RAVEN_API void Rdiff_view_set_line_clicked_callback(Rid diff_view, RDiffViewLineCallback callback, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_TOOLS_H */
