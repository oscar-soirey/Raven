# Raven's Java part, kept by R8 / ProGuard in the apps that use Raven's AAR (its proguard.txt).
# libraven.so loads its classes by name through the activity's class loader
# (raven.android.RavenTextView, raven.android.RavenDevice), calls their public static
# methods by name (RavenTextView: attach, detach, setField, setState, showKeyboard,
# setBackWanted, backDefault; RavenDevice: attach, detach, permissionStatus,
# requestPermission, openSettings, notify, fileOpen, fileSave, cameraAvailable, takePhoto,
# pickPhotos, share, openUrl, download, downloadError, hapticSupported, haptic,
# clipboardSet, clipboardGet, clipboardHasText) and registers their native methods
# (nativeOp, nativePermission, nativeComplete) with RegisterNatives: none of them may be
# renamed or removed. RavenRequestActivity and RavenFileProvider are named by the manifest.
-keep class raven.android.** {
    public static <methods>;
    native <methods>;
}
