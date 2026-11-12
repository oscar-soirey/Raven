/**
 * What is said of a tag, an attribute or a value when the pointer rests on it.
 */
import { isOpaque, startTagAt, walk, XDocument } from "./xmlParser";
import { Context, contextFor, Resolver } from "./model";
import { BUILTIN_ACTIONS, describeKind, findAttribute } from "./schema";

export interface HoverResult {
	start: number;
	end: number;
	/** Markdown. */
	contents: string;
}

export function hover(doc: XDocument, offset: number, workspace: Context): HoverResult | null {
	if (isOpaque(doc, offset)) return null;
	const resolver = new Resolver(contextFor(doc, workspace));

	/* the name of an end tag */
	for (const element of walk(doc.roots)) {
		if (element.closeStart >= 0 && offset >= element.closeNameStart && offset <= element.closeNameEnd) {
			const result = tagHover(resolver, element);
			return result ? { start: element.closeNameStart, end: element.closeNameEnd, contents: result } : null;
		}
	}

	const element = startTagAt(doc, offset);
	if (!element) return null;
	if (offset >= element.nameStart && offset <= element.nameEnd) {
		const result = tagHover(resolver, element);
		return result ? { start: element.nameStart, end: element.nameEnd, contents: result } : null;
	}
	const tag = resolver.resolve(element).tag;
	for (const a of element.attributes) {
		if (offset >= a.nameStart && offset <= a.nameEnd) {
			const info = tag && findAttribute(tag, a.name);
			if (!info) return null;
			const group = info.group && info.group !== tag!.name ? ` · ${info.group}` : "";
			const contents = [`**${info.name}**${group}`, info.description, `*Value:* ${describeKind(info)}`].filter(Boolean).join("\n\n");
			return { start: a.nameStart, end: a.nameEnd, contents };
		}
		if (a.quoted && a.value && offset >= a.valueStart && offset <= a.valueEnd) {
			const info = tag && findAttribute(tag, a.name);
			if (!info || info.kind !== "action") return null;
			const value = a.value;
			const builtin = BUILTIN_ACTIONS.find((b) => (b.name.endsWith(":") ? value.toLowerCase().startsWith(b.name) : value.toLowerCase() === b.name));
			const contents = builtin
				? `**${value}** · action of Raven\n\n${builtin.description}\n\nIt applies when no function is bound to the name (Raction_bind replaces it).`
				: `**${value}** · action of the application\n\nBind it from C: \`Raction_bind("${value}", callback, user_data)\`.`;
			return { start: a.valueStart, end: a.valueEnd, contents };
		}
	}
	return null;
}

function tagHover(resolver: Resolver, element: Parameters<Resolver["resolve"]>[0]): string | null {
	const resolution = resolver.resolve(element);
	if (!resolution.tag) return null;
	const tag = resolution.tag;
	const lines: string[] = [];
	const kind =
		resolution.kind === "part" ? `read by <${element.parent ? element.parent.name : ""}>` :
		resolution.kind === "composition" ? "composition (rewritten before the interface is built)" :
		resolution.kind === "declared" ? "declared tag" :
		resolution.kind === "custom" ? "tag registered from C" :
		resolution.kind === "file" ? "tag from a file" : "Raven widget";
	lines.push(`**<${tag.name}>** · ${kind}`);
	if (tag.description) lines.push(tag.description);
	if (tag.aliases.length) lines.push(`*Other names:* ${tag.aliases.join(", ")}`);
	if (resolution.kind === "library" || resolution.kind === "part") {
		const children = tag.children === "none" ? (tag.parts.length ? `only ${tag.parts.map((p) => `<${p}>`).join(", ")}` : "none") : tag.children === "single" ? "a single element" : "any number";
		lines.push(`*Children:* ${children}` + (tag.parts.length && tag.children !== "none" ? `; also reads ${tag.parts.map((p) => `<${p}>`).join(", ")}` : ""));
	}
	return lines.join("\n\n");
}
