-- A window described in XML, driven from Lua (5.1 to 5.5, or LuaJIT) through the C module.
--
--     LUA_CPATH="<folder of raven.so>/?.so;;" RAVEN_LIBRARY=/path/to/libraven.so lua example.lua
--
-- `lua example.lua --test` runs without a window system and without a GPU,
-- clicks its own button and checks what happened: it is how the build of
-- Raven checks this binding (also from a program that embeds Lua: host.c).

local r = require("raven")

local XML = [[
<Window Title="Raven from Lua" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Lua" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>]]

local test = false
for _, a in ipairs(arg or {}) do
	if a == "--test" then test = true end
end

-- Clicks the middle of a widget.
local function click(window, widget)
	local x, y, width, height = r.Rwidget_get_rect(widget)
	assert(width > 0 and height > 0)
	r.Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2)
	r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 1)
	r.Rwindow_inject_mouse_button(window, r.R_MOUSE_LEFT, 0)
end

-- Whether a value is still referenced from somewhere but a weak table.
local function alive(weak)
	collectgarbage()
	collectgarbage()
	return next(weak) ~= nil
end

local function check(window, title, name, clicks, errors)
	r.Rdraw_frame()
	assert(r.Rwindow_get_title(window) == "Raven from Lua")
	assert(r.Rwidget_get_type(title) == r.R_WIDGET_LABEL)
	r.Rtext_input_set_text(name, "Zoé")
	local greet = r.Rwidget_find(window, "greet")
	click(window, greet)
	assert(#clicks == 1 and clicks[1] == "greet", "the button did not fire its action")
	assert(r.Rlabel_get_text(title) == "Hello, Zoé!", r.Rlabel_get_text(title))

	-- handles: R_INVALID_ID is 0xFFFFFFFF in every version of Lua
	assert(r.Rwidget_find(window, "nothing") == r.R_INVALID_ID)
	assert(r.R_INVALID_ID == 4294967295)

	-- values written through pointers come back as results
	local width, height = r.Rwindow_get_size(window)
	assert(width == 360 and height == 200, width .. " x " .. height)

	-- enumerations: numbers, or the names of their items as strings (as in LuaJIT)
	local created = r.Rwidget_create("R_WIDGET_BUTTON")
	assert(r.Rwidget_get_type(created) == r.R_WIDGET_BUTTON)
	r.Rwidget_destroy(created)

	-- structures: tables, filled by C (a new one, or the one given)
	local stats = r.Rmemory_get_stats()
	assert(stats.allocation_count > 0)
	local given = {}
	assert(r.Rmemory_get_stats(given) == given and given.used_bytes > 0)

	-- a mistake reported in a structure
	local document, xml_error = r.Rxml_document_parse("<A><B></A>")
	assert(document == r.R_INVALID_ID)
	assert(xml_error.line == 1 and xml_error.message:find("does not match"), xml_error.message)

	-- documents, texts that are absent, texts with what must be escaped
	document = r.Rxml_document_parse("<VBox><Label Text='x'/></VBox>")
	local root = r.Rxml_document_get_root(document)
	assert(r.Rxml_node_get_tag(root) == "VBox" and r.Rxml_node_get_child_count(root) == 1)
	assert(r.Rxml_node_get_attribute(root, "Nope") == nil)
	r.Rxml_node_set_attribute(r.Rxml_node_get_child(root, 0), "Text", "é & <")
	assert(r.Rxml_document_to_string(document):find('Text="é &amp; &lt;"', 1, true))
	r.Rxml_document_destroy(document)

	-- a structure in and out: the table given is updated
	local spring = { value = 0, velocity = 0 }
	r.Rspring_set(spring, 5)
	assert(spring.value == 5, spring.value)

	-- a callback that returns a text, and one that fills values through pointers
	local list = r.Rwidget_create(r.R_WIDGET_LIST_VIEW)
	r.Rlist_view_set_virtual_items(list, 3, function(view, index) return "row " .. index end)
	assert(r.Rlist_view_get_item_text(list, 2) == "row 2")
	r.Rwidget_destroy(list)

	-- an error of the caller reaches the error callback as text
	r.Rwidget_set_tooltip(0x12345, "x")
	local last = errors[#errors]
	assert(last and last[1] == r.R_ERROR_INVALID_ID and last[2]:find("Rwidget_set_tooltip"))
	-- ...and a result and a text written through a pointer come back together
	local code, detail = r.Rerror_get_last()
	assert(code == r.R_ERROR_INVALID_ID and detail:find("Rwidget_set_tooltip"))

	-- files: bytes in, bytes out (Lua strings hold any byte), with no bytes too
	local path = os.tmpname()
	assert(r.Rfile_write(path, "a\0b") ~= 0)
	assert(r.Rfile_read(path) == "a\0b")
	assert(r.Rfile_write(path, "") ~= 0)
	assert(r.Rfile_read(path) == "")
	os.remove(path)

	-- pixels: a string of width * height * 4 bytes, both ways
	local pixels = string.rep("\255\0\0\255", 4)
	local png = r.Rimage_encode_png(pixels, 2, 2)
	assert(png:sub(2, 4) == "PNG")
	local decoded, w, h = r.Rimage_decode(png)
	assert(decoded == pixels and w == 2 and h == 2)
	assert(not pcall(r.Rimage_encode_png, "short", 2, 2), "too few bytes must be refused")

	-- several texts: a list given to C, and an event (a table) that brings them back
	local dropped = {}
	r.Rwindow_set_drop_callback(window, function(target, event)
		if event.phase == r.R_DROP_DROPPED then dropped[#dropped + 1] = event.paths end
		return 1
	end)
	assert(r.Rwindow_inject_drop(window, 10, 10, { "a.txt", "dossier/é.png" }, nil, nil) ~= 0)
	assert(#dropped == 1 and dropped[1][1] == "a.txt" and dropped[1][2] == "dossier/é.png")

	-- the user_data of a callback is any Lua value, given back as the last argument
	local seen
	r.Raction_bind("tagged", function(sender, action, user_data) seen = user_data end, { tag = "mine" })
	r.Raction_fire("tagged", r.R_INVALID_ID)
	assert(seen and seen.tag == "mine")
	r.Raction_unbind("tagged")
	local box = r.Rwidget_create(r.R_WIDGET_CHECKBOX)

	-- callbacks are released when replaced and when their widget is destroyed
	local weak = setmetatable({}, { __mode = "k" })
	local function watched(f) weak[f] = true return f end
	r.Rcheckbox_set_changed_callback(box, watched(function() end))
	assert(alive(weak))
	r.Rcheckbox_set_changed_callback(box, nil)
	assert(not alive(weak), "a replaced callback must be released")
	r.Rcheckbox_set_changed_callback(box, watched(function() end))
	r.Rwidget_set_destroy_callback(box, watched(function() end))
	r.Rwidget_destroy(box)
	assert(not alive(weak), "the callbacks of a destroyed widget must be released")
	r.Raction_bind("released", watched(function() end))
	r.Raction_unbind("released")
	assert(not alive(weak), "an unbound action must be released")

	-- callbacks stay alive through a collection
	collectgarbage()
	for k in pairs(clicks) do clicks[k] = nil end
	click(window, greet)
	assert(#clicks == 1)

	-- an error in a callback is caught, and goes to the error callback
	r.Raction_bind("boom", function() error("from a callback") end)
	r.Raction_fire("boom", r.R_INVALID_ID)
	last = errors[#errors]
	assert(last[1] == r.R_ERROR_SCRIPT and last[2]:find("from a callback"), last[2])

	-- a task posted from here, a one-shot timer, and a job on a worker thread that comes back here
	local posted, fired, finished = 0, 0, false
	r.Rtask_post(function() posted = posted + 1 end)
	r.Rtimer_create(0.001, 0, function() fired = fired + 1 end)
	r.Rtask_run_async(function(user_data) finished = user_data end, "done")
	for _ = 1, 200 do
		r.Rwait_frame(0.01)
		if posted > 0 and fired > 0 and finished then break end
	end
	assert(posted == 1 and fired == 1 and finished == "done", posted .. " " .. fired .. " " .. tostring(finished))
end

local started
if test then
	started = r.Rinit_ex({ backend = r.R_BACKEND_NULL, headless = 1 })
else
	started = r.Rinit(r.R_BACKEND_DEFAULT)
end
if started == 0 then
	print("cannot start Raven: " .. tostring(select(2, r.Rerror_get_last())))
	os.exit(1)
end

local errors = {}
r.Rerror_set_callback(function(code, severity, detail) errors[#errors + 1] = { code, detail } end)

local window = r.Rwindow_create_from_xml(XML)
local title = r.Rwidget_find(window, "title")
local name = r.Rwidget_find(window, "name")
local clicks = {}

r.Raction_bind("greet", function(sender, action)
	clicks[#clicks + 1] = action
	local who = r.Rtext_input_get_text(name)
	r.Rlabel_set_text(title, "Hello, " .. (who ~= "" and who or "you") .. "!")
end)
r.Raction_bind("quit", function() r.Rquit(0) end)

if test then
	check(window, title, name, clicks, errors)
	r.Raction_fire("quit", r.R_INVALID_ID)
	assert(r.Ris_running() == 0)
	r.Rshutdown()
	print("lua binding (" .. _VERSION .. "): ok")
else
	while r.Rwait_frame(-1.0) ~= 0 do end
	r.Rshutdown()
end
