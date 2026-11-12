/**
 * Raven XML for VS Code: completion, hovers and diagnostics in the XML
 * files of Raven. The work is done by src/core, which knows nothing of VS
 * Code; this file connects it.
 */
import * as path from "path";
import * as vscode from "vscode";
import { parseXml, XDocument } from "./core/xmlParser";
import { Context, Declaration, declarationsOf, emptyContext, looksLikeRaven } from "./core/model";
import { diagnose, Problem } from "./core/diagnostics";
import { complete, Item, ItemKind } from "./core/completion";
import { hover } from "./core/hover";

const SELECTOR: vscode.DocumentSelector = [
	{ language: "xml", scheme: "file" },
	{ language: "xml", scheme: "untitled" },
];
const EXCLUDED_FOLDERS = "**/{node_modules,.git,build,build-*,dist,dist-*,out,third_party}/**";
const EXCLUDED_PATH = /\/(node_modules|\.git|build|build-[^/]*|dist|dist-[^/]*|out|third_party)\//;

let workspace: Context = emptyContext();
const parsed = new Map<string, { version: number; doc: XDocument }>();
const problemsOf = new WeakMap<vscode.TextDocument, Problem[]>();

function settings() {
	return vscode.workspace.getConfiguration("raven.xml");
}

function parse(document: vscode.TextDocument): XDocument {
	const key = document.uri.toString();
	const cached = parsed.get(key);
	if (cached && cached.version === document.version) return cached.doc;
	const doc = parseXml(document.getText());
	parsed.set(key, { version: document.version, doc });
	return doc;
}

function matches(document: vscode.TextDocument, patterns: string[]): boolean {
	return patterns.some((pattern) => vscode.languages.match({ pattern }, document) > 0);
}

/** Is it a file of Raven: by the settings, else by its root element. */
function isRaven(document: vscode.TextDocument): boolean {
	if (document.languageId !== "xml" || !settings().get<boolean>("enable", true)) return false;
	if (matches(document, settings().get<string[]>("exclude", []))) return false;
	if (matches(document, settings().get<string[]>("files", []))) return true;
	return looksLikeRaven(document.getText());
}

function context(): Context {
	const custom = settings().get<string[]>("customTags", []);
	return { ...workspace, customTags: new Set(custom.map((t) => t.toLowerCase())) };
}

// ---------------------------------------------------------------------------
//  Declared tags of the workspace
// ---------------------------------------------------------------------------

const declarationsByFile = new Map<string, Declaration[]>();
const xmlFiles = new Set<string>();

async function indexFile(uri: vscode.Uri) {
	xmlFiles.add(uri.toString());
	try {
		const bytes = await vscode.workspace.fs.readFile(uri);
		const text = Buffer.from(bytes).toString("utf8");
		const head = text.replace(/<\?[\s\S]*?\?>|<!--[\s\S]*?-->/g, "").trimStart();
		/* declarations are read from <Widgets> and <Widget> files, and from any file that holds a <Widget> */
		if (!/<Widget[\s>]/i.test(text) && !/^<Widgets[\s>]/i.test(head)) {
			declarationsByFile.delete(uri.toString());
			return;
		}
		const file = vscode.workspace.asRelativePath(uri);
		declarationsByFile.set(uri.toString(), declarationsOf(parseXml(text), file));
	} catch {
		declarationsByFile.delete(uri.toString());
	}
}

function rebuildWorkspace() {
	const declarations = new Map<string, Declaration>();
	for (const list of declarationsByFile.values()) for (const d of list) declarations.set(d.name.toLowerCase(), d);
	const tagFiles = new Set([...xmlFiles].map((u) => path.basename(vscode.Uri.parse(u).path, ".xml").toLowerCase()));
	workspace = { declarations, tagFiles, customTags: new Set() };
}

async function indexWorkspace() {
	const files = await vscode.workspace.findFiles("**/*.xml", EXCLUDED_FOLDERS, 5000);
	await Promise.all(files.map(indexFile));
	rebuildWorkspace();
}

// ---------------------------------------------------------------------------
//  Diagnostics
// ---------------------------------------------------------------------------

const SEVERITY: Record<Problem["severity"], vscode.DiagnosticSeverity> = {
	error: vscode.DiagnosticSeverity.Error,
	warning: vscode.DiagnosticSeverity.Warning,
	information: vscode.DiagnosticSeverity.Information,
	hint: vscode.DiagnosticSeverity.Hint,
};

function refresh(collection: vscode.DiagnosticCollection, document: vscode.TextDocument) {
	if (!isRaven(document) || !settings().get<boolean>("diagnostics", true)) {
		collection.delete(document.uri);
		return;
	}
	const problems = diagnose(parse(document), context());
	problemsOf.set(document, problems);
	collection.set(
		document.uri,
		problems.map((p) => {
			const diagnostic = new vscode.Diagnostic(new vscode.Range(document.positionAt(p.start), document.positionAt(p.end)), p.message, SEVERITY[p.severity]);
			diagnostic.source = "raven";
			if (p.code) diagnostic.code = p.code;
			return diagnostic;
		}),
	);
}

// ---------------------------------------------------------------------------
//  Completion and hovers
// ---------------------------------------------------------------------------

const KINDS: Record<ItemKind, vscode.CompletionItemKind> = {
	tag: vscode.CompletionItemKind.Class,
	part: vscode.CompletionItemKind.Struct,
	composition: vscode.CompletionItemKind.Keyword,
	declared: vscode.CompletionItemKind.Module,
	attribute: vscode.CompletionItemKind.Property,
	value: vscode.CompletionItemKind.EnumMember,
	action: vscode.CompletionItemKind.Event,
	color: vscode.CompletionItemKind.Color,
	reference: vscode.CompletionItemKind.Reference,
	endTag: vscode.CompletionItemKind.Class,
};

function toCompletion(document: vscode.TextDocument, item: Item): vscode.CompletionItem {
	const result = new vscode.CompletionItem(item.label, KINDS[item.kind]);
	result.insertText = item.snippet ? new vscode.SnippetString(item.insert) : item.insert;
	result.range = new vscode.Range(document.positionAt(item.start), document.positionAt(item.end));
	result.sortText = item.sort;
	if (item.filter) result.filterText = item.filter;
	if (item.detail) result.detail = item.detail;
	if (item.documentation) result.documentation = new vscode.MarkdownString(item.documentation);
	if (item.retrigger) result.command = { title: "Suggest", command: "editor.action.triggerSuggest" };
	if (item.kind === "color" && item.label !== "#") result.detail = item.label;
	return result;
}

export function activate(extension: vscode.ExtensionContext) {
	const collection = vscode.languages.createDiagnosticCollection("raven");
	extension.subscriptions.push(collection);

	const timers = new Map<string, NodeJS.Timeout>();
	const schedule = (document: vscode.TextDocument, delay = 250) => {
		const key = document.uri.toString();
		clearTimeout(timers.get(key));
		timers.set(key, setTimeout(() => {
			timers.delete(key);
			refresh(collection, document);
		}, delay));
	};
	const refreshAll = () => vscode.workspace.textDocuments.forEach((d) => refresh(collection, d));

	extension.subscriptions.push(
		vscode.workspace.onDidOpenTextDocument((d) => schedule(d, 0)),
		vscode.workspace.onDidChangeTextDocument((e) => schedule(e.document)),
		vscode.workspace.onDidCloseTextDocument((d) => {
			collection.delete(d.uri);
			parsed.delete(d.uri.toString());
		}),
		vscode.workspace.onDidChangeConfiguration((e) => {
			if (e.affectsConfiguration("raven.xml")) refreshAll();
		}),
	);

	extension.subscriptions.push(
		vscode.languages.registerCompletionItemProvider(
			SELECTOR,
			{
				provideCompletionItems(document, position) {
					if (!isRaven(document)) return undefined;
					const items = complete(parse(document), document.offsetAt(position), context());
					return items.map((item) => toCompletion(document, item));
				},
			},
			"<", "/", " ", '"', ":", ".",
		),
		vscode.languages.registerHoverProvider(SELECTOR, {
			provideHover(document, position) {
				if (!isRaven(document)) return undefined;
				const result = hover(parse(document), document.offsetAt(position), context());
				if (!result) return undefined;
				return new vscode.Hover(new vscode.MarkdownString(result.contents), new vscode.Range(document.positionAt(result.start), document.positionAt(result.end)));
			},
		}),
		vscode.languages.registerCodeActionsProvider(
			SELECTOR,
			{
				provideCodeActions(document, range) {
					const problems = problemsOf.get(document) || [];
					const start = document.offsetAt(range.start);
					const end = document.offsetAt(range.end);
					return problems
						.filter((p) => p.fix && p.start <= end && p.end >= start)
						.map((p) => {
							const action = new vscode.CodeAction(p.fix!.title, vscode.CodeActionKind.QuickFix);
							action.edit = new vscode.WorkspaceEdit();
							for (const edit of p.fix!.edits) {
								action.edit.replace(document.uri, new vscode.Range(document.positionAt(edit.start), document.positionAt(edit.end)), edit.text);
							}
							action.isPreferred = true;
							return action;
						});
				},
			},
			{ providedCodeActionKinds: [vscode.CodeActionKind.QuickFix] },
		),
	);

	/* the declared tags of the workspace, kept up to date */
	const watcher = vscode.workspace.createFileSystemWatcher("**/*.xml");
	let pending: NodeJS.Timeout | undefined;
	const changed = async (uri: vscode.Uri, removed = false) => {
		if (EXCLUDED_PATH.test(uri.path)) return;
		if (removed) {
			xmlFiles.delete(uri.toString());
			declarationsByFile.delete(uri.toString());
		} else {
			await indexFile(uri);
		}
		clearTimeout(pending);
		pending = setTimeout(() => {
			rebuildWorkspace();
			refreshAll();
		}, 300);
	};
	extension.subscriptions.push(
		watcher,
		watcher.onDidCreate((u) => changed(u)),
		watcher.onDidChange((u) => changed(u)),
		watcher.onDidDelete((u) => changed(u, true)),
	);

	refreshAll();
	indexWorkspace().then(refreshAll, () => undefined);
}

export function deactivate() {
	parsed.clear();
}
