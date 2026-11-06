/**
 * What each element of a document is, for Raven: a widget, an element a
 * widget reads itself, an element of composition, a declared tag, a tag
 * registered from C, or nothing known.
 */
import { attribute, sameName, walk, XDocument, XElement } from "./xmlParser";
import { compositionTag, libraryTag, partTag, TagInfo, TRANSPARENT } from "./schema";

export interface Declaration {
	name: string;
	parameters: { name: string; value: string }[];
	/** Where it is declared: a path, or empty for the document itself. */
	file: string;
	offset: number;
}

export interface Context {
	/** Tags declared with <Widget>, in the document and in the workspace (lower-case name). */
	declarations: Map<string, Declaration>;
	/** Names of the XML files of the workspace without extension (a tag <Card/> is looked for as Card.xml). */
	tagFiles: Set<string>;
	/** Tags registered from C (setting raven.xml.customTags), lower case. */
	customTags: Set<string>;
}

export type Resolution =
	| { kind: "library" | "part" | "composition"; tag: TagInfo }
	| { kind: "declared"; tag: TagInfo; declaration: Declaration }
	| { kind: "custom" | "file"; tag: TagInfo }
	| { kind: "unknown"; tag: null };

export function emptyContext(): Context {
	return { declarations: new Map(), tagFiles: new Set(), customTags: new Set() };
}

/** The declarations (<Widget Name="...">) of a document. */
export function declarationsOf(doc: XDocument, file = ""): Declaration[] {
	const result: Declaration[] = [];
	for (const element of walk(doc.roots)) {
		if (!sameName(element.name, "Widget")) continue;
		const name = attribute(element, "Name");
		if (!name || !name.value) continue;
		result.push({
			name: name.value,
			parameters: element.attributes.filter((a) => !sameName(a.name, "Name")).map((a) => ({ name: a.name, value: a.value ?? "" })),
			file,
			offset: name.valueStart,
		});
	}
	return result;
}

/** The context of a document: the one of the workspace, with its own declarations. */
export function contextFor(doc: XDocument, workspace: Context): Context {
	const declarations = new Map(workspace.declarations);
	for (const declaration of declarationsOf(doc)) declarations.set(declaration.name.toLowerCase(), declaration);
	return { ...workspace, declarations };
}

/** The parent that matters: elements of composition (<If>, <Repeat>, <ForEach>) are seen through. */
export function effectiveParent(element: XElement): XElement | null {
	let parent = element.parent;
	while (parent && TRANSPARENT.has(parent.name.toLowerCase())) parent = parent.parent;
	return parent;
}

/** The children that matter: those of <If>, <Repeat>, <ForEach> count as children. */
export function effectiveChildren(element: XElement): XElement[] {
	const result: XElement[] = [];
	for (const child of element.children) {
		if (TRANSPARENT.has(child.name.toLowerCase())) result.push(...effectiveChildren(child));
		else result.push(child);
	}
	return result;
}

function declaredTag(declaration: Declaration): TagInfo {
	const where = declaration.file ? `declared in ${declaration.file}` : "declared in this file";
	return {
		name: declaration.name,
		description: `A tag ${where} with <Widget>.` + (declaration.parameters.length ? ` Parameters: ${declaration.parameters.map((p) => `${p.name}="${p.value}"`).join(", ")}.` : ""),
		children: "many",
		attributes: declaration.parameters.map((p) => ({ name: p.name, kind: "text", description: `Parameter of ${declaration.name} (default "${p.value}").`, group: declaration.name })),
		kind: "widget",
		parts: [],
		dottedParts: false,
		/* the other attributes go to the root of its body: anything it accepts */
		strict: false,
		shared: true,
		aliases: [],
		synonyms: [],
	};
}

function looseTag(name: string, description: string): TagInfo {
	return { name, description, children: "many", attributes: [], kind: "widget", parts: [], dottedParts: false, strict: false, shared: true, aliases: [], synonyms: [] };
}

export class Resolver {
	private cache = new Map<XElement, Resolution>();
	constructor(readonly context: Context) {}

	resolve(element: XElement): Resolution {
		let result = this.cache.get(element);
		if (!result) {
			result = this.compute(element);
			this.cache.set(element, result);
		}
		return result;
	}

	/** What an element of this name would be, inside this parent (null: at the root). */
	resolveName(name: string, parent: XElement | null): Resolution {
		if (parent) {
			const parentResolution = this.resolve(parent);
			if (parentResolution.tag) {
				const parentTag = parentResolution.tag;
				if (parentTag.parts.some((p) => sameName(p, name))) {
					if (parentResolution.kind === "part" && sameName(parentTag.name, name)) return { kind: "part", tag: parentTag };
					const part = partTag(parentTag.name, name);
					if (part) return { kind: part.kind === "composition" ? "composition" : "part", tag: part };
				}
			}
		}
		const composition = compositionTag(name);
		if (composition) return { kind: "composition", tag: composition };
		const library = libraryTag(name);
		if (library) return { kind: "library", tag: library };
		const declaration = this.context.declarations.get(name.toLowerCase());
		if (declaration) return { kind: "declared", tag: declaredTag(declaration), declaration };
		if (this.context.customTags.has(name.toLowerCase())) return { kind: "custom", tag: looseTag(name, "A tag registered from C with Rxml_register_tag (setting raven.xml.customTags).") };
		if (this.context.tagFiles.has(name.toLowerCase())) return { kind: "file", tag: looseTag(name, `A tag looked for as the file ${name}.xml.`) };
		return { kind: "unknown", tag: null };
	}

	private compute(element: XElement): Resolution {
		return this.resolveName(element.name, effectiveParent(element));
	}
}

/** The names given to widgets (Name, Id) in a document. */
export function widgetNames(doc: XDocument): Map<string, number> {
	const names = new Map<string, number>();
	for (const element of walk(doc.roots)) {
		if (sameName(element.name, "Widget")) continue;
		for (const a of element.attributes) {
			if ((sameName(a.name, "Name") || sameName(a.name, "Id")) && a.value && !names.has(a.value)) names.set(a.value, a.valueStart);
		}
	}
	return names;
}

/** Is the document one of Raven? Its root is a widget of Raven, a <Window>, or declarations. */
export function isRavenDocument(doc: XDocument): boolean {
	const root = doc.roots[0];
	if (!root) return false;
	const name = root.name.toLowerCase();
	if (name === "theme") return false;
	return name === "window" || name === "widgets" || name === "widget" || libraryTag(root.name) !== undefined;
}

/** Quick test on the text, without parsing everything. */
export function looksLikeRaven(text: string): boolean {
	const body = text.replace(/<\?[\s\S]*?\?>|<!--[\s\S]*?-->|<!DOCTYPE[^>]*>/g, "");
	const m = /<([A-Za-z_][\w.:-]*)/.exec(body);
	if (!m) return false;
	const name = m[1].toLowerCase();
	if (name === "theme") return false;
	return name === "window" || name === "widgets" || name === "widget" || libraryTag(m[1]) !== undefined;
}
