# Raven tools.
#
#   raven_designer       the IDE of Raven (tools/designer): projects, interfaces
#                        edited visually (.xml), code, translations, build and run
#   raven_theme_editor   makes theme files (tools/theme_editor): every color, size
#                        and style of a theme, edited over a live preview
#
# embed.py and subset_font.py are the scripts that produce the files of
# src/embedded; they are not needed to build.

add_executable(raven_designer designer/main.c designer/ide.h designer/os.c designer/project.c designer/editors.c
	designer/build.c designer/strings.c designer/welcome.c designer/accessibility.c designer/island.c designer/drawer.c)
# the headers of Raven, for the completion of its API (an installed IDE reads ../include)
target_compile_definitions(raven_designer PRIVATE "RAVEN_IDE_INCLUDE_DIR=\"${PROJECT_SOURCE_DIR}/include\"")
target_link_libraries(raven_designer PRIVATE raven::raven)
set_target_properties(raven_designer PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tools")
if(MSVC)
	target_compile_definitions(raven_designer PRIVATE _CRT_SECURE_NO_WARNINGS)
	# no console window behind the designer
	set_target_properties(raven_designer PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
elseif(MINGW)
	target_link_options(raven_designer PRIVATE -mwindows)
else()
	target_link_libraries(raven_designer PRIVATE m)
endif()

add_executable(raven_theme_editor theme_editor/main.c theme_editor/model.c theme_editor/editor.h)
target_link_libraries(raven_theme_editor PRIVATE raven::raven)
set_target_properties(raven_theme_editor PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tools")
if(MSVC)
	target_compile_definitions(raven_theme_editor PRIVATE _CRT_SECURE_NO_WARNINGS)
	set_target_properties(raven_theme_editor PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
elseif(MINGW)
	target_link_options(raven_theme_editor PRIVATE -mwindows)
else()
	target_link_libraries(raven_theme_editor PRIVATE m)
endif()

include(GNUInstallDirs)
install(TARGETS raven_designer raven_theme_editor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# The designer driving itself, without a window system and without a GPU:
# adds, moves, wraps, deletes, undoes, and checks the XML after each step.
if(RAVEN_BUILD_TESTS)
	add_test(NAME designer_script COMMAND raven_designer --null --frames 3 --script
		"do file.new.window f 2 path 0/0 set Text Bonjour expect Bonjour pick 2 do palette.add expect <Grid~Columns do edit.duplicate do edit.up do edit.wrap.Border expect <Border> do edit.unwrap do edit.delete do edit.undo do edit.undo do edit.redo do file.new.form expect <Field~Label path 2/1 set Hint typed expect Hint=\"typed\" do edit.duplicate do edit.out do edit.in do view.theme.glass_light do view.interact do view.run f 3 do view.theme.dark do file.new.panel expect <Border~Padding f 2")

	# The IDE: a new project (CMakeLists.txt, main.c, main.xml, strings.csv), its interface and its
	# program, a translation edited and shown in the preview in another language, everything saved.
	set(_ide "${CMAKE_CURRENT_BINARY_DIR}/ide_test")
	add_test(NAME designer_ide_clean COMMAND "${CMAKE_COMMAND}" -E remove_directory "${_ide}")
	add_test(NAME designer_ide_folder COMMAND "${CMAKE_COMMAND}" -E make_directory "${_ide}")
	set_tests_properties(designer_ide_clean PROPERTIES FIXTURES_SETUP designer_ide_clean)
	set_tests_properties(designer_ide_folder PROPERTIES FIXTURES_SETUP designer_ide FIXTURES_REQUIRED designer_ide_clean)
	add_test(NAME designer_ide COMMAND raven_designer --null --frames 2 --script
		"f 2 project ${_ide} demo C expecttabs main.c expecttabs main.xml preview ${_ide}/demo/main.xml expect @hello.button expect OnClicked=\"hello.clicked\" expectfile ${_ide}/demo/CMakeLists.txt add_executable(demo~main.c~main.xml~strings.csv) expectfile ${_ide}/demo/main.c Rwindow_create_from_xml_file(\"main.xml\") expectfile ${_ide}/demo/main.c Raction_bind(\"hello.clicked\" expectlabel status You~have~not~clicked~yet. cell hello.none 2 Rien~encore language fr f 1 expectlabel status Rien~encore language - f 1 expectlabel status You~have~not~clicked~yet. path 0/0 set Text Hi do file.save_all expectfile ${_ide}/demo/main.xml Text=\"Hi\" expectfile ${_ide}/demo/main.c Rtr_plural(\"clicks\",~g_clicks) expectfile ${_ide}/demo/strings.csv ,Rien~encore f 2")
	set_tests_properties(designer_ide PROPERTIES FIXTURES_REQUIRED designer_ide)

	# An interface opens as code and the island is its preview; the visual editor (its panels)
	# only comes when asked (Preview), and its file goes back to a code editor with its changes.
	add_test(NAME designer_preview COMMAND raven_designer --null --frames 2 --script
		"f 2 project ${_ide} live C f 2 expectpanel panel.preview 0 expectpanel panel.widgets 0 expectpanel source 0 expecttabs main.xml expectisland status You~have~not~clicked~yet. cell hello.none 2 Rien~encore language fr f 1 expectisland status Rien~encore language - f 1 preview ${_ide}/live/main.xml f 2 expectpanel panel.preview 1 expectpanel panel.widgets 1 expectisland - - path 0/1 set Text Typed f 1 do view.preview.close f 2 expectpanel panel.preview 0 expecttabs main.xml expectisland status Typed do view.reset f 1 expectpanel panel.preview 0 do file.save_all expectfile ${_ide}/live/main.xml Text=\"Typed\" f 2")
	set_tests_properties(designer_preview PROPERTIES FIXTURES_REQUIRED designer_ide)

	# The other languages: main.xml and the program that loads it, with what the tools of the language need.
	add_test(NAME designer_languages COMMAND raven_designer --null --frames 2 --script
		"f 1 project ${_ide} ui_rs Rust expectfile ${_ide}/ui_rs/src/main.rs Rwindow_create_from_xml_file expectfile ${_ide}/ui_rs/Cargo.toml raven-gui expectfile ${_ide}/ui_rs/main.xml OnClicked=\"hello.clicked\" expecttabs main.rs expecttabs main.xml project ${_ide} ui_cpp C++ expectfile ${_ide}/ui_cpp/main.cpp Raction_bind project ${_ide} ui_py Python expectfile ${_ide}/ui_py/main.py r.Raction_bind(\"hello.clicked\" project ${_ide} ui_go Go expectfile ${_ide}/ui_go/main.go raven.ActionBind(\"hello.clicked\" expectfile ${_ide}/ui_go/go.mod replace~raven project ${_ide} ui_cs C# expectfile ${_ide}/ui_cs/Program.cs R.Raction_bind(\"hello.clicked\" expectfile ${_ide}/ui_cs/ui_cs.csproj Raven.cs project ${_ide} ui_zig Zig expectfile ${_ide}/ui_zig/src/main.zig r.Raction_bind(\"hello.clicked\" expectfile ${_ide}/ui_zig/raven/raven.zig Rwindow_create_from_xml_file project ${_ide} ui_lua Lua expectfile ${_ide}/ui_lua/main.lua r.Raction_bind(\"hello.clicked\" f 1")
	set_tests_properties(designer_languages PROPERTIES FIXTURES_REQUIRED designer_ide)

	# Native projects: the language alone and a Hello world, nothing of Raven.
	add_test(NAME designer_native COMMAND raven_designer --null --frames 2 --script
		"f 1 nativeproject ${_ide} native_c C expectfile ${_ide}/native_c/main.c Hello,~world! expecttabs main.c expectpanel panel.preview 0 nativeproject ${_ide} native_rs Rust expectfile ${_ide}/native_rs/src/main.rs Hello,~world! nativeproject ${_ide} native_py Python expectfile ${_ide}/native_py/main.py Hello,~world! nativeproject ${_ide} native_cs C# expectfile ${_ide}/native_cs/Program.cs Hello,~world! f 1")
	set_tests_properties(designer_native PROPERTIES FIXTURES_REQUIRED designer_ide)

	# The theme editor driving itself: a look, an accent, colors, sizes and rules edited and seen
	# in the preview, a palette generated, undo and redo, the contrast checked, a file saved and read back.
	set(_te "${CMAKE_CURRENT_BINARY_DIR}/theme_editor_test.xml")
	add_test(NAME theme_editor COMMAND raven_theme_editor --null --frames 2 --script
		"f 2 name Night~Owl look WindowsDark expectxml Look=\"WindowsDark\" accent #FF9F0A expectxml Accent=\"#FF9F0A\" expectbase Accent #FF9F0A color Window #101018 color Text #FFF4E0 expectpreview Window #101018 expectpreview Text #FFF4E0 expectxml Window=\"#101018\" metric CornerRadius 9 expectmetric CornerRadius 9 expectxml CornerRadius=\"9\" syntax Keyword #FF7AB2 expectxml Keyword=\"#FF7AB2\" f 1 expectundo 7 do te.undo noxml Keyword= do te.redo expectxml Keyword= reset Text noxml Text=\"#FFF4E0\" styles <Style~Widget=\"Button\"~Class=\"danger\"~Accent=\"#E5484D\"/> expecterror - expectxml Class=\"danger\" styles <Style~Widget=\"Nope\"/> expecterror Nope styles - snippet 2 expectxml CornerRadius=\"14\" look RavenGlassDark color Selection #FF9F0A55 expectxml Selection=\"#FF9F0A55\" window f 2 do te.generate expectxml AccentText= expectcheck 16 save ${_te} expectfile ${_te} Name=\"Night~Owl\" do te.new.2 noxml Night~Owl open ${_te} expectxml Name=\"Night~Owl\" expectxml Look=\"RavenGlassDark\" expectpreview Accent #FF9F0A expectundo 0 f 2")
	# Contrast fixed one pair at a time and all at once, the saved version compared, the theme copied as C code.
	add_test(NAME theme_editor_check COMMAND raven_theme_editor --null --frames 2 --script
		"look RavenLight accent #FFD60A do te.generate expectcheck 15 color Text #CCCCCC expectcheck 9 fix 0 expectcheck 13 do te.fix.all expectcheck 15 name Night~Owl do te.copy.c expectclipboard night_owl_theme expectclipboard Rtheme_use(\"Night~Owl\") color Window #102030 save ${_te}.check color Window #FF0000 expectpreview Window #FF0000 compare 1 expectpreview Window #102030 compare 0 expectpreview Window #FF0000 f 2")
endif()
