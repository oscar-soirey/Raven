# Raven.Gui

C# bindings for [Raven](https://github.com/oscar-soirey/Raven), a lightweight,
multithreaded GUI library in C for editors and tools. The native library is
included for Windows x64, Linux x64 and macOS (Apple silicon and Intel).

```sh
dotnet add package Raven.Gui
```

```csharp
using Raven;

R.Rinit(RBackend.R_BACKEND_VULKAN);
uint window = R.Rwindow_create_from_xml("<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>");
R.Raction_bind("quit", R.Keep<RActionCallback>((sender, action, user) => R.Rquit(0)), IntPtr.Zero);
while (R.Rwait_frame(-1.0) != 0) { }
R.Rshutdown();
```

The whole C API is in the static class `Raven.R`, under its own names. Texts
are strings; a parameter the C function writes through is an `out`; a
callback is a delegate (pass it through `R.Keep` so that the garbage collector
leaves it alone).

Raven draws with Vulkan: the machine needs a Vulkan driver (on macOS,
[MoltenVK](https://github.com/KhronosGroup/MoltenVK)). Targets .NET Standard
2.0: .NET 5 and later, .NET Core 3, .NET Framework 4.7.2. License: Apache 2.0.
