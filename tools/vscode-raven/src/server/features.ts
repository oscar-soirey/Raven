/**
 * The features of the language server, as plain functions of a project
 * (project.ts): diagnostics, hover, completion, signature help, go to
 * definition, references, rename, symbols, formatting. server.ts connects
 * them to the protocol; the tests call them directly.
 */
import {
	CompletionItem,
	CompletionItemKind,
	Diagnostic,
	DiagnosticRelatedInformation,
	DiagnosticSeverity,
	DocumentSymbol,
	Hover,
	InsertTextFormat,
	Location,
	MarkupKind,
	Range,
	SignatureHelp,
	SymbolKind,
	TextEdit,
	WorkspaceEdit,
} from "vscode-languageserver";
import { CoreDiagnostic, CoreLocation, CoreSymbol, ScriptCore } from "./scriptCore";
import { Handler, Project, scriptCode, Text, unitAt, unitRange } from "./project";

const SOURCE = "raven-script";

function range(text: Text, start: number, end: number): Range {
	return { start: text.positionAt(start), end: text.positionAt(Math.max(start, end)) };
}

function severity(level: number): DiagnosticSeverity {
	return level === 0 ? DiagnosticSeverity.Error : level === 1 ? DiagnosticSeverity.Warning : DiagnosticSeverity.Information;
}

/** The text of a diagnostic of the core: its title, its explanation, its help. */
function messageOf(d: CoreDiagnostic): string {
	const parts = [d.title];
	if (d.message) parts.push(d.message);
	if (d.help) parts.push(d.help);
	return parts.join("\n");
}

function relatedOf(project: Project, location: CoreLocation | null, note?: string): DiagnosticRelatedInformation[] | undefined {
	if (!location || location.unit < 0) return undefined;
	const where = unitRange(project, location.unit, location.start, location.end);
	if (!where) return undefined;
	return [{ location: { uri: where.uri, range: range(where.document, where.start, where.end) }, message: note || "declared here" }];
}

// ---------------------------------------------------------------------------
//  Diagnostics
// ---------------------------------------------------------------------------

/** The diagnostics of the document of the project (those of a file it names are summed up on its Src). */
export function diagnostics(project: Project): Diagnostic[] {
	const out: Diagnostic[] = [];
	const perSource = new Map<number, { errors: number; warnings: number; first: CoreDiagnostic }>();
	for (const d of project.run.analysis.diagnostics) {
		/* (the checks of the actions come with their element, below) */
		if (!d.location || d.location.unit < 0) continue;
		const unit = project.units[d.location.unit];
		if (!unit) continue;
		if (unit.uri !== project.uri) {
			const sum = perSource.get(d.location.unit) || { errors: 0, warnings: 0, first: d };
			if (d.severity === 0) sum.errors++;
			else if (d.severity === 1) sum.warnings++;
			perSource.set(d.location.unit, sum);
			continue;
		}
		const where = unitRange(project, d.location.unit, d.location.start, d.location.end)!;
		out.push({
			range: range(project.document, where.start, where.end),
			severity: severity(d.severity),
			code: d.code,
			source: SOURCE,
			message: messageOf(d),
			relatedInformation: relatedOf(project, d.related, d.relatedNote),
		});
	}
	for (const source of project.sources) {
		const sum = perSource.get(source.unit);
		if (!sum) continue;
		const a = source.attribute;
		const first = sum.first;
		out.push({
			range: range(project.document, a.valueStart, a.valueEnd),
			severity: sum.errors ? DiagnosticSeverity.Error : DiagnosticSeverity.Warning,
			code: first.code,
			source: SOURCE,
			message: `${a.value}: ${sum.errors} error${sum.errors === 1 ? "" : "s"}, ${sum.warnings} warning${sum.warnings === 1 ? "" : "s"} (the first: ${first.title})`,
			relatedInformation: relatedOf(project, first.location, messageOf(first)),
		});
	}
	for (const missing of project.missing) {
		const a = missing.attribute;
		out.push({
			range: range(project.document, a.valueStart, a.valueEnd),
			severity: DiagnosticSeverity.Error,
			code: "RS101",
			source: SOURCE,
			message: `Cannot read the script '${missing.path}': the scripts of this document would not be installed.`,
		});
	}
	project.handlers.forEach((handler, i) => {
		for (const d of project.run.handlers[i] || []) {
			out.push({
				range: range(project.document, handler.start, handler.end),
				severity: severity(d.severity),
				code: d.code,
				source: SOURCE,
				message: messageOf(d),
				relatedInformation: relatedOf(project, d.related, "the function"),
			});
		}
	});
	return out;
}

// ---------------------------------------------------------------------------
//  What is at a place
// ---------------------------------------------------------------------------

/** The handler (action attribute) whose function name holds the offset. */
export function handlerAt(project: Project, uri: string, offset: number): Handler | null {
	if (uri !== project.uri) return null;
	return project.handlers.find((h) => offset >= h.start && offset <= h.end) || null;
}

/** The symbol referenced or declared at a place. */
export function symbolAt(project: Project, uri: string, offset: number): CoreSymbol | null {
	const handler = handlerAt(project, uri, offset);
	if (handler) {
		const fn = project.run.analysis.functions.find((f) => f.name === handler.action);
		return fn && fn.symbol >= 0 ? project.run.analysis.symbols[fn.symbol] : null;
	}
	const at = unitAt(project, uri, offset);
	if (!at) return null;
	let best: [number, number, number, number] | null = null;
	for (const reference of project.run.analysis.references) {
		const [, unit, start, end] = reference;
		if (unit !== at.unit || at.byte < start || at.byte > end) continue;
		if (!best || end - start < best[3] - best[2]) best = reference;
	}
	return best ? project.run.analysis.symbols[best[0]] : null;
}

function eventLine(core: ScriptCore, tag: string): string {
	const events = core.language().events || [];
	const event = events.find((e) => e.tags.toLowerCase().split("|").includes(tag.toLowerCase()));
	if (!event) return `<${tag}> gives its function no argument.`;
	return `${event.event} gives (${event.parameters.map((p) => `${p.name}: ${p.type}`).join(", ")}), ${event.description}.`;
}

export function hover(core: ScriptCore, project: Project, uri: string, offset: number): Hover | null {
	const handler = handlerAt(project, uri, offset);
	if (handler) {
		const fn = project.run.analysis.functions.find((f) => f.name === handler.action);
		if (!fn) return null;
		return {
			contents: { kind: MarkupKind.Markdown, value: "```rvs\nfn " + fn.name + fn.signature + "\n```\n\n" + eventLine(core, handler.element.name) },
			range: range(project.document, handler.start, handler.end),
		};
	}
	const at = unitAt(project, uri, offset);
	if (!at) return null;
	let best: { start: number; end: number; text: string } | null = null;
	for (const h of project.run.analysis.hovers) {
		if (h.unit !== at.unit || at.byte < h.start || at.byte > h.end) continue;
		if (!best || h.end - h.start < best.end - best.start) best = h;
	}
	if (!best) return null;
	const where = unitRange(project, at.unit, best.start, best.end)!;
	return { contents: { kind: MarkupKind.Markdown, value: best.text }, range: range(where.document, where.start, where.end) };
}

export function definition(project: Project, uri: string, offset: number): Location | null {
	const symbol = symbolAt(project, uri, offset);
	if (!symbol || symbol.unit < 0) return null;
	const where = unitRange(project, symbol.unit, symbol.start, symbol.end);
	return where ? { uri: where.uri, range: range(where.document, where.start, where.end) } : null;
}

export function references(project: Project, uri: string, offset: number, includeDeclaration: boolean): Location[] {
	const symbol = symbolAt(project, uri, offset);
	if (!symbol) return [];
	const out: Location[] = [];
	for (const [id, unit, start, end] of project.run.analysis.references) {
		if (id !== symbol.id) continue;
		if (!includeDeclaration && unit === symbol.unit && start === symbol.start) continue;
		const where = unitRange(project, unit, start, end);
		if (where) out.push({ uri: where.uri, range: range(where.document, where.start, where.end) });
	}
	if (symbol.kind === "function" || symbol.kind === "declared") {
		for (const h of project.handlers) if (h.action === symbol.name) out.push({ uri: project.uri, range: range(project.document, h.start, h.end) });
	}
	return out;
}

const IDENTIFIER = /^[A-Za-z_][A-Za-z0-9_]*$/;
const DOTTED = /^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)*$/;
const KEYWORDS = new Set(["fn", "let", "const", "if", "else", "for", "while", "break", "continue", "return", "true", "false", "null", "declare", "number", "string", "bool", "void"]);

/** The range of the symbol at a place, when it can be renamed (it is declared in the scripts of the project). */
export function prepareRename(project: Project, uri: string, offset: number): Range | null {
	const symbol = symbolAt(project, uri, offset);
	if (!symbol || symbol.unit < 0 || symbol.kind === "native" || symbol.kind === "external") return null;
	const handler = handlerAt(project, uri, offset);
	if (handler) return range(project.document, handler.start, handler.end);
	const at = unitAt(project, uri, offset)!;
	for (const [id, unit, start, end] of project.run.analysis.references) {
		if (id === symbol.id && unit === at.unit && at.byte >= start && at.byte <= end) {
			const where = unitRange(project, unit, start, end)!;
			return range(where.document, where.start, where.end);
		}
	}
	return null;
}

export function rename(project: Project, uri: string, offset: number, newName: string): WorkspaceEdit | string {
	const symbol = symbolAt(project, uri, offset);
	if (!symbol || symbol.unit < 0 || symbol.kind === "native" || symbol.kind === "external") return "Only what the scripts declare can be renamed.";
	const isFunction = symbol.kind === "function" || symbol.kind === "declared";
	if (!(isFunction ? DOTTED : IDENTIFIER).test(newName) || newName.split(".").some((part) => KEYWORDS.has(part))) {
		return isFunction ? `'${newName}' is not a name of function (app.save, helper...).` : `'${newName}' is not a name of variable.`;
	}
	const changes: { [uri: string]: TextEdit[] } = {};
	for (const location of references(project, uri, offset, true)) {
		(changes[location.uri] ||= []).push({ range: location.range, newText: newName });
	}
	return { changes };
}

export function documentSymbols(project: Project, uri: string): DocumentSymbol[] {
	const out: DocumentSymbol[] = [];
	for (const symbol of project.run.analysis.symbols) {
		if (symbol.unit < 0) continue;
		const unit = project.units[symbol.unit];
		if (!unit || unit.uri !== uri) continue;
		const kind =
			symbol.kind === "function" || symbol.kind === "declared" ? SymbolKind.Function : symbol.kind === "global" ? SymbolKind.Variable : symbol.kind === "const" ? SymbolKind.Constant : null;
		if (kind === null) continue;
		const where = unitRange(project, symbol.unit, symbol.start, symbol.end)!;
		const r = range(where.document, where.start, where.end);
		out.push({ name: symbol.name, detail: symbol.signature || symbol.type, kind, range: r, selectionRange: r });
	}
	return out;
}

// ---------------------------------------------------------------------------
//  Completion and signature help
// ---------------------------------------------------------------------------

const KINDS: Record<string, CompletionItemKind> = {
	function: CompletionItemKind.Function,
	method: CompletionItemKind.Method,
	property: CompletionItemKind.Property,
	variable: CompletionItemKind.Variable,
	constant: CompletionItemKind.Constant,
	parameter: CompletionItemKind.Variable,
	keyword: CompletionItemKind.Keyword,
	module: CompletionItemKind.Module,
	type: CompletionItemKind.TypeParameter,
};

/** Completion in a script (the project was analyzed with `complete` at this offset), or of the function of an action. */
export function completion(core: ScriptCore, project: Project, uri: string, offset: number): CompletionItem[] {
	const text = project.document.text;
	const handler = project.kind === "xml" ? actionValueAt(project, offset) : null;
	if (handler) {
		const start = handler.start;
		return project.run.analysis.functions.map((f) => ({
			label: f.name,
			kind: CompletionItemKind.Function,
			detail: f.signature,
			documentation: eventLine(core, handler.element.name),
			textEdit: { range: range(project.document, start, offset), newText: f.name },
			sortText: "0" + f.name,
		}));
	}
	if (!project.run.completion) return [];
	let wordStart = offset;
	while (wordStart > 0 && /[A-Za-z0-9_]/.test(text[wordStart - 1])) wordStart--;
	const replace = range(project.document, wordStart, offset);
	return project.run.completion.map((item, i) => ({
		label: item.label,
		kind: KINDS[item.kind] ?? CompletionItemKind.Text,
		detail: item.detail,
		documentation: item.doc ? { kind: MarkupKind.Markdown, value: item.doc } : undefined,
		textEdit: { range: replace, newText: item.insertText || item.label },
		insertTextFormat: item.snippet ? InsertTextFormat.Snippet : InsertTextFormat.PlainText,
		sortText: (item.kind === "keyword" ? "2" : item.kind === "module" ? "1" : "0") + String(i).padStart(4, "0"),
	}));
}

/** The action attribute whose value holds the offset (completion of the names of functions). */
function actionValueAt(project: Project, offset: number): Handler | null {
	for (const h of project.handlers) {
		if (offset >= h.attribute.valueStart && offset <= h.attribute.valueEnd) return { ...h, start: h.attribute.valueStart };
	}
	/* an empty value is not a handler yet: OnClicked="|" */
	if (!project.xml) return null;
	for (const element of walkAll(project)) {
		for (const a of element.attributes) {
			if (/^(action|onclicked|onchanged|oncommitted|onactivated)$/i.test(a.name) && a.quoted && offset >= a.valueStart && offset <= a.valueEnd) {
				return { element, attribute: a, action: "", start: a.valueStart, end: a.valueEnd };
			}
		}
	}
	return null;
}

function* walkAll(project: Project) {
	const stack = [...(project.xml?.roots || [])];
	while (stack.length) {
		const element = stack.pop()!;
		yield element;
		stack.push(...element.children);
	}
}

export function signatureHelp(project: Project, uri: string, offset: number): SignatureHelp | null {
	const at = unitAt(project, uri, offset);
	if (!at) return null;
	let best = null as null | (typeof project.run.analysis.calls)[number];
	for (const call of project.run.analysis.calls) {
		if (call.unit !== at.unit || at.byte <= call.open || at.byte > call.close) continue;
		/* (after the ')': not inside any more) */
		const unit = project.units[at.unit];
		if (at.byte === call.close && unit.text[unit.bytes.utf16(call.close) - 1] === ")") continue;
		if (!best || call.open > best.open) best = call;
	}
	if (!best) return null;
	const active = best.commas.filter((c) => c < at.byte).length;
	return {
		signatures: [
			{
				label: best.label,
				documentation: best.doc ? { kind: MarkupKind.Markdown, value: best.doc } : undefined,
				parameters: best.parameters.map(([s, e]) => ({ label: [s, e] as [number, number] })),
			},
		],
		activeSignature: 0,
		activeParameter: Math.min(active, Math.max(0, best.parameters.length - 1)),
	};
}

// ---------------------------------------------------------------------------
//  Formatting
// ---------------------------------------------------------------------------

/** A whole .rvs file. */
export function formatScript(core: ScriptCore, text: string, indent: string): TextEdit[] {
	const formatted = core.format(text, indent);
	if (formatted === null || formatted === text) return [];
	const document = new Text(text);
	return [{ range: range(document, 0, text.length), newText: formatted }];
}

/** The <Script> elements of an XML document, each indented one level under its tag. */
export function formatXmlScripts(core: ScriptCore, project: Project, indent: string): TextEdit[] {
	const edits: TextEdit[] = [];
	const text = project.document.text;
	for (const unit of project.units) {
		const element = unit.element;
		if (!element || !element.script || element.closeStart < 0) continue;
		let lineStart = element.start;
		while (lineStart > 0 && text[lineStart - 1] !== "\n") lineStart--;
		const tagIndent = /^[ \t]*/.exec(text.slice(lineStart, element.start))![0];
		const inner = tagIndent + indent;
		const raw = text.slice(element.script.start, element.script.end);
		const cdata = /^\s*<!\[CDATA\[([\s\S]*)\]\]>\s*$/.exec(raw);
		const code = cdata ? cdata[1] : scriptCode(text, element);
		const formatted = core.format(code, indent, inner);
		if (formatted === null) continue;
		const body = cdata ? `\n${inner}<![CDATA[\n${formatted}${inner}]]>\n${tagIndent}` : `\n${formatted}${tagIndent}`;
		if (body !== raw) edits.push({ range: range(project.document, element.script.start, element.script.end), newText: body });
	}
	return edits;
}

/** One level of indentation from the options of the editor. */
export function indentOf(options: { tabSize: number; insertSpaces: boolean }): string {
	return options.insertSpaces ? " ".repeat(Math.max(1, Math.min(16, options.tabSize))) : "\t";
}
