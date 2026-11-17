# Raven on iOS and Android (experimental)

One UI written in C, `shared/showcase.c` with its layout in
`shared/assets/showcase.xml`, run by two small apps:

| Folder | What it is |
|---|---|
| `shared/` | The showcase: a bottom tab bar with List (1,000 rows and a detail page), Controls (counter, switch, slider, progress, dialog, toast), Glass (blur over a picture) and Form (text fields with keyboard types) |
| `shared/assets/` | The files the app reads at run time: the layout and the picture |
| `ios/` | The iOS app: `src/main.m` (UIKit scene, `CADisplayLink` that drives `Rdraw_frame`), `Info.plist`, `build.sh` |
| `android/` | The Android app: `main.c` (`android_main`, NativeActivity), `AndroidManifest.xml`, `build.sh` |

Both build scripts compile Raven itself with CMake, then the app by hand, so no
Xcode project and no Gradle are needed. The Android script builds Raven's AAR
and makes the APK from it; the iOS script can link Raven.xcframework instead of
its own build (see [Packaging](#packaging)).

## Files of the app

On Android and iOS a relative path that names no file held in memory
(`Rfile_register_memory`) and no file on disk is read from the app's package:
the APK's `assets/` folder (through the `AAssetManager`) or the resources of
the iOS bundle (`[NSBundle mainBundle].resourcePath`). It works for every file
Raven reads: `Rwidget_create_from_xml_file("showcase.xml")`, `Source="photo.png"`,
`Font="fonts/..."`, `Rtexture_create_from_file`, `Rfont_create_from_file`,
`Rfile_read`, themes and translation tables. Paths inside a document are relative
to its folder, as on a desktop. The package is read-only, is not watched for live
reload, and its folders are not listed, so the automatic loading of widget
declarations and translation tables found beside a document does not look into it
(register them by name instead).

Both build scripts put `shared/assets/` into the package (`aapt2 link -A` on
Android, a copy into `Hello.app` on iOS), with the font of the titles copied from
`tools/designer/fonts` into `fonts/`.

| iOS Simulator | | | Android (Xiaomi 15T) | Android emulator |
|---|---|---|---|---|
| ![The list on iOS](screenshots/ios-list.png) | ![Glass on iOS](screenshots/ios-glass.png) | ![Keyboard suggestions on iOS](screenshots/ios-keyboard-suggestions.png) | ![Gboard suggestions on Android](screenshots/android-keyboard-suggestions.png) | ![Glass on Android](screenshots/android-glass.png) |

This is an early stage port: see the platform sources
(`src/platform/uikit/uikit_platform.m`, `src/platform/android/`) and grep for
`SHORTCUT` for the places that are known to be simplified.

## iOS (Simulator)

Needs Xcode with an iOS Simulator runtime, CMake and Ninja. Raven draws with
Metal (`-DRAVEN_BACKEND_VULKAN=OFF`): MoltenVK is not needed.

Boot a simulator, then `cd ios && ./build.sh run`. `UDID=<id>` picks another
one than the booted simulator.

The simulator treats the Mac's keyboard as a hardware keyboard by default and
then shows no on-screen keyboard: turn "Connect Hardware Keyboard" off in the
Simulator to see it.

Testing hooks in `src/main.m`, set at launch with `SIMCTL_CHILD_` in front:
`RAVEN_ROTATE=landscape,portrait` (with `RAVEN_ROTATE_EVERY=<seconds>`)
rotates the scene, `RAVEN_DISMISS=<seconds>` closes the keyboard from outside
Raven, `RAVEN_MEMORY_WARNING=<seconds>` posts UIKit's memory warning.

`XCFRAMEWORK=1 ./build.sh run` links the simulator slice of
`Raven.xcframework` (built first by `tools/mobile/build-xcframework.sh`) instead
of the script's own CMake build; `XCFRAMEWORK=<path>` uses a given one.

### A real iPhone or iPad

`./build.sh device` builds `Hello.app` for `ios-arm64` (the `iphoneos` SDK, CMake
build in `build-ios-device`). It compiles
and links; it has not run on a device yet, because that needs an Apple
developer account:

1. A signing identity (`security find-identity -v -p codesigning`, made by
   Xcode from the account) and a provisioning profile (`.mobileprovision`) that
   allows the bundle id and the device. With a free account, let Xcode make
   one for a bundle id of your own and pass it as `BUNDLE_ID`.
2. `SIGN_IDENTITY="Apple Development: ..." PROFILE=<profile> BUNDLE_ID=<id>
   ./build.sh device` signs the app with the entitlements of the
   profile.
3. `DEVICE=<id of xcrun devicectl list devices> ... ./build.sh device run`
   installs and launches it (`xcrun devicectl`), with the device in developer
   mode.

Steps 2 and 3 are written but untested.

## Android

Needs the Android SDK with an NDK, build tools and a platform (API 29 or
later), a JDK (for `javac` and `d8`), CMake and Ninja. The script takes the
newest NDK, build tools and platform it finds in `$ANDROID_HOME` (default
`~/Library/Android/sdk`); `ANDROID_NDK`, `ANDROID_BUILD_TOOLS` and
`ANDROID_JAR` override them.

`cd android && ./build.sh install` builds `raven-hello.apk` (arm64-v8a,
signed with a debug key made on the first build) and starts it on the device
`adb` sees (`ANDROID_SERIAL` picks one).

The script builds Raven as `libraven.so` and its AAR with CMake
(`RAVEN_BUILD_SHARED=ON`, target `raven_aar`), then makes the APK from it: the
app's `libravenhello.so` (`main.c`, the showcase and `native_app_glue`) links
the AAR's `libraven.so`, both go into `lib/arm64-v8a/`, and `classes.dex` is
Raven's Java part as CMake dexed it.

Raven's Java part, the `InputConnection` of the on-screen keyboard and the
`AccessibilityNodeProvider` that TalkBack reads (`src/platform/android/java`),
is in the APK's `classes.dex`. Without it Raven falls back to key events (no
suggestions, ASCII only) and has no screen reader.

An emulator whose device has a hardware keyboard (`hw.keyboard=yes`) only
shows Gboard's small toolbar, not the full keyboard.

## Packaging

Raven for app developers, made from the same sources: an AAR for Android and
an xcframework for iOS. Both are experimental.

### Android: the Java part and the AAR

With the NDK toolchain, CMake also compiles Raven's Java part
(`src/platform/android/java`) with `javac` and `d8`: the target `raven_java`,
built by default, leaves `<build>/android/classes.jar` and
`<build>/android/classes.dex`. It takes the SDK from `ANDROID_HOME`,
`ANDROID_SDK_ROOT` or the SDK that holds the NDK, its newest build tools and
platform, and `javac` from `JAVA_HOME` or the `PATH`; `RAVEN_ANDROID_SDK`,
`RAVEN_ANDROID_BUILD_TOOLS` and `RAVEN_ANDROID_JAR` override them. When a tool
is missing, CMake warns and skips the Java part and the AAR (libraven still
builds); `-DRAVEN_ANDROID_JAVA=OFF` skips them without the warning.
`libraven.so` is linked for 16 KB pages (`-z max-page-size=16384`).

`cmake --build <build> --target raven_aar` (with `-DRAVEN_BUILD_SHARED=ON`)
packs `<build>/raven-<version>-<abi>.aar` for the ABI of that build, and

    tools/mobile/build-android-aar.sh [abi ...]    # default: arm64-v8a x86_64

builds one per ABI and merges them into `build-aar/raven-<version>.aar`:

```
raven-0.2.3.aar
├── AndroidManifest.xml          package raven.android, minSdk 29
├── classes.jar                  Raven's Java part
├── proguard.txt                 R8 keeps what libraven.so calls by name
├── R.txt                        empty (no resources)
├── jni/
│   ├── arm64-v8a/libraven.so
│   └── x86_64/libraven.so
└── prefab/                      the same libraries with the headers
    ├── prefab.json
    └── modules/raven/
        ├── module.json
        ├── include/             Raven.h, raven/
        └── libs/android.<abi>/  abi.json, libraven.so
```

A Gradle app (not tried here: no Gradle in this setup) adds the AAR and
builds its own native code, the `android_main` with `native_app_glue`,
against it with prefab:

```kotlin
// app/build.gradle.kts
android {
    defaultConfig { minSdk = 29 }
    buildFeatures { prefab = true }
    externalNativeBuild { cmake { path = file("src/main/cpp/CMakeLists.txt") } }
}
dependencies { implementation(files("libs/raven-0.2.3.aar")) }
```

```cmake
# src/main/cpp/CMakeLists.txt
find_package(raven REQUIRED CONFIG)
add_library(app SHARED main.c ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(app PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
target_link_libraries(app raven::raven android log)
target_link_options(app PRIVATE -uANativeActivity_onCreate)
```

The manifest declares `android.app.NativeActivity` with
`<meta-data android:name="android.app.lib_name" android:value="app" />`, as
`android/AndroidManifest.xml` does. Gradle packs `libraven.so` from the AAR's
`jni/` next to `libapp.so`, the dynamic linker finds it there when
NativeActivity loads `libapp.so`, and `classes.jar` goes into the app's dex
with the consumer rules applied. Without Gradle, do what `android/build.sh`
does: link with `-lraven` against the AAR's `libraven.so`, put both libraries
in the APK's `lib/<abi>/` and dex `classes.jar` (or take CMake's
`classes.dex`).

GameActivity is not supported yet: Raven is written against the NDK's
`android_native_app_glue` (`struct android_app`, `onInputEvent`), and
GameActivity brings its own glue with another `android_app`.

### iOS: Raven.xcframework

    tools/mobile/build-xcframework.sh [output folder]    # default: build-xcframework

builds `libraven.a` for devices and for the Simulator with CMake and joins them
with `xcodebuild -create-xcframework` (`CONFIG`, `MIN_IOS`, default 17.0, and
`SIM_ARCHS`, default `arm64;x86_64`, change the build):

```
Raven.xcframework
├── Info.plist
├── ios-arm64/
│   ├── libraven.a
│   └── Headers/                 Raven.h, raven/, module.modulemap (module Raven)
└── ios-arm64_x86_64-simulator/
    ├── libraven.a               arm64 + x86_64
    └── Headers/
```

In Xcode, add `Raven.xcframework` to the target (Frameworks, Libraries, and
Embedded Content: "Do Not Embed", it is a static library), and link the
system frameworks Raven uses: UIKit, QuartzCore, Metal, CoreText,
CoreGraphics. C and Objective-C include `<Raven.h>` (or `@import Raven;`),
Swift does `import Raven`. Raven draws with Metal, so nothing else is
embedded (no MoltenVK). The app's resources (layouts, pictures, fonts) go into
its bundle, as `ios/build.sh` does.

The minimum is iOS 17: the UIKit platform calls `registerForTraitChanges`
(iOS 17) without an availability check, so `MIN_IOS=15.0` builds with a
warning and would fail on iOS 15 and 16. A Swift package (`binaryTarget`) is
not made yet.
