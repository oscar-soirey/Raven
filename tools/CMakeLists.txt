# Raven tools.
#
#   raven_designer       makes interfaces (tools/designer): the .xml of a folder
#                        edited visually, its translations, its themes in the preview
#   raven_theme_editor   makes theme files (tools/theme_editor): every color, size
#                        and style of a theme, edited over a live preview
#
# embed.py and subset_font.py are the scripts that produce the files of
# src/embedded; they are not needed to build.

# (the Theme tab is the theme editor itself, built in: designer/theme_tab.c)
add_executable(raven_designer designer/main.c designer/designer.h designer/os.c designer/files.c designer/island.c
	designer/strings.c designer/welcome.c designer/accessibility.c designer/icons.c designer/theme_tab.c theme_editor/model.c theme_editor/editor.h
	designer/fonts.c designer/font_body.c designer/font_body_bold.c designer/font_title.c designer/font_code.c
	crash_report.c crash_report.h)
	
if(WIN32)
    target_sources(raven_designer PRIVATE designer/icon.rc)
endif()

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

add_executable(raven_theme_editor theme_editor/main.c theme_editor/model.c theme_editor/editor.h crash_report.c crash_report.h)
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
	add_test(NAME designer_folder_make COMMAND "${CMAKE_COMMAND}" -E make_directory "${_ds}/folder/sub" "${_ds}/script" "${_ds}/fuzz" "${_ds}/tabs" "${_ds}/layout" "${_ds}/library" "${_ds}/window" "${_ds}/keys" "${_ds}/notch" "${_ds}/select" "${_ds}/clipboard" "${_ds}/tree" "${_ds}/files/ui/a/b" "${_ds}/other")
	set_tests_properties(designer_folder_clean PROPERTIES FIXTURES_SETUP designer_folder_clean)
	set_tests_properties(designer_folder_make PROPERTIES FIXTURES_SETUP designer_folder FIXTURES_REQUIRED designer_folder_clean)

	# The designer driving itself, without a window system and without a GPU: a new window is empty,
	# then adds, moves, wraps, deletes, undoes, and checks the XML after each step.
	add_test(NAME designer_script COMMAND raven_designer --null --frames 3 --script
		"open ${_ds}/script do files.new.window f 2 expect <VBox~Spacing=\"8\"~Slot.Padding=\"16\"/> path 0 set Text Bonjour expect Bonjour pick 2 do palette.add expect <Grid~Columns do edit.duplicate do edit.up do edit.wrap.Border expect <Border> do edit.unwrap do edit.delete do edit.undo do edit.undo do edit.redo do files.new.form expect <Field~Label path 2/1 set Hint typed expect Hint=\"typed\" do edit.duplicate do edit.out do edit.in do view.theme.glass_light do view.interact do view.run f 3 do view.theme.dark do files.new.panel expect <Border~Padding expectfiles 3 f 2")
	set_tests_properties(designer_script PROPERTIES FIXTURES_REQUIRED designer_folder)

	# A folder: its interfaces, a theme and a table found, an interface open by itself, the preview in
	# another language, a theme and the size of a phone; the island adds a widget, wraps it, opens a file;
	# a file changed by another program is read again; a new interface; everything saved.
	add_test(NAME designer_folder COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/folder/main.xml <Window~Title=\"A\"~Width=\"400\"~Height=\"300\"><VBox><Label~Name=\"greet\"~Text=\"@hello\"/></VBox></Window> write ${_ds}/folder/sub/card.xml <Border~Padding=\"8\"><Label~Text=\"Card\"/></Border> write ${_ds}/folder/night.xml <Theme~Name=\"Night\"~Look=\"RavenDark\"~Accent=\"#FF9F0A\"/> write ${_ds}/folder/strings.csv key,en,fr|hello,Hello,Bonjour| open ${_ds}/folder f 2 expectpage workspace expectfiles 2 expectthemes 1 expecttitle main.xml expectlabel greet Hello language fr f 1 expectlabel greet Bonjour cell hello 2 Salut expectlabel greet Salut path 0/0 expectselected Label search add~button f 1 expectselected Button expect <Button expecttitle Button search wrap~in~a~vbox expect <VBox> search theme~night expecttheme Night device 2 expectsize 390 844 do file.save_all expectfile ${_ds}/folder/main.xml <Button expectfile ${_ds}/folder/strings.csv Salut write ${_ds}/folder/main.xml <Window~Width=\"300\"~Height=\"200\"><Label~Name=\"greet\"~Text=\"Changed\"/></Window> refresh expectlabel greet Changed do files.new.panel expecttitle panel.xml search open~card expecttitle card.xml f 2")
	set_tests_properties(designer_folder PROPERTIES FIXTURES_REQUIRED designer_folder)

	# Random clicks, drags, keys, text and actions over a folder (meant for a build with AddressSanitizer:
	# "fuzz STEPS SEED" replays a run).
	add_test(NAME designer_fuzz COMMAND raven_designer --null --frames 1 --script
		"write ${_ds}/fuzz/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Text=\"@hello\"/><Button~Text=\"Go\"/></VBox></Window> write ${_ds}/fuzz/strings.csv key,en,fr|hello,Hello,Bonjour| write ${_ds}/fuzz/night.xml <Theme~Name=\"FuzzNight\"~Look=\"RavenDark\"/> open ${_ds}/fuzz f 2 fuzz 700 7 fuzz 700 99 f 2")
	# (a home of its own: the recent folders it may open are not the folders of the other tests)
	set_tests_properties(designer_fuzz PROPERTIES FIXTURES_REQUIRED designer_folder
		ENVIRONMENT "HOME=${_ds}/fuzz-home;XDG_CONFIG_HOME=${_ds}/fuzz-home/config;USERPROFILE=${_ds}/fuzz-home;APPDATA=${_ds}/fuzz-home/config")

	# The tabs kept apart: widgets dropped into the preview and an element dragged in it, the preview
	# window following live, a widget library in the Widget tab, a theme edited, saved, undone and saved
	# again in the Theme tab, the Translations tab, and back to the window as it was.
	add_test(NAME designer_tabs COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/tabs/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Name=\"greet\"~Text=\"Hi\"/><Button~Text=\"Go\"/></VBox></Window> write ${_ds}/tabs/night.xml <Theme~Name=\"Night\"~Look=\"RavenDark\"~Accent=\"#FF9F0A\"/> write ${_ds}/tabs/strings.csv key,en,fr|hello,Hello,Bonjour| open ${_ds}/tabs f 2 expecttab window expecttitle main.xml dropon Slider 0/0 after expect <Slider dropon Checkbox 0/1 before expect <Checkbox moveto 0/0 0/3 after path 0/0 expectselected Checkbox path 0/3 expectselected Label do view.run f 2 expectrun greet path 0/3 set Name moved f 2 expectrun moved do edit.undo f 2 expectrun greet tab widget expecttab widget do files.new.widgets f 2 expect <Widgets tab theme expecttab theme f 2 do te.reset.colors do file.save expectfile ${_ds}/tabs/night.xml Name=\"Night\" do edit.undo do file.save do file.save expectfile ${_ds}/tabs/night.xml Accent=\"#FF9F0A\" tab strings expecttab strings tab window expecttab window expecttitle main.xml do view.theme.dark f 2")
	set_tests_properties(designer_tabs PROPERTIES FIXTURES_REQUIRED designer_folder)

	# The preview and the selection: an interface opens with nothing selected, a click around the window
	# or Escape selects nothing, the window fits the panel and follows its size (the size of the XML too),
	# the palette shows the widgets by family.
	add_test(NAME designer_layout COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/layout/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Name=\"greet\"~Text=\"Hi\"/><Button~Text=\"Go\"/></VBox></Window> open ${_ds}/layout f 2 expecttab window expectnothing expectfit path 0/0 expectselected Label clickaround expectnothing clickin key 256 0 expectnothing path 0/1 expectselected Button tab widget f 1 tab window expectfit size 1100 760 f 2 expectfit device 1 expectsize 400 300 path 0 clickaround expectnothing device 0 expectfit expectgroup Button Controls expectgroup VBox Layout expectgroup ListView Lists~and~data expectgroup Grid Layout f 1")
	set_tests_properties(designer_layout PROPERTIES FIXTURES_REQUIRED designer_folder)

	# The Widget tab edited as a window: the bodies of the declarations are selected, widgets dropped
	# into them and moved, and the library keeps its parameters ({Title}) and its <Children/>.
	add_test(NAME designer_library COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/library/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Card~Title=\"Hi\"/></VBox></Window> write ${_ds}/library/widgets.xml <Widgets><Widget~Name=\"Card\"~Title=\"Title\"~Text=\"\"><Border~Padding=\"14\"><VBox~Spacing=\"6\"><Label~Text=\"{Title}\"/><Label~Text=\"{Text}\"/><Children/></VBox></Border></Widget></Widgets> open ${_ds}/library f 2 tab widget f 2 expecttab widget expectnothing path 0/0/0/0 expectselected Label clickaround expectnothing dropon Button 0/0/0/1 after expect <Button expect {Text} expect <Children/> moveto 0/0/0/2 0/0/0/0 before f 1 path 0/0/0/0 expectselected Button do edit.undo expect Name=\"Card\"~Title=\"Title\"~Text=\"\" f 1")
	set_tests_properties(designer_library PROPERTIES FIXTURES_REQUIRED designer_folder)

	# F7 opens the window itself (its title, its size, its island), which follows every change.
	add_test(NAME designer_window COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/window/main.xml <Window~Title=\"My~app\"~Width=\"420\"~Height=\"310\"><Island><Label~Name=\"isl\"~Text=\"Island\"/><VBox/></Island><VBox><Label~Name=\"greet\"~Text=\"Hi\"/></VBox></Window> open ${_ds}/window f 2 do view.run f 2 expectrunwindow My~app 420 310 expectrun greet expectrun isl path 1/0 set Name moved f 2 expectrun moved do edit.undo f 2 expectrun greet path . set Title Other f 2 expectrunwindow Other 420 310 device 2 f 1 expectrunwindow Other 390 844 f 1")
	set_tests_properties(designer_window PROPERTIES FIXTURES_REQUIRED designer_folder)

	# "@" in a text: the keys of the tables are listed under it, narrowed as it is typed, and one is chosen.
	add_test(NAME designer_keys COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/keys/strings.csv key,en,fr|app.hello,Hello,Bonjour|app.help,Help,Aide|menu.file,File,Fichier| write ${_ds}/keys/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Name=\"greet\"~Text=\"Hi\"/></VBox></Window> open ${_ds}/keys f 2 path 0/0 do island.search f 2 focus island.text settext island.text - text @ f 1 expectkeys 3 text he f 1 expectkeys 2 text lp f 1 expectkeys 1 key 264 0 key 257 0 f 1 expect Text=\"@app.help\" expectkeys 0 expectlabel greet Help f 1")
	set_tests_properties(designer_keys PROPERTIES FIXTURES_REQUIRED designer_folder)

	# Several elements selected: Ctrl + click adds or removes one, Shift + click the ones between (in the
	# preview and in the Tree), and the Edit menu acts on all of them: wrapped together, dragged together,
	# duplicated, deleted, copied and pasted; a click around the window selects nothing.
	add_test(NAME designer_select COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/select/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Text=\"A\"/><Button~Text=\"B\"/><Checkbox~Text=\"C\"/><Slider/></VBox></Window> open ${_ds}/select f 2 clickpath 0/0 0 expectselected Label expectcount 1 clickpath 0/2 2 expectcount 2 expectselected Checkbox clickpath 0/3 1 expectcount 3 expectselected Slider clickpath 0/0 2 expectcount 2 do edit.wrap.Border expectcount 1 expectselected Border expect <Border> do edit.undo f 1 path 0/0 clickpath 0/2 2 moveto 0/0 0/3 after f 1 expectcount 2 path 0/2 expectselected Label path 0/3 expectselected Checkbox path 0/0 clickpath 0/3 1 expectcount 4 do edit.duplicate expectcount 4 do edit.delete expectcount 1 expectselected VBox do edit.undo f 1 path 0/1 clickpath 0/2 2 do edit.copy path 0 do edit.paste f 1 expectcount 2 clickaround expectnothing expectcount 0 f 1")
	set_tests_properties(designer_select PROPERTIES FIXTURES_REQUIRED designer_folder)

	# The keys of the clipboard on the elements: Ctrl+X, Ctrl+C, Ctrl+V; Backspace deletes as Delete does,
	# and a text being typed keeps them for itself.
	add_test(NAME designer_clipboard COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/clipboard/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Text=\"A\"/><Button~Text=\"B\"/><Checkbox~Text=\"C\"/><Slider/></VBox></Window> open ${_ds}/clipboard f 2 clickpath 0/0 0 clickpath 0/1 2 expectcount 2 key 88 2 f 1 expectselected VBox expectcount 1 expectnot <Label expectnot <Button key 86 2 f 1 expectcount 2 expectselected Button expect <Label clickpath 0/2 0 key 67 2 key 86 2 f 1 expectselected Label expectcount 1 clickpath 0/3 0 key 259 0 f 1 expectselected Button clickpath 0/0 0 key 261 0 f 1 expectselected Slider expectnot <Checkbox focus source key 259 0 f 1 expectselected Slider expect <Slider/> f 1")
	set_tests_properties(designer_clipboard PROPERTIES FIXTURES_REQUIRED designer_folder)

	# The Tree takes drops: its own rows moved before, after or into another (several at once), an element
	# of the preview, a widget of the palette.
	add_test(NAME designer_tree COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/tree/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Text=\"A\"/><Button~Text=\"B\"/><HBox><Checkbox~Text=\"C\"/></HBox><Slider/></VBox></Window> open ${_ds}/tree f 2 treedrag tree:0/0 0/3 after f 1 path 0/3 expectselected Label treedrag tree:0/0 0/1 inside f 1 path 0/0/1 expectselected Button treedrag stage:0/1 0/0/0 before f 1 path 0/0/0 expectselected Slider treedrag palette:2 0/0 inside f 1 expect <Grid path 0/0 clickpath 0/1 2 treedrag tree:0/1 0/0/0 before f 1 expectcount 2 f 1")
	set_tests_properties(designer_tree PROPERTIES FIXTURES_REQUIRED designer_folder)

	# Files: the whole folder as a tree; a click opens a file where it is edited (a table deep in it,
	# a widget library); folders made, files made in them, renamed (the file open follows) and deleted;
	# tags made by the application in C shown as stand-ins, not as problems; the tags a folder declares
	# are forgotten when another folder opens.
	add_test(NAME designer_files COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/files/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Card~Title=\"Hi\"/></VBox></Window> write ${_ds}/files/lib.xml <Widgets><Widget~Name=\"Card\"~Title=\"\"><Label~Text=\"{Title}\"/></Widget></Widgets> write ${_ds}/files/ui/a/b/deep.csv key,en|hi,Hello| write ${_ds}/other/main.xml <Window><VBox><Label~Text=\"x\"/></VBox></Window> open ${_ds}/files f 2 expectdeclared Card 1 expectproblems 0 fileclick ui/a/b/deep.csv f 1 expecttab strings cell hi 1 Bonjour do file.save_all expectfile ${_ds}/files/ui/a/b/deep.csv Bonjour newfolder fresh expectexists ui/a/b/fresh 1 fileclick main.xml newfolder screens expectexists screens 1 fileclick screens do files.new.window f 2 expectexists screens/window.xml 1 expectopen window.xml expecttab window rename screens/window.xml settings f 1 expectexists screens/settings.xml 1 expectopen settings.xml rename screens views f 1 expectexists views/settings.xml 1 expectopen settings.xml path 0 set Spacing 4 do file.save expectfile ${_ds}/files/views/settings.xml Spacing=\"4\" expectexists screens 0 delete views f 1 expectexists views 0 write ${_ds}/files/app.xml <Window><VBox><Icon~Glyph=\"play\"~Size=\"20\"~Slot.Padding=\"4\"/><Cover~Album=\"current\"/></VBox></Window> refresh f 2 fileclick app.xml f 2 expectopen app.xml expectproblems 0 fileclick lib.xml f 2 expecttab widget expectopen lib.xml open ${_ds}/other f 2 expectdeclared Card 0 f 1")
	set_tests_properties(designer_files PROPERTIES FIXTURES_REQUIRED designer_folder)

	# On a MacBook the island lives in the camera housing (a window of its own, anchored to the screen):
	# the null platform fakes one (RAVEN_FAKE_NOTCH). The island goes there, the camera in the middle of its
	# ears; a text typed in it takes the keyboard and gives it back; a click in the designer folds it.
	add_test(NAME designer_notch COMMAND raven_designer --null --frames 2 --script
		"write ${_ds}/notch/strings.csv key,en,fr|app.hello,Hello,Bonjour| write ${_ds}/notch/main.xml <Window~Width=\"400\"~Height=\"300\"><VBox><Label~Name=\"greet\"~Text=\"Hi\"/></VBox></Window> open ${_ds}/notch f 2 expectnotch 1 expectcamera 220 32 expectkeyboard designer path 0/0 do island.search f 3 expectkeyboard island focus island.text settext island.text - text @app.he f 1 expectkeys 1 key 264 0 key 257 0 f 1 expect Text=\"@app.hello\" key 256 0 f 2 expectkeyboard designer clickin f 30 expectcamera 220 32 do view.run f 2 expectnotch 1 tab widget f 1 tab window f 2 expectnotch 1")
	set_tests_properties(designer_notch PROPERTIES FIXTURES_REQUIRED designer_folder ENVIRONMENT "RAVEN_FAKE_NOTCH=200x32")

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
