/*
 * Raven - platform/android/java/raven/android/RavenFileProvider.java
 *
 * STUB (device services, raven_device.h): declared in the manifest, which
 * creates every provider when the application starts, so the class must
 * exist; it serves nothing yet. The Android platform makes it the provider
 * that hands files of the application's cache to other applications as
 * content:// URIs (androidx's FileProvider is not in an APK built without
 * Gradle):
 *
 *  - Rshare_show with a file: Intent.ACTION_SEND with EXTRA_STREAM and
 *    FLAG_GRANT_READ_URI_PERMISSION;
 *  - Rcamera_take_photo: the EXTRA_OUTPUT the camera application writes to.
 *
 * Authority: the package name + ".raven.files" (the manifest of
 * examples/mobile declares dev.raven.hello.raven.files). Only files under
 * getCacheDir()/raven are served: openFile() refuses any other path.
 * See docs/device-services.md.
 */

package raven.android;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

public final class RavenFileProvider extends ContentProvider {
	@Override
	public boolean onCreate() {
		return true;
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection, String[] arguments, String order) {
		return null;
	}

	@Override
	public String getType(Uri uri) {
		return null;
	}

	@Override
	public Uri insert(Uri uri, ContentValues values) {
		return null;
	}

	@Override
	public int delete(Uri uri, String selection, String[] arguments) {
		return 0;
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection, String[] arguments) {
		return 0;
	}
}
