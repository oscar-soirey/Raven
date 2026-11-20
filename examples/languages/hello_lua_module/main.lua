-- hello_lua_module: the program (Lua 5.1 to 5.5, or LuaJIT, with the C module
-- of bindings/lua). Its interface is main.xml, loaded when it starts: save
-- main.xml while it runs and the window follows.
local r = require("raven")

if r.Rinit(r.R_BACKEND_DEFAULT) == 0 then os.exit(1) end
r.Rlocale_load_csv("strings.csv")
r.Rlocale_set_language(r.Rlocale_get_system_language())
local window = r.Rwindow_create_from_xml_file("main.xml")
local clicks = 0

-- OnClicked="hello.clicked" in main.xml fires this action
r.Raction_bind("hello.clicked", function(sender, action)
	clicks = clicks + 1
	-- the widget with Name="status"
	r.Rlabel_set_text(r.Rwidget_find(window, "status"), r.Rtr_plural("clicks", clicks))
end)

while r.Rdraw_frame() ~= 0 do end
r.Rshutdown()
