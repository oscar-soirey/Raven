/**
 * Compiles the core of Raven Script (src/script/rs_*.c) to WebAssembly for
 * the language server: wasm/raven_script.wasm.
 *
 * The file is kept in the repository, so that the extension builds without
 * this step; run it again (npm run wasm) after a change in src/script.
 *
 * Needs clang with the wasm32 target and a WASI sysroot:
 *   - Ubuntu / Debian: apt install clang lld wasi-libc libclang-rt-18-dev-wasm32
 *     (the sysroot is /usr);
 *   - elsewhere: the wasi-sdk (https://github.com/WebAssembly/wasi-sdk), with
 *     CLANG=<wasi-sdk>/bin/clang WASI_SYSROOT=<wasi-sdk>/share/wasi-sysroot.
 */
const { execFileSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const core = path.resolve(root, "../../src/script");
const output = path.join(root, "wasm", "raven_script.wasm");
const clang = process.env.CLANG || "clang";
const sysroot = process.env.WASI_SYSROOT || "/usr";

const sources = [
	"rs_common.c", "rs_builtins.c", "rs_lexer.c", "rs_parser.c", "rs_checker.c", "rs_compiler.c",
	"rs_vm.c", "rs_batch.c", "rs_format.c", "rs_analysis.c",
].map((file) => path.join(core, file));

const args = [
	"--target=wasm32-wasi",
	`--sysroot=${sysroot}`,
	"-std=c11",
	"-O2",
	"-Wall",
	"-Wextra",
	"-nostartfiles",
	"-Wl,--no-entry",
	"-Wl,--strip-all",
	/* (the stack of the parser and of the checker: deeply nested scripts are refused before they need more) */
	"-Wl,-z,stack-size=1048576",
	"-o",
	output,
	path.join(root, "wasm", "raven_script_wasm.c"),
	...sources,
];

execFileSync(clang, args, { stdio: "inherit" });
console.log(`${path.relative(process.cwd(), output)}: ${fs.statSync(output).size} bytes`);
