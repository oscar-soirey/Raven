# Raven tools.
#
#   raven_designer       makes interfaces (tools/designer): the .xml of a folder
#                        edited visually, its translations, its themes in the preview
#   raven_theme_editor   makes theme files (tools/theme_editor): every color, size
#                        and style of a theme, edited over a live preview
#
# embed.py and subset_font.py are the scripts that produce the files of
# src/embedded; they are not needed to build.

add_executable(raven_designer designer/main.c designer/designer.h designer/os.c designer/files.c designer/island.c
	designer/strings.c designer/welcome.c designer/accessibility.c)
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

if(RAVEN_BUILD_TESTS)
	set(_ds "${CMAKE_CURRENT_BINARY_DIR}/designer_test")
	add_test(NAME designer_folder_clean COMMAND "${CMAKE_COMMAND}" -E remove_directory "${_ds}")
	add_test(NAME designer_folder_make COMMAND "${CMAKE_COMMAND}" -E make_directory "${_ds}/folder/sub" "${_ds}/script" "${_ds}/fuzz")
	set_tests_properties(designer_folder_clean PROPERTIES FIXTURES_SETUP designer_folder_clean)
	set_tests_properties(designer_folder_make PROPERTIES FIXTURES_SETUP designer_folder FIXTURES_REQUIRED designer_folder_clean)

	# The designer driving itself, without a window system and without a GPU:
	# adds, moves, wraps, deletes, undoes, and checks the XML after each step.
	add_test(NAME designer_script COMMAND raven_designer --null --frames 3 --script
		"open ${_ds}/script do files.new.window f 2 path 0/0 set Text Bonjour expect Bonjour pick 2 do palette.add expect <Grid~Columns do edit.duplicate do edit.up do edit.wrap.Border expect <Border> do edit.unwrap do edit.delete do edit.undo do edit.undo do edit.redo do files.new.form expect <Field~Label path 2/1 set Hint typed expect Hint=\"typed\" do edit.duplicate do edit.out do edit.in do view.theme.glass_light do view.interact do view.run f 3 do view.theme.dark do files.new.panel expect <Border~Padding expectfiles 3 f 2")
	set_tests_properties(designer_script PROPERTIES FIXTURES_REQUIRED designer_folder)

	# A folder: its interfaces, a theme and a table found, an interface open by itself, the preview in
	# another language, a theme and the size of a phone; the island adds a widget, wraps it, opens a file;
	# a file changed by another program is read again; a new interface; everything saved.
	add_test(NAME designer_folder COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/folder/main.xml <Window~Title=\"A\"~Width=\"400\"~Height=\"300\"><VBox><Label~Name=\"greet\"~Text=\"@hello\"/></VBox></Window> write ${_ds}/folder/sub/card.xml <Border~Padding=\"8\"><Label~Text=\"Card\"/></Border> write ${_ds}/folder/night.xml <Theme~Name=\"Night\"~Look=\"RavenDark\"~Accent=\"#FF9F0A\"/> write ${_ds}/folder/strings.csv key,en,fr|hello,Hello,Bonjour| open ${_ds}/folder f 2 expectpage workspace expectfiles 2 expectthemes 1 expecttitle main.xml expectlabel greet Hello language fr f 1 expectlabel greet Bonjour cell hello 2 Salut expectlabel greet Salut path 0/0 expectselected Label search add~button f 1 expectselected Button expect <Button expecttitle Button search wrap~in~a~vbox expect <VBox> search theme~night expecttheme Night device 1 expectsize 390 844 do file.save_all expectfile ${_ds}/folder/main.xml <Button expectfile ${_ds}/folder/strings.csv Salut write ${_ds}/folder/main.xml <Window~Width=\"300\"~Height=\"200\"><Label~Name=\"greet\"~Text=\"Changed\"/></Window> refresh expectlabel greet Changed do files.new.panel expecttitle panel.xml search open~card expecttitle card.xml f 2")
	set_tests_properties(designer_folder PROPERTIES FIXTURES_REQUIRED designer_folder)

	# Random clicks, drags, keys, text and actions over a folder (meant for a build with AddressSanitizer:
	# "fuzz STEPS SEED" replays a run).
	add_test(NAME designer_fuzz COMMAND raven_designer --null --frames 1 --script
		"write ${_ds}/fuzz/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Text=\"@hello\"/><Button~Text=\"Go\"/></VBox></Window> write ${_ds}/fuzz/strings.csv key,en,fr|hello,Hello,Bonjour| write ${_ds}/fuzz/night.xml <Theme~Name=\"FuzzNight\"~Look=\"RavenDark\"/> open ${_ds}/fuzz f 2 fuzz 700 7 fuzz 700 99 f 2")
	set_tests_properties(designer_fuzz PROPERTIES FIXTURES_REQUIRED designer_folder)

	# The theme editor driving itself: a look, an accent, colors, sizes and rules edited and seen
	# in the preview, a palette generated, undo and redo, the contrast checked, a file saved and read back.
	set(_te "${CMAKE_CURRENT_BINARY_DIR}/theme_editor_test.xml")
	add_test(NAME theme_editor COMMAND raven_theme_editor --null --frames 2 --script
		"f 2 name Night~Owl look WindowsDark expectxml Look=\"WindowsDark\" accent #FF9F0A expectxml Accent=\"#FF9F0A\" expectbase Accent #FF9F0A color Window #101018 color Text #FFF4E0 expectpreview Window #101018 expectpreview Text #FFF4E0 expectxml Window=\"#101018\" metric CornerRadius 9 expectmetric CornerRadius 9 expectxml CornerRadius=\"9\" syntax Keyword #FF7AB2 expectxml Keyword=\"#FF7AB2\" f 1 expectundo 7 do te.undo noxml Keyword= do te.redo expectxml Keyword= reset Text noxml Text=\"#FFF4E0\" styles <Style~Widget=\"Button\"~Class=\"danger\"~Accent=\"#E5484D\"/> expecterror - expectxml Class=\"danger\" styles <Style~Widget=\"Nope\"/> expecterror Nope styles - snippet 2 expectxml CornerRadius=\"14\" look RavenGlassDark color Selection #FF9F0A55 expectxml Selection=\"#FF9F0A55\" window f 2 do te.generate expectxml AccentText= expectcheck 16 save ${_te} expectfile ${_te} Name=\"Night~Owl\" do te.new.2 noxml Night~Owl open ${_te} expectxml Name=\"Night~Owl\" expectxml Look=\"RavenGlassDark\" expectpreview Accent #FF9F0A expectundo 0 f 2")
	# Random clicks, drags, keys, text and actions (meant for a build with AddressSanitizer: "fuzz STEPS SEED" replays a run).
	add_test(NAME theme_editor_fuzz COMMAND raven_theme_editor --null --frames 1 --script "fuzz 800 7 fuzz 800 1234 f 2")
	# Contrast fixed one pair at a time and all at once, the saved version compared, the theme copied as C code.
	add_test(NAME theme_editor_check COMMAND raven_theme_editor --null --frames 2 --script
		"look RavenLight accent #FFD60A do te.generate expectcheck 15 color Text #CCCCCC expectcheck 9 fix 0 expectcheck 13 do te.fix.all expectcheck 15 name Night~Owl do te.copy.c expectclipboard night_owl_theme expectclipboard Rtheme_use(\"Night~Owl\") color Window #102030 save ${_te}.check color Window #FF0000 expectpreview Window #FF0000 compare 1 expectpreview Window #102030 compare 0 expectpreview Window #FF0000 f 2")
endif()
