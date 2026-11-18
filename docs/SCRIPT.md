# Raven Script

Raven Script is the scripting language of the interfaces of Raven: a small,
statically typed language with the syntax of TypeScript, run by an
interpreter of Raven itself (no other engine). Its functions are **actions**:
widgets name them in XML exactly as they name functions bound in C.

```xml
<Window Title="Counter">
    <Script>
        let counter: number = 0;

        fn app.increment() {
            counter += 1;
            ui.setText("counter", counter.toString());
        }

        fn app.volume(value: number) {
            app.log("Volume: " + value.toString());
        }
    </Script>

    <VBox>
        <Label Name="counter" Text="0"/>
        <Button Text="Increment" OnClicked="app.increment"/>
        <Slider Min="0" Max="100" OnChanged="app.volume"/>
    </VBox>
</Window>
```

- [Where scripts live](#where-scripts-live)
- [The language](#the-language)
- [Events: what a widget gives its function](#events-what-a-widget-gives-its-function)
- [The functions of Raven](#the-functions-of-raven)
- [Functions of the application (C)](#functions-of-the-application-c)
- [Errors and safety](#errors-and-safety)
- [Diagnostics](#diagnostics)
- [The C API](#the-c-api)
- [Tools](#tools)
- [How it works](#how-it-works)

## Where scripts live

**In a document**, in `<Script>` elements, anywhere in it (usually first in
the `<Window>`). Their content is **raw text, as in HTML**: `<`, `&&` and `&`
are written as they are, and the script runs up to `</Script>` (so a script
cannot contain that text, even in a string). A `<![CDATA[ ... ]]>` around the
whole code is accepted too.

**In a file** (`.rvs`) named by the document:

```xml
<Script Src="scripts/main.rvs"/>
```

The path is relative to the document (then looked for where Raven looks for
files). Both ways give the same language.

All the scripts of a document (its `<Script>` elements and the files they
name) are **compiled together**: they see each other's functions and globals,
whatever their order. They are compiled and checked **before the widgets are
built**, and installed only when there is no error at all: a mistake in one
script installs none of the document (the interface still loads). The
initializers of the globals run **after** the widgets are built, so
`let title = ui.getText("title");` finds its widget.

Scripts loaded by different documents share one global space: a function
defined again by another document (or by the same document reloaded) replaces
the previous one, and a global declared again keeps its place (the functions
of the other documents see the new one).

With live reload (`Rxml_set_live_reload`), changing a document or one of its
`.rvs` files compiles its scripts again and replaces their functions.

A script can also be loaded from C without XML: `Rscript_load` and
`Rscript_load_file`.

## The language

### Functions

```ts
fn app.quit() {
    app.log("Goodbye!");
}

fn math.add(a: number, b: number): number {
    return a + b;
}
```

- `fn`, a name, the parameters **between parentheses (mandatory)**, each with
  its type, then the return type after `:` (`void` when it is left out).
- A name with dots (`namespace.action`) is the name of an action: widgets
  name it, `Raction_fire` fires it. Every function of a script is an action.
- A function that returns a value must return it on every path.
- Functions are declared at the top level only; they are not values (no
  closure, no function passed as an argument).

### Variables

```ts
let counter: number = 0;     // changes
const name = "Raven";        // never changes (assigning it is error RS105)
let items: string[] = [];
let ready: bool;             // the default value: false
```

The type is written after `:`, or inferred from the value. A variable without
a value takes the default of its type: `0`, `""`, `false`, `[]`, `null`.
Variables at the top level are **globals**; in a function, a variable lives
in its block (`{ }`) and may hide one of the same name outside.

### Types

| Type | Values |
|---|---|
| `number` | `42`, `3.14`, `1e-3`, `0x1F` (64-bit floating point) |
| `string` | `"text"` or `'text'`; escapes `\n \t \r \\ \" \' é \u{1F600}` |
| `bool` | `true`, `false` |
| `void` | what a function without a value returns |
| `null` | `null` |
| `T[]` | arrays of one type: `number[]`, `string[][]` |

There is **no implicit conversion**: `1 + "a"` is an error; convert with
`value.toString()` or `number.parse(text)`. `x == null` may be written with
any type.

### Operators

| | |
|---|---|
| arithmetic | `+ - * / %` (numbers; `+` also joins two strings) |
| comparison | `== !=` (two values of the same type; strings by their text, arrays by identity), `< <= > >=` (numbers, or strings) |
| logic | `&& \|\| !` (bool only; `&&` and `\|\|` stop early) |
| assignment | `= += -= *= /= %=` (`+=` joins strings too) |

There is no `++`, no `===`, no `?:`.

### Statements

```ts
if (age >= 18) {
    app.log("Adult");
}
else if (age >= 13) {
    app.log("Teenager");
}
else {
    app.log("Child");
}

for (let i = 0; i < 10; i += 1) {
    if (i == 2) {
        continue;
    }
    if (i == 5) {
        break;
    }
}

while (count < 5) {
    count += 1;
}
```

Conditions are `bool` (`if (10)` is an error: write a comparison). The bodies
of `if`, `else`, `for` and `while` are blocks `{ }`.

Comments: `// ...` and `/* ... */`.

### Methods

| On | |
|---|---|
| every `number`, `bool`, `string` | `toString()` |
| `number` | `toFixed(digits)` (0 to 20 digits) |
| `string` | `length`, `toUpperCase()`, `toLowerCase()` (ASCII and Latin-1 letters), `trim()`, `contains(t)`, `startsWith(t)`, `endsWith(t)`, `indexOf(t)` (-1 when missing), `substring(start, end)`, `split(separator)` |
| arrays | `length`, `push(item)`, `pop()`, `items[i]`, `items[i] = value` |

Positions in strings count characters (not bytes). `number.parse("42")`
reads a number from a text (an error RS104 when it is not one).

Arrays are shared, not copied: `let b = a; b.push(1);` changes `a`.

### Declaring functions of the application

```ts
declare fn app.save(path: string): bool;
```

declares a function the application binds in C (see below), so that the type
checker knows it. It is checked against what the application bound, and
called when it runs.

## Events: what a widget gives its function

A widget names its function with `OnClicked`, `OnChanged`, `OnCommitted`,
`OnActivated` or `Action` (they are the same: a widget has one action), as
`"app.save"` or `"app.save()"`. When its event fires, the function is called
with what the event gives:

| Widget | Event | Arguments |
|---|---|---|
| `Button`, `MenuItem`, other widgets | when clicked / activated | none |
| `Checkbox` | checked or unchecked | `checked: bool` |
| `Slider`, `SpinBox`, `Rating` | the value changes | `value: number` |
| `TextInput` | Enter is pressed | `text: string` |
| `ComboBox` | another item is chosen | `index: number, text: string` (`-1, ""` for none) |
| `ListView` | double click or Enter | `index: number, text: string` |
| `TabView` | another tab is shown | `index: number` |
| `Pagination`, `Carousel` | another page | `page: number` |
| `Expander`, `Island` | opens or closes | `expanded: bool` |
| `ColorPicker` | the color changes | `color: string` (`"#RRGGBB"`) |
| `DatePicker` | a date is chosen | `year: number, month: number, day: number` |
| `TimePicker` | the time changes | `hour: number, minute: number, second: number` |

The rule: **a function may ignore arguments the event gives, but cannot
require arguments it does not give**, and the types of those it takes must
match. `fn app.changed()` fits a `Slider`; `fn app.changed(value: bool)` does
not (error RS103, reported when the document loads, with the place of the
widget and of the parameter). The value returned by a handler is ignored.

`Raction_fire(name, sender)` gives the arguments of the event of `sender`;
without a sender, there are none.

## The functions of Raven

| Function | |
|---|---|
| `app.log(message: string)` | writes to the log (the standard output, or `Rscript_set_log_callback`) |
| `app.quit()` | quits |
| `app.fire(action: string)` | fires any action: a function of C or of a script, or a built-in action (`"show:details"`, `"theme.dark"`) |
| `ui.setText(id, text)`, `ui.getText(id): string` | the text of the widget named `id` |
| `ui.setVisible(id, visible)`, `ui.isVisible(id): bool` | shown, or hidden (`Collapsed`) |
| `ui.setEnabled(id, enabled)`, `ui.isEnabled(id): bool` | |
| `ui.setValue(id, value)`, `ui.getValue(id): number` | Slider, SpinBox, Rating, ProgressBar, ComboBox / ListView (the item chosen), TabView, Pagination, Carousel |
| `ui.setChecked(id, checked)`, `ui.isChecked(id): bool` | Checkbox, Expander (open), MenuItem |
| `ui.setAttribute(id, name, value)` | any XML attribute, as written in a document |
| `ui.focus(id)` | the keyboard focus |
| `ui.exists(id): bool` | whether a widget has that name |
| `window.close()`, `window.minimize()`, `window.maximize()`, `window.restore()`, `window.setTitle(title)` | the window |

`id` is the `Name` (or `Id`) of a widget. It is looked for in the window of
the widget whose event runs the script (or of the document while its
initializers run), then in the other windows. A name that no widget of the
document has is a warning (RS301) when the script is compiled, and an error
when it runs. The actions Raven knows by itself without parameters
(`theme.toggle`, `island.expand`...) can be called too: `theme.toggle();`.

## Functions of the application (C)

`Raction_bind` keeps its meaning, and a function bound with it can be called
from a script, without arguments:

```c
Raction_bind("file.save", on_save, NULL);   /* a script: file.save(); */
```

`Raction_bind_with_parameters` binds a function that takes typed arguments
and returns a value:

```c
static void on_add(RActionContext* context, void* user_data) {
    double a = Raction_get_number(context, 0);
    double b = Raction_get_number(context, 1);
    Raction_return_number(context, a + b);
}

Raction_bind_with_parameters("math.add", "(a: number, b: number): number", on_add, NULL);
```

```ts
fn app.compute() {
    app.log(math.add(2, 3).toString());     // 5
}
```

- The signature lists the parameters (`number`, `string`, `bool`, with a
  name or not) and the return type after `:` (`void` when left out). The type
  checker sees it: a script that calls the function with the wrong arguments
  does not compile. Bind the function **before** the document that uses it
  is loaded (or `declare fn` it in the script).
- `Raction_get_number`, `Raction_get_string`, `Raction_get_bool` read the
  arguments; `Raction_return_number` / `_string` / `_bool` give the value;
  `Raction_fail(context, "why")` stops the script with error RS205;
  `Raction_get_sender` is the widget of the event (`R_INVALID_ID` from a script).
- Fired by a widget, the function gets the arguments of its event, as a
  script function does (same rule).
- One registry: binding a name replaces what it was bound to, whether C or
  script; `Raction_unbind` removes it. A script cannot declare a function of
  a name the application already bound (RS107), nor of a native of Raven.
- A function bound with parameters replaces the native of the same name
  (`app.log`, for example, can be redirected).

## Errors and safety

Nothing a script does can crash the application or stop it:

- **before running**: syntax, unknown names, types, event handlers, missing
  returns are reported, and a document whose scripts have an error installs
  none of them;
- **while running**: a division by zero (RS201), an index out of bounds or
  `pop()` on an empty array (RS204), a text that is not a number (RS104), a
  native that fails (RS205), a widget that does not exist (RS301) stop **the
  script that is running, and only it**; the next event runs as usual;
- **limits** (`Rscript_set_limits`): an invocation runs at most 10 000 000
  instructions (RS203: a loop that never ends), calls are nested 200 deep at
  most (RS202), strings and arrays take 64 MB at most (RS206);
- the bytecode is **validated** before it runs (every jump, operand and stack
  depth), and the machine checks the type of every value it works on: a
  function replaced by another of a different signature gives an error, not
  a crash;
- scripts reach nothing but the functions above: no file, no network, no
  memory, no system call.

## Diagnostics

Every diagnostic has a severity, a stable code, a title, its place (file,
line, column, an excerpt), an explanation and often a suggestion:

```text
error[RS103]: Invalid event handler signature.

   --> interface.xml:12:5
    |
 12 |     <Slider OnChanged="app.on_volume_changed"/>
    |     ^^^^^^^

  Slider.OnChanged expects a function with
  1 parameter of type 'number'.

  But 'app.on_volume_changed' has:
  1 parameter of type 'bool'.

   --> scripts/main.rvs:4:33
    |
  4 | fn app.on_volume_changed(value: bool) {
    |                                 ^^^^

  Suggestion:
  Change the parameter type from 'bool' to 'number'.
```

Places in a `<Script>` are places in the XML document.

| Code | Category | |
|---|---|---|
| RS001 | Syntax | Unexpected token (`function` instead of `fn`, `++`, `===`, code at the top level...) |
| RS002 | Syntax | Expected token (a missing `;`, parentheses, `{`...) |
| RS003 | Syntax | Invalid token (a character, a number, a string never closed, an escape) |
| RS101 | Name | Unknown function or variable |
| RS102 | Type | Type mismatch |
| RS103 | Type | Invalid event handler signature |
| RS104 | Type | Invalid conversion |
| RS105 | Variable | Assignment to a constant |
| RS106 | Function | Invalid argument count |
| RS107 | Name | Duplicate declaration (or a name already taken by Raven or the application) |
| RS108 | Function | Missing return |
| RS109 | Function | Invalid return (a value in a void function, none in another) |
| RS110 | Control | `break` / `continue` outside of a loop |
| RS111 | Warning | Unused expression |
| RS112 | Warning | Unreachable code |
| RS201 | Runtime | Division by zero |
| RS202 | Runtime | Maximum recursion depth exceeded |
| RS203 | Runtime | Instruction limit exceeded |
| RS204 | Runtime | Invalid array index |
| RS205 | Runtime | Native function failure |
| RS206 | Runtime | Memory limit exceeded |
| RS301 | Binding | Unknown UI element |

They go to the error callback of Raven (`Rerror_set_callback`, code
`R_ERROR_SCRIPT`, the whole text as the detail), or to
`Rscript_set_diagnostic_callback`.

## The C API

`raven_script.h` (included by `Raven.h`):

| | |
|---|---|
| `Rscript_load(source, size, name)`, `Rscript_load_file(path)` | loads a script (not XML) |
| `Rscript_check(source, size, name)`, `Rscript_check_file(path)` | checks a script or an XML document (its scripts and the functions its widgets name) without running anything; returns the number of errors |
| `Rscript_format(source, size, indent)` | formats a script |
| `Rscript_reset()` | removes every script function (their actions too) and global |
| `Rscript_has_function(name)` | |
| `Rscript_set_enabled(enabled)` | `R_FALSE`: the `<Script>` elements are read but not run (the designer does this) |
| `Rscript_set_limits(instructions, depth, memory)` | |
| `Rscript_set_log_callback`, `Rscript_set_diagnostic_callback` | |

And in `raven_core.h`: `Raction_bind_with_parameters` and the `Raction_get_*`,
`Raction_return_*`, `Raction_fail` functions of its context.

## Tools

- **Command line** (`tools/script`, built with the tools):
  `raven_script check FILE...` (`.rvs` and `.xml`), `raven_script format
  [--write] [--indent N|tab] FILE...` (the `<Script>` of XML documents too),
  `raven_script run FILE [FUNCTION]`.
- **VS Code** (`tools/vscode-raven`): a language server for the `<Script>` of
  the documents and the `.rvs` files: diagnostics, completion, hovers,
  signature help, go to definition, references, rename, symbols, formatting,
  coloring. It compiles with the same core as the library (built to
  WebAssembly), so the editor and the application agree.

## How it works

`src/script`:

| File | |
|---|---|
| `rs_lexer.c` | tokens |
| `rs_parser.c` | syntax tree; after an error, it goes on with the next statement |
| `rs_checker.c` | names and types, event handler checks, the records of the editor |
| `rs_compiler.c` | bytecode, and its validation |
| `rs_vm.c` | values (counted references), the machine, the limits |
| `rs_batch.c` | the scripts of one document, compiled and installed together |
| `rs_builtins.c` | the natives, the events of the widgets, the methods |
| `rs_format.c`, `rs_analysis.c` | the formatter; what the language server needs |
| `rs_common.c` | memory, texts, numbers (independent of the locale), diagnostics |
| `script_raven.c` | the bridge with the library: actions, events, natives, documents |

The `rs_*` files depend on nothing of the library: the library, the tests
(`tests/test_script_core.c`), the command line and the language server compile
the same code.

Not in this version: classes, generics, exceptions, async, threads, closures,
imports, reflection.
