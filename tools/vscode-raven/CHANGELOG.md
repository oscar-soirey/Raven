# Changelog

## 0.2.0

- Raven Script: a language server (diagnostics, completion, hovers, signature
  help, go to definition, find references, rename, symbols, formatting) for
  the `<Script>` elements of the XML documents and the `.rvs` files, built on
  the core of Raven compiled to WebAssembly.
- The `raven-script` language: coloring of `.rvs` files and of the code
  inside `<Script>`, comments, brackets.
- XML: the code of a `<Script>` is raw text (no false errors on `<` or `&&`);
  `<Script>` and its `Src` attribute are known; the hover of an action names
  scripts as a way to write it.
- Setting `raven.script.functions`; command *Raven: Format the Scripts of the
  Document*.

## 0.1.0

- Completion of tags, attributes and values in the XML files of Raven.
- Hovers on tags, attributes and actions.
- Diagnostics: XML mistakes, unknown tags and attributes (with quick fixes),
  invalid values, structure.
- Schema generated from `src/xml/xml_schema.c`, `src/widget/registry.c` and
  `src/widgets/*.c`.
