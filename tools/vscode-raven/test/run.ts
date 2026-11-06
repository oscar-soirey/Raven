/**
 * Tests of src/core (no VS Code needed): npm test.
 * With the repository of Raven around (../..), the examples must give no warning.
 */
import * as assert from "assert";
import * as fs from "fs";
import * as path from "path";
import { parseXml } from "../src/core/xmlParser";
import { diagnose, Problem } from "../src/core/diagnostics";
import { complete } from "../src/core/completion";
import { hover } from "../src/core/hover";
import { Context, declarationsOf, emptyContext, looksLikeRaven } from "../src/core/model";
import { checkValue } from "../src/core/values";
import { findAttribute, libraryTag } from "../src/core/schema";

let failures = 0;
let count = 0;
function test(name: string, body: () => void) {
	count++;
	try {
		body();
	} catch (e) {
		failures++;
		console.error(`FAIL ${name}\n  ${(e as Error).message.split("\n").join("\n  ")}`);
	}
}

const ctx = (): Context => {
	const c = emptyContext();
	c.customTags.add("webview");
	return c;
};
const problems = (xml: string, context = ctx()): Problem[] => diagnose(parseXml(xml), context);
const messages = (xml: string) => problems(xml).map((p) => `${p.severity}: ${p.message}`);
/** Completion at the place of '|' in the text. */
const at = (xml: string) => {
	const offset = xml.indexOf("|");
	const text = xml.slice(0, offset) + xml.slice(offset + 1);
	return complete(parseXml(text), offset, ctx());
};
const labels = (xml: string) => at(xml).map((i) => i.label);

// --- parsing --------------------------------------------------------------

test("a correct document has no problem", () => {
	assert.deepStrictEqual(messages(`<?xml version="1.0"?>\n<!-- c -->\n<Window Title="T"><VBox Spacing="8"><Label>Hi</Label><Button Text="Ok" Style="Primary" OnClicked="app.quit"/></VBox></Window>`), []);
});

test("tags are matched without regard to case", () => {
	assert.deepStrictEqual(messages(`<vbox><label text="a"/></VBOX>`), []);
});

test("unclosed and mismatched tags", () => {
	const m = messages(`<VBox><Label></VBox>`);
	assert.ok(m.some((x) => x.includes("<Label> is not closed")), m.join("\n"));
	assert.ok(messages(`<VBox></HBox></VBox>`).some((x) => x.includes("</HBox> closes no open element")));
});

test("values must be quoted, & must be an entity", () => {
	assert.ok(messages(`<VBox Spacing=8/>`).some((x) => x.includes("between quotes")));
	assert.ok(messages(`<Label Text="a & b"/>`).some((x) => x.includes("&amp;")));
	assert.deepStrictEqual(messages(`<Label Text="a &amp; b &#233; &#x41;"/>`), []);
});

test("declarations beside the root", () => {
	assert.deepStrictEqual(messages(`<Widget Name="Card" Title="x"><Label Text="{Title}"/></Widget>\n<VBox><Card Title="a" Slot.Fill="1"/></VBox>`), []);
	assert.ok(messages(`<VBox/><VBox/>`).some((x) => x.includes("single root element")));
});

// --- tags -----------------------------------------------------------------

test("unknown tags, with a suggestion", () => {
	const p = problems(`<VBox><Lable Text="a"></Lable></VBox>`);
	assert.strictEqual(p.length, 1);
	assert.match(p[0].message, /Did you mean <Label>\?/);
	assert.strictEqual(p[0].fix!.edits.length, 2);
});

test("aliases are known", () => {
	assert.deepStrictEqual(messages(`<VerticalBox><TextBlock Text="a"/><CheckBox Checked="true"/></VerticalBox>`), []);
	assert.strictEqual(libraryTag("Notch")!.name, "Island");
});

test("custom tags from the settings", () => {
	assert.deepStrictEqual(messages(`<VBox><WebView Url="https://x" Slot.Fill="1"/></VBox>`), []);
});

test("parts are only read inside their widget", () => {
	assert.deepStrictEqual(messages(`<ComboBox><Item>A</Item><Item Text="B"/></ComboBox>`), []);
	assert.deepStrictEqual(messages(`<TreeView><Item Text="a" Expanded="true"><Item Text="b"/></Item></TreeView>`), []);
	assert.deepStrictEqual(messages(`<Table><Column Header="A" Sortable="true" Align="Right"/><Row Cells="1"/><Row><Cell>x</Cell></Row></Table>`), []);
	const m = messages(`<VBox><Item Text="a"/></VBox>`);
	assert.ok(m[0].includes("only read inside") && m[0].includes("<ComboBox>"), m.join("\n"));
});

test("node graphs and timelines", () => {
	const xml = `<NodeGraph Link.Shape="Step" Preset="Neon"><Type Name="n" Color="#64D2FF" Shape="Diamond"/>
		<Node Name="a" Title="A" X="1" Y="2" Icon="clock.svg" Header.Gradient="#000"><Output Name="v" Type="n"/><Slider Value="0.4"/></Node>
		<Node Name="b"><Input Name="x"/></Node><Link From="a.v" To="b.x"/></NodeGraph>`;
	assert.deepStrictEqual(messages(xml), []);
	assert.deepStrictEqual(messages(`<Timeline Duration="12" Clip.Glow="0.5"><Track Name="Cam" Color="#0A84FF"><Clip Start="0" Length="3.5" Label="Wide"/><Key Time="6"/></Track><Marker Time="4"/></Timeline>`), []);
});

test("children of widgets that take none or one", () => {
	assert.ok(messages(`<Label><Button/></Label>`).some((x) => x.startsWith("error") && x.includes("cannot contain <Button>")));
	assert.ok(messages(`<Border><Label/><Label/></Border>`).some((x) => x.includes("single element")));
	assert.ok(messages(`<Window><VBox/><HBox/></Window>`).some((x) => x.includes("single content element")));
	assert.deepStrictEqual(messages(`<Window><Shortcut Keys="Ctrl+S" Action="file.save"/><Island><Label/><Label/></Island><VBox/></Window>`), []);
	assert.deepStrictEqual(messages(`<Menu Title="File"><MenuItem Text="Open" Shortcut="Ctrl+O"/><Separator/><Menu Text="Recent"/></Menu>`), []);
});

test("composition is seen through", () => {
	assert.deepStrictEqual(messages(`<ComboBox><Repeat Count="3" Var="i"><Item Text="{i}"/></Repeat></ComboBox>`), []);
	assert.deepStrictEqual(messages(`<VBox><ForEach Items="a|b" Var="x"><Label Text="{x}" If="{x != 'a'}"/></ForEach><If Test="true"><Spacer/></If></VBox>`), []);
	assert.ok(messages(`<VBox><Children/></VBox>`).some((x) => x.includes("inside a <Widget>")));
});

// --- attributes and values ------------------------------------------------

test("unknown attributes, with a suggestion", () => {
	const p = problems(`<Label Colour="red"/>`);
	assert.match(p[0].message, /Did you mean Color\?/);
	assert.ok(problems(`<VBox Slot.Fil="1"/>`)[0].message.includes("Slot.Fill"));
	assert.ok(problems(`<VBox Color.Acent="#fff"/>`).length === 1);
});

test("shared attributes and synonyms are accepted", () => {
	assert.deepStrictEqual(messages(`<VBox Name="a" Id="b" Tooltip="t" Color.Accent="#E5484D" Metric.CornerRadius="0" Touch.Mode="Scroll" AccessibleRole="Group" Visibility="Collapsed"/>`), []);
	assert.deepStrictEqual(messages(`<Label FontSize="12" Text="x"/>`), []);
});

test("values are checked as Raven reads them", () => {
	const v = (tag: string, name: string, value: string) => checkValue(findAttribute(libraryTag(tag)!, name)!, value);
	assert.strictEqual(v("Window", "Resizable", "yes"), null);
	assert.notStrictEqual(v("Window", "Resizable", "maybe"), null);
	assert.strictEqual(v("Label", "Opacity", "-0.5e1"), null);
	assert.notStrictEqual(v("Label", "Opacity", "0,5"), null);
	assert.strictEqual(v("Label", "Color.Text", "#FF375F60"), null);
	assert.strictEqual(v("Label", "Color.Text", "Gray"), null);
	assert.notStrictEqual(v("Label", "Color.Text", "#FF375"), null);
	assert.strictEqual(v("Label", "Slot.Padding", "1,2,3,4"), null);
	assert.notStrictEqual(v("Label", "Slot.Padding", "1,2,3"), null);
	assert.strictEqual(v("Label", "Visibility", "collapsed"), null);
	assert.notStrictEqual(v("Label", "Visibility", "Gone"), null);
	assert.strictEqual(v("Label", "Opacity", "{x}"), null);
	assert.strictEqual(v("Window", "Theme", "mytheme.xml"), null);
	assert.notStrictEqual(v("Button", "Action", "page:steps"), null);
	assert.strictEqual(v("Button", "Action", "page:steps=+1"), null);
});

test("actions that name a widget", () => {
	const p = problems(`<VBox><Button OnClicked="toggle:help"/><Label Name="info"/><Button OnClicked="show:info"/></VBox>`);
	assert.strictEqual(p.length, 1);
	assert.strictEqual(p[0].severity, "information");
	assert.match(p[0].message, /"help"/);
});

// --- completion -----------------------------------------------------------

test("tags at the root and inside a panel", () => {
	const root = labels(`<|`);
	assert.ok(root.includes("Window") && root.includes("Widgets") && root.includes("VBox"));
	const inside = labels(`<VBox>\n\t<|\n</VBox>`);
	assert.ok(inside.includes("Label") && inside.includes("Repeat") && !inside.includes("Children"));
	assert.ok(labels(`<Widget Name="x"><VBox><|</VBox></Widget>`).includes("Children"));
});

test("tags inside widgets with parts", () => {
	assert.deepStrictEqual(labels(`<ComboBox><|</ComboBox>`).filter((l) => !["If", "Repeat", "ForEach"].includes(l)), ["Item"]);
	const graph = labels(`<NodeGraph><Node><|</Node></NodeGraph>`);
	assert.ok(graph.includes("Input") && graph.includes("Output") && graph.includes("Slider"));
	assert.ok(labels(`<Window><|</Window>`).includes("Shortcut"));
});

test("a tag being typed gets a snippet", () => {
	const items = at(`<VBox><Lab|</VBox>`);
	const label = items.find((i) => i.label === "Label")!;
	assert.strictEqual(label.insert, 'Label Text="$1"/>');
	assert.strictEqual(items.find((i) => i.label === "HBox")!.insert, "HBox$1>$0</HBox>");
});

test("end tags", () => {
	const items = at(`<VBox><Label/></|`);
	assert.strictEqual(items[0].insert, "VBox>");
});

test("attributes", () => {
	const items = at(`<Button Text="a" |/>`);
	const names = items.map((i) => i.label);
	assert.ok(names.includes("Style") && names.includes("OnClicked") && names.includes("Color.Accent") && !names.includes("Text"));
	assert.ok(!names.includes("Slot.Fill"), "no slot attributes at the root");
	assert.ok(labels(`<VBox><Button |/></VBox>`).includes("Slot.Fill"));
	assert.ok(labels(`<Window |>`).includes("Touch.Slop"));
	assert.ok(!labels(`<Window |>`).includes("Tooltip"));
	assert.strictEqual(items.find((i) => i.label === "Style")!.retrigger, true);
});

test("values", () => {
	assert.deepStrictEqual(labels(`<Button Style="|"/>`), ["Default", "Primary", "Flat"].filter((x) => labels(`<Button Style="|"/>`).includes(x)));
	assert.deepStrictEqual(labels(`<Window Resizable="|"/>`), ["true", "false"]);
	const actions = labels(`<VBox><Label Name="help"/><Button OnClicked="|"/></VBox>`);
	assert.ok(actions.includes("app.quit") && actions.includes("toggle:"));
	assert.ok(labels(`<VBox><Label Name="help"/><Button OnClicked="toggle:|"/></VBox>`).includes("toggle:help"));
	assert.ok(labels(`<NodeGraph><Node Name="a"><Output Name="v"/></Node><Link From="|"/></NodeGraph>`).includes("a.v"));
});

test("no completion in comments", () => {
	assert.deepStrictEqual(at(`<VBox><!-- <| --></VBox>`), []);
});

// --- hovers ---------------------------------------------------------------

test("hovers", () => {
	const text = `<VBox><TextBlock Text="a" Slot.Fill="1" Tooltip="t"/><Button OnClicked="app.quit"/></VBox>`;
	const doc = parseXml(text);
	const h = (needle: string) => hover(doc, text.indexOf(needle) + 1, ctx())?.contents || "";
	assert.match(h("TextBlock"), /<Label>/);
	assert.match(h("Slot.Fill"), /Share of the free space/);
	assert.match(h("app.quit"), /Quits/);
	assert.match(h("VBox"), /top to bottom/);
});

// --- detection ------------------------------------------------------------

test("detection of the files of Raven", () => {
	assert.ok(looksLikeRaven(`<?xml version="1.0"?>\n<!-- x -->\n<Window/>`));
	assert.ok(looksLikeRaven(`<Widgets/>`));
	assert.ok(looksLikeRaven(`<vbox/>`));
	assert.ok(!looksLikeRaven(`<project xmlns="http://maven.apache.org/POM/4.0.0"/>`));
	assert.ok(!looksLikeRaven(`<protocol name="wayland"/>`));
	assert.ok(!looksLikeRaven(`<Theme Name="x"/>`));
});

// --- the examples of Raven ------------------------------------------------

const root = process.env.RAVEN_ROOT || path.resolve(__dirname, "..", "..", "..");
const examples = path.join(root, "examples");
if (fs.existsSync(examples)) {
	const files: string[] = [];
	const visit = (dir: string) => {
		for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
			const full = path.join(dir, entry.name);
			if (entry.isDirectory()) visit(full);
			else if (entry.name.endsWith(".xml")) files.push(full);
		}
	};
	visit(examples);
	const context = ctx();
	for (const file of files) {
		for (const d of declarationsOf(parseXml(fs.readFileSync(file, "utf8")), file)) context.declarations.set(d.name.toLowerCase(), d);
		context.tagFiles.add(path.basename(file, ".xml").toLowerCase());
	}
	for (const file of files) {
		const text = fs.readFileSync(file, "utf8");
		if (!looksLikeRaven(text)) continue;
		test(`example ${path.relative(root, file)}`, () => {
			const found = problems(text, context).map((p) => `${text.slice(0, p.start).split("\n").length}: ${p.message}`);
			assert.deepStrictEqual(found, []);
		});
	}
}

console.log(`${count - failures}/${count} tests passed`);
if (failures) process.exit(1);
