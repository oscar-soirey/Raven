-- hello_lua: the program (LuaJIT). Its interface is main.xml, loaded when it starts:
-- save main.xml while it runs and the window follows. F5 runs it.
local ffi = require("ffi")
local r = require("raven")

if r.Rinit(r.R_BACKEND_VULKAN) == 0 then os.exit(1) end
r.Rlocale_load_csv("strings.csv")
r.Rlocale_set_language(r.Rlocale_get_system_language())
local window = r.Rwindow_create_from_xml_file("main.xml")
local clicks = 0

-- OnClicked="hello.clicked" in main.xml fires this action (kept in a variable: the library calls it)
local on_hello = ffi.cast("RActionCallback", function(sender, action, user_data)
	clicks = clicks + 1
	-- the widget with Name="status"
	r.Rlabel_set_text(r.Rwidget_find(window, "status"), r.Rtr_plural("clicks", clicks))
end)
r.Raction_bind("hello.clicked", on_hello, nil)

while r.Rdraw_frame() ~= 0 do end
r.Rshutdown()
