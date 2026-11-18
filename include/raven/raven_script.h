/**
 * Raven
 * raven_script.h : Raven Script, the scripts of the interfaces.
 *
 * A small statically typed language with the syntax of TypeScript, run by
 * an interpreter of Raven (no other engine). Its functions are actions:
 * widgets call them by name, as they call functions bound in C.
 *
 *     <Window Title="Counter">
 *         <Script>
 *             let counter: number = 0;
 *
 *             fn app.increment() {
 *                 counter += 1;
 *                 ui.setText("counter", counter.toString());
 *             }
 *
 *             fn app.volume(value: number) {
 *                 app.log("Volume: " + value.toString());
 *             }
 *         </Script>
 *         <VBox>
 *             <Label Name="counter" Text="0"/>
 *             <Button Text="Increment" OnClicked="app.increment"/>
 *             <Slider OnChanged="app.volume"/>
 *         </VBox>
 *     </Window>
 *
 * A <Script> holds code (raw text, as in HTML: '<' and '&&' are written as
 * they are; its text ends at </Script>) or names a file: <Script Src="main.rvs"/>.
 * The scripts of a document are compiled together and checked before
 * anything runs: a type error, an unknown function, or an event handler
 * whose parameters do not fit its widget is reported with its place, and the
 * scripts of that document are not installed. The rest of the interface
 * loads all the same.
 *
 * Events give arguments to their function: a Slider its value (number), a
 * Checkbox its state (bool), a TextInput its text when Enter is pressed
 * (string), a ComboBox and a ListView the index and the text of the item...
 * A function may take fewer parameters than its event gives, never more.
 * The language, the functions it provides (app, ui, window) and the events
 * are described in docs/SCRIPT.md.
 *
 * Nothing a script does can crash the application: an error (a division
 * by zero, an index out of bounds, a loop that never ends, a recursion too
 * deep, too much memory) stops the script that made it and is reported;
 * the next event runs as usual. Diagnostics go to the error callback
 * (R_ERROR_SCRIPT), or to Rscript_set_diagnostic_callback.
 *
 * The functions are for the UI thread.
 */

#ifndef RAVEN_SCRIPT_H
#define RAVEN_SCRIPT_H

#include "raven_core.h"

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Loads a script given as text (not an XML document): its
	 * functions become actions, its globals are initialized.
	 * @param name Names it in the diagnostics (a file name); may be NULL.
	 * @return R_FALSE when it has an error (nothing of it is installed).
	 */
	RAVEN_API int Rscript_load(const char* source, size_t size, const char* name);
	/** @brief The same from a file (.rvs). */
	RAVEN_API int Rscript_load_file(const char* path);

	/**
	 * @brief Checks a script, or the scripts of an XML document and the
	 * functions its widgets name, without running anything nor building a
	 * widget. The diagnostics are reported as usual.
	 * @param source A script, or an XML document (text that starts with '<').
	 * @return The number of errors.
	 */
	RAVEN_API int Rscript_check(const char* source, size_t size, const char* name);
	/** @return The number of errors in the file (a .rvs or an .xml), -1 when it cannot be read. */
	RAVEN_API int Rscript_check_file(const char* path);

	/**
	 * @brief Formats a script: one statement per line, blocks indented,
	 * spaces around operators; comments are kept.
	 * @param indent One level of indentation; NULL for four spaces.
	 * @return The text, valid until the next call; NULL when the script has a
	 * syntax error (it is not changed).
	 */
	RAVEN_API const char* Rscript_format(const char* source, size_t size, const char* indent);

	/** @brief Removes every script function (their actions are unbound) and every global. Not while a script runs. */
	RAVEN_API void Rscript_reset(void);
	/** @return R_TRUE when a script function has that name. */
	RAVEN_API int Rscript_has_function(const char* name);

	/**
	 * @brief R_FALSE: the <Script> elements of the documents are read but not
	 * compiled nor run (a design tool that shows interfaces without running
	 * them). On by default.
	 */
	RAVEN_API void Rscript_set_enabled(int enabled);
	RAVEN_API int Rscript_is_enabled(void);

	/**
	 * @brief The limits of the scripts. 0 keeps a value.
	 * @param max_instructions Instructions one invocation (an event) may run (10 000 000).
	 * @param max_depth Calls nested (200).
	 * @param max_memory Bytes of strings and arrays alive (64 MB).
	 */
	RAVEN_API void Rscript_set_limits(Ruint max_instructions, int max_depth, size_t max_memory);

	/** @brief Where app.log writes: the standard output when NULL. */
	typedef void (*RScriptLogCallback)(const char* message, void* user_data);
	RAVEN_API void Rscript_set_log_callback(RScriptLogCallback callback, void* user_data);

	/**
	 * @brief Receives the diagnostics of the scripts instead of the error
	 * callback. `text` is the whole message, as the command line shows it;
	 * `code` is its code ("RS102"); `line` and `column` start at 1 (0 when
	 * the diagnostic has no place). NULL gives them back to the error callback.
	 */
	typedef void (*RScriptDiagnosticCallback)(RSeverity severity, const char* code, const char* file, int line, int column, const char* text, void* user_data);
	RAVEN_API void Rscript_set_diagnostic_callback(RScriptDiagnosticCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_SCRIPT_H */
