# raven-gui (Java)

Java bindings for [Raven](https://github.com/oscar-soirey/Raven), a lightweight,
multithreaded GUI library in C for editors and tools.

```xml
<dependency>
  <groupId>io.github.oscar-soirey</groupId>
  <artifactId>raven-gui</artifactId>
  <version>0.2.2</version>
</dependency>
```

or, with Gradle, `implementation("io.github.oscar-soirey:raven-gui:0.2.2")`.

Java 22 or later: the binding uses the Foreign Function & Memory API
(`java.lang.foreign`), so it has no dependency and no JNI. The jar of Maven
Central holds the native library for Windows x64, Linux x64 and macOS.

```java
import static raven.R.*;

public class Hello {
    public static void main(String[] args) {
        Rinit(R_BACKEND_VULKAN);
        Rwindow_create_from_xml("<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>");
        Raction_bind("quit", (sender, action, user) -> Rquit(0));
        while (Rwait_frame(-1.0) != 0) { }
        Rshutdown();
    }
}
```

Start the program with `--enable-native-access=ALL-UNNAMED` (Java warns
without it; a jar started with `java -jar` can say it in its manifest:
`Enable-Native-Access: ALL-UNNAMED`).

`raven.R` is the whole C API under its own names. Texts are `String`, a
parameter the C function writes through is an array of one element
(`int[] w = {0}, h = {0}; Rwindow_get_size(window, w, h);`), a structure is
a class with an accessor per field (`new RInitDesc().backend(R_BACKEND_NULL)`),
a callback is a lambda. The comment at the top of `R.java` says the rest.

## Without Maven

    javac -d classes src/main/java/raven/R.java
    RAVEN_LIBRARY=/path/to/libraven.so java --enable-native-access=ALL-UNNAMED -cp classes example/Example.java

The library is looked for in the system property `raven.library` and the
`RAVEN_LIBRARY` environment variable, then in the jar, then where the
system looks for libraries.

License: Apache 2.0.
