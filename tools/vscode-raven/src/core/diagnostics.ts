/**
 * The mistakes of a document, as the loader of Raven would report them
 * (xml_loader.c, xml_components.c), found before running it.
 */
import { sameName, walk, XDocument, XElement } from "./xmlParser";
import { Context, contextFor, effectiveChildren, effectiveParent, Resolver, widgetNames } from "./model";
import { allAttributes, COMPOSITION_TAGS, findAttribute, libraryTag, partsInside, TagInfo, WIDGET_TAGS } from "./schema";
import { actionTarget, checkValue } from "./values";

export type Severity = "error" | "warning" | "information" | "hint";

export interface Problem {
	start: number;
	end: number;
	severity: Severity;
	message: string;
	code?: string;
	/** A correction: the edits that replace a typo by the closest name. */
	fix?: { title: string; edits: { start: number; end: number; text: string }[] };
}

/** Prefixes of attributes that are known groups: an unknown name there is a typo. */
const GROUPS = ["slot.", "color.", "metric.", "touch.", "pen.", "trackpad."];

export function diagnose(doc: XDocument, workspace: Context): Problem[] {
	const problems: Problem[] = doc.errors.map((e) => ({ start: e.start, end: e.end, severity: "error" as Severity, message: e.message, code: "xml" }));
	const context = contextFor(doc, workspace);
	const resolver = new Resolver(context);
	const names = widgetNames(doc);
	/* names may come from an included file */
	const usesOtherFiles = [...walk(doc.roots)].some((e) => sameName(e.name, "Include"));

	for (const element of walk(doc.roots)) {
		const resolution = resolver.resolve(element);
		const parent = effectiveParent(element);

		if (resolution.kind === "unknown") {
			const suggestion = closestTag(element.name);
			const edits = [{ start: element.nameStart, end: element.nameEnd, text: suggestion || "" }];
			if (element.closeNameStart >= 0) edits.push({ start: element.closeNameStart, end: element.closeNameEnd, text: suggestion || "" });
			problems.push({
				start: element.nameStart,
				end: element.nameEnd,
				severity: "warning",
				message: unknownTagMessage(element, parent ? resolver.resolve(parent).tag : null),
				code: "unknown-tag",
				fix: suggestion ? { title: `Change to <${suggestion}>`, edits } : undefined,
			});
			continue;
		}
		const tag = resolution.tag;

		checkPlace(element, tag, problems);
		checkChildren(element, tag, resolver, problems);

		for (const a of element.attributes) {
			if (/^xmlns(:|$)/i.test(a.name)) continue;
			const info = findAttribute(tag, a.name);
			if (!info) {
				if (!tag.strict) continue;
				const dotted = a.name.includes(".") && !GROUPS.some((g) => a.name.toLowerCase().startsWith(g));
				if (dotted && tag.dottedParts) continue;
				const suggestion = closest(a.name, allAttributes(tag).map((x) => x.name));
				problems.push({
					start: a.nameStart,
					end: a.nameEnd,
					severity: "warning",
					message: unknownAttributeMessage(a.name, tag, suggestion),
					code: "unknown-attribute",
					fix: suggestion ? { title: `Change to ${suggestion}`, edits: [{ start: a.nameStart, end: a.nameEnd, text: suggestion }] } : undefined,
				});
				continue;
			}
			if (a.value === null) continue;
			const message = checkValue(info, a.value);
			if (message) {
				problems.push({ start: a.valueStart, end: Math.max(a.valueEnd, a.valueStart + 1), severity: "warning", message, code: "invalid-value" });
				continue;
			}
			if (info.kind === "action") {
				const target = actionTarget(a.value);
				if (target && !names.has(target.name) && !usesOtherFiles && a.value === a.raw) {
					problems.push({
						start: a.valueStart + target.start,
						end: a.valueStart + target.start + target.name.length,
						severity: "information",
						message: `No widget is named "${target.name}" in this file (Name="${target.name}")`,
						code: "unknown-target",
					});
				}
			}
		}
	}

	/* one element besides the declarations (xml_components.c) */
	const built = doc.roots.filter((r) => !["widget", "strings", "include"].includes(r.name.toLowerCase()));
	for (const extra of built.slice(1)) {
		problems.push({ start: extra.nameStart, end: extra.nameEnd, severity: "error", message: "a document has a single root element besides its <Widget> declarations", code: "structure" });
	}

	const root = doc.roots[0];
	if (root && sameName(root.name, "Widgets")) {
		for (const child of root.children) {
			if (!sameName(child.name, "Widget") && !sameName(child.name, "Include") && !sameName(child.name, "Strings")) {
				problems.push({ start: child.nameStart, end: child.nameEnd, severity: "warning", message: "a <Widgets> file only holds <Widget> declarations", code: "structure" });
			}
		}
	}
	return problems;
}

function checkPlace(element: XElement, tag: TagInfo, problems: Problem[]) {
	const name = tag.name;
	if (name === "Window" && element.parent) {
		problems.push({ start: element.nameStart, end: element.nameEnd, severity: "error", message: "<Window> can only be the root of a document", code: "structure" });
	}
	if (name === "Children" && !insideWidget(element)) {
		problems.push({ start: element.nameStart, end: element.nameEnd, severity: "warning", message: "<Children/> only has a meaning inside a <Widget> declaration", code: "structure" });
	}
}

function insideWidget(element: XElement): boolean {
	for (let p = element.parent; p; p = p.parent) if (sameName(p.name, "Widget")) return true;
	return false;
}

function checkChildren(element: XElement, tag: TagInfo, resolver: Resolver, problems: Problem[]) {
	if (tag.kind === "composition" && tag.name !== "Widget") return;
	const children = effectiveChildren(element);
	const isDeclaration = (child: XElement) => ["widget", "include", "strings", "children"].includes(child.name.toLowerCase());

	if (tag.children === "none") {
		for (const child of children) {
			const r = resolver.resolve(child);
			if (r.kind === "part" || r.kind === "unknown" || sameName(child.name, "Children")) continue;
			problems.push({
				start: child.nameStart,
				end: child.nameEnd,
				severity: "error",
				message: tag.parts.length
					? `<${element.name}> cannot contain <${child.name}>: it reads ${tag.parts.map((p) => `<${p}>`).join(", ")}`
					: `<${element.name}> cannot contain <${child.name}>`,
				code: "structure",
			});
		}
		return;
	}
	if (tag.children !== "single") return;

	if (tag.kind === "window") {
		let islands = 0;
		const content: XElement[] = [];
		for (const child of children) {
			const r = resolver.resolve(child);
			if (sameName(child.name, "Shortcut") || isDeclaration(child)) continue;
			if (r.kind === "library" && r.tag.name === "Island" && islands === 0) {
				islands++;
				continue;
			}
			content.push(child);
		}
		for (const extra of content.slice(1)) {
			problems.push({ start: extra.nameStart, end: extra.nameEnd, severity: "error", message: "a <Window> has a single content element; wrap them in a panel (VBox, HBox, Grid...)", code: "structure" });
		}
		return;
	}
	const content = children.filter((child) => !isDeclaration(child));
	for (const extra of content.slice(1)) {
		problems.push({ start: extra.nameStart, end: extra.nameEnd, severity: tag.name === "Widget" ? "error" : "warning", message: `<${element.name}> holds a single element; wrap them in a panel (VBox, HBox...)`, code: "structure" });
	}
}

function unknownTagMessage(element: XElement, parentTag: TagInfo | null): string {
	const name = element.name;
	const readers = WIDGET_TAGS.filter((t) => partsInside(t.name).some((p) => sameName(p.name, name))).map((t) => `<${t.name}>`);
	if (sameName(name, "Shortcut")) return "<Shortcut> is only read as a child of <Window>";
	if (readers.length) return `<${name}> is only read inside ${readers.join(", ")}` + (parentTag ? `, not inside <${parentTag.name}>` : "");
	const close = closestTag(name);
	return (
		`Unknown tag <${name}>: not a widget of Raven nor a tag declared with <Widget>` +
		(close ? `. Did you mean <${close}>?` : ". If it is registered from C (Rxml_register_tag), add it to the setting raven.xml.customTags")
	);
}

function closestTag(name: string): string | null {
	if (WIDGET_TAGS.some((t) => partsInside(t.name).some((p) => sameName(p.name, name))) || sameName(name, "Shortcut")) return null;
	return closest(name, [...WIDGET_TAGS.map((t) => t.name), ...WIDGET_TAGS.flatMap((t) => t.aliases), ...COMPOSITION_TAGS.map((t) => t.name)]);
}

function unknownAttributeMessage(name: string, tag: TagInfo, close: string | null): string {
	const library = libraryTag(tag.name);
	const shown = library ? library.name : tag.name;
	return `<${shown}> has no attribute ${name}` + (close ? `. Did you mean ${close}?` : "");
}

/** The closest name, when it is close enough to be a typo. */
export function closest(name: string, candidates: string[]): string | null {
	const lower = name.toLowerCase();
	let best: string | null = null;
	let bestDistance = Infinity;
	for (const candidate of candidates) {
		const other = candidate.toLowerCase();
		/* a swap of letters (Lable) is closer than another word (Table) */
		const anagram = [...lower].sort().join("") === [...other].sort().join("");
		const d = distance(lower, other) - (anagram ? 0.5 : 0);
		if (d < bestDistance) {
			bestDistance = d;
			best = candidate;
		}
	}
	const limit = name.length <= 4 ? 1 : name.length <= 8 ? 2 : 3;
	return bestDistance <= limit ? best : null;
}

/** Edits between two names, a swap of two letters counting as one (optimal string alignment). */
function distance(a: string, b: string): number {
	const d: number[][] = Array.from({ length: a.length + 1 }, (_, i) => Array.from({ length: b.length + 1 }, (_, j) => (i === 0 ? j : j === 0 ? i : 0)));
	for (let i = 1; i <= a.length; i++) {
		for (let j = 1; j <= b.length; j++) {
			const cost = a[i - 1] === b[j - 1] ? 0 : 1;
			d[i][j] = Math.min(d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost);
			if (i > 1 && j > 1 && a[i - 1] === b[j - 2] && a[i - 2] === b[j - 1]) d[i][j] = Math.min(d[i][j], d[i - 2][j - 2] + 1);
		}
	}
	return d[a.length][b.length];
}
