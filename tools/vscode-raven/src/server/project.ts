/**
 * What the language server compiles for a document: its scripts (the
 * <Script> elements of an XML document and the files they name, or a .rvs
 * file alone), the actions of its widgets, the names of its widgets; and
 * the mapping between the byte offsets of the core and the places in the
 * documents.
 */
import * as path from "path";
import { attribute, parseXml, sameName, walk, XAttribute, XDocument, XElement } from "../core/xmlParser";
import { CoreHandler, CoreRun, CoreUnit, ScriptCore } from "./scriptCore";

/** A text and its lines: offsets <-> positions (0-based lines, UTF-16 characters). */
export class Text {
	readonly text: string;
	private lines: number[];

	constructor(text: string) {
		this.text = text;
		this.lines = [0];
		for (let i = 0; i < text.length; i++) if (text.charCodeAt(i) === 10) this.lines.push(i + 1);
	}

	positionAt(offset: number): { line: number; character: number } {
		offset = Math.max(0, Math.min(offset, this.text.length));
		let low = 0;
		let high = this.lines.length;
		while (high - low > 1) {
			const middle = (low + high) >> 1;
			if (this.lines[middle] <= offset) low = middle;
			else high = middle;
		}
		return { line: low, character: offset - this.lines[low] };
	}

	offsetAt(position: { line: number; character: number }): number {
		if (position.line >= this.lines.length) return this.text.length;
		if (position.line < 0) return 0;
		const start = this.lines[position.line];
		const end = position.line + 1 < this.lines.length ? this.lines[position.line + 1] - 1 : this.text.length;
		return Math.min(start + position.character, Math.max(start, end));
	}
}

/** UTF-8 byte offsets (what the core counts) of a text -> UTF-16 offsets (what the editor counts). */
export class ByteMap {
	private toUtf16: Uint32Array;
	private toBytes: Uint32Array;

	constructor(text: string) {
		const bytes: number[] = [];
		const units: number[] = [];
		let byte = 0;
		for (let i = 0; i < text.length; i++) {
			const code = text.codePointAt(i)!;
			const size = code < 0x80 ? 1 : code < 0x800 ? 2 : code < 0x10000 ? 3 : 4;
			for (let k = 0; k < size; k++) bytes.push(i);
			units.push(byte);
			if (code > 0xffff) {
				units.push(byte);
				i++;
			}
			byte += size;
		}
		bytes.push(text.length);
		units.push(byte);
		this.toUtf16 = Uint32Array.from(bytes);
		this.toBytes = Uint32Array.from(units);
	}

	utf16(byteOffset: number): number {
		if (byteOffset < 0) return 0;
		return this.toUtf16[Math.min(byteOffset, this.toUtf16.length - 1)];
	}

	byte(utf16Offset: number): number {
		if (utf16Offset < 0) return 0;
		return this.toBytes[Math.min(utf16Offset, this.toBytes.length - 1)];
	}
}

/** A unit of the batch and where its text is. */
export interface Unit {
	/** The document (URI) that holds the text. */
	uri: string;
	/** The text of that whole document. */
	document: Text;
	/** Where the unit starts in it (UTF-16). */
	offset: number;
	/** The text of the unit, as given to the core. */
	text: string;
	bytes: ByteMap;
	/** The <Script> element of an inline unit. */
	element?: XElement;
}

export interface Handler {
	element: XElement;
	attribute: XAttribute;
	/** The name of the function: the value without "()" (OnClicked="app.quit()" names app.quit). */
	action: string;
	/** Where the name is in the document. */
	start: number;
	end: number;
}

export interface Project {
	uri: string;
	kind: "xml" | "rvs";
	document: Text;
	xml?: XDocument;
	units: Unit[];
	handlers: Handler[];
	/** <Script Src="..."> that could not be read. */
	missing: { attribute: XAttribute; path: string }[];
	/** The file of a <Script Src="...">: its unit. */
	sources: { attribute: XAttribute; unit: number }[];
	names: string[] | null;
	run: CoreRun;
}

export const ACTION_ATTRIBUTES = /^(action|onclicked|onchanged|oncommitted|onactivated)$/i;

/** The name an action attribute gives: "app.quit()" names app.quit (xml_attributes.c). */
export function actionName(value: string): { name: string; length: number } {
	const match = /^\s*([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\s*(\(\s*\))?\s*$/.exec(value);
	if (!match) return { name: value, length: value.length };
	return { name: match[1], length: match[1].length };
}

/** The 1-based line and column (in code points) of an offset, as the core counts them. */
function lineColumn(text: Text, offset: number): { line: number; column: number } {
	const position = text.positionAt(offset);
	const lineStart = offset - position.character;
	const column = [...text.text.slice(lineStart, offset)].length + 1;
	return { line: position.line + 1, column };
}

/** The code of a <Script> as the core reads it: a CDATA section around it becomes spaces (xml_parser.c). */
export function scriptCode(text: string, element: XElement): string {
	const code = text.slice(element.script!.start, element.script!.end);
	const inner = /^(\s*)<!\[CDATA\[([\s\S]*)\]\]>(\s*)$/.exec(code);
	if (!inner) return code;
	return inner[1] + " ".repeat(9) + inner[2] + " ".repeat(3) + inner[3];
}

export interface Files {
	/** The text of a file: the open document, else the disk; null when it cannot be read. */
	read(uri: string): string | null;
	/** file:///a/b.xml + "c.rvs" -> file:///a/c.rvs */
	resolve(base: string, relative: string): string;
	/** The path shown in the messages. */
	display(uri: string): string;
}

export interface Settings {
	declared: Map<string, string>;
}

/** Builds and compiles the project of a document. `complete`: a completion at a place (UTF-16 offset) of one of its files. */
export function analyze(core: ScriptCore, uri: string, text: string, kind: "xml" | "rvs", files: Files, settings: Settings, complete?: { uri: string; offset: number }): Project {
	const document = new Text(text);
	const project: Project = {
		uri,
		kind,
		document,
		units: [],
		handlers: [],
		missing: [],
		sources: [],
		names: null,
		run: { errors: 0, analysis: { diagnostics: [], units: [], symbols: [], references: [], hovers: [], calls: [], functions: [] }, handlers: [] },
	};
	const coreUnits: CoreUnit[] = [];
	const coreHandlers: CoreHandler[] = [];
	let completeAt: { unit: number; offset: number } | undefined;

	const addUnit = (unit: Unit, file: string, line: number, column: number) => {
		if (complete && complete.uri === unit.uri && complete.offset >= unit.offset && complete.offset <= unit.offset + unit.text.length) {
			completeAt = { unit: project.units.length, offset: unit.bytes.byte(complete.offset - unit.offset) };
		}
		project.units.push(unit);
		coreUnits.push({ file, text: unit.text, line, column });
	};

	if (kind === "rvs") {
		addUnit({ uri, document, offset: 0, text, bytes: new ByteMap(text) }, files.display(uri), 1, 1);
	} else {
		const xml = parseXml(text);
		project.xml = xml;
		const names: string[] = [];
		for (const element of walk(xml.roots)) {
			const name = attribute(element, "Name") || attribute(element, "Id");
			if (name && name.value) names.push(name.value);
			if (sameName(element.name, "Script")) {
				const src = attribute(element, "Src");
				if (src && src.value) {
					const target = files.resolve(uri, src.value);
					const content = files.read(target);
					if (content === null) {
						project.missing.push({ attribute: src, path: src.value });
					} else {
						project.sources.push({ attribute: src, unit: project.units.length });
						addUnit({ uri: target, document: new Text(content), offset: 0, text: content, bytes: new ByteMap(content) }, files.display(target), 1, 1);
					}
				}
				if (element.script && element.hasText) {
					const code = scriptCode(text, element);
					const at = lineColumn(document, element.script.start);
					addUnit({ uri, document, offset: element.script.start, text: code, bytes: new ByteMap(code), element }, files.display(uri), at.line, at.column);
				}
				continue;
			}
			for (const a of element.attributes) {
				if (!ACTION_ATTRIBUTES.test(a.name) || !a.value || !a.quoted) continue;
				const { name, length } = actionName(a.raw || "");
				const lead = (a.raw || "").length - (a.raw || "").trimStart().length;
				const handler: Handler = { element, attribute: a, action: name, start: a.valueStart + lead, end: a.valueStart + lead + length };
				project.handlers.push(handler);
				const at = lineColumn(document, element.start);
				coreHandlers.push({ tag: element.name, attribute: a.name, action: name, line: at.line, column: at.column });
			}
		}
		project.names = names;
	}
	project.run = core.run(coreUnits, coreHandlers, settings.declared, project.names, completeAt);
	return project;
}

/** The unit and the byte offset of a place of a document, or null when it is in no unit. */
export function unitAt(project: Project, uri: string, offset: number): { unit: number; byte: number } | null {
	for (let i = 0; i < project.units.length; i++) {
		const unit = project.units[i];
		if (unit.uri !== uri) continue;
		if (offset >= unit.offset && offset <= unit.offset + unit.text.length) return { unit: i, byte: unit.bytes.byte(offset - unit.offset) };
	}
	return null;
}

/** The range (document offsets) of a byte range of a unit. */
export function unitRange(project: Project, unit: number, start: number, end: number): { uri: string; document: Text; start: number; end: number } | null {
	const u = project.units[unit];
	if (!u) return null;
	return { uri: u.uri, document: u.document, start: u.offset + u.bytes.utf16(start), end: u.offset + u.bytes.utf16(Math.max(start, end)) };
}

export function fileUriToPath(uri: string): string | null {
	if (!uri.startsWith("file://")) return null;
	let p = decodeURIComponent(uri.slice("file://".length));
	if (/^\/[A-Za-z]:/.test(p)) p = p.slice(1);
	return p;
}

export function pathToFileUri(file: string): string {
	let p = file.replace(/\\/g, "/");
	if (!p.startsWith("/")) p = "/" + p;
	return "file://" + p.split("/").map((part, i) => (i === 1 && /^[A-Za-z]:$/.test(part) ? part : encodeURIComponent(part))).join("/");
}

export function resolveUri(base: string, relative: string): string {
	const basePath = fileUriToPath(base);
	if (!basePath) return relative;
	if (path.isAbsolute(relative)) return pathToFileUri(relative);
	return pathToFileUri(path.join(path.dirname(basePath), relative));
}
