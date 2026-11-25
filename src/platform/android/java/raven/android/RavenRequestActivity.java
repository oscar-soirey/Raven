/*
 * Raven - platform/android/java/raven/android/RavenRequestActivity.java
 *
 * The activity that asks the system on behalf of Raven (raven_device.h):
 * NativeActivity gets neither onRequestPermissionsResult nor
 * onActivityResult, so RavenDevice starts this one, transparent
 * (Theme.Translucent.NoTitleBar in the manifest), with the request in its
 * extras. It shows the screen of the system, takes the answer on the Java
 * main thread, finishes at once, and answers native:
 *
 *  - OP_PERMISSION: requestPermissions(), then the status as it now is
 *    (RavenDevice.permissionStatus: granted, denied, or blocked when
 *    shouldShowRequestPermissionRationale() is false after a refusal);
 *  - OP_OPEN: ACTION_OPEN_DOCUMENT; OP_PHOTOS: MediaStore.ACTION_PICK_IMAGES
 *    (Android 13+, or 11+ with the SDK extension), ACTION_GET_CONTENT
 *    image/* before; each URI is copied (ContentResolver, on a thread) into
 *    <cache>/raven/picked/<request id>/<display name>, photos made upright
 *    JPEG or PNG files that Raven decodes;
 *  - OP_SAVE: ACTION_CREATE_DOCUMENT, then the file native gave is copied
 *    into the document (on a thread);
 *  - OP_CAMERA: MediaStore.ACTION_IMAGE_CAPTURE, EXTRA_OUTPUT a URI of
 *    RavenFileProvider for <cache>/raven/camera/<request id>.jpg, then
 *    turned upright from its EXIF orientation.
 *
 * Every request is answered exactly once: the screen of the system closed
 * without a result, or this activity going away before it came, is
 * R_DEVICE_CANCELLED (a permission: its status as it now is).
 *
 * When Android ends the process while the system's screen is in front, it
 * recreates this activity in a new process (with its saved state) to give
 * it the result. The request carries the session of Raven that made it
 * (EXTRA_SESSION): when that is not the session attached now, the answer is
 * copied the same way, into folders of its own (another session may use the
 * same request id), and RavenDevice.answer stores it for
 * Rdevice_set_restored_callback. A permission asked that way needs nothing:
 * the application reads its status again. See docs/device-services.md.
 */

package raven.android;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ext.SdkExtensions;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;

public final class RavenRequestActivity extends Activity {
	private static final String TAG = RavenDevice.TAG;

	static final int OP_PERMISSION = 1, OP_OPEN = 2, OP_SAVE = 3, OP_CAMERA = 4, OP_PHOTOS = 5;

	static final String EXTRA_OP = "raven.op";
	static final String EXTRA_ID = "raven.id";
	static final String EXTRA_SESSION = "raven.session";
	static final String EXTRA_PERMISSION = "raven.permission";
	static final String EXTRA_NAMES = "raven.names";
	static final String EXTRA_MIMES = "raven.mimes";
	static final String EXTRA_MULTIPLE = "raven.multiple";
	static final String EXTRA_TITLE = "raven.title";
	static final String EXTRA_SOURCE = "raven.source";
	static final String EXTRA_TEMPORARY = "raven.temporary";
	static final String EXTRA_MAX = "raven.max";
	private static final String STATE_STATUS = "raven.status";

	/* (one question or screen per instance) */
	private static final int CODE = 0x5241;

	private int op, id;
	/* the session of Raven that made the request (RavenDevice.session) */
	private long session;
	/* the status of the permission before the question (OP_PERMISSION) */
	private int before = -1;
	private boolean answered;

	static Intent intent(Context context, int op, int id) {
		Intent intent = new Intent(context, RavenRequestActivity.class);
		intent.putExtra(EXTRA_OP, op);
		intent.putExtra(EXTRA_ID, id);
		intent.putExtra(EXTRA_SESSION, RavenDevice.session());
		return intent;
	}

	/** Starts it over Raven's activity, without an animation (main thread). */
	static boolean start(Activity from, Intent intent) {
		try {
			from.startActivity(intent);
			from.overridePendingTransition(0, 0);
			return true;
		} catch (RuntimeException e) {
			Log.w(TAG, "RavenRequestActivity: could not start", e);
			return false;
		}
	}

	@Override
	protected void onCreate(Bundle state) {
		super.onCreate(state);
		Intent intent = getIntent();
		op = intent.getIntExtra(EXTRA_OP, 0);
		id = intent.getIntExtra(EXTRA_ID, 0);
		session = intent.getLongExtra(EXTRA_SESSION, 0);
		if (state != null) {
			/* recreated while the system's screen is open (also in a new process): its result comes to this instance */
			before = state.getInt(STATE_STATUS, -1);
			if (!RavenDevice.current(session)) Log.i(TAG, "request " + id + " of an earlier session: waiting for its result");
			return;
		}
		try {
			switch (op) {
			case OP_PERMISSION: askPermission(intent); break;
			case OP_OPEN: pickDocuments(intent); break;
			case OP_SAVE: createDocument(intent); break;
			case OP_CAMERA: takePhoto(); break;
			case OP_PHOTOS: pickPhotos(intent); break;
			default: close(); break;
			}
		} catch (ActivityNotFoundException e) {
			fail("no application of the system for it: " + e.getMessage());
		} catch (IOException | RuntimeException e) {
			fail(e.toString());
		}
	}

	@Override
	protected void onSaveInstanceState(Bundle state) {
		super.onSaveInstanceState(state);
		state.putInt(STATE_STATUS, before);
	}

	@Override
	protected void onDestroy() {
		/* gone before the answer (and not only recreated): the request still gets one */
		if (isFinishing() && !answered) {
			answered = true;
			if (op == OP_PERMISSION) RavenDevice.answerPermission(getIntent().getIntExtra(EXTRA_PERMISSION, 0));
			else if (op == OP_SAVE) {
				deleteTemporary(getIntent());
				reply(this, RavenDevice.CANCELLED, null, null);
			} else if (op != 0) reply(this, RavenDevice.CANCELLED, null, null);
		}
		super.onDestroy();
	}

	/* The answer of the request: to its session, or stored for after a restart (any thread). */
	private void reply(Context context, int result, RavenDevice.Picked[] files, String error) {
		RavenDevice.answer(context.getApplicationContext(), session, op, id, result, files, error);
	}

	/* The name of the copies of the request: the id, and a stamp when it is of an earlier session (whose ids start again at 1). */
	private String key() {
		return RavenDevice.current(session) ? Integer.toString(id) : id + "-" + Long.toHexString(System.currentTimeMillis());
	}

	private void close() {
		finish();
		overridePendingTransition(0, 0);
	}

	private void fail(String error) {
		if (!answered) {
			answered = true;
			if (op == OP_PERMISSION) RavenDevice.answerPermission(getIntent().getIntExtra(EXTRA_PERMISSION, 0));
			else reply(this, RavenDevice.FAILED, null, error);
		}
		close();
	}

	/* ------------------------------------------------------------------ permissions */

	private void askPermission(Intent intent) {
		int permission = intent.getIntExtra(EXTRA_PERMISSION, 0);
		String[] names = intent.getStringArrayExtra(EXTRA_NAMES);
		before = RavenDevice.permissionStatus(permission);
		RavenDevice.setAsked(permission, true);
		requestPermissions(names, CODE);
	}

	@Override
	public void onRequestPermissionsResult(int code, String[] names, int[] results) {
		if (code != CODE || op != OP_PERMISSION || answered) return;
		int permission = getIntent().getIntExtra(EXTRA_PERMISSION, 0);
		String[] asked = getIntent().getStringArrayExtra(EXTRA_NAMES);
		boolean granted = false;
		for (int result : results) granted |= result == android.content.pm.PackageManager.PERMISSION_GRANTED;
		/*
		 * The first refusal makes shouldShowRequestPermissionRationale() true.
		 * Still false after the first question: it was dismissed without an
		 * answer (Android 11+: a tap outside), which the system asks again
		 * next time: still "not asked".
		 */
		if (!granted && before == RavenDevice.NOT_ASKED && asked != null && asked.length > 0 && !shouldShowRequestPermissionRationale(asked[0]))
			RavenDevice.setAsked(permission, false);
		answered = true;
		RavenDevice.answerPermission(permission);
		close();
	}

	/* ------------------------------------------------------------------ pickers, camera */

	private void pickDocuments(Intent request) {
		String[] mimes = request.getStringArrayExtra(EXTRA_MIMES);
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		if (mimes != null && mimes.length == 1) {
			intent.setType(mimes[0]);
		} else {
			intent.setType("*/*");
			if (mimes != null && mimes.length > 1) intent.putExtra(Intent.EXTRA_MIME_TYPES, mimes);
		}
		intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, request.getBooleanExtra(EXTRA_MULTIPLE, false));
		startActivityForResult(intent, CODE);
	}

	/* the system's photo picker: Android 13+, or 11+ with the SDK extension 2 (MediaStore.ACTION_PICK_IMAGES) */
	private static boolean photoPicker() {
		if (Build.VERSION.SDK_INT >= 33) return true;
		return Build.VERSION.SDK_INT >= 30 && SdkExtensions.getExtensionVersion(Build.VERSION_CODES.R) >= 2;
	}

	private void pickPhotos(Intent request) {
		int max = request.getIntExtra(EXTRA_MAX, 0);
		Intent intent;
		if (photoPicker()) {
			intent = new Intent(MediaStore.ACTION_PICK_IMAGES);
			intent.setType("image/*");
			if (max != 1) {
				int limit = MediaStore.getPickImagesMaxLimit();
				intent.putExtra(MediaStore.EXTRA_PICK_IMAGES_MAX, max <= 0 || max > limit ? limit : max);
			}
		} else {
			intent = new Intent(Intent.ACTION_GET_CONTENT);
			intent.addCategory(Intent.CATEGORY_OPENABLE);
			intent.setType("image/*");
			intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, max != 1);
		}
		startActivityForResult(intent, CODE);
	}

	private void createDocument(Intent request) {
		String[] mimes = request.getStringArrayExtra(EXTRA_MIMES);
		Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		intent.setType(mimes != null && mimes.length > 0 ? mimes[0] : "application/octet-stream");
		intent.putExtra(Intent.EXTRA_TITLE, request.getStringExtra(EXTRA_TITLE));
		startActivityForResult(intent, CODE);
	}

	private File photoFile() {
		return new File(new File(RavenFileProvider.root(this), "camera"), id + ".jpg");
	}

	private void takePhoto() throws IOException {
		File file = photoFile();
		File folder = file.getParentFile();
		if (folder != null && !folder.isDirectory() && !folder.mkdirs()) throw new IOException("cannot create " + folder);
		Uri uri = RavenFileProvider.uri(this, file);
		Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		intent.putExtra(MediaStore.EXTRA_OUTPUT, uri);
		intent.setClipData(ClipData.newRawUri("", uri));
		intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_READ_URI_PERMISSION);
		startActivityForResult(intent, CODE);
	}

	@Override
	protected void onActivityResult(int code, int result, final Intent data) {
		if (code != CODE || answered) return;
		answered = true;
		final Context context = getApplicationContext();
		final Intent request = getIntent();
		final boolean ok = result == RESULT_OK;
		if (!ok) {
			close();
			if (op == OP_SAVE) deleteTemporary(request);
			if (op == OP_CAMERA) photoFile().delete();
			reply(context, RavenDevice.CANCELLED, null, null);
			return;
		}
		final ArrayList<Uri> uris = new ArrayList<Uri>();
		if (data != null) {
			ClipData clip = data.getClipData();
			if (clip != null) {
				for (int i = 0; i < clip.getItemCount(); i++) if (clip.getItemAt(i).getUri() != null) uris.add(clip.getItemAt(i).getUri());
			}
			if (uris.isEmpty() && data.getData() != null) uris.add(data.getData());
		}
		/*
		 * The access to the URIs a picker gives (the photo picker's above all)
		 * may end with this activity: they are opened here, before it finishes,
		 * and only read on the thread (an open stream stays readable).
		 */
		final ArrayList<Opened> opened = new ArrayList<Opened>();
		try {
			if (op == OP_OPEN || op == OP_PHOTOS) {
				int max = op == OP_PHOTOS ? request.getIntExtra(EXTRA_MAX, 0) : 0;
				int count = max > 0 && uris.size() > max ? max : uris.size();
				for (int i = 0; i < count; i++) opened.add(Opened.read(context.getContentResolver(), uris.get(i)));
			} else if (op == OP_SAVE && !uris.isEmpty()) {
				opened.add(Opened.write(context.getContentResolver(), uris.get(0)));
			}
		} catch (IOException | RuntimeException e) {
			Opened.closeAll(opened);
			close();
			Log.w(TAG, "request " + id + ": cannot open what was chosen", e);
			if (op == OP_SAVE) deleteTemporary(request);
			reply(context, RavenDevice.FAILED, null, e.toString());
			return;
		}
		close();
		final File photo = photoFile();
		final Bitmap thumbnail = op == OP_CAMERA && data != null ? (Bitmap)data.getParcelableExtra("data") : null;
		/* (copies and conversions off the main thread: a large file must not freeze the UI) */
		new Thread(new Runnable() {
			@Override public void run() {
				try {
					switch (op) {
					case OP_OPEN: answerCopies(context, opened, false); break;
					case OP_PHOTOS: answerCopies(context, opened, true); break;
					case OP_SAVE: answerSave(context, request, opened.isEmpty() ? null : opened.get(0)); break;
					case OP_CAMERA: answerPhoto(context, photo, thumbnail); break;
					default: break;
					}
				} catch (IOException | RuntimeException e) {
					Log.w(TAG, "request " + id + " failed", e);
					if (op == OP_SAVE) deleteTemporary(request);
					reply(context, RavenDevice.FAILED, null, e.toString());
				} finally {
					Opened.closeAll(opened);
				}
			}
		}, "Raven request " + id).start();
	}

	/* A URI that was chosen, resolved and opened on the main thread while its access lasts. */
	private static final class Opened {
		String name, mime;
		InputStream input;
		OutputStream output;

		static Opened read(ContentResolver resolver, Uri uri) throws IOException {
			Opened opened = new Opened();
			opened.name = displayName(resolver, uri);
			opened.mime = resolver.getType(uri);
			opened.input = resolver.openInputStream(uri);
			if (opened.input == null) throw new IOException("cannot read " + uri);
			return opened;
		}

		static Opened write(ContentResolver resolver, Uri uri) throws IOException {
			Opened opened = new Opened();
			opened.name = displayName(resolver, uri);
			opened.mime = resolver.getType(uri);
			opened.output = resolver.openOutputStream(uri, "w");
			if (opened.output == null) throw new IOException("cannot write " + uri);
			return opened;
		}

		static void closeAll(ArrayList<Opened> list) {
			for (Opened opened : list) {
				try {
					if (opened.input != null) opened.input.close();
					if (opened.output != null) opened.output.close();
				} catch (IOException e) {
					Log.w(TAG, "closing a stream", e);
				}
				opened.input = null;
				opened.output = null;
			}
		}
	}

	/* ------------------------------------------------------------------ answers (copying thread) */

	/** A name that is one file name ("" and "." and ".." become `fallback`). */
	static String safeName(String name, String fallback) {
		if (name == null) return fallback;
		String safe = name.replace('/', '_').replace('\\', '_').replace('\0', '_').trim();
		if (safe.isEmpty() || safe.equals(".") || safe.equals("..")) return fallback;
		/* (most file systems: 255 bytes) */
		while (safe.getBytes(java.nio.charset.StandardCharsets.UTF_8).length > 200) safe = safe.substring(safe.length() - 150);
		return safe;
	}

	private static String displayName(ContentResolver resolver, Uri uri) {
		Cursor cursor = null;
		try {
			cursor = resolver.query(uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null);
			if (cursor != null && cursor.moveToFirst() && !cursor.isNull(0)) return cursor.getString(0);
		} catch (RuntimeException e) {
			Log.w(TAG, "no display name for " + uri, e);
		} finally {
			if (cursor != null) cursor.close();
		}
		return uri.getLastPathSegment();
	}

	private void answerCopies(Context context, ArrayList<Opened> sources, boolean photos) throws IOException {
		File folder = new File(new File(RavenFileProvider.root(context), "picked"), key());
		if (!folder.isDirectory() && !folder.mkdirs()) throw new IOException("cannot create " + folder);
		if (sources.isEmpty()) {
			reply(context, RavenDevice.CANCELLED, null, null);
			return;
		}
		int count = sources.size();
		RavenDevice.Picked[] files = new RavenDevice.Picked[count];
		for (int i = 0; i < count; i++) {
			Opened source = sources.get(i);
			RavenDevice.Picked file = new RavenDevice.Picked();
			file.name = safeName(source.name, "file");
			file.mime = source.mime;
			File copy = new File(folder, file.name);
			for (int n = 2; copy.exists(); n++) copy = new File(folder, n + "-" + file.name);
			try {
				file.size = RavenFileProvider.copy(source.input, copy);
			} finally {
				source.input.close();
				source.input = null;
			}
			file.path = copy.getAbsolutePath();
			if (photos) uprightImage(file);
			files[i] = file;
		}
		reply(context, RavenDevice.DONE, files, null);
	}

	private void answerSave(Context context, Intent request, Opened target) throws IOException {
		String source = request.getStringExtra(EXTRA_SOURCE);
		if (target == null) {
			deleteTemporary(request);
			reply(context, RavenDevice.CANCELLED, null, null);
			return;
		}
		RavenDevice.Picked file = new RavenDevice.Picked();
		OutputStream output = target.output;
		target.output = null;
		try {
			InputStream input = RavenFileProvider.open(context, source);
			try {
				file.size = RavenFileProvider.copy(input, output);
			} finally {
				input.close();
			}
		} finally {
			output.close();
		}
		deleteTemporary(request);
		file.name = target.name;
		if (file.name == null) file.name = request.getStringExtra(EXTRA_TITLE);
		file.mime = target.mime;
		/* (path NULL: the place is the user's, the application cannot write there again) */
		reply(context, RavenDevice.DONE, new RavenDevice.Picked[] { file }, null);
	}

	private void answerPhoto(Context context, File photo, Bitmap thumbnail) throws IOException {
		if ((!photo.isFile() || photo.length() == 0) && thumbnail != null) {
			/* a camera application that ignored EXTRA_OUTPUT: only its small picture */
			Log.w(TAG, "camera: no photo in EXTRA_OUTPUT, keeping the thumbnail");
			writeJpeg(thumbnail, photo);
		}
		if (!photo.isFile() || photo.length() == 0) {
			photo.delete();
			reply(context, RavenDevice.FAILED, null, "the camera gave no photo");
			return;
		}
		String key = key();
		if (!key.equals(Integer.toString(id))) {
			/* (of an earlier session: a request of this one may soon write <id>.jpg) */
			File renamed = new File(photo.getParentFile(), key + ".jpg");
			if (!photo.renameTo(renamed)) throw new IOException("cannot rename to " + renamed);
			photo = renamed;
		}
		RavenDevice.Picked file = new RavenDevice.Picked();
		file.path = photo.getAbsolutePath();
		file.name = photo.getName();
		file.mime = "image/jpeg";
		uprightImage(file);
		reply(context, RavenDevice.DONE, new RavenDevice.Picked[] { file }, null);
	}

	private static void deleteTemporary(Intent request) {
		String source = request.getStringExtra(EXTRA_SOURCE);
		if (source != null && request.getBooleanExtra(EXTRA_TEMPORARY, false)) new File(source).delete();
	}

	/* ------------------------------------------------------------------ images */

	private static void writeJpeg(Bitmap bitmap, File file) throws IOException {
		OutputStream output = new FileOutputStream(file);
		try {
			if (!bitmap.compress(Bitmap.CompressFormat.JPEG, 90, output)) throw new IOException("JPEG encoding failed");
		} finally {
			output.close();
		}
	}

	private static int exifOrientation(String path) {
		try {
			int orientation = new ExifInterface(path).getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
			return orientation >= ExifInterface.ORIENTATION_NORMAL && orientation <= ExifInterface.ORIENTATION_ROTATE_270 ? orientation
			                                                                                                             : ExifInterface.ORIENTATION_NORMAL;
		} catch (IOException | RuntimeException e) {
			return ExifInterface.ORIENTATION_NORMAL;
		}
	}

	/* the four orientations whose stored picture is turned a quarter: width and height swap */
	private static boolean swapsSides(int orientation) {
		return orientation == ExifInterface.ORIENTATION_TRANSPOSE || orientation == ExifInterface.ORIENTATION_ROTATE_90
			|| orientation == ExifInterface.ORIENTATION_TRANSVERSE || orientation == ExifInterface.ORIENTATION_ROTATE_270;
	}

	/* what turns the stored picture upright: a rotation, a mirror (front cameras), or both */
	private static Matrix uprightMatrix(int orientation) {
		Matrix matrix = new Matrix();
		switch (orientation) {
		case ExifInterface.ORIENTATION_FLIP_HORIZONTAL: matrix.setScale(-1, 1); break;
		case ExifInterface.ORIENTATION_ROTATE_180: matrix.setRotate(180); break;
		case ExifInterface.ORIENTATION_FLIP_VERTICAL: matrix.setRotate(180); matrix.postScale(-1, 1); break;
		case ExifInterface.ORIENTATION_TRANSPOSE: matrix.setRotate(90); matrix.postScale(-1, 1); break;
		case ExifInterface.ORIENTATION_ROTATE_90: matrix.setRotate(90); break;
		case ExifInterface.ORIENTATION_TRANSVERSE: matrix.setRotate(-90); matrix.postScale(-1, 1); break;
		case ExifInterface.ORIENTATION_ROTATE_270: matrix.setRotate(-90); break;
		default: return null;
		}
		return matrix;
	}

	/*
	 * A photo as Raven decodes it: JPEG, PNG, GIF or BMP, upright (Raven does
	 * not read EXIF orientations). Others (HEIC, WebP, AVIF) and those with an
	 * EXIF orientation (turned, mirrored, or both) are decoded by the system,
	 * made upright and written again as JPEG; the size in pixels is filled in.
	 */
	private static void uprightImage(RavenDevice.Picked file) throws IOException {
		BitmapFactory.Options bounds = new BitmapFactory.Options();
		bounds.inJustDecodeBounds = true;
		BitmapFactory.decodeFile(file.path, bounds);
		if (bounds.outWidth <= 0 || bounds.outHeight <= 0) return;
		String type = bounds.outMimeType != null ? bounds.outMimeType.toLowerCase(Locale.ROOT) : "";
		boolean readable = type.equals("image/jpeg") || type.equals("image/png") || type.equals("image/gif") || type.equals("image/bmp")
			|| type.equals("image/x-ms-bmp");
		int orientation = exifOrientation(file.path);
		Matrix matrix = uprightMatrix(orientation);
		file.width = swapsSides(orientation) ? bounds.outHeight : bounds.outWidth;
		file.height = swapsSides(orientation) ? bounds.outWidth : bounds.outHeight;
		if (readable && matrix == null) return;
		BitmapFactory.Options options = new BitmapFactory.Options();
		options.inSampleSize = 1;
		/* (a 200 MP photo would not fit in the heap: halved until under 48 MP) */
		while ((long)(bounds.outWidth / options.inSampleSize) * (bounds.outHeight / options.inSampleSize) > 48000000L) options.inSampleSize *= 2;
		Bitmap bitmap = BitmapFactory.decodeFile(file.path, options);
		if (bitmap == null) return;
		if (matrix != null) {
			Bitmap rotated = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
			if (rotated != bitmap) bitmap.recycle();
			bitmap = rotated;
		}
		File source = new File(file.path);
		String base = source.getName();
		int dot = base.lastIndexOf('.');
		File jpeg = new File(source.getParentFile(), (dot > 0 ? base.substring(0, dot) : base) + ".jpg");
		File temporary = new File(source.getParentFile(), ".raven-upright.jpg");
		writeJpeg(bitmap, temporary);
		file.width = bitmap.getWidth();
		file.height = bitmap.getHeight();
		bitmap.recycle();
		source.delete();
		if (!temporary.renameTo(jpeg)) throw new IOException("cannot rename to " + jpeg);
		Log.i(TAG, "photo " + file.name + ": " + type + (matrix != null ? ", EXIF orientation " + orientation + " made upright" : "") + ", written as JPEG");
		if (!file.name.toLowerCase(Locale.ROOT).endsWith(".jpg")) file.name = jpeg.getName();
		file.path = jpeg.getAbsolutePath();
		file.mime = "image/jpeg";
		file.size = jpeg.length();
	}
}
