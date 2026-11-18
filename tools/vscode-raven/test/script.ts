/**
 * Tests of the language server of Raven Script (src/server), without VS
 * Code: the core in WebAssembly, the projects of documents, every feature,
 * the TextMate grammars, and the server itself over the protocol.
 */
import * as assert from "assert";
import * as fs from "fs";
import * as os from "os";
import * as path from "path";
import { spawn } from "child_process";
import { ScriptCore } from "../src/server/scriptCore";
import { analyze, ByteMap, Files, Project, resolveUri, Text, pathToFileUri, actionName } from "../src/server/project";
import * as features from "../src/server/features";
import { parseXml } from "../src/core/xmlParser";
import { diagnose } from "../src/core/diagnostics";
import { emptyContext } from "../src/core/model";
import { complete as completeXml } from "../src/core/completion";

let failures = 0;
let count = 0;
const pending: Promise<void>[] = [];
function test(name: string, body: () => void | Promise<void>) {
	count++;
	const fail = (e: unknown) => {
		failures++;
		console.error(`FAIL ${name}\n  ${((e as Error).stack || String(e)).split("\n").slice(0, 8).join("\n  ")}`);
	};
	try {
		const result = body();
		if (result instanceof Promise) pending.push(result.catch(fail));
	} catch (e) {
		fail(e);
	}
}

const core = ScriptCore.load(path.join(__dirname, "..", "wasm", "raven_script.wasm"));

/* files in memory */
const memory = new Map<string, string>();
const files: Files = {
	read: (uri) => (memory.has(uri) ? memory.get(uri)! : null),
	resolve: resolveUri,
	display: (uri) => path.basename(uri),
};
const settings = { declared: new Map<string, string>() };

/** A document with '|' marking a place: the text without it, and the offset. */
function place(text: string): { text: string; offset: number } {
	const offset = text.indexOf("|");
	return { text: text.slice(0, offset) + text.slice(offset + 1), offset };
}

function xml(text: string, complete?: number, uri = "file:///w/ui.xml"): Project {
	memory.set(uri, text);
	return analyze(core, uri, text, "xml", files, settings, complete === undefined ? undefined : { uri, offset: complete });
}

function rvs(text: string, complete?: number, uri = "file:///w/main.rvs"): Project {
	memory.set(uri, text);
	return analyze(core, uri, text, "rvs", files, settings, complete === undefined ? undefined : { uri, offset: complete });
}

const codes = (project: Project) => features.diagnostics(project).map((d) => d.code).join(" ");

// --- the core --------------------------------------------------------------

test("the core loads and knows the language", () => {
	const language = core.language();
	assert.ok(language.natives.some((n) => n.name === "app.log"));
	assert.ok(language.events.some((e) => e.event === "Slider.OnChanged"));
});

test("bytes and characters", () => {
	const map = new ByteMap("aé😀b");
	assert.strictEqual(map.utf16(0), 0);
	assert.strictEqual(map.utf16(1), 1); /* é starts at byte 1 */
	assert.strictEqual(map.utf16(3), 2); /* 😀 at byte 3, UTF-16 2 */
	assert.strictEqual(map.utf16(7), 4); /* b */
	assert.strictEqual(map.byte(4), 7);
	assert.strictEqual(map.byte(2), 3);
	const text = new Text("ab\ncd\n");
	assert.deepStrictEqual(text.positionAt(4), { line: 1, character: 1 });
	assert.strictEqual(text.offsetAt({ line: 1, character: 1 }), 4);
	assert.strictEqual(actionName("app.quit()").name, "app.quit");
	assert.strictEqual(actionName("show:panel").name, "show:panel");
});

// --- diagnostics -----------------------------------------------------------

test("a correct document has no diagnostic", () => {
	const project = xml(`<Window>\n  <Script>\n    let n = 0;\n    fn app.go() {\n        if (n < 3 && true) {\n            n += 1;\n        }\n    }\n  </Script>\n  <Button OnClicked="app.go()"/>\n</Window>`);
	assert.strictEqual(codes(project), "");
	/* the XML features see the script as text, not as tags, and complete nothing inside */
	assert.deepStrictEqual(diagnose(parseXml(project.document.text), emptyContext()).map((p) => p.message), []);
	assert.deepStrictEqual(completeXml(parseXml(project.document.text), project.document.text.indexOf("n < 3"), emptyContext()), []);
	/* <Script> is known to them: offered where elements go, no unknown tag */
	const offered = completeXml(parseXml("<Window><VBox><</VBox></Window>"), 15, emptyContext()).map((i) => i.label);
	assert.ok(offered.includes("Script"), offered.join(","));
});

test("type errors are placed in the document", () => {
	const text = `<Window>\n  <Script>\n    let count: number = "hello";\n  </Script>\n  <Label/>\n</Window>`;
	const project = xml(text);
	const list = features.diagnostics(project);
	assert.strictEqual(list.length, 1);
	assert.strictEqual(list[0].code, "RS102");
	assert.deepStrictEqual(list[0].range, { start: { line: 2, character: 24 }, end: { line: 2, character: 31 } });
	assert.ok(list[0].message.startsWith("Type mismatch.\nExpected 'number', but got 'string'."), list[0].message);
});

test("event handler signatures (12.5, 12.6)", () => {
	const text = `<Window>\n<Script>\nfn app.v(value: bool) {\n}\nfn app.two(a: number, b: bool) {\n}\n</Script>\n<VBox>\n<Slider OnChanged="app.v"/>\n<Slider OnChanged="app.two"/>\n<Checkbox OnChanged="app.v"/>\n</VBox>\n</Window>`;
	const project = xml(text);
	const list = features.diagnostics(project);
	assert.strictEqual(list.map((d) => d.code).join(" "), "RS103 RS103");
	const first = list[0];
	assert.strictEqual(text.slice(project.document.offsetAt(first.range.start), project.document.offsetAt(first.range.end)), "app.v");
	assert.ok(first.message.includes("Slider.OnChanged expects a function with"), first.message);
	assert.ok(first.relatedInformation && first.relatedInformation[0].location.range.start.line === 2);
	assert.ok(list[1].message.includes("provides 1 argument"), list[1].message);
});

test("syntax errors keep the rest of the analysis", () => {
	const project = rvs(`fn app.a() {\n    let x = ;\n}\nfn app.b() {\n    let y: string = 1;\n}\n`);
	assert.strictEqual(codes(project), "RS002 RS102");
});

test("warnings and the names of the widgets", () => {
	const project = xml(`<Window>\n<Script>\nfn app.a() {\n    ui.setText("title", "x");\n    ui.setText("nothing", "x");\n}\n</Script>\n<Label Name="title"/>\n</Window>`);
	const list = features.diagnostics(project);
	assert.strictEqual(list.map((d) => d.code).join(" "), "RS301");
	assert.strictEqual(list[0].severity, 2);
});

test("a .rvs named by a document: summed up on its Src, and seen with the functions of the document", () => {
	memory.set("file:///w/lib.rvs", `fn lib.twice(x: number): number {\n    return x * 2;\n}\nfn lib.bad() {\n    let s: string = 1;\n}\n`);
	const project = xml(`<Window>\n<Script Src="lib.rvs"/>\n<Script>\nfn app.go() {\n    app.log(lib.twice(2).toString());\n}\n</Script>\n<Script Src="missing.rvs"/>\n<Button OnClicked="app.go"/>\n</Window>`);
	const list = features.diagnostics(project);
	assert.strictEqual(list.map((d) => d.code).join(" "), "RS102 RS101");
	assert.ok(list[0].message.startsWith("lib.rvs: 1 error"), list[0].message);
	assert.ok(list[1].message.includes("missing.rvs"));
	/* definition across files */
	const offset = project.document.text.indexOf("lib.twice(2)") + 5;
	const location = features.definition(project, project.uri, offset);
	assert.ok(location && location.uri === "file:///w/lib.rvs" && location.range.start.line === 0, JSON.stringify(location));
});

// --- hover, definition, references, rename ---------------------------------

const counter = `<Window>
  <Script>
    let counter: number = 0;
    fn app.increment(step: number) {
        counter += step;
        ui.setText("counter", counter.toString());
    }
  </Script>
  <VBox>
    <Label Name="counter"/>
    <Slider OnChanged="app.increment"/>
    <Slider OnChanged="app.increment()"/>
  </VBox>
</Window>`;

test("hover", () => {
	const project = xml(counter);
	const on = (needle: string, delta = 1) => features.hover(core, project, project.uri, counter.indexOf(needle) + delta);
	assert.ok(JSON.stringify(on("counter += ")).includes("let counter: number"));
	assert.ok(JSON.stringify(on("step;")).includes("(parameter) step: number"));
	assert.ok(JSON.stringify(on("ui.setText", 4)).includes("(native) fn ui.setText(id: string, text: string): void"));
	assert.ok(JSON.stringify(on("toString", 2)).includes("(method) number.toString(): string"));
	/* the action of a widget */
	const action = on('"app.increment"/>', 3);
	assert.ok(JSON.stringify(action).includes("fn app.increment(step: number): void"), JSON.stringify(action));
	assert.ok(JSON.stringify(action).includes("Slider.OnChanged gives (value: number)"));
});

test("definition and references", () => {
	const project = xml(counter);
	const use = counter.indexOf("counter += ") + 2;
	const location = features.definition(project, project.uri, use)!;
	assert.strictEqual(location.range.start.line, 2);
	assert.strictEqual(location.range.start.character, 8);
	const refs = features.references(project, project.uri, use, true);
	assert.strictEqual(refs.length, 3);
	/* a function: its declaration and the actions that name it */
	const fn = counter.indexOf('OnChanged="app.increment"') + 13;
	const all = features.references(project, project.uri, fn, true);
	assert.strictEqual(all.length, 3);
	assert.ok(features.definition(project, project.uri, fn)!.range.start.line === 3);
	/* a native has no place to go */
	assert.strictEqual(features.definition(project, project.uri, counter.indexOf("ui.setText") + 4), null);
});

test("rename", () => {
	const project = xml(counter);
	const fn = counter.indexOf("fn app.increment") + 5;
	assert.ok(features.prepareRename(project, project.uri, fn));
	const edit = features.rename(project, project.uri, fn, "app.add") as { changes: Record<string, { range: unknown; newText: string }[]> };
	const edits = edit.changes[project.uri];
	assert.strictEqual(edits.length, 3);
	/* applied: everything still fits */
	const document = new Text(counter);
	const sorted = [...edits].sort((a, b) => document.offsetAt((b.range as { start: { line: number; character: number } }).start) - document.offsetAt((a.range as { start: { line: number; character: number } }).start));
	let renamed = counter;
	for (const e of sorted) {
		const r = e.range as { start: { line: number; character: number }; end: { line: number; character: number } };
		renamed = renamed.slice(0, document.offsetAt(r.start)) + e.newText + renamed.slice(document.offsetAt(r.end));
	}
	assert.ok(renamed.includes("fn app.add(step") && renamed.includes('OnChanged="app.add"') && renamed.includes('OnChanged="app.add()"'), renamed);
	assert.strictEqual(codes(xml(renamed)), "");
	/* a variable */
	const variable = features.rename(project, project.uri, counter.indexOf("counter += ") + 1, "total") as { changes: Record<string, unknown[]> };
	assert.strictEqual(variable.changes[project.uri].length, 3);
	/* invalid names, natives */
	assert.strictEqual(typeof features.rename(project, project.uri, counter.indexOf("counter += ") + 1, "for"), "string");
	assert.strictEqual(typeof features.rename(project, project.uri, fn, "app."), "string");
	assert.strictEqual(features.prepareRename(project, project.uri, counter.indexOf("ui.setText") + 4), null);
});

test("symbols", () => {
	const project = xml(counter);
	const symbols = features.documentSymbols(project, project.uri);
	assert.deepStrictEqual(symbols.map((s) => s.name).sort(), ["app.increment", "counter"]);
});

// --- completion, signature help -------------------------------------------

test("completion in a script", () => {
	const after = place(counter.replace("ui.setText", "ui.|setText"));
	const items = features.completion(core, xml(after.text, after.offset), "file:///w/ui.xml", after.offset);
	assert.ok(items.some((i) => i.label === "setText"), items.map((i) => i.label).join(","));
	assert.ok(!items.some((i) => i.label === "log"));
	const word = place(counter.replace("counter += step;", "counter += st|;"));
	const words = features.completion(core, xml(word.text, word.offset), "file:///w/ui.xml", word.offset);
	assert.ok(words.some((i) => i.label === "step") && words.some((i) => i.label === "counter") && words.some((i) => i.label === "app"));
	const member = place(`fn a.b() {\n    let s = "x";\n    s.|\n}\n`);
	const methods = features.completion(core, rvs(member.text, member.offset), "file:///w/main.rvs", member.offset).map((i) => i.label);
	assert.ok(methods.includes("toUpperCase") && methods.includes("length") && !methods.includes("push"), methods.join(","));
});

test("completion of the function of an action", () => {
	const text = place(counter.replace('<Label Name="counter"/>', '<Button OnClicked="|"/>'));
	const items = features.completion(core, xml(text.text, text.offset), "file:///w/ui.xml", text.offset);
	assert.deepStrictEqual(items.map((i) => i.label), ["app.increment"]);
});

test("signature help", () => {
	const text = place(`fn math.add(a: number, b: number): number {\n    return a + b;\n}\nfn x.y() {\n    math.add(1, |2);\n}\n`);
	const help = features.signatureHelp(rvs(text.text), "file:///w/main.rvs", text.offset)!;
	assert.strictEqual(help.signatures[0].label, "math.add(a: number, b: number): number");
	assert.strictEqual(help.activeParameter, 1);
});

// --- formatting ------------------------------------------------------------

test("formatting a .rvs", () => {
	const edits = features.formatScript(core, "fn a.b(){let x=1;}", "    ");
	assert.strictEqual(edits[0].newText, "fn a.b() {\n    let x = 1;\n}\n");
	assert.deepStrictEqual(features.formatScript(core, "fn a.b( {", "    "), []);
});

test("formatting the scripts of a document", () => {
	const text = `<Window>\n    <Script>\nfn app.a(){if(1<2&&true){app.log("x");}}\n    </Script>\n    <Script><![CDATA[\nlet n=1;\n]]></Script>\n    <Label/>\n</Window>\n`;
	const project = xml(text);
	const edits = features.formatXmlScripts(core, project, "    ");
	let result = text;
	for (const e of [...edits].reverse()) result = result.slice(0, project.document.offsetAt(e.range.start)) + e.newText + result.slice(project.document.offsetAt(e.range.end));
	assert.strictEqual(
		result,
		`<Window>\n    <Script>\n        fn app.a() {\n            if (1 < 2 && true) {\n                app.log("x");\n            }\n        }\n    </Script>\n    <Script>\n        <![CDATA[\n        let n = 1;\n        ]]>\n    </Script>\n    <Label/>\n</Window>\n`,
	);
	/* again: nothing to change */
	assert.deepStrictEqual(features.formatXmlScripts(core, xml(result), "    "), []);
});

// --- the grammars ----------------------------------------------------------

test("the grammars color Raven Script, alone and in XML", async () => {
	const textmate = await import("vscode-textmate");
	const oniguruma = await import("vscode-oniguruma");
	const wasm = fs.readFileSync(path.join(__dirname, "..", "node_modules", "vscode-oniguruma", "release", "onig.wasm"));
	await oniguruma.loadWASM(wasm.buffer.slice(wasm.byteOffset, wasm.byteOffset + wasm.byteLength) as ArrayBuffer);
	const xmlGrammar = {
		scopeName: "text.xml",
		patterns: [{ begin: "(</?)([\\w:.-]+)", end: "(/?>)", name: "meta.tag.xml" }, { match: "&\\w+;", name: "constant.character.entity.xml" }],
	};
	const registry = new textmate.Registry({
		onigLib: Promise.resolve({ createOnigScanner: (p: string[]) => new oniguruma.OnigScanner(p), createOnigString: (s: string) => new oniguruma.OnigString(s) }),
		loadGrammar: async (scope: string) => {
			const file = scope === "source.rvs" ? "rvs.tmLanguage.json" : scope === "raven-script.injection.xml" ? "rvs-xml.tmLanguage.json" : null;
			if (scope === "text.xml") return textmate.parseRawGrammar(JSON.stringify(xmlGrammar), "xml.json");
			if (!file) return null;
			const full = path.join(__dirname, "..", "syntaxes", file);
			return textmate.parseRawGrammar(fs.readFileSync(full, "utf8"), full);
		},
		getInjections: (scope: string) => (scope === "text.xml" ? ["raven-script.injection.xml"] : undefined),
	});
	const scopesOf = async (scope: string, lines: string[]) => {
		const grammar = (await registry.loadGrammar(scope))!;
		let state = textmate.INITIAL;
		const tokens: { text: string; scopes: string[] }[] = [];
		for (const line of lines) {
			const result = grammar.tokenizeLine(line, state);
			for (const t of result.tokens) tokens.push({ text: line.slice(t.startIndex, t.endIndex), scopes: t.scopes });
			state = result.ruleStack;
		}
		return tokens;
	};
	const has = (tokens: { text: string; scopes: string[] }[], text: string, scope: string) =>
		assert.ok(tokens.some((t) => t.text.trim() === text && t.scopes.some((s) => s.startsWith(scope))), `${text} -> ${scope}: ${JSON.stringify(tokens.filter((t) => t.text.trim() === text))}`);
	const alone = await scopesOf("source.rvs", ['fn app.save(path: string): bool {', '    let n: number[] = [1, 0x1F];', '    if (n.length < 2 && true) { return "a\\n"; } // done', "}"]);
	has(alone, "fn", "storage.type.function");
	has(alone, "save", "entity.name.function");
	has(alone, "path", "variable.parameter");
	has(alone, "string", "support.type.primitive");
	has(alone, "0x1F", "constant.numeric");
	has(alone, "&&", "keyword.operator.logical");
	has(alone, "\\n", "constant.character.escape");
	has(alone, "done", "comment.line");
	const embedded = await scopesOf("text.xml", ["<Window>", "  <Script>", "    fn app.go() { if (1 < 2) { } }", "  </Script>", '  <Script Src="a.rvs"/>', "  <Label/>", "</Window>"]);
	has(embedded, "go", "entity.name.function");
	has(embedded, "<", "keyword.operator.comparison");
	assert.ok(embedded.some((t) => t.text === "<Label" && t.scopes.includes("meta.tag.xml") && !t.scopes.some((s) => s.includes("rvs"))), "after </Script>: XML again");
});

// --- the server over the protocol ------------------------------------------

test("the server answers over the protocol", async () => {
	const server = path.join(__dirname, "server.js");
	if (!fs.existsSync(server)) {
		/* (built by npm run build; the test bundles it itself when it is missing) */
		const { execFileSync } = await import("child_process");
		execFileSync(path.join(__dirname, "..", "node_modules", ".bin", "esbuild"), ["src/server/server.ts", "--bundle", "--platform=node", "--format=cjs", `--outfile=${server}`, "--log-level=warning"], { cwd: path.join(__dirname, "..") });
	}
	const folder = fs.mkdtempSync(path.join(os.tmpdir(), "raven-lsp-"));
	const uri = pathToFileUri(path.join(folder, "ui.xml"));
	const child = spawn(process.execPath, [server, "--stdio"], { stdio: ["pipe", "pipe", "inherit"] });
	let buffer = Buffer.alloc(0);
	const waiting = new Map<number, (value: unknown) => void>();
	const notifications: { method: string; params: { uri: string; diagnostics: { code: string }[] } }[] = [];
	child.stdout.on("data", (chunk: Buffer) => {
		buffer = Buffer.concat([buffer, chunk]);
		for (;;) {
			const header = buffer.indexOf("\r\n\r\n");
			if (header < 0) return;
			const length = Number(/Content-Length: (\d+)/i.exec(buffer.slice(0, header).toString())![1]);
			if (buffer.length < header + 4 + length) return;
			const message = JSON.parse(buffer.slice(header + 4, header + 4 + length).toString());
			buffer = buffer.slice(header + 4 + length);
			if (message.id !== undefined && waiting.has(message.id)) {
				waiting.get(message.id)!(message.result);
				waiting.delete(message.id);
			} else if (message.method) {
				if (message.id !== undefined) send({ jsonrpc: "2.0", id: message.id, result: null });
				else notifications.push(message);
			}
		}
	});
	const send = (message: object) => {
		const body = JSON.stringify(message);
		child.stdin.write(`Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`);
	};
	let next = 1;
	const request = (method: string, params: object) =>
		new Promise((resolve) => {
			const id = next++;
			waiting.set(id, resolve);
			send({ jsonrpc: "2.0", id, method, params });
		});
	const until = async (check: () => boolean) => {
		for (let i = 0; i < 200 && !check(); i++) await new Promise((r) => setTimeout(r, 25));
		assert.ok(check(), "timed out");
	};
	try {
		const init = (await request("initialize", { processId: process.pid, rootUri: pathToFileUri(folder), capabilities: {}, workspaceFolders: [{ uri: pathToFileUri(folder), name: "w" }] })) as { capabilities: { completionProvider: unknown } };
		assert.ok(init.capabilities.completionProvider);
		send({ jsonrpc: "2.0", method: "initialized", params: {} });
		const text = `<Window>\n<Script>\nfn app.v(value: bool) {\n}\n</Script>\n<Slider OnChanged="app.v"/>\n</Window>\n`;
		send({ jsonrpc: "2.0", method: "textDocument/didOpen", params: { textDocument: { uri, languageId: "xml", version: 1, text } } });
		await until(() => notifications.some((n) => n.method === "textDocument/publishDiagnostics" && n.params.uri === uri));
		const published = notifications.filter((n) => n.method === "textDocument/publishDiagnostics" && n.params.uri === uri).pop()!;
		assert.deepStrictEqual(published.params.diagnostics.map((d) => d.code), ["RS103"]);
		const items = (await request("textDocument/completion", { textDocument: { uri }, position: { line: 3, character: 0 } })) as { label: string }[];
		assert.ok(items.some((i) => i.label === "fn") && items.some((i) => i.label === "value"), JSON.stringify(items.map((i) => i.label)));
		const hover = (await request("textDocument/hover", { textDocument: { uri }, position: { line: 5, character: 22 } })) as { contents: { value: string } };
		assert.ok(hover.contents.value.includes("fn app.v(value: bool): void"));
		await request("shutdown", {});
		send({ jsonrpc: "2.0", method: "exit" });
	} finally {
		child.kill();
		fs.rmSync(folder, { recursive: true, force: true });
	}
});

Promise.all(pending).then(() => {
	console.log(`${count - failures}/${count} script tests passed`);
	if (failures) process.exit(1);
});
