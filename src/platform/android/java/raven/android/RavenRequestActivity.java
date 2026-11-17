/*
 * Raven - platform/android/java/raven/android/RavenRequestActivity.java
 *
 * STUB (device services, raven_device.h): declared in the manifest so that
 * the Android platform can be filled in without touching it. It does nothing
 * yet but close itself; the Android platform makes it the activity that asks
 * the system on behalf of Raven, because NativeActivity gets neither
 * onRequestPermissionsResult nor onActivityResult:
 *
 *  - runtime permissions: requestPermissions(), then the answer
 *    (granted, denied, or denied for good when
 *    shouldShowRequestPermissionRationale() is false after a refusal) goes
 *    to native, which calls rvn_permission_answer;
 *  - the pickers and the camera: startActivityForResult() with
 *    ACTION_OPEN_DOCUMENT, ACTION_CREATE_DOCUMENT, MediaStore.ACTION_PICK_IMAGES
 *    (ACTION_GET_CONTENT before Android 13) or MediaStore.ACTION_IMAGE_CAPTURE
 *    (EXTRA_OUTPUT: a content:// URI of RavenFileProvider); the result is
 *    copied into the cache (ContentResolver) and handed to native, which
 *    calls rvn_service_complete.
 *
 * Started by native code with the request in its Intent extras; transparent
 * (Theme.Translucent.NoTitleBar in the manifest) and finished as soon as it
 * has the answer. See docs/device-services.md.
 */

package raven.android;

import android.app.Activity;
import android.os.Bundle;

public final class RavenRequestActivity extends Activity {
	@Override
	protected void onCreate(Bundle state) {
		super.onCreate(state);
		finish();
	}
}
