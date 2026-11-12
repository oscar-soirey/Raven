/**
 * What can be written where the cursor is: tags, attributes, values.
 * The items are neutral (offsets, snippet text): the extension turns them
 * into the ones of VS Code.
 */
import { attribute, elementAround, isOpaque, sameName, startTagAt, walk, XAttribute, XDocument, XElement } from "./xmlParser";
import { Context, contextFor, Resolver, widgetNames } from "./model";
import { allAttributes, AttributeInfo, BUILTIN_ACTIONS, COLOR_NAMES, COMPOSITION_TAGS, describeKind, findAttribute, partsInside, TagInfo, TRANSPARENT, WIDGET_TAGS } from "./schema";

export type ItemKind = "tag" | "part" | "composition" | "declared" | "attribute" | "value" | "action" | "color" | "reference" | "endTag";

export interface Item {
	label: string;
	kind: ItemKind;
	/** Snippet syntax ($1, $0) when snippet is true. */
	insert: string;
	snippet: boolean;
	start: number;
	end: number;
	detail?: string;
	documentation?: string;
	sort: string;
	/** Open the completion again after inserting (an attribute whose values are known). */
	retrigger?: boolean;
	filter?: string;
}

const GROUP_ORDER: Record<string, string> = { Widget: "1", Slot: "2", Touch: "4", Colors: "5", Metrics: "6" };

export function complete(doc: XDocument, offset: number, workspace: Context): Item[] {
	if (isOpaque(doc, offset)) return [];
	const resolver = new Resolver(contextFor(doc, workspace));
	const text = doc.text;
	const element = startTagAt(doc, offset);

	if (element) {
		if (offset >= element.nameStart && offset <= element.nameEnd) {
			const closed = element.startTagClosed || element.attributes.length > 0;
			return tagItems(doc, resolver, element.parent, element.nameStart, element.nameEnd, !closed);
		}
		for (const a of element.attributes) {
			if (a.quoted && offset >= a.valueStart && offset <= a.valueEnd) return valueItems(doc, resolver, element, a, offset);
			if (offset >= a.nameStart && offset <= a.nameEnd) return attributeItems(resolver, element, a.nameStart, a.nameEnd, a.raw === null);
		}
		if (/\s/.test(text[offset - 1] || "")) return attributeItems(resolver, element, offset, offset, true);
		return [];
	}

	const before = text.slice(Math.max(0, offset - 200), offset);
	const endTag = /<\/([A-Za-z_][\w.:-]*)?$/.exec(before);
	if (endTag) {
		const typed = endTag[1] || "";
		const open = elementAround(doc, offset - endTag[0].length);
		if (!open) return [];
		const hasGt = text[offset] === ">";
		return [{ label: `/${open.name}`, kind: "endTag", insert: open.name + (hasGt ? "" : ">"), snippet: false, start: offset - typed.length, end: offset, sort: "0", filter: open.name }];
	}
	const startTag = /<([A-Za-z_][\w.:-]*)?$/.exec(before);
	if (startTag) {
		const typed = startTag[1] || "";
		const parent = elementAround(doc, offset - startTag[0].length);
		return tagItems(doc, resolver, parent, offset - typed.length, offset, true);
	}
	return [];
}

// ---------------------------------------------------------------------------

function seeThrough(parent: XElement | null): XElement | null {
	while (parent && TRANSPARENT.has(parent.name.toLowerCase())) parent = parent.parent;
	return parent;
}

function tagItems(doc: XDocument, resolver: Resolver, rawParent: XElement | null, start: number, end: number, full: boolean): Item[] {
	const parent = seeThrough(rawParent);
	const items: Item[] = [];
	const add = (tag: TagInfo, kind: ItemKind, sort: string, name = tag.name) => {
		items.push({
			label: name,
			kind,
			insert: full ? tagSnippet(tag, name) : name,
			snippet: full,
			start,
			end,
			detail: kind === "part" ? `read by <${parent ? parent.name : ""}>` : kind === "composition" ? "composition" : kind === "declared" ? "declared tag" : "Raven widget",
			documentation: tag.description,
			sort: sort + name,
		});
	};

	if (!parent) {
		add(compositionByName("Widgets"), "composition", "1");
		add(compositionByName("Widget"), "composition", "1");
		add(compositionByName("Include"), "composition", "3");
		add(compositionByName("Strings"), "composition", "3");
		items.push(windowItem(start, end, full));
		for (const tag of WIDGET_TAGS) add(tag, "tag", "2");
		for (const declaration of resolver.context.declarations.values()) {
			const declared = resolver.resolveName(declaration.name, null);
			if (declared.tag) add(declared.tag, "declared", "2");
		}
		return items;
	}

	const resolution = resolver.resolve(parent);
	const parentTag = resolution.tag;
	const insideWidget = (() => {
		for (let p: XElement | null = rawParent; p; p = p.parent) if (sameName(p.name, "Widget")) return true;
		return false;
	})();

	if (parentTag && sameName(parentTag.name, "Widgets")) {
		add(compositionByName("Widget"), "composition", "0");
		add(compositionByName("Include"), "composition", "1");
		add(compositionByName("Strings"), "composition", "1");
		return items;
	}

	if (parentTag) {
		const parts = resolution.kind === "part" ? parentTag.parts.map((p) => (sameName(p, parentTag.name) ? parentTag : partsInside(parentTag.name).find((t) => sameName(t.name, p)))) : partsInside(parentTag.name);
		for (const part of parts) if (part) add(part, "part", "0");
		if (parentTag.children === "none" && parentTag.kind !== "composition") {
			for (const name of ["If", "Repeat", "ForEach"]) add(compositionByName(name), "composition", "3");
			return items;
		}
	}
	/* a node of a graph holds its pins and any widgets */
	for (const tag of WIDGET_TAGS) add(tag, "tag", "1");
	for (const declaration of resolver.context.declarations.values()) {
		const declared = resolver.resolveName(declaration.name, parent);
		if (declared.tag) add(declared.tag, "declared", "1");
	}
	for (const tag of COMPOSITION_TAGS) {
		if (tag.name === "Widgets") continue;
		if (tag.name === "Children" && !insideWidget) continue;
		add(tag, "composition", "3");
	}
	return items;
}

function compositionByName(name: string): TagInfo {
	return COMPOSITION_TAGS.find((t) => t.name === name)!;
}

function windowItem(start: number, end: number, full: boolean): Item {
	return {
		label: "Window",
		kind: "tag",
		insert: full ? 'Window Title="${1:Title}" Width="${2:800}" Height="${3:600}">\n\t<VBox Spacing="8" Slot.Padding="12">\n\t\t$0\n\t</VBox>\n</Window>' : "Window",
		snippet: full,
		start,
		end,
		detail: "Raven window",
		documentation: "A window; it contains one element, and optionally an Island and Shortcut elements.",
		sort: "0Window",
	};
}

function tagSnippet(tag: TagInfo, name: string): string {
	const hasText = tag.attributes.some((a) => a.name === "Text") && (tag.kind === "part" || tag.children !== "many");
	const text = hasText ? ' Text="$1"' : "$1";
	if (tag.children === "none" && tag.parts.length === 0) return `${name}${text}/>`;
	if (tag.name === "Widget") return `${name} Name="$1">\n\t$0\n</${name}>`;
	return `${name}${text}>$0</${name}>`;
}

// ---------------------------------------------------------------------------

function attributeItems(resolver: Resolver, element: XElement, start: number, end: number, withValue: boolean): Item[] {
	const tag = resolver.resolve(element).tag;
	if (!tag) return [];
	const present = new Set(element.attributes.filter((a) => a.nameStart !== start).map((a) => a.name.toLowerCase()));
	const hasParent = !!element.parent;
	const items: Item[] = [];
	for (const info of allAttributes(tag)) {
		if (present.has(info.name.toLowerCase())) continue;
		if (info.group === "Slot" && !hasParent) continue;
		const order = info.group in GROUP_ORDER ? GROUP_ORDER[info.group] : "0";
		const known = info.kind === "choice" || info.kind === "bool" || info.kind === "action" || info.kind === "color";
		items.push({
			label: info.name,
			kind: "attribute",
			insert: withValue ? `${info.name}="$1"` : info.name,
			snippet: withValue,
			start,
			end,
			detail: `${info.group} · ${info.kind}`,
			documentation: attributeDocumentation(info),
			sort: order + info.name,
			retrigger: withValue && known,
		});
	}
	return items;
}

export function attributeDocumentation(info: AttributeInfo): string {
	return (info.description ? info.description + "\n\n" : "") + "Value: " + describeKind(info);
}

// ---------------------------------------------------------------------------

function valueItems(doc: XDocument, resolver: Resolver, element: XElement, a: XAttribute, offset: number): Item[] {
	const tag = resolver.resolve(element).tag;
	if (!tag) return [];
	const info = findAttribute(tag, a.name);
	const start = a.valueStart;
	const end = a.valueEnd;
	const item = (label: string, kind: ItemKind, documentation?: string, sort = "1", insert = label, snippet = false): Item => ({ label, kind, insert, snippet, start, end, documentation, sort: sort + label });
	const names = () => [...widgetNames(doc).keys()];
	const lower = a.name.toLowerCase();

	if (lower === "slot.dockto") return names().map((n) => item(n, "reference"));
	if (sameName(element.name, "Link") && (lower === "from" || lower === "to")) return pins(element, lower === "from" ? "Output" : "Input").map((p) => item(p, "reference"));
	if (!info) return [];

	switch (info.kind) {
		case "choice":
			return (info.choices || []).map((choice, i) => item(choice, "value", undefined, String(i).padStart(2, "0")));
		case "bool":
			return [item("true", "value"), item("false", "value")];
		case "color":
			return [item("#", "color", "A color as #RRGGBB or #RRGGBBAA", "0", "#${1:FFFFFF}", true), ...COLOR_NAMES.map((name) => item(name, "color"))];
		case "action": {
			const typed = (a.raw || "").slice(0, offset - start);
			const target = /^(show|hide|toggle|enable|disable|focus|page):/i.exec(typed);
			if (target) {
				const prefix = target[0];
				return names().map((n) => item(prefix + n, "reference", `The widget named ${n}`, "0", prefix + n + (prefix.toLowerCase() === "page:" ? "=${1:+1}" : ""), prefix.toLowerCase() === "page:"));
			}
			const used = new Set<string>();
			for (const other of walk(doc.roots)) {
				for (const attr of other.attributes) {
					if (attr === a || !attr.value || attr.value.includes(":")) continue;
					if (/^(action|on(clicked|changed|committed|activated))$/i.test(attr.name)) used.add(attr.value);
				}
			}
			return [
				...[...used].map((u) => item(u, "action", "An action of the application used in this file", "0")),
				...BUILTIN_ACTIONS.map((b) =>
					ACTION_WITH_NAME.test(b.name)
						? { ...item(b.name, "action", b.description, "1"), retrigger: true }
						: item(b.name, "action", b.description, "1", b.snippet || b.name, !!b.snippet),
				),
			];
		}
		default:
			return [];
	}
}

const ACTION_WITH_NAME = /^(show|hide|toggle|enable|disable|focus|page):$/;

/** "node.pin" for each pin of a kind in the graph of a link. */
function pins(link: XElement, kind: "Input" | "Output"): string[] {
	const graph = link.parent;
	if (!graph) return [];
	const result: string[] = [];
	for (const node of graph.children) {
		if (!sameName(node.name, "Node")) continue;
		const nodeName = attribute(node, "Name")?.value;
		if (!nodeName) continue;
		for (const pin of node.children) {
			if (!sameName(pin.name, kind)) continue;
			const pinName = attribute(pin, "Name")?.value;
			if (pinName) result.push(`${nodeName}.${pinName}`);
		}
	}
	return result;
}
