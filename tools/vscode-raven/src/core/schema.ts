/**
 * What can be written in a Raven document: the tables generated from
 * xml_schema.c (schema.json), with what that file does not list: the
 * elements a widget reads itself (<Item>, <Node>, <Track>...), the
 * elements of composition (<Widget>, <Repeat>, <If>...) and the actions of
 * Raven itself.
 */
import generated from "../schema.json";

export type ValueKind = "text" | "bool" | "number" | "choice" | "color" | "action" | "file" | "numbers" | "list";
export type Children = "none" | "single" | "many";

export interface AttributeInfo {
	name: string;
	kind: ValueKind;
	choices?: string[];
	description: string;
	/** Where it comes from: the tag itself, or Widget, Slot, Touch, Colors, Metrics. */
	group: string;
}

export interface TagInfo {
	name: string;
	description: string;
	children: Children;
	attributes: AttributeInfo[];
	/** widget: a class of Raven; part: an element a widget reads itself; composition: rewritten before building. */
	kind: "window" | "widget" | "part" | "composition";
	/** Elements it reads itself (parts), by name. */
	parts: string[];
	/** Attributes Part.Property are free (NodeGraph, Timeline and their items). */
	dottedParts: boolean;
	/** Unknown attributes are reported (false where the widget reads more than this lists). */
	strict: boolean;
	/** Whether the shared attributes of the widgets apply. */
	shared: boolean;
	aliases: string[];
	/** Names the widget accepts that the schema does not describe (other or older names): not proposed, not reported. */
	synonyms: string[];
}

interface Generated {
	shared: Record<string, { name: string; kind: ValueKind; choices?: string[]; description: string }[]>;
	windowTouch: { name: string; kind: ValueKind; choices?: string[]; description: string }[];
	tags: { name: string; description: string; children: Children; attributes: { name: string; kind: ValueKind; choices?: string[]; description: string }[]; accepted?: string[] }[];
	aliases: Record<string, string>;
}

const schema = generated as unknown as Generated;

const t = (name: string, description: string): AttributeInfo => ({ name, kind: "text", description, group: "" });
const b = (name: string, description: string): AttributeInfo => ({ name, kind: "bool", description, group: "" });
const n = (name: string, description: string): AttributeInfo => ({ name, kind: "number", description, group: "" });
const k = (name: string, description: string): AttributeInfo => ({ name, kind: "color", description, group: "" });
const f = (name: string, description: string): AttributeInfo => ({ name, kind: "file", description, group: "" });
const c = (name: string, choices: string, description: string): AttributeInfo => ({ name, kind: "choice", choices: choices.split("|"), description, group: "" });

/** Attributes every widget takes that xml_schema.c does not list under these names. */
const EXTRA_SHARED: AttributeInfo[] = [
	t("Id", "Another name of Name."),
	{ name: "OnClicked", kind: "action", description: "Another spelling of Action: the action fired by the main event of the widget.", group: "" },
	{ name: "OnChanged", kind: "action", description: "Another spelling of Action: the action fired by the main event of the widget.", group: "" },
	{ name: "OnCommitted", kind: "action", description: "Another spelling of Action: the action fired by the main event of the widget.", group: "" },
	{ name: "OnActivated", kind: "action", description: "Another spelling of Action: the action fired by the main event of the widget.", group: "" },
	t("If", "The element is kept only when this is true (`{expression}` inside a <Widget>, <Repeat> or <ForEach>)."),
	t("Into", "On a child given to a declared tag: goes to its <Children Name=\"...\"/>."),
];

const STYLE_ITEM = "and the properties of its part (Glow, BorderColor, CornerRadius...)";

/** Elements read by a widget itself, by parent. */
const PARTS: Record<string, Omit<TagInfo, "kind" | "shared" | "aliases" | "dottedParts" | "strict" | "synonyms"> & { strict?: boolean; dottedParts?: boolean }> = {
	"ComboBox/Item": { name: "Item", description: "A choice of the list (its text, or Text).", children: "none", parts: [], attributes: [t("Text", "Text of the choice (or the text inside the element).")], strict: true },
	"ListView/Item": { name: "Item", description: "A row of the list (its text, or Text).", children: "none", parts: [], attributes: [t("Text", "Text of the row (or the text inside the element).")], strict: true },
	"Breadcrumb/Item": { name: "Item", description: "A link of the path (its text, or Text).", children: "none", parts: [], attributes: [t("Text", "Text of the link (or the text inside the element).")], strict: true },
	"TreeView/Item": {
		name: "Item",
		description: "A row of the tree; Item elements inside it are its children.",
		children: "many",
		parts: ["Item"],
		attributes: [t("Text", "Text of the row (or the text inside the element)."), b("Expanded", "Its children are shown.")],
		strict: true,
	},
	"Menu/Separator": { name: "Separator", description: "A line between items of the menu.", children: "none", parts: [], attributes: [], strict: true },
	"Menu/MenuSeparator": { name: "MenuSeparator", description: "A line between items of the menu.", children: "none", parts: [], attributes: [], strict: true },
	"Table/Column": {
		name: "Column",
		description: "A column of the table (its header is its text, or Header).",
		children: "none",
		parts: [],
		attributes: [
			t("Header", "Text of the header (or the text inside the element)."),
			n("Width", "Width of the column; 0 lets Fill decide."),
			n("Fill", "Share of the free width the column takes."),
			b("Sortable", "A click on the header sorts the rows."),
			{ name: "Align", kind: "choice", choices: ["Left", "Center", "Right", "Start", "End"], description: "Where the texts sit: Left, Center, Right (or a number from 0 to 1).", group: "" },
		],
		strict: true,
	},
	"Table/Row": {
		name: "Row",
		description: "A row of the table: Cells=\"a|b|c\", its text, or Cell elements.",
		children: "many",
		parts: ["Cell"],
		attributes: [t("Cells", "The texts of the cells, separated by '|'.")],
		strict: true,
	},
	"Row/Cell": { name: "Cell", description: "A cell of the row (its text, or Text): a text that may hold a '|'.", children: "none", parts: [], attributes: [t("Text", "Text of the cell.")], strict: true },
	"Timeline/Track": {
		name: "Track",
		description: "A track of the timeline: Clip and Key elements.",
		children: "many",
		parts: ["Clip", "Key"],
		attributes: [
			t("Name", "Name of the track."),
			k("Color", "Color of the track."),
			n("Height", "Height of the track."),
			b("Locked", "Its items cannot be changed."),
			b("Muted", "The track is off."),
			c("ClipStyle", "Solid|Gradient|Tint|Outline", "How its clips are filled."),
			c("KeyShape", "Diamond|Circle|Ring|Square|Triangle", "The form of its keys."),
			n("KeySize", "Half the size of its keys."),
			k("Background", "Color of its lane."),
		],
		dottedParts: true,
	},
	"Track/Clip": {
		name: "Clip",
		description: `A clip of the track: Start, Length or End, Label (or its text), ${STYLE_ITEM}.`,
		children: "none",
		parts: [],
		attributes: [
			n("Start", "Where it starts, in seconds."),
			n("Length", "How long it lasts, in seconds."),
			n("End", "Where it ends, in seconds (instead of Length)."),
			t("Label", "Its text (or the text inside the element)."),
			k("Color", "Its color."),
			b("Selected", "Selected."),
			c("Style", "Solid|Gradient|Tint|Outline", "How it is filled."),
		],
		dottedParts: true,
	},
	"Track/Key": {
		name: "Key",
		description: `A key of the track, ${STYLE_ITEM}.`,
		children: "none",
		parts: [],
		attributes: [n("Time", "When, in seconds."), t("Label", "Its text."), k("Color", "Its color."), c("Shape", "Diamond|Circle|Ring|Square|Triangle", "Its form.")],
		dottedParts: true,
	},
	"Timeline/Marker": {
		name: "Marker",
		description: `A flag on the ruler, ${STYLE_ITEM}.`,
		children: "none",
		parts: [],
		attributes: [n("Time", "When, in seconds."), t("Label", "Its text."), k("Color", "Its color.")],
		dottedParts: true,
	},
	"NodeGraph/Node": {
		name: "Node",
		description: "A node of the graph: Input and Output elements, and any widgets shown inside it.",
		children: "many",
		parts: ["Input", "Output"],
		attributes: [
			t("Name", "Name of the node, for the links (From=\"node.output\") and Rnode_graph_find_node."),
			t("Title", "Text of its header."),
			n("X", "Position across the canvas."),
			n("Y", "Position down the canvas."),
			k("Color", "Color of its header."),
			b("Selected", "Selected."),
			c("HeaderStyle", "Solid|Gradient|Stripe|Tint|Outline|None", "How the header wears its color."),
			k("HeaderColor", "Color of its header."),
			k("HeaderGradient", "Second color of a gradient header."),
			k("TitleColor", "Color of the title."),
			f("Icon", "Image before the title."),
			k("Background", "Color of its body."),
			k("BorderColor", "Color of its border."),
			n("BorderWidth", "Width of its border."),
			n("CornerRadius", "Radius of its corners."),
			n("Glow", "Light around it, 0..1."),
		],
		dottedParts: true,
	},
	"Node/Input": {
		name: "Input",
		description: "A pin on the left of the node, where links arrive.",
		children: "none",
		parts: [],
		attributes: pinAttributes(),
		dottedParts: true,
	},
	"Node/Output": {
		name: "Output",
		description: "A pin on the right of the node, where links leave.",
		children: "none",
		parts: [],
		attributes: pinAttributes(),
		dottedParts: true,
	},
	"NodeGraph/Link": {
		name: "Link",
		description: "A link between two pins, written after the nodes it links.",
		children: "none",
		parts: [],
		attributes: [t("From", "The output: \"node.output\"."), t("To", "The input: \"node.input\".")],
		dottedParts: true,
	},
	"NodeGraph/Type": {
		name: "Type",
		description: "The look of the pins of a type of data and of their links.",
		children: "none",
		parts: [],
		attributes: [
			t("Name", "Name of the type (the Type of the pins)."),
			k("Color", "Color of its pins and links."),
			c("Shape", "Circle|Ring|Square|Diamond|Triangle|Arrow", "Form of its pins."),
			n("Size", "Half the size of its pins."),
			n("Glow", "Light around its pins, 0..1."),
			c("LinkShape", "Bezier|Straight|Step|Angled", "Form of its links."),
			n("LinkWidth", "Thickness of its links."),
			n("LinkGlow", "Light around its links, 0..1."),
		],
		dottedParts: true,
	},
	"BlockEditor/Kind": {
		name: "Kind",
		description: "A kind of block of the palette.",
		children: "none",
		parts: [],
		attributes: [t("Name", "Name of the kind."), t("Label", "Text of the block."), t("Shape", "Form of the block."), t("Category", "Name of its Category.")],
		strict: true,
	},
	"BlockEditor/Block": {
		name: "Block",
		description: "Another name of Kind: a kind of block of the palette.",
		children: "none",
		parts: [],
		attributes: [t("Name", "Name of the kind."), t("Label", "Text of the block."), t("Shape", "Form of the block."), t("Category", "Name of its Category.")],
		strict: true,
	},
	"BlockEditor/Category": {
		name: "Category",
		description: "A category of blocks and its color.",
		children: "none",
		parts: [],
		attributes: [t("Name", "Name of the category."), k("Color", "Its color.")],
		strict: true,
	},
	"Plot/Series": {
		name: "Series",
		description: "A series of the plot: its points (X and Y, or Points) and how they are drawn.",
		children: "none",
		parts: [],
		attributes: [
			t("Name", "Its name in the legend (none: not in the legend)."),
			k("Color", "Its color (by default the next one of a palette of ten)."),
			c("Line", "Solid|Dashed|Dotted|DashDot|None", "How its line is drawn."),
			n("LineWidth", "Width of its line (default 1.75)."),
			c("Marker", "None|Circle|Square|Diamond|TriangleUp|TriangleDown|Cross|Plus|Asterisk", "The mark on each point."),
			n("MarkerSize", "Radius of the marks (default 4)."),
			b("MarkerFilled", "Filled marks (or only their outline)."),
			b("Step", "The line goes in steps."),
			t("Fill", "Shades down to zero: true, or an opacity from 0 to 1."),
			c("XAxis", "X|X2", "The horizontal axis it is drawn against."),
			c("Axis", "Y|Y2", "The vertical axis it is drawn against (YAxis is another name)."),
			c("YAxis", "Y|Y2", "The vertical axis it is drawn against."),
			n("MaxPoints", "Keeps only the last points (data that streams)."),
			b("Visible", "Shown (a click on its name in the legend hides it)."),
			t("X", "The X of its points, separated by spaces or commas (none: 0, 1, 2...)."),
			t("Y", "The Y of its points, separated by spaces or commas."),
			t("Points", "Its points as pairs: \"x,y x,y ...\" (or the text inside the element)."),
		],
		strict: true,
	},
	"Plot/Line": {
		name: "Line",
		description: "Another name of Series.",
		children: "none",
		parts: [],
		attributes: [
			t("Name", "Its name in the legend (none: not in the legend)."),
			k("Color", "Its color (by default the next one of a palette of ten)."),
			c("Line", "Solid|Dashed|Dotted|DashDot|None", "How its line is drawn."),
			n("LineWidth", "Width of its line (default 1.75)."),
			c("Marker", "None|Circle|Square|Diamond|TriangleUp|TriangleDown|Cross|Plus|Asterisk", "The mark on each point."),
			n("MarkerSize", "Radius of the marks (default 4)."),
			b("MarkerFilled", "Filled marks (or only their outline)."),
			b("Step", "The line goes in steps."),
			t("Fill", "Shades down to zero: true, or an opacity from 0 to 1."),
			c("XAxis", "X|X2", "The horizontal axis it is drawn against."),
			c("Axis", "Y|Y2", "The vertical axis it is drawn against (YAxis is another name)."),
			c("YAxis", "Y|Y2", "The vertical axis it is drawn against."),
			n("MaxPoints", "Keeps only the last points (data that streams)."),
			b("Visible", "Shown (a click on its name in the legend hides it)."),
			t("X", "The X of its points, separated by spaces or commas (none: 0, 1, 2...)."),
			t("Y", "The Y of its points, separated by spaces or commas."),
			t("Points", "Its points as pairs: \"x,y x,y ...\" (or the text inside the element)."),
		],
		strict: true,
	},
	"Plot/Scatter": {
		name: "Scatter",
		description: "A series drawn as markers only (circles unless Marker says otherwise).",
		children: "none",
		parts: [],
		attributes: [
			t("Name", "Its name in the legend (none: not in the legend)."),
			k("Color", "Its color (by default the next one of a palette of ten)."),
			c("Line", "Solid|Dashed|Dotted|DashDot|None", "How its line is drawn."),
			n("LineWidth", "Width of its line (default 1.75)."),
			c("Marker", "None|Circle|Square|Diamond|TriangleUp|TriangleDown|Cross|Plus|Asterisk", "The mark on each point."),
			n("MarkerSize", "Radius of the marks (default 4)."),
			b("MarkerFilled", "Filled marks (or only their outline)."),
			b("Step", "The line goes in steps."),
			t("Fill", "Shades down to zero: true, or an opacity from 0 to 1."),
			c("XAxis", "X|X2", "The horizontal axis it is drawn against."),
			c("Axis", "Y|Y2", "The vertical axis it is drawn against (YAxis is another name)."),
			c("YAxis", "Y|Y2", "The vertical axis it is drawn against."),
			n("MaxPoints", "Keeps only the last points (data that streams)."),
			b("Visible", "Shown (a click on its name in the legend hides it)."),
			t("X", "The X of its points, separated by spaces or commas (none: 0, 1, 2...)."),
			t("Y", "The Y of its points, separated by spaces or commas."),
			t("Points", "Its points as pairs: \"x,y x,y ...\" (or the text inside the element)."),
		],
		strict: true,
	},
	"Plot/DragLine": {
		name: "DragLine",
		description: "A line across the plot that the user drags: vertical at an X, horizontal at a Y.",
		children: "none",
		parts: [],
		attributes: [
			c("Axis", "X|X2|Y|Y2", "X or X2: a vertical line at an X; Y or Y2: a horizontal line at a Y."),
			n("Value", "Where the line is."),
			t("Label", "Written before its value (or the text inside the element)."),
			k("Color", "Its color (by default the accent of the theme)."),
		],
		strict: true,
	},
	"Window/Shortcut": {
		name: "Shortcut",
		description: "Fires an action when the keys are pressed anywhere in the window.",
		children: "none",
		parts: [],
		attributes: [
			{ name: "Keys", kind: "text", description: "The keys: Ctrl+S, Ctrl+Shift+Z, F5, Alt+Enter.", group: "" },
			{ name: "Action", kind: "action", description: "The action fired.", group: "" },
		],
		strict: true,
	},
};

function pinAttributes(): AttributeInfo[] {
	return [
		t("Name", "Name of the pin, for the links."),
		t("Label", "Its text."),
		t("Type", "Type of data (free text): pins of different types do not link."),
		c("Shape", "Circle|Ring|Square|Diamond|Triangle|Arrow", "Its form."),
		n("Size", "Half its size."),
		k("Color", "Its color."),
		n("Glow", "Light around it, 0..1."),
		c("LinkShape", "Bezier|Straight|Step|Angled", "Form of its links."),
		n("LinkWidth", "Thickness of its links."),
		k("LinkColor", "Color of its links."),
	];
}

/** Elements that are rewritten before the interface is built (xml_components.c). */
const COMPOSITION: TagInfo[] = [
	composition("Widget", "Declares a tag: its attributes other than Name are its parameters, with their default values; it contains one element, what the tag stands for.", "single", [t("Name", "Name of the declared tag.")]),
	composition("Widgets", "The root of a file that only holds declarations (<Widget> elements).", "many", []),
	composition("Children", "Where the children given where the declared tag is used go.", "none", [t("Name", "Takes only the children marked Into=\"this name\".")]),
	composition("If", "Keeps what it contains when the test is true.", "many", [t("Test", "The condition: an expression such as {Count > 2}.")]),
	composition("Repeat", "Repeats what it contains.", "many", [n("Count", "How many times."), t("Var", "Name of the variable: {i} is 0, 1, 2...")]),
	composition("ForEach", "Repeats what it contains for each text of a list.", "many", [
		t("Items", "The list: \"a|b|c\"."),
		t("Var", "Name of the variable that holds the text."),
		t("Index", "Name of the variable that holds its index."),
		t("Separator", "What separates the texts (default '|')."),
	]),
	composition("Include", "Brings in another file: its declarations, and its element if it has one.", "none", [f("Source", "Path of the XML file.")]),
	composition("Strings", "Loads a table of translations (raven_locale.h).", "none", [f("Source", "Path of the CSV file.")]),
];

function composition(name: string, description: string, children: Children, attributes: AttributeInfo[]): TagInfo {
	return { name, description, children, attributes: attributes.map((a) => ({ ...a, group: name })), kind: "composition", parts: [], dottedParts: false, strict: name !== "Widget", shared: false, aliases: [], synonyms: [] };
}

/** Elements that are transparent: their children are seen as children of their parent. */
export const TRANSPARENT = new Set(["if", "repeat", "foreach"]);

/** Built-in actions (XML.md, "Actions that need no C"). */
export const BUILTIN_ACTIONS: { name: string; description: string; snippet?: string }[] = [
	{ name: "app.quit", description: "Quits the application (Rquit(0))." },
	{ name: "window.close", description: "Closes the window of the widget." },
	{ name: "window.minimize", description: "Minimizes the window of the widget." },
	{ name: "window.maximize", description: "Maximizes the window; restores it when it already is." },
	{ name: "window.restore", description: "Restores the window of the widget." },
	{ name: "island.expand", description: "Unfolds the island of the window." },
	{ name: "island.fold", description: "Folds the island of the window." },
	{ name: "island.toggle", description: "Folds or unfolds the island of the window." },
	{ name: "theme.dark", description: "The dark look of Raven." },
	{ name: "theme.light", description: "The light look of Raven." },
	{ name: "theme.toggle", description: "Switches between the dark and the light look." },
	{ name: "theme:", description: "Rtheme_use(NAME): a theme built in, or loaded with Rtheme_load.", snippet: "theme:${1:RavenDark}" },
	{ name: "language:", description: "The language of the translations.", snippet: "language:${1:fr}" },
	{ name: "open:", description: "Opens a web page, a mailto: or a file.", snippet: "open:${1:https://}" },
	{ name: "copy:", description: "Copies the text to the clipboard.", snippet: "copy:${1:text}" },
	{ name: "show:", description: "Shows the widget named NAME.", snippet: "show:${1:name}" },
	{ name: "hide:", description: "Hides the widget named NAME (Collapsed).", snippet: "hide:${1:name}" },
	{ name: "toggle:", description: "Shows or hides the widget named NAME.", snippet: "toggle:${1:name}" },
	{ name: "enable:", description: "Enables the widget named NAME.", snippet: "enable:${1:name}" },
	{ name: "disable:", description: "Disables the widget named NAME.", snippet: "disable:${1:name}" },
	{ name: "focus:", description: "Gives the keyboard to the widget named NAME.", snippet: "focus:${1:name}" },
	{ name: "page:", description: "What a Switcher, TabView, ComboBox or ListView shows: page:NAME=2, page:NAME=+1, page:NAME=-1 (the steps go round).", snippet: "page:${1:name}=${2:+1}" },
];

/** The prefixes of the actions that name a widget. */
export const ACTIONS_ON_WIDGETS = ["show", "hide", "toggle", "enable", "disable", "focus", "page"];

export const COLOR_NAMES = ["transparent", "white", "black", "red", "green", "blue", "yellow", "orange", "cyan", "magenta", "gray", "grey"];

// ---------------------------------------------------------------------------

const withGroup = (list: Generated["windowTouch"], group: string): AttributeInfo[] => list.map((a) => ({ ...a, group }));

export const SHARED_ATTRIBUTES: AttributeInfo[] = [
	...Object.entries(schema.shared).flatMap(([group, list]) => withGroup(list, group)),
	...EXTRA_SHARED.map((a) => ({ ...a, group: "Widget" })),
];

const tagsByName = new Map<string, TagInfo>();
const aliasesOf = new Map<string, string[]>();
for (const [alias, target] of Object.entries(schema.aliases)) {
	aliasesOf.set(target, [...(aliasesOf.get(target) || []), alias]);
}

/** Parts of each widget, by parent name (lower case). */
const partsOf = new Map<string, Map<string, TagInfo>>();
for (const [key, part] of Object.entries(PARTS)) {
	const [parent] = key.split("/");
	const info: TagInfo = {
		...part,
		attributes: part.attributes.map((a) => ({ ...a, group: part.name })),
		kind: parent === "Window" ? "composition" : "part",
		shared: false,
		aliases: [],
		synonyms: [],
		dottedParts: !!part.dottedParts,
		strict: part.strict ?? !part.dottedParts,
	};
	const map = partsOf.get(parent.toLowerCase()) || new Map<string, TagInfo>();
	map.set(part.name.toLowerCase(), info);
	partsOf.set(parent.toLowerCase(), map);
}

for (const tag of schema.tags) {
	const isWindow = tag.name === "Window";
	const own = tag.attributes.map((a) => ({ ...a, group: tag.name }));
	const info: TagInfo = {
		name: tag.name,
		description: tag.description,
		children: tag.children,
		attributes: isWindow ? [...own, ...withGroup(schema.windowTouch, "Touch")] : own,
		kind: isWindow ? "window" : "widget",
		parts: [...(partsOf.get(tag.name.toLowerCase())?.values() || [])].map((p) => p.name),
		dottedParts: tag.name === "NodeGraph" || tag.name === "Timeline",
		strict: true,
		shared: !isWindow,
		aliases: aliasesOf.get(tag.name) || [],
		synonyms: tag.accepted || [],
	};
	tagsByName.set(tag.name.toLowerCase(), info);
}
/* <Plot> reads the attributes of its four axes as a prefix and a name (XLabel, Y2Min...); xml_schema.c describes a few of them. */
{
	const plot = tagsByName.get("plot");
	const properties = ["Label", "Scale", "LinearThreshold", "Threshold", "Min", "Max", "Range", "Format", "Grid", "Inverted", "Locked", "Visible", "AutoFit"];
	for (const prefix of plot ? ["X", "Y", "X2", "Y2"] : []) {
		for (const property of properties) {
			const name = prefix + property;
			if (!plot!.attributes.some((a) => a.name === name) && !plot!.synonyms.includes(name)) plot!.synonyms.push(name);
		}
	}
}
for (const [alias, target] of Object.entries(schema.aliases)) {
	const info = tagsByName.get(target.toLowerCase());
	if (info && !tagsByName.has(alias.toLowerCase())) tagsByName.set(alias.toLowerCase(), info);
}
const compositionByName = new Map(COMPOSITION.map((tag) => [tag.name.toLowerCase(), tag]));

export const WIDGET_TAGS: TagInfo[] = schema.tags.map((tag) => tagsByName.get(tag.name.toLowerCase())!).filter((tag) => tag.kind === "widget");
export const COMPOSITION_TAGS = COMPOSITION;

/** A tag of the library (or one of its other names). */
export function libraryTag(name: string): TagInfo | undefined {
	return tagsByName.get(name.toLowerCase());
}

export function compositionTag(name: string): TagInfo | undefined {
	return compositionByName.get(name.toLowerCase());
}

/** An element a widget reads itself, inside the parent named. */
export function partTag(parentName: string, name: string): TagInfo | undefined {
	const parent = libraryTag(parentName);
	const key = (parent ? parent.name : parentName).toLowerCase();
	return partsOf.get(key)?.get(name.toLowerCase());
}

export function partsInside(parentName: string): TagInfo[] {
	const parent = libraryTag(parentName);
	const key = (parent ? parent.name : parentName).toLowerCase();
	return [...(partsOf.get(key)?.values() || [])];
}

export function allAttributes(tag: TagInfo): AttributeInfo[] {
	return tag.shared ? [...tag.attributes, ...SHARED_ATTRIBUTES] : tag.attributes;
}

export function findAttribute(tag: TagInfo, name: string): AttributeInfo | undefined {
	const lower = name.toLowerCase();
	const found = tag.attributes.find((a) => a.name.toLowerCase() === lower) || (tag.shared ? SHARED_ATTRIBUTES.find((a) => a.name.toLowerCase() === lower) : undefined);
	if (found) return found;
	const synonym = tag.synonyms.find((a) => a.toLowerCase() === lower);
	return synonym ? { name: synonym, kind: "text", description: `Accepted by <${tag.name}> (another name of one of its attributes).`, group: tag.name } : undefined;
}

/** Text of a value kind, for hovers. */
export function describeKind(attribute: AttributeInfo): string {
	switch (attribute.kind) {
		case "bool":
			return "boolean (true / false, yes / no, 1 / 0)";
		case "number":
			return "number";
		case "numbers":
			return "numbers separated by commas";
		case "color":
			return "color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA or a name)";
		case "choice":
			return "one of: " + (attribute.choices || []).join(", ");
		case "action":
			return "action name";
		case "file":
			return "path of a file (next to the XML file, in its subfolders, then the base path, the current folder, PATH)";
		case "list":
			return "list separated by '|'";
		default:
			return "text";
	}
}
