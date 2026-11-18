/**
 * The little of WebAssembly the server uses (Node has it; the "ES2022" lib
 * of TypeScript does not declare it, and the "DOM" lib would bring the
 * browser with it).
 */
declare namespace WebAssembly {
	class Module {
		constructor(bytes: Uint8Array);
		static imports(module: Module): { module: string; name: string; kind: string }[];
		static exports(module: Module): { name: string; kind: string }[];
	}
	class Instance {
		constructor(module: Module, imports?: Record<string, Record<string, unknown>>);
		readonly exports: Record<string, unknown>;
	}
	class Memory {
		readonly buffer: ArrayBuffer;
	}
}
