/**
 * The core of Raven Script, compiled to WebAssembly from src/script/rs_*.c
 * (wasm/raven_script.wasm, made by scripts/build-wasm.js). The editor
 * checks scripts with the very code the library runs.
 */
import * as fs from "fs";
import * as path from "path";

export interface CoreLocation {
	unit: number;
	start: number;
	end: number;
	file?: string;
	line: number;
	column: number;
}

export interface CoreDiagnostic {
	/** 0 error, 1 warning, 2 information */
	severity: number;
	code: string;
	title: string;
	message: string;
	help: string;
	location: CoreLocation | null;
	related: CoreLocation | null;
	relatedNote?: string;
}

export interface CoreSymbol {
	id: number;
	kind: "function" | "declared" | "native" | "global" | "const" | "local" | "localConst" | "parameter" | "external";
	name: string;
	type: string;
	signature?: string;
	/** -1: declared outside of the units (a native, the application) */
	unit: number;
	start: number;
	end: number;
	scopeEnd: number;
	function: number;
	doc?: string;
}

export interface CoreCall {
	unit: number;
	open: number;
	close: number;
	commas: number[];
	label: string;
	doc: string;
	parameters: [number, number][];
}

export interface CoreFunction {
	name: string;
	signature: string;
	declare: boolean;
	symbol: number;
	unit: number;
	start: number;
	end: number;
}

export interface CoreAnalysis {
	diagnostics: CoreDiagnostic[];
	units: { file: string; length: number }[];
	symbols: CoreSymbol[];
	/** [symbol, unit, start, end] */
	references: [number, number, number, number][];
	hovers: { unit: number; start: number; end: number; text: string }[];
	calls: CoreCall[];
	functions: CoreFunction[];
}

export interface CoreCompletion {
	label: string;
	kind: "function" | "method" | "property" | "variable" | "constant" | "parameter" | "keyword" | "module" | "type";
	detail?: string;
	doc?: string;
	insertText?: string;
	snippet?: boolean;
}

export interface CoreUnit {
	file: string;
	text: string;
	line: number;
	column: number;
}

export interface CoreHandler {
	tag: string;
	attribute: string;
	action: string;
	line: number;
	column: number;
}

export interface CoreRun {
	errors: number;
	analysis: CoreAnalysis;
	handlers: CoreDiagnostic[][];
	/** Completion at a byte offset of a unit, when asked. */
	completion?: CoreCompletion[];
}

interface Exports {
	memory: WebAssembly.Memory;
	rsw_alloc(size: number): number;
	rsw_free(pointer: number): void;
	rsw_begin(): void;
	rsw_end(): void;
	rsw_declare(name: number, signature: number): number;
	rsw_set_names(names: number): void;
	rsw_add(file: number, text: number, length: number, line: number, column: number): number;
	rsw_compile(): number;
	rsw_check_handler(tag: number, attribute: number, action: number, line: number, column: number): number;
	rsw_analysis(): number;
	rsw_complete(unit: number, offset: number): number;
	rsw_format(text: number, length: number, indent: number, prefix: number): number;
	rsw_language(): number;
}

const encoder = new TextEncoder();
const decoder = new TextDecoder("utf-8");

export class ScriptCore {
	private exports: Exports;

	private constructor(exports: Exports) {
		this.exports = exports;
	}

	/** Loads the module (wasm/raven_script.wasm next to the extension, or the path given). */
	static load(file?: string): ScriptCore {
		const where = file || ScriptCore.defaultPath();
		const module = new WebAssembly.Module(fs.readFileSync(where));
		let memory: WebAssembly.Memory | null = null;
		/* the module only writes to stderr when it runs out of memory: no file, no clock, no exit */
		const wasi = {
			fd_write: (_fd: number, iovs: number, count: number, written: number) => {
				if (!memory) return 0;
				const view = new DataView(memory.buffer);
				let total = 0;
				for (let i = 0; i < count; i++) total += view.getUint32(iovs + i * 8 + 4, true);
				view.setUint32(written, total, true);
				return 0;
			},
			fd_close: () => 0,
			fd_seek: () => 0,
		};
		const instance = new WebAssembly.Instance(module, { wasi_snapshot_preview1: wasi });
		const exports = instance.exports as unknown as Exports;
		memory = exports.memory;
		return new ScriptCore(exports);
	}

	static defaultPath(): string {
		/* dist/server.js -> ../wasm, src/server/scriptCore.ts -> ../../wasm (tests) */
		const candidates = [path.join(__dirname, "..", "wasm", "raven_script.wasm"), path.join(__dirname, "..", "..", "wasm", "raven_script.wasm")];
		return candidates.find((c) => fs.existsSync(c)) || candidates[0];
	}

	private string(text: string): number {
		const bytes = encoder.encode(text);
		const pointer = this.exports.rsw_alloc(bytes.length + 1);
		const memory = new Uint8Array(this.exports.memory.buffer, pointer, bytes.length + 1);
		memory.set(bytes);
		memory[bytes.length] = 0;
		return pointer;
	}

	private bytes(bytes: Uint8Array): number {
		const pointer = this.exports.rsw_alloc(bytes.length + 1);
		const memory = new Uint8Array(this.exports.memory.buffer, pointer, bytes.length + 1);
		memory.set(bytes);
		memory[bytes.length] = 0;
		return pointer;
	}

	private read(pointer: number): string | null {
		if (!pointer) return null;
		const memory = new Uint8Array(this.exports.memory.buffer);
		let end = pointer;
		while (memory[end] !== 0) end++;
		return decoder.decode(memory.subarray(pointer, end));
	}

	private withStrings<T>(texts: string[], body: (pointers: number[]) => T): T {
		const pointers = texts.map((t) => this.string(t));
		try {
			return body(pointers);
		} finally {
			for (const p of pointers) this.exports.rsw_free(p);
		}
	}

	/**
	 * Compiles units together (the scripts of a document), checks the
	 * actions of its widgets, and returns everything the editor needs.
	 * `declared`: functions the application binds ("name" -> "(a: number): bool").
	 * `names`: the names of the widgets of the document (null: unknown).
	 */
	run(units: CoreUnit[], handlers: CoreHandler[], declared: Map<string, string>, names: string[] | null, complete?: { unit: number; offset: number }): CoreRun {
		const e = this.exports;
		e.rsw_begin();
		try {
			for (const [name, signature] of declared) this.withStrings([name, signature], ([n, s]) => e.rsw_declare(n, s));
			if (names) this.withStrings([names.join("\n")], ([n]) => e.rsw_set_names(n));
			for (const unit of units) {
				const bytes = encoder.encode(unit.text);
				const text = this.bytes(bytes);
				this.withStrings([unit.file], ([file]) => e.rsw_add(file, text, bytes.length, unit.line, unit.column));
				e.rsw_free(text);
			}
			const errors = e.rsw_compile();
			const results: CoreDiagnostic[][] = handlers.map((h) =>
				this.withStrings([h.tag, h.attribute, h.action], ([tag, attribute, action]) => JSON.parse(this.read(e.rsw_check_handler(tag, attribute, action, h.line, h.column)) || "[]")),
			);
			const analysis = JSON.parse(this.read(e.rsw_analysis()) || "{}") as CoreAnalysis;
			const run: CoreRun = { errors, analysis, handlers: results };
			if (complete) run.completion = JSON.parse(this.read(e.rsw_complete(complete.unit, complete.offset)) || "[]");
			return run;
		} finally {
			e.rsw_end();
		}
	}

	/** The text formatted, or null when it has a syntax error. */
	format(text: string, indent: string, prefix = ""): string | null {
		const e = this.exports;
		const bytes = encoder.encode(text);
		const pointer = this.bytes(bytes);
		try {
			return this.withStrings([indent, prefix], ([i, p]) => this.read(e.rsw_format(pointer, bytes.length, i, p)));
		} finally {
			e.rsw_free(pointer);
		}
	}

	/** The natives and the events of the widgets. */
	language(): { natives: { name: string; signature: string; doc: string }[]; events: { tags: string; event: string; description: string; parameters: { name: string; type: string }[] }[] } {
		return JSON.parse(this.read(this.exports.rsw_language()) || "{}");
	}
}
