# Raven's Java part, kept by R8 / ProGuard in the apps that use Raven's AAR (its proguard.txt).
# libraven.so loads its classes by name through the activity's class loader (today
# raven.android.RavenTextView), calls their static methods by name (attach, detach,
# setField, setState, showKeyboard, setBackWanted, backDefault) and registers their
# native methods (nativeOp) with RegisterNatives: none of them may be renamed or removed.
-keep class raven.android.** {
    public static <methods>;
    native <methods>;
}
