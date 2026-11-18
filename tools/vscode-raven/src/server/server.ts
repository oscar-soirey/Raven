/**
 * The language server of Raven Script: the scripts of the XML documents of
 * Raven (<Script> elements and the .rvs files they name) and the .rvs files.
 *
 * Every analysis compiles the scripts with the core of the library
 * (WebAssembly, scriptCore.ts): diagnostics (with the checks of the event
 * handlers), completion, hovers, signature help, go to definition, find
 * references, rename, symbols and formatting.
 */
import * as fs from "fs";
import * as path from "path";
import {
	createConnection,
	DidChangeConfigurationNotification,
	DocumentFormattingRequest,
	InitializeResult,
	ProposedFeatures,
	TextDocumentSyncKind,
	TextDocuments,
} from "vscode-languageserver/node";
import { TextDocument } from "vscode-languageserver-textdocument";
import { looksLikeRaven } from "../core/model";
import { ScriptCore } from "./scriptCore";
import { analyze, fileUriToPath, Files, Project, resolveUri } from "./project";
import * as features from "./features";

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);
let core: ScriptCore;
let declared = new Map<string, string>();
let enabled = true;
let configurable = false;
let dynamicFormatting = false;
/** .rvs file (URI) -> the XML documents (URIs) that name it with <Script Src>. */
const users = new Map<string, Set<string>>();
let workspaceFolders: string[] = [];

const files: Files = {
	read(uri: string): string | null {
		const open = documents.get(uri);
		if (open) return open.getText();
		const file = fileUriToPath(uri);
		if (!file) return null;
		try {
			return fs.readFileSync(file, "utf8");
		} catch {
			return null;
		}
	},
	resolve: resolveUri,
	display(uri: string): string {
		const file = fileUriToPath(uri);
		if (!file) return uri;
		for (const folder of workspaceFolders) {
			const relative = path.relative(folder, file);
			if (!relative.startsWith("..") && !path.isAbsolute(relative)) return relative.replace(/\\/g, "/");
		}
		return path.basename(file);
	},
};

function kindOf(document: TextDocument): "xml" | "rvs" | null {
	if (document.languageId === "raven-script" || document.uri.toLowerCase().endsWith(".rvs")) return "rvs";
	if (document.languageId === "xml" && looksLikeRaven(document.getText())) return "xml";
	return null;
}

/** The XML document a .rvs is part of (to see the functions of the other scripts of that document), if one names it. */
function contextOf(uri: string): string | null {
	const list = users.get(uri);
	if (!list) return null;
	for (const user of list) if (files.read(user) !== null) return user;
	return null;
}

/** Notes the files of scripts an XML document names. */
function noteUsers(uri: string, text: string) {
	for (const set of users.values()) set.delete(uri);
	const pattern = /<script\b[^>]*?\bsrc\s*=\s*("([^"]*)"|'([^']*)')/gi;
	let match: RegExpExecArray | null;
	while ((match = pattern.exec(text))) {
		const target = resolveUri(uri, match[2] ?? match[3] ?? "");
		if (!users.has(target)) users.set(target, new Set());
		users.get(target)!.add(uri);
	}
}

/** The project of a document: a .rvs is analyzed with the document that names it, when there is one. */
function projectOf(document: TextDocument, complete?: number): Project | null {
	const kind = kindOf(document);
	if (!kind || !enabled) return null;
	const settings = { declared };
	const target = complete === undefined ? undefined : { uri: document.uri, offset: complete };
	if (kind === "rvs") {
		const user = contextOf(document.uri);
		if (user) {
			const project = analyze(core, user, files.read(user)!, "xml", files, settings, target);
			if (project.units.some((u) => u.uri === document.uri)) return project;
		}
	} else {
		noteUsers(document.uri, document.getText());
	}
	return analyze(core, document.uri, document.getText(), kind, files, settings, target);
}

/** Diagnostics of a document: those of its own scripts (a .rvs analyzed in its document shows those of its unit). */
function validate(document: TextDocument) {
	const kind = kindOf(document);
	if (!kind || !enabled) {
		connection.sendDiagnostics({ uri: document.uri, diagnostics: [] });
		return;
	}
	try {
		const project = projectOf(document);
		if (!project) return;
		let list = features.diagnostics(project);
		if (project.uri !== document.uri) {
			/* a .rvs seen from its document: the diagnostics of its own unit */
			const own = { ...project, uri: document.uri, document: project.units.find((u) => u.uri === document.uri)!.document, handlers: [], sources: [], missing: [] };
			list = features.diagnostics(own);
		}
		connection.sendDiagnostics({ uri: document.uri, diagnostics: list });
	} catch (error) {
		connection.console.error(`Raven Script: ${(error as Error).stack || error}`);
		/* (a module that failed is loaded again: the next analysis starts clean) */
		core = ScriptCore.load();
	}
}

const pending = new Map<string, NodeJS.Timeout>();
function schedule(document: TextDocument) {
	clearTimeout(pending.get(document.uri));
	pending.set(
		document.uri,
		setTimeout(() => {
			pending.delete(document.uri);
			validate(document);
			/* the documents that use a .rvs see its change too */
			if (kindOf(document) === "rvs") for (const user of users.get(document.uri) || []) {
				const open = documents.get(user);
				if (open) validate(open);
			}
			if (kindOf(document) === "xml") for (const [rvs, set] of users) {
				const open = documents.get(rvs);
				if (open && set.has(document.uri)) validate(open);
			}
		}, 150),
	);
}

function readSettings(value: unknown) {
	const settings = (value as { raven?: { script?: { enable?: boolean; functions?: Record<string, string> | string[] } } })?.raven?.script || {};
	enabled = settings.enable !== false;
	declared = new Map();
	const functions = settings.functions || {};
	if (Array.isArray(functions)) {
		/* "my.save(path: string): bool" */
		for (const entry of functions) {
			const match = /^\s*([A-Za-z_][\w.]*)\s*(\(.*)$/.exec(entry);
			if (match) declared.set(match[1], match[2]);
		}
	} else {
		for (const [name, signature] of Object.entries(functions)) declared.set(name, signature);
	}
}

/** Indexes the XML files of the workspace that name .rvs files. */
function indexWorkspace() {
	const skip = /^(node_modules|\.git|build|build-.*|dist|dist-.*|out|third_party)$/;
	const visit = (folder: string, depth: number) => {
		if (depth > 12) return;
		let entries: fs.Dirent[];
		try {
			entries = fs.readdirSync(folder, { withFileTypes: true });
		} catch {
			return;
		}
		for (const entry of entries) {
			const full = path.join(folder, entry.name);
			if (entry.isDirectory()) {
				if (!skip.test(entry.name)) visit(full, depth + 1);
			} else if (entry.name.toLowerCase().endsWith(".xml")) {
				try {
					const text = fs.readFileSync(full, "utf8");
					if (/<script\b[^>]*\bsrc\s*=/i.test(text)) noteUsers(resolveUri("file:///", full), text);
				} catch {
					/* unreadable: skipped */
				}
			}
		}
	};
	for (const folder of workspaceFolders) visit(folder, 0);
}

connection.onInitialize((params): InitializeResult => {
	core = ScriptCore.load();
	configurable = !!params.capabilities.workspace?.configuration;
	dynamicFormatting = !!params.capabilities.textDocument?.formatting?.dynamicRegistration;
	readSettings(params.initializationOptions);
	workspaceFolders = (params.workspaceFolders || []).map((f) => fileUriToPath(f.uri)).filter((p): p is string => !!p);
	return {
		capabilities: {
			textDocumentSync: TextDocumentSyncKind.Incremental,
			completionProvider: { triggerCharacters: [".", '"'], resolveProvider: false },
			hoverProvider: true,
			signatureHelpProvider: { triggerCharacters: ["(", ","], retriggerCharacters: [","] },
			definitionProvider: true,
			referencesProvider: true,
			renameProvider: { prepareProvider: true },
			documentSymbolProvider: true,
			/* (formatting is registered for .rvs files only, below: the XML formatter keeps XML files) */
			documentFormattingProvider: !dynamicFormatting,
		},
	};
});

connection.onInitialized(() => {
	if (configurable) connection.client.register(DidChangeConfigurationNotification.type, { section: "raven.script" });
	if (dynamicFormatting) connection.client.register(DocumentFormattingRequest.type, { documentSelector: [{ language: "raven-script" }] });
	indexWorkspace();
	for (const document of documents.all()) schedule(document);
});

connection.onDidChangeConfiguration(async (change) => {
	if (configurable) {
		const script = await connection.workspace.getConfiguration("raven.script");
		readSettings({ raven: { script } });
	} else {
		readSettings(change.settings);
	}
	for (const document of documents.all()) schedule(document);
});

documents.onDidOpen((event) => schedule(event.document));
documents.onDidChangeContent((event) => schedule(event.document));
documents.onDidClose((event) => {
	if (kindOf(event.document) === "xml") noteUsers(event.document.uri, files.read(event.document.uri) || "");
	connection.sendDiagnostics({ uri: event.document.uri, diagnostics: [] });
});

function at(uri: string, position: { line: number; character: number }, complete = false) {
	const document = documents.get(uri);
	if (!document) return null;
	const offset = document.offsetAt(position);
	const project = projectOf(document, complete ? offset : undefined);
	return project ? { document, offset, project } : null;
}

function guarded<T>(fallback: T, body: () => T): T {
	try {
		return body();
	} catch (error) {
		connection.console.error(`Raven Script: ${(error as Error).stack || error}`);
		core = ScriptCore.load();
		return fallback;
	}
}

connection.onCompletion((params) => guarded([], () => {
	const place = at(params.textDocument.uri, params.position, true);
	return place ? features.completion(core, place.project, place.document.uri, place.offset) : [];
}));

connection.onHover((params) => guarded(null, () => {
	const place = at(params.textDocument.uri, params.position);
	return place ? features.hover(core, place.project, place.document.uri, place.offset) : null;
}));

connection.onSignatureHelp((params) => guarded(null, () => {
	const place = at(params.textDocument.uri, params.position);
	return place ? features.signatureHelp(place.project, place.document.uri, place.offset) : null;
}));

connection.onDefinition((params) => guarded(null, () => {
	const place = at(params.textDocument.uri, params.position);
	return place ? features.definition(place.project, place.document.uri, place.offset) : null;
}));

connection.onReferences((params) => guarded([], () => {
	const place = at(params.textDocument.uri, params.position);
	return place ? features.references(place.project, place.document.uri, place.offset, params.context.includeDeclaration) : [];
}));

connection.onPrepareRename((params) => guarded(null, () => {
	const place = at(params.textDocument.uri, params.position);
	return place ? features.prepareRename(place.project, place.document.uri, place.offset) : null;
}));

connection.onRenameRequest((params) => guarded(null, () => {
	const place = at(params.textDocument.uri, params.position);
	if (!place) return null;
	const result = features.rename(place.project, place.document.uri, place.offset, params.newName);
	if (typeof result === "string") throw new Error(result);
	return result;
}));

connection.onDocumentSymbol((params) => guarded([], () => {
	const document = documents.get(params.textDocument.uri);
	const project = document ? projectOf(document) : null;
	return project && document ? features.documentSymbols(project, document.uri) : [];
}));

connection.onDocumentFormatting((params) => guarded([], () => {
	const document = documents.get(params.textDocument.uri);
	if (!document) return [];
	const indent = features.indentOf(params.options);
	const kind = kindOf(document);
	if (kind === "rvs") return features.formatScript(core, document.getText(), indent);
	return [];
}));

/** The command "Raven: Format the scripts" of an XML document. */
connection.onRequest("raven/formatScripts", (params: { uri: string; tabSize: number; insertSpaces: boolean }) => guarded([], () => {
	const document = documents.get(params.uri);
	if (!document || kindOf(document) !== "xml") return [];
	const project = analyze(core, document.uri, document.getText(), "xml", files, { declared });
	return features.formatXmlScripts(core, project, features.indentOf(params));
}));

documents.listen(connection);
connection.listen();
