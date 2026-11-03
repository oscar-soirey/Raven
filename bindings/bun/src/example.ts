/**
 * A window described in XML, driven from Bun.
 *
 *     RAVEN_LIBRARY=/path/to/libraven.so bun run src/example.ts
 *
 * `bun run src/example.ts --test` runs without a window system and without a
 * GPU, clicks its own button and checks what happened: it is how the build of
 * Raven checks this binding.
 */

import { strict as assert } from "node:assert";
import * as r from "./raven";

const XML = `
<Window Title="Raven from Bun" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Bun" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>`;

/** Clicks the middle of a widget. */
function click(window: r.Rid, widget: r.Rid): void {
	const [x, y, width, height] = r.Rwidget_get_rect(widget);
	assert.ok(width > 0 && height > 0);
	r.Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2);
	r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 1);
	r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 0);
}

/** The binding, checked. */
async function check(window: r.Rid, title: r.Rid, name: r.Rid, clicks: string[], errors: [r.RError, string | null][]): Promise<void> {
	r.Rdraw_frame();
	assert.equal(r.Rwindow_get_title(window), "Raven from Bun");
	assert.equal(r.Rwidget_get_type(title), r.R_WIDGET_LABEL);
	r.Rtext_input_set_text(name, "Zoé");
	const greet = r.Rwidget_find(window, "greet");
	click(window, greet);
	assert.deepEqual(clicks, ["greet"]);
	assert.equal(r.Rlabel_get_text(title), "Hello, Zoé!");

	// the handle of nothing keeps its high bit
	assert.equal(r.Rwidget_find(window, "nothing"), r.R_INVALID_ID);
	assert.equal(r.R_INVALID_ID, 0xffffffff);

	// values written through pointers come back as results
	assert.deepEqual(r.Rwindow_get_size(window), [360, 200]);

	// structures: scalars, 64-bit values
	const stats = new r.RMemoryStats();
	r.Rmemory_get_stats(stats);
	assert.ok(stats.allocation_count > 0);
	assert.equal(typeof stats.total_allocations, "bigint");

	// a mistake reported in a structure
	const error = new r.RXmlError();
	assert.equal(r.Rxml_document_parse("<A><B></A>", error), r.R_INVALID_ID);
	assert.equal(error.line, 1);
	assert.ok(error.message.includes("does not match"), error.message);

	// documents, texts that are absent, texts with what must be escaped
	const document = r.Rxml_document_parse("<VBox><Label Text='x'/></VBox>", null);
	const root = r.Rxml_document_get_root(document);
	assert.equal(r.Rxml_node_get_tag(root), "VBox");
	assert.equal(r.Rxml_node_get_child_count(root), 1);
	assert.equal(r.Rxml_node_get_attribute(root, "Nope"), null);
	r.Rxml_node_set_attribute(r.Rxml_node_get_child(root, 0), "Text", "é & <");
	assert.ok(r.Rxml_document_to_string(document)?.includes('Text="é &amp; &lt;"'));
	r.Rxml_document_destroy(document);

	// a callback that returns a text
	const list = r.Rwidget_create(r.R_WIDGET_LIST_VIEW);
	r.Rlist_view_set_virtual_items(list, 3, (_view, index) => `row ${index}`);
	assert.equal(r.Rlist_view_get_item_text(list, 2), "row 2");
	r.Rwidget_destroy(list);

	// an error of the caller reaches the callback as text
	r.Rwidget_set_tooltip(0x12345, "x");
	assert.equal(errors.at(-1)?.[0], r.R_ERROR_INVALID_ID);
	assert.ok(errors.at(-1)?.[1]?.includes("Rwidget_set_tooltip"));
	// ...and a result and a text written through a pointer come back together
	const [code, detail] = r.Rerror_get_last();
	assert.equal(code, r.R_ERROR_INVALID_ID);
	assert.ok(detail?.includes("Rwidget_set_tooltip"));

	// files: bytes in, bytes out, with no bytes too
	const path = `raven_bun_test_${process.pid}.bin`;
	assert.ok(r.Rfile_write(path, new Uint8Array([97, 0, 98])));
	assert.deepEqual(r.Rfile_read(path), new Uint8Array([97, 0, 98]));
	assert.ok(r.Rfile_write(path, new Uint8Array(0)));
	assert.equal(r.Rfile_read(path)?.length, 0);
	await Bun.file(path).delete();

	// several texts: a list of texts given to C and an event that reads them back
	const dropped: string[][] = [];
	r.Rwindow_set_drop_callback(window, (_target, event) => {
		if (event?.phase === r.R_DROP_DROPPED) {
			dropped.push(event.paths);
		}
		return 1;
	});
	assert.ok(r.Rwindow_inject_drop(window, 10, 10, ["a.txt", "dossier/é.png"], null));
	assert.deepEqual(dropped, [["a.txt", "dossier/é.png"]]);

	// a structure with a text and an array of structures
	const filter = new r.RFileFilter();
	filter.name = "Images";
	filter.patterns = "*.png;*.jpg";
	const dialog = new r.RFileDialogDesc();
	dialog.title = "Pick";
	dialog.filters = [filter];
	assert.equal(dialog.filter_count, 1);
	assert.equal(dialog.title, "Pick");
	assert.equal(r.RFileFilter.from(dialog.filters as r.Pointer)?.patterns, "*.png;*.jpg");

	// callbacks stay alive through a collection
	Bun.gc(true);
	clicks.length = 0;
	click(window, greet);
	assert.deepEqual(clicks, ["greet"]);

	// an exception in a callback is reported and does not cross C
	const report = console.error;
	const reported: string[] = [];
	console.error = (...values: (string | Error)[]): void => void reported.push(values.join(" "));
	r.Raction_bind("boom", () => {
		throw new Error("from a callback");
	});
	r.Raction_fire("boom", r.R_INVALID_ID);
	console.error = report;
	assert.ok(reported.join().includes("from a callback"));

	// the loop leaves the event loop running: a timer fires, then quits
	let fired = false;
	setTimeout(() => {
		fired = true;
		r.Rquit(0);
	}, 5);
	await r.runLoop();
	assert.ok(fired);
	assert.equal(r.Ris_running(), 0);
}

const test = process.argv.includes("--test");
let started: number;
if (test) {
	const desc = new r.RInitDesc();
	desc.backend = r.R_BACKEND_NULL;
	desc.headless = 1;
	started = r.Rinit_ex(desc);
} else {
	started = r.Rinit(r.R_BACKEND_VULKAN);
}
if (!started) {
	console.error("cannot start Raven");
	process.exit(1);
}

const errors: [r.RError, string | null][] = [];
r.Rerror_set_callback((code, _severity, detail) => void errors.push([code, detail]));

const window = r.Rwindow_create_from_xml(XML);
const title = r.Rwidget_find(window, "title");
const name = r.Rwidget_find(window, "name");
const clicks: string[] = [];

r.Raction_bind("greet", (_sender, action) => {
	clicks.push(action ?? "");
	r.Rlabel_set_text(title, `Hello, ${r.Rtext_input_get_text(name) || "you"}!`);
});
r.Raction_bind("quit", () => r.Rquit(0));

if (test) {
	await check(window, title, name, clicks, errors);
	r.Rshutdown();
	console.log("bun binding: ok");
} else {
	await r.runLoop();
	r.Rshutdown();
}
