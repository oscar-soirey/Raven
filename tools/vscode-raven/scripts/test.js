/**
 * npm test: the tests of the XML features (test/run.ts) and of the language
 * server of Raven Script (test/script.ts), bundled and run with Node. Both
 * run, whatever the first gives; the exit code is not 0 when one fails.
 */
const { execFileSync, spawnSync } = require("child_process");
const path = require("path");

const root = path.resolve(__dirname, "..");
const esbuild = path.join(root, "node_modules", ".bin", process.platform === "win32" ? "esbuild.cmd" : "esbuild");
const suites = [
	["test/run.ts", "out/test.js"],
	["test/script.ts", "out/script.js"],
];
let failed = false;
for (const [source, output] of suites) {
	execFileSync(esbuild, [source, "--bundle", "--platform=node", "--format=cjs", `--outfile=${output}`, "--log-level=warning"], { cwd: root, stdio: "inherit", shell: process.platform === "win32" });
	const result = spawnSync(process.execPath, [output], { cwd: root, stdio: "inherit" });
	if (result.status !== 0) failed = true;
}
process.exit(failed ? 1 : 0);
