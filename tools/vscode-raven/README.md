# Raven XML

Completion, hovers and diagnostics for the XML interfaces of
[Raven](https://github.com/oscar-soirey/Raven).

## Features

**Completion**

- tags: the widgets of Raven, the elements a widget reads itself (`<Item>` in a
  `ComboBox`, `<Node>` and `<Link>` in a `NodeGraph`, `<Track>` in a
  `Timeline`, `<Column>` and `<Row>` in a `Table`...), the tags declared with
  `<Widget>` in the file and in the workspace, and `<If>`, `<Repeat>`,
  `<ForEach>`, `<Include>`, `<Children/>`;
- attributes: those of the widget, then those of every widget (`Name`,
  `Tooltip`, `Visibility`...), the slot (`Slot.Fill`, `Slot.Padding`...),
  `Color.<Role>`, `Metric.<Name>`, `Touch.*`; the parameters of a declared tag;
- values: the choices of an attribute (`Visibility`, `Style`, `Appear`...),
  booleans, color names, the actions of Raven (`app.quit`, `theme.toggle`,
  `show:NAME`, `page:NAME=+1`...) with the names of the widgets of the file,
  the pins of a graph for `<Link From="..." To="...">`;
- end tags.

**Hovers**: what a tag or an attribute is, the kind of value it takes, its
other names (`TextBlock` is a `Label`), and what an action does.

**Diagnostics**, as the loader of Raven would report them, before running the
application:

- XML mistakes: unclosed or mismatched tags, values without quotes, `&` or `<`
  not written as entities;
- unknown tags and attributes, with a quick fix for typos
  (`<Lable>` → `<Label>`, `Colour` → `Color`);
- invalid values: booleans, numbers, colors, margins, choices, `page:` actions;
- structure: a widget that takes no child or a single one, a `<Window>` with
  more than one content element, `<Item>` outside of the widgets that read it,
  `<Children/>` outside of a `<Widget>`, several root elements;
- `show:NAME`, `toggle:NAME`... naming no widget of the file (information).

Values with `{expressions}` and translations (`@key`) are not checked.

## Which files

An XML file is a Raven file when its root element is `<Window>`, `<Widgets>`,
`<Widget>` or a widget of Raven. Theme files (`<Theme>`) and other XML files
(`pom.xml`, Wayland protocols...) are left alone.

| Setting | |
|---|---|
| `raven.xml.enable` | turns everything off |
| `raven.xml.files` | glob patterns of files always treated as Raven files |
| `raven.xml.exclude` | glob patterns of files never treated as Raven files |
| `raven.xml.customTags` | tags registered from C with `Rxml_register_tag` (default: `WebView`): accepted with any attribute |
| `raven.xml.diagnostics` | turns the diagnostics off, keeping completion and hovers |

## Where the knowledge comes from

`src/schema.json` is generated from the sources of Raven, which the script
only reads:

- `src/xml/xml_schema.c`: the tags, their attributes, the kinds of values and
  the descriptions (the tables behind `Rxml_get_attribute_info`);
- `src/widget/registry.c`: the other names of the tags;
- `src/widgets/*.c`: which widgets take no child or a single one, and the other
  names their `set_attribute` accepts.

After a change of these files:

```sh
npm run schema
```

The elements a widget reads itself, the elements of composition and the
built-in actions are described in `src/core/schema.ts` (from `docs/XML.md`).

## Building

```sh
npm install
npm test          # tests of src/core, and the examples of Raven must give no warning
npm run package   # raven-xml-<version>.vsix
```

Install the `.vsix` with *Extensions → ... → Install from VSIX*, or
`code --install-extension raven-xml-0.1.0.vsix`.

## Publishing

1. Create the publisher `oscar-soirey` on
   [marketplace.visualstudio.com/manage](https://marketplace.visualstudio.com/manage)
   (or change `publisher` in `package.json` to yours).
2. Create a Personal Access Token in Azure DevOps (organization *All accessible
   organizations*, scope *Marketplace → Manage*).
3. `npx vsce login oscar-soirey`, then `npm run publish`
   (or upload the `.vsix` on the page of the publisher).

For [Open VSX](https://open-vsx.org) (VSCodium, Cursor...):
`npx ovsx publish raven-xml-0.1.0.vsix -p <token>`.

## License

Apache-2.0, as Raven.
