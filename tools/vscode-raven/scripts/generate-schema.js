#!/usr/bin/env node
/**
 * Builds src/schema.json from the sources of Raven, which it only reads:
 *
 *   src/xml/xml_schema.c   tags, attributes, kinds of values, descriptions
 *   src/widget/registry.c  other names of the tags (TextBlock, Notch...)
 *   src/widgets/*.c        which classes take no child or a single one
 *
 * Usage: node scripts/generate-schema.js [path of the Raven repository]
 * (default: the repository that holds this folder, ../..).
 */
"use strict";

const fs = require("fs");
const path = require("path");

const root = path.resolve(process.argv[2] || path.join(__dirname, "..", "..", ".."));
const out = path.join(__dirname, "..", "src", "schema.json");

function read(rel) {
	const file = path.join(root, rel);
	if (!fs.existsSync(file)) {
		console.error(`generate-schema: ${file} not found (give the path of the Raven repository)`);
		process.exit(1);
	}
	return fs.readFileSync(file, "utf8");
}

function stripComments(text) {
	let result = "";
	let i = 0;
	while (i < text.length) {
		const c = text[i];
		if (c === '"' || c === "'") {
			const start = i++;
			while (i < text.length && text[i] !== c) i += text[i] === "\\" ? 2 : 1;
			result += text.slice(start, ++i);
		} else if (c === "/" && text[i + 1] === "*") {
			const end = text.indexOf("*/", i + 2);
			i = end < 0 ? text.length : end + 2;
			result += " ";
		} else if (c === "/" && text[i + 1] === "/") {
			while (i < text.length && text[i] !== "\n") i++;
		} else {
			result += c;
			i++;
		}
	}
	return result;
}

/** Tokens of C: strings (unescaped), identifiers, punctuation. */
function tokenize(text) {
	const tokens = [];
	let i = 0;
	while (i < text.length) {
		const c = text[i];
		if (/\s/.test(c)) { i++; continue; }
		if (c === '"') {
			let value = "";
			i++;
			while (i < text.length && text[i] !== '"') {
				if (text[i] === "\\") {
					const n = text[i + 1];
					value += n === "n" ? "\n" : n === "t" ? "\t" : n;
					i += 2;
				} else {
					value += text[i++];
				}
			}
			i++;
			const last = tokens[tokens.length - 1];
			/* "a" "b" is one string in C */
			if (last && last.type === "string") last.value += value;
			else tokens.push({ type: "string", value });
			continue;
		}
		const id = /^[A-Za-z_][A-Za-z0-9_]*/.exec(text.slice(i, i + 200));
		if (id) {
			tokens.push({ type: "id", value: id[0] });
			i += id[0].length;
			continue;
		}
		tokens.push({ type: "punct", value: c });
		i++;
	}
	return tokens;
}

const schemaText = stripComments(read("src/xml/xml_schema.c"));

/* #define NAME "text" */
const macros = {};
for (const m of schemaText.matchAll(/^[ \t]*#define[ \t]+([A-Z_][A-Z0-9_]*)[ \t]+((?:"(?:[^"\\]|\\.)*"[ \t]*)+)$/gm)) {
	macros[m[1]] = tokenize(m[2])[0].value;
}

const KINDS = { T: "text", B: "bool", N: "number", C: "choice", K: "color", A: "action", F: "file", V: "numbers", L: "list" };

function argument(token) {
	if (!token) return null;
	if (token.type === "string") return token.value;
	if (token.type === "id" && token.value === "NULL") return null;
	if (token.type === "id" && token.value in macros) return macros[token.value];
	throw new Error(`generate-schema: unexpected ${token.value} in xml_schema.c`);
}

/** The calls MACRO(a, b, c) between two indexes of tokens. */
function calls(tokens, from, to) {
	const result = [];
	for (let i = from; i < to; i++) {
		const t = tokens[i];
		if (t.type !== "id" || !tokens[i + 1] || tokens[i + 1].value !== "(") continue;
		const args = [];
		let j = i + 2;
		let current = [];
		for (; j < to && tokens[j].value !== ")"; j++) {
			if (tokens[j].value === ",") { args.push(current); current = []; } else current.push(tokens[j]);
		}
		args.push(current);
		result.push({ name: t.value, args: args.map((a) => a[0]) });
		i = j;
	}
	return result;
}

const tokens = tokenize(schemaText);
const tables = {};
let tags = [];

for (let i = 0; i < tokens.length; i++) {
	/* static const rvn_schema_attribute_t g_name[] = { ... }; */
	if (tokens[i].value === "rvn_schema_attribute_t" && tokens[i + 1] && tokens[i + 1].type === "id" && tokens[i + 2] && tokens[i + 2].value === "[") {
		const name = tokens[i + 1].value;
		let open = i;
		while (tokens[open].value !== "{") open++;
		let close = open;
		while (tokens[close].value !== "}" || tokens[close + 1].value !== ";") close++;
		tables[name] = calls(tokens, open + 1, close)
			.filter((c) => c.name in KINDS)
			.map((c) => {
				const kind = KINDS[c.name];
				const attribute = { name: argument(c.args[0]), kind };
				if (kind === "choice") {
					attribute.choices = argument(c.args[1]).split("|");
					attribute.description = argument(c.args[2]) || "";
				} else {
					attribute.description = argument(c.args[1]) || "";
				}
				return attribute;
			});
		i = close;
	}
	/* static const rvn_schema_tag_t g_tags[] = { TAG(...), ... }; */
	if (tokens[i].value === "rvn_schema_tag_t" && tokens[i + 1] && tokens[i + 1].value === "g_tags") {
		let open = i;
		while (tokens[open].value !== "{") open++;
		let close = open;
		while (tokens[close].value !== "}" || tokens[close + 1].value !== ";") close++;
		tags = calls(tokens, open + 1, close).map((c) => {
			if (c.name === "TAG") return { name: argument(c.args[0]), table: c.args[1].value, description: argument(c.args[2]) };
			if (c.name === "TAG_PLAIN") return { name: argument(c.args[0]), table: null, description: argument(c.args[1]) };
			throw new Error(`generate-schema: unexpected ${c.name} in g_tags`);
		});
		i = close;
	}
}

if (!tags.length) throw new Error("generate-schema: g_tags not found in xml_schema.c");

/* the groups every widget has, in the order of Rxml_get_attribute_info */
const SHARED = [["g_widget", "Widget"], ["g_slot", "Slot"], ["g_touch", "Touch"], ["g_colors", "Colors"], ["g_metrics", "Metrics"]];
const shared = {};
for (const [table, group] of SHARED) {
	if (!tables[table]) throw new Error(`generate-schema: ${table} not found in xml_schema.c`);
	shared[group] = tables[table];
}

/* other names of the tags */
const registry = stripComments(read("src/widget/registry.c"));
const aliases = {};
const typeOfAlias = {};
for (const m of registry.matchAll(/\{\s*"([A-Za-z]+)"\s*,\s*(R_WIDGET_[A-Z_]+)\s*\}/g)) typeOfAlias[m[1]] = m[2];

/* class name, type and flags of each widget class */
const classes = {};
const widgetsDir = path.join(root, "src", "widgets");
for (const file of fs.readdirSync(widgetsDir).filter((f) => f.endsWith(".c"))) {
	const text = stripComments(fs.readFileSync(path.join(widgetsDir, file), "utf8"));
	for (const m of text.matchAll(/rvn_widget_class_t\s+rvn_class_\w+\s*=\s*\{([\s\S]*?)\n\};/g)) {
		const body = m[1];
		const name = /\.name\s*=\s*"([^"]+)"/.exec(body);
		const type = /\.type\s*=\s*(R_WIDGET_[A-Z_]+)/.exec(body);
		const flags = /\.flags\s*=\s*([^,\n]+)/.exec(body);
		if (!name) continue;
		/* the names its set_attribute function accepts: some are other names of those of the schema */
		const accepted = [];
		const setter = /\.set_attribute\s*=\s*(\w+)/.exec(body);
		if (setter) {
			const start = text.search(new RegExp(`\\b${setter[1]}\\s*\\([^;{]*\\)\\s*\\{`));
			if (start >= 0) {
				const end = text.indexOf("\n}", start);
				for (const a of text.slice(start, end < 0 ? undefined : end).matchAll(/RVN_ATTR\("([^"]+)"\)/g)) accepted.push(a[1]);
			}
		}
		classes[name[1]] = {
			type: type ? type[1] : null,
			flags: flags ? flags[1] : "",
			xmlChild: /\.xml_child\s*=/.test(body),
			accepted,
		};
	}
}
for (const [alias, type] of Object.entries(typeOfAlias)) {
	const cls = Object.entries(classes).find(([, c]) => c.type === type);
	if (cls) aliases[alias] = cls[0];
}

const schema = {
	generated: "by scripts/generate-schema.js from src/xml/xml_schema.c, src/widget/registry.c and src/widgets/*.c of Raven: do not edit",
	shared,
	windowTouch: tables.g_window_touch || [],
	tags: tags.map((tag) => {
		const cls = classes[tag.name];
		let children = "many";
		if (tag.name === "Window") children = "single";
		else if (cls && /RVN_CF_NO_CHILDREN/.test(cls.flags)) children = "none";
		else if (cls && /RVN_CF_SINGLE_CHILD/.test(cls.flags)) children = "single";
		const attributes = tag.table ? tables[tag.table] || [] : [];
		const known = new Set(attributes.map((a) => a.name.toLowerCase()));
		const sharedNames = new Set(Object.values(shared).flat().map((a) => a.name.toLowerCase()));
		const synonyms = [...new Set((cls ? cls.accepted : []).filter((a) => !known.has(a.toLowerCase()) && !sharedNames.has(a.toLowerCase())))];
		return {
			name: tag.name,
			description: tag.description,
			children,
			attributes,
			/* accepted by the widget, not described by the schema (other names, older names) */
			accepted: synonyms,
		};
	}),
	aliases,
};

fs.writeFileSync(out, JSON.stringify(schema, null, "\t") + "\n");
const count = schema.tags.reduce((n, t) => n + t.attributes.length, 0);
console.log(`generate-schema: ${schema.tags.length} tags, ${count} attributes of their own, ${Object.keys(aliases).length} aliases -> ${path.relative(process.cwd(), out)}`);
