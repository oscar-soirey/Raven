# Raven for Python

[Raven](https://github.com/oscar-soirey/Raven) is a lightweight, multithreaded
GUI library in C for editors and tools. This package is its Python binding,
with the library itself included: nothing else to install.

```sh
pip install raven-gui
```

```python
import raven as r

r.Rinit(r.R_BACKEND_VULKAN)
window = r.Rwindow_create_from_xml("""
    <Window Title="Hello" Width="320" Height="160">
        <VBox Spacing="10" Slot.Padding="16">
            <TextInput Name="name" Placeholder="Your name"/>
            <Button Text="Greet" Style="Primary" OnClicked="greet"/>
        </VBox>
    </Window>""")
name = r.Rwidget_find(window, "name")
r.Raction_bind("greet", lambda sender, action, user: print("Hello,", r.Rtext_input_get_text(name)))
while r.Rwait_frame(-1.0):
    pass
r.Rshutdown()
```

Every function of the C API is there under its own name. Texts are `str`,
callbacks are Python functions, and the values a C function writes through
pointers are returned.

Raven draws with Vulkan: the machine needs a Vulkan driver (current graphics
drivers include one on Windows and Linux; on macOS, install
[MoltenVK](https://github.com/KhronosGroup/MoltenVK)). On Linux, libX11 is
used for the windows.

Wheels: Windows x64, Linux x86_64 (glibc 2.28 or newer), macOS 11 or newer
(Apple silicon and Intel). License: Apache 2.0.
