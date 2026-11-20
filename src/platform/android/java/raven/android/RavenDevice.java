/*
 * Raven - platform/android/java/raven/android/RavenDevice.java
 *
 * The services of the device (raven_device.h) on the Java side: what
 * android_platform.c calls through JNI for permissions, notifications, the
 * pickers, the camera, haptics, sharing, opening URLs, downloads and the
 * clipboard. See docs/device-services.md.
 *
 * Threads:
 *  - native calls the static methods below from android_main (Raven's UI
 *    thread; download from Raven's worker threads). They return at once;
 *    what touches views or starts activities is posted to the Java main
 *    thread.
 *  - a screen of the system that answers (a permission question, a picker,
 *    the camera) is shown by RavenRequestActivity, which gets the result on
 *    the Java main thread, copies files on a thread of its own and answers
 *    with nativeComplete / nativePermission (registered by native with
 *    RegisterNatives; thread-safe: the core posts the answer to android_main).
 *  - after detach (Rshutdown) answers are dropped here; a native method is
 *    only called while attached (a process restarted under a picker has no
 *    native side yet: calling it would throw).
 *
 * Every constant below mirrors an enum of raven_device.h as an offset from
 * its first value (native adds it back).
 */

package raven.android;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.Settings;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.webkit.MimeTypeMap;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

public final class RavenDevice {
	static final String TAG = "Raven";

	/* RPermission - R_PERMISSION_CAMERA */
	static final int CAMERA = 0, NOTIFICATIONS = 1, PHOTOS = 2, MICROPHONE = 3, LOCATION = 4, PERMISSION_COUNT = 5;
	/* RPermissionStatus - R_PERMISSION_UNSUPPORTED */
	static final int UNSUPPORTED = 0, NOT_ASKED = 1, GRANTED = 2, LIMITED = 3, DENIED = 4, BLOCKED = 5;
	/* RDeviceResult - R_DEVICE_DONE */
	static final int DONE = 0, CANCELLED = 1, REFUSED = 2, NO_SERVICE = 3, BUSY = 4, FAILED = 5;
	/* RHaptic - R_HAPTIC_LIGHT */
	static final int HAPTIC_LIGHT = 0, HAPTIC_MEDIUM = 1, HAPTIC_HEAVY = 2, HAPTIC_SELECTION = 3,
		HAPTIC_SUCCESS = 4, HAPTIC_WARNING = 5, HAPTIC_ERROR = 6;

	/* downloads (rvn_platform_t::download): Raven's limits */
	private static final int DOWNLOAD_MAX = 32 * 1024 * 1024;
	private static final int DOWNLOAD_TIMEOUT_MS = 20000;

	/** The answer to permission_request (status: offset of RPermissionStatus). Any thread. */
	private static native void nativePermission(int permission, int status);
	/**
	 * The end of a request (rvn_service_complete). Paths, names and MIME types are
	 * UTF-8 bytes (a String's modified UTF-8 would garble emoji in a file name).
	 */
	private static native void nativeComplete(int id, int result, byte[][] paths, byte[][] names, byte[][] mimes,
		long[] sizes, int[] widths, int[] heights, byte[] error);

	static final Handler MAIN = new Handler(Looper.getMainLooper());
	private static volatile Activity sActivity;
	private static volatile Context sContext;
	private static volatile boolean sAttached;
	/* the permissions RavenRequestActivity is asking for now (one question each) */
	private static final boolean[] sAsking = new boolean[PERMISSION_COUNT];
	private static String[] sDeclared;
	private static int sNotificationId;
	private static final ThreadLocal<String> sDownloadError = new ThreadLocal<String>();

	private RavenDevice() {}

	/* ------------------------------------------------------------------ answers to native */

	static void answerPermission(int permission) {
		int status = permissionStatus(permission);
		synchronized (sAsking) {
			sAsking[permission] = false;
		}
		Log.i(TAG, "permission " + permission + " answered: status " + status);
		if (!sAttached) return;
		try {
			nativePermission(permission, status);
		} catch (UnsatisfiedLinkError e) {
			Log.w(TAG, "permission answer after Raven stopped", e);
		}
	}

	/** One picked or produced file. */
	static final class Picked {
		String path, name, mime;
		long size;
		int width, height;
	}

	private static byte[] utf8(String text) {
		return text != null ? text.getBytes(StandardCharsets.UTF_8) : null;
	}

	static String string(byte[] bytes) {
		return bytes != null ? new String(bytes, StandardCharsets.UTF_8) : null;
	}

	static void complete(int id, int result, Picked[] files, String error) {
		int count = files != null ? files.length : 0;
		byte[][] paths = new byte[count][], names = new byte[count][], mimes = new byte[count][];
		long[] sizes = new long[count];
		int[] widths = new int[count], heights = new int[count];
		for (int i = 0; i < count; i++) {
			paths[i] = utf8(files[i].path);
			names[i] = utf8(files[i].name);
			mimes[i] = utf8(files[i].mime);
			sizes[i] = files[i].size;
			widths[i] = files[i].width;
			heights[i] = files[i].height;
		}
		Log.i(TAG, "request " + id + ": result " + result + ", " + count + " file(s)" + (error != null ? ": " + error : ""));
		if (!sAttached) return;
		try {
			nativeComplete(id, result, paths, names, mimes, sizes, widths, heights, utf8(error));
		} catch (UnsatisfiedLinkError e) {
			Log.w(TAG, "answer of request " + id + " after Raven stopped", e);
		}
	}

	static void complete(int id, int result, String error) {
		complete(id, result, null, error);
	}

	static Context context() {
		return sContext;
	}

	/* ------------------------------------------------------------------ called by native: lifecycle */

	/**
	 * Raven starts (platform init, android_main): keeps the activity and
	 * empties what the copies of the last run left (the header promises that
	 * they last until the next start at most). Returns the cache folder.
	 */
	public static String attach(Activity activity) {
		sActivity = activity;
		sContext = activity.getApplicationContext();
		sAttached = true;
		File root = RavenFileProvider.root(sContext);
		for (String folder : new String[] { "picked", "camera", "shared", "save" }) deleteTree(new File(root, folder));
		return sContext.getCacheDir().getAbsolutePath();
	}

	/** Raven stops (platform shutdown): late answers are dropped from now on. */
	public static void detach() {
		sAttached = false;
		sActivity = null;
		synchronized (sAsking) {
			for (int i = 0; i < PERMISSION_COUNT; i++) sAsking[i] = false;
		}
	}

	static void deleteTree(File file) {
		File[] children = file.listFiles();
		if (children != null) for (File child : children) deleteTree(child);
		file.delete();
	}

	/* ------------------------------------------------------------------ permissions */

	/* the permissions of the manifest asked for an RPermission, the first one deciding */
	private static String[] permissionNames(int permission) {
		switch (permission) {
		case CAMERA: return new String[] { "android.permission.CAMERA" };
		case NOTIFICATIONS: return Build.VERSION.SDK_INT >= 33 ? new String[] { "android.permission.POST_NOTIFICATIONS" } : new String[0];
		case PHOTOS:
			if (Build.VERSION.SDK_INT >= 34) return new String[] { "android.permission.READ_MEDIA_IMAGES", "android.permission.READ_MEDIA_VISUAL_USER_SELECTED" };
			if (Build.VERSION.SDK_INT >= 33) return new String[] { "android.permission.READ_MEDIA_IMAGES" };
			return new String[] { "android.permission.READ_EXTERNAL_STORAGE" };
		case MICROPHONE: return new String[] { "android.permission.RECORD_AUDIO" };
		case LOCATION: return new String[] { "android.permission.ACCESS_FINE_LOCATION", "android.permission.ACCESS_COARSE_LOCATION" };
		default: return new String[0];
		}
	}

	private static synchronized boolean declared(String name) {
		if (sDeclared == null) {
			try {
				PackageInfo info = sContext.getPackageManager().getPackageInfo(sContext.getPackageName(), PackageManager.GET_PERMISSIONS);
				sDeclared = info.requestedPermissions != null ? info.requestedPermissions : new String[0];
			} catch (PackageManager.NameNotFoundException e) {
				sDeclared = new String[0];
			}
		}
		for (String item : sDeclared) if (item.equals(name)) return true;
		return false;
	}

	/** The permissions to ask for: those of the manifest only (asking for another one is refused at once). */
	static String[] declaredNames(int permission) {
		String[] names = permissionNames(permission);
		int count = 0;
		for (String name : names) if (declared(name)) count++;
		String[] result = new String[count];
		count = 0;
		for (String name : names) if (declared(name)) result[count++] = name;
		return result;
	}

	private static SharedPreferences preferences() {
		return sContext.getSharedPreferences("raven.permissions", Context.MODE_PRIVATE);
	}

	/* Android tells "never asked" and "refused for good" apart from nothing: Raven remembers that it asked */
	static boolean wasAsked(int permission) {
		return preferences().getBoolean("asked." + permission, false);
	}

	static void setAsked(int permission, boolean asked) {
		preferences().edit().putBoolean("asked." + permission, asked).apply();
	}

	private static boolean granted(String name) {
		return sContext.checkSelfPermission(name) == PackageManager.PERMISSION_GRANTED;
	}

	/** The status of an RPermission, without asking. Cheap: no question, no I/O after the first call. */
	public static int permissionStatus(int permission) {
		Context context = sContext;
		if (context == null || permission < 0 || permission >= PERMISSION_COUNT) return UNSUPPORTED;
		if (permission == NOTIFICATIONS && Build.VERSION.SDK_INT < 33) {
			/* (no question before Android 13: allowed unless turned off in the Settings) */
			NotificationManager manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
			return manager != null && manager.areNotificationsEnabled() ? GRANTED : BLOCKED;
		}
		String[] names = declaredNames(permission);
		if (names.length == 0) return UNSUPPORTED;
		if (granted(names[0])) {
			if (permission == NOTIFICATIONS) {
				NotificationManager manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
				if (manager != null && !manager.areNotificationsEnabled()) return BLOCKED;
			}
			return GRANTED;
		}
		/* some photos only (Android 14), the approximate location only */
		for (int i = 1; i < names.length; i++) if (granted(names[i])) return LIMITED;
		if (!wasAsked(permission)) return NOT_ASKED;
		Activity activity = sActivity;
		if (activity == null) return DENIED;
		return activity.shouldShowRequestPermissionRationale(names[0]) ? DENIED : BLOCKED;
	}

	/** Shows the question of the system (RavenRequestActivity); answered with nativePermission. */
	public static void requestPermission(final int permission) {
		final Activity activity = sActivity;
		final String[] names = declaredNames(permission);
		if (activity == null || names.length == 0) {
			answerPermission(permission);
			return;
		}
		synchronized (sAsking) {
			/* (a notification and Rpermission_request may ask together: one question) */
			if (sAsking[permission]) return;
			sAsking[permission] = true;
		}
		MAIN.post(new Runnable() {
			@Override public void run() {
				Intent intent = RavenRequestActivity.intent(activity, RavenRequestActivity.OP_PERMISSION, 0);
				intent.putExtra(RavenRequestActivity.EXTRA_PERMISSION, permission);
				intent.putExtra(RavenRequestActivity.EXTRA_NAMES, names);
				if (!RavenRequestActivity.start(activity, intent)) answerPermission(permission);
			}
		});
	}

	/** The page of the application in the Settings of the system. */
	public static boolean openSettings() {
		Activity activity = sActivity;
		if (activity == null) return false;
		Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.fromParts("package", activity.getPackageName(), null));
		try {
			activity.startActivity(intent);
			return true;
		} catch (ActivityNotFoundException | SecurityException e) {
			Log.w(TAG, "no Settings page for the app", e);
			return false;
		}
	}

	/* ------------------------------------------------------------------ notifications */

	/* one channel per RNotificationUrgency: the importance of a channel cannot change once made */
	private static String channel(NotificationManager manager, int urgency) {
		String id = urgency <= 0 ? "raven.low" : urgency >= 2 ? "raven.urgent" : "raven.normal";
		if (manager.getNotificationChannel(id) == null) {
			String name = urgency <= 0 ? "Quiet" : urgency >= 2 ? "Urgent" : "Notifications";
			int importance = urgency <= 0 ? NotificationManager.IMPORTANCE_LOW
				: urgency >= 2 ? NotificationManager.IMPORTANCE_HIGH : NotificationManager.IMPORTANCE_DEFAULT;
			manager.createNotificationChannel(new NotificationChannel(id, name, importance));
		}
		return id;
	}

	/**
	 * Posts a notification (rvn_platform_t::notify; native has checked the
	 * permission). urgency: RNotificationUrgency - R_NOTIFICATION_LOW.
	 * Tapping it brings the application back (its launcher intent).
	 */
	public static boolean notify(byte[] title, byte[] body, int urgency) {
		Context context = sContext;
		if (context == null) return false;
		NotificationManager manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
		if (manager == null) return false;
		try {
			int icon = context.getApplicationInfo().icon;
			Notification.Builder builder = new Notification.Builder(context, channel(manager, urgency))
				.setSmallIcon(icon != 0 ? icon : android.R.drawable.ic_dialog_info)
				.setContentTitle(string(title))
				.setContentText(string(body))
				.setStyle(new Notification.BigTextStyle().bigText(string(body)))
				.setAutoCancel(true);
			if (urgency >= 2) builder.setCategory(Notification.CATEGORY_REMINDER);
			Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
			if (launch != null) {
				launch.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
				builder.setContentIntent(PendingIntent.getActivity(context, 0, launch, PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT));
			}
			int id;
			synchronized (RavenDevice.class) {
				id = ++sNotificationId;
			}
			manager.notify(id, builder.build());
			Log.i(TAG, "notification " + id + " posted (urgency " + urgency + ")");
			return true;
		} catch (RuntimeException e) {
			Log.w(TAG, "notification failed", e);
			return false;
		}
	}

	/* ------------------------------------------------------------------ pickers, camera */

	/** MIME types for extensions ("png" -> image/png); none known: any file. */
	public static String[] mimeTypes(String[] extensions) {
		java.util.ArrayList<String> types = new java.util.ArrayList<String>();
		if (extensions != null) {
			for (String extension : extensions) {
				String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension.toLowerCase(Locale.ROOT));
				if (type != null && !types.contains(type)) types.add(type);
			}
		}
		return types.toArray(new String[0]);
	}

	static String mimeOfName(String name) {
		int dot = name != null ? name.lastIndexOf('.') : -1;
		String type = dot >= 0 ? MimeTypeMap.getSingleton().getMimeTypeFromExtension(name.substring(dot + 1).toLowerCase(Locale.ROOT)) : null;
		return type != null ? type : "application/octet-stream";
	}

	private static void startRequest(final Intent intent, final int id) {
		final Activity activity = sActivity;
		if (activity == null) {
			complete(id, FAILED, "no activity");
			return;
		}
		MAIN.post(new Runnable() {
			@Override public void run() {
				if (!RavenRequestActivity.start(activity, intent)) complete(id, FAILED, "RavenRequestActivity could not be started (is it in the manifest?)");
			}
		});
	}

	/** The document picker (ACTION_OPEN_DOCUMENT); extensions: lowercase, none = any file. */
	public static void fileOpen(int id, String[] extensions, boolean multiple) {
		Activity activity = sActivity;
		if (activity == null) {
			complete(id, FAILED, "no activity");
			return;
		}
		Intent intent = RavenRequestActivity.intent(activity, RavenRequestActivity.OP_OPEN, id);
		intent.putExtra(RavenRequestActivity.EXTRA_MIMES, mimeTypes(extensions));
		intent.putExtra(RavenRequestActivity.EXTRA_MULTIPLE, multiple);
		startRequest(intent, id);
	}

	/**
	 * ACTION_CREATE_DOCUMENT, then the file `source` (absolute, or a path of
	 * the APK's assets) is copied there; `temporary`: source is Raven's copy
	 * of the bytes, deleted afterwards.
	 */
	public static void fileSave(int id, byte[] name, byte[] source, boolean temporary) {
		Activity activity = sActivity;
		if (activity == null) {
			complete(id, FAILED, "no activity");
			return;
		}
		String title = string(name);
		Intent intent = RavenRequestActivity.intent(activity, RavenRequestActivity.OP_SAVE, id);
		intent.putExtra(RavenRequestActivity.EXTRA_TITLE, title);
		intent.putExtra(RavenRequestActivity.EXTRA_MIMES, new String[] { mimeOfName(title) });
		intent.putExtra(RavenRequestActivity.EXTRA_SOURCE, string(source));
		intent.putExtra(RavenRequestActivity.EXTRA_TEMPORARY, temporary);
		startRequest(intent, id);
	}

	/** A camera, and an application that takes a photo for others (MediaStore.ACTION_IMAGE_CAPTURE). */
	public static boolean cameraAvailable() {
		Context context = sContext;
		if (context == null) return false;
		PackageManager manager = context.getPackageManager();
		if (!manager.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY)) return false;
		return new Intent(android.provider.MediaStore.ACTION_IMAGE_CAPTURE).resolveActivity(manager) != null;
	}

	public static void takePhoto(int id) {
		Activity activity = sActivity;
		if (activity == null) {
			complete(id, FAILED, "no activity");
			return;
		}
		startRequest(RavenRequestActivity.intent(activity, RavenRequestActivity.OP_CAMERA, id), id);
	}

	/** The photo picker of the system; max 0: no limit. */
	public static void pickPhotos(int id, int max) {
		Activity activity = sActivity;
		if (activity == null) {
			complete(id, FAILED, "no activity");
			return;
		}
		Intent intent = RavenRequestActivity.intent(activity, RavenRequestActivity.OP_PHOTOS, id);
		intent.putExtra(RavenRequestActivity.EXTRA_MAX, max);
		startRequest(intent, id);
	}

	/* ------------------------------------------------------------------ share */

	/**
	 * The chooser with ACTION_SEND: text and / or a file (served by
	 * RavenFileProvider; copied into <cache>/raven/shared first when it is
	 * not under <cache>/raven). Answered DONE once the chooser is started:
	 * Android does not tell what the user did.
	 */
	public static void share(final int id, final byte[] text, final byte[] path, final byte[] mime, final byte[] title) {
		final Activity activity = sActivity;
		if (activity == null) {
			complete(id, FAILED, "no activity");
			return;
		}
		new Thread(new Runnable() {
			@Override public void run() {
				final String file = string(path);
				Uri uri = null;
				String type = string(mime);
				try {
					if (file != null) {
						File served = RavenFileProvider.servable(activity, file, id);
						uri = RavenFileProvider.uri(activity, served);
						if (type == null) type = mimeOfName(served.getName());
					}
				} catch (IOException e) {
					complete(id, FAILED, "share: " + file + ": " + e.getMessage());
					return;
				}
				final Uri stream = uri;
				final String streamType = type;
				MAIN.post(new Runnable() {
					@Override public void run() {
						Intent send = new Intent(Intent.ACTION_SEND);
						String message = string(text);
						if (message != null) send.putExtra(Intent.EXTRA_TEXT, message);
						if (stream != null) {
							send.setType(streamType);
							send.putExtra(Intent.EXTRA_STREAM, stream);
							send.setClipData(ClipData.newRawUri("", stream));
							send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
						} else {
							send.setType("text/plain");
						}
						Intent chooser = Intent.createChooser(send, string(title));
						if (stream != null) chooser.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
						try {
							activity.startActivity(chooser);
							complete(id, DONE, null);
						} catch (ActivityNotFoundException | SecurityException e) {
							complete(id, FAILED, "share: " + e);
						}
					}
				});
			}
		}, "Raven share").start();
	}

	/* ------------------------------------------------------------------ URLs, downloads */

	/** ACTION_VIEW: the browser for http(s), the application of any other scheme. */
	public static boolean openUrl(byte[] address) {
		Activity activity = sActivity;
		String url = string(address);
		if (activity == null || url == null) return false;
		try {
			activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
			return true;
		} catch (ActivityNotFoundException | SecurityException e) {
			Log.w(TAG, "no application opens " + url, e);
			return false;
		}
	}

	/**
	 * Downloads http(s) into memory (rvn_platform_t::download), on Raven's
	 * worker thread (it blocks). Redirections followed, also from http to
	 * https (HttpURLConnection follows only those within one protocol).
	 * null on failure: downloadError() says why.
	 */
	public static byte[] download(byte[] location) {
		String address = string(location);
		sDownloadError.set(null);
		HttpURLConnection connection = null;
		try {
			URL url = new URL(address);
			for (int redirects = 0; ; redirects++) {
				String protocol = url.getProtocol();
				if (!"http".equals(protocol) && !"https".equals(protocol)) throw new IOException("not http(s): " + url);
				connection = (HttpURLConnection)url.openConnection();
				connection.setConnectTimeout(DOWNLOAD_TIMEOUT_MS);
				connection.setReadTimeout(DOWNLOAD_TIMEOUT_MS);
				connection.setInstanceFollowRedirects(false);
				int code = connection.getResponseCode();
				if (code >= 300 && code < 400 && connection.getHeaderField("Location") != null) {
					if (redirects >= 8) throw new IOException("too many redirections");
					url = new URL(url, connection.getHeaderField("Location"));
					connection.disconnect();
					connection = null;
					continue;
				}
				if (code < 200 || code > 299) throw new IOException("HTTP " + code + " " + connection.getResponseMessage());
				long length = connection.getContentLengthLong();
				if (length > DOWNLOAD_MAX) throw new IOException("larger than 32 MiB");
				InputStream input = connection.getInputStream();
				ByteArrayOutputStream output = new ByteArrayOutputStream(length > 0 ? (int)length : 65536);
				byte[] buffer = new byte[65536];
				int read;
				try {
					while ((read = input.read(buffer)) > 0) {
						if (output.size() + read > DOWNLOAD_MAX) throw new IOException("larger than 32 MiB");
						output.write(buffer, 0, read);
					}
				} finally {
					input.close();
				}
				return output.toByteArray();
			}
		} catch (IOException | RuntimeException e) {
			/* (SecurityException: no INTERNET in the manifest) */
			String why = e.getMessage() != null ? e.getClass().getSimpleName() + ": " + e.getMessage() : e.toString();
			if (e instanceof SecurityException) why += " (is android.permission.INTERNET in the manifest?)";
			sDownloadError.set(why);
			return null;
		} finally {
			if (connection != null) connection.disconnect();
		}
	}

	/** Why the last download of this thread failed. */
	public static byte[] downloadError() {
		return utf8(sDownloadError.get());
	}

	/* ------------------------------------------------------------------ haptics */

	public static boolean hapticSupported() {
		Context context = sContext;
		if (context == null) return false;
		Vibrator vibrator = (Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE);
		return vibrator != null && vibrator.hasVibrator();
	}

	/* RHaptic -> HapticFeedbackConstants: what the system's own controls play */
	private static int feedback(int haptic) {
		boolean api30 = Build.VERSION.SDK_INT >= 30;
		switch (haptic) {
		case HAPTIC_LIGHT: return HapticFeedbackConstants.KEYBOARD_TAP;
		case HAPTIC_MEDIUM: return HapticFeedbackConstants.VIRTUAL_KEY;
		case HAPTIC_HEAVY: return HapticFeedbackConstants.LONG_PRESS;
		case HAPTIC_SELECTION: return Build.VERSION.SDK_INT >= 34 ? HapticFeedbackConstants.SEGMENT_TICK : HapticFeedbackConstants.CLOCK_TICK;
		case HAPTIC_SUCCESS: return api30 ? HapticFeedbackConstants.CONFIRM : HapticFeedbackConstants.VIRTUAL_KEY;
		case HAPTIC_WARNING:
		case HAPTIC_ERROR: return api30 ? HapticFeedbackConstants.REJECT : HapticFeedbackConstants.LONG_PRESS;
		default: return HapticFeedbackConstants.VIRTUAL_KEY;
		}
	}

	/* the Vibrator, when there is no view to play it (VIBRATE in the manifest) */
	private static boolean vibrate(int haptic) {
		Context context = sContext;
		Vibrator vibrator = context != null ? (Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE) : null;
		if (vibrator == null || !vibrator.hasVibrator()) return false;
		int effect = haptic == HAPTIC_HEAVY || haptic == HAPTIC_ERROR ? VibrationEffect.EFFECT_HEAVY_CLICK
			: haptic == HAPTIC_WARNING ? VibrationEffect.EFFECT_DOUBLE_CLICK
			: haptic == HAPTIC_LIGHT || haptic == HAPTIC_SELECTION ? VibrationEffect.EFFECT_TICK : VibrationEffect.EFFECT_CLICK;
		try {
			vibrator.vibrate(VibrationEffect.createPredefined(effect));
			return true;
		} catch (SecurityException e) {
			Log.w(TAG, "haptic: no VIBRATE in the manifest", e);
			return false;
		}
	}

	/**
	 * Plays an RHaptic: View.performHapticFeedback on the activity's decor
	 * view (on the main thread), which follows the user's "touch feedback"
	 * setting. Returns at once.
	 */
	public static boolean haptic(final int haptic) {
		final Activity activity = sActivity;
		if (activity == null) return false;
		MAIN.post(new Runnable() {
			@Override public void run() {
				View view = activity.getWindow() != null ? activity.getWindow().getDecorView() : null;
				int constant = feedback(haptic);
				if (view != null) {
					boolean played = view.performHapticFeedback(constant);
					Log.i(TAG, "haptic " + haptic + ": performHapticFeedback(" + constant + ") -> " + played);
				} else {
					Log.i(TAG, "haptic " + haptic + ": no view, Vibrator -> " + vibrate(haptic));
				}
			}
		});
		return true;
	}

	/* ------------------------------------------------------------------ clipboard */

	private static ClipboardManager clipboard() {
		Context context = sContext;
		return context != null ? (ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE) : null;
	}

	public static boolean clipboardSet(byte[] text) {
		ClipboardManager clipboard = clipboard();
		if (clipboard == null) return false;
		try {
			clipboard.setPrimaryClip(ClipData.newPlainText("Raven", string(text)));
			return true;
		} catch (RuntimeException e) {
			Log.w(TAG, "clipboard: could not set", e);
			return false;
		}
	}

	/** The text of the clipboard (Android 10+: only while the app has the focus; 12+ shows a toast), or null. */
	public static byte[] clipboardGet() {
		ClipboardManager clipboard = clipboard();
		if (clipboard == null || sContext == null) return null;
		try {
			ClipData clip = clipboard.getPrimaryClip();
			if (clip == null || clip.getItemCount() < 1) return null;
			CharSequence text = clip.getItemAt(0).coerceToText(sContext);
			return text != null ? utf8(text.toString()) : null;
		} catch (RuntimeException e) {
			Log.w(TAG, "clipboard: could not read", e);
			return null;
		}
	}

	/** Text in the clipboard, from its description only: nothing is read, no toast. */
	public static boolean clipboardHasText() {
		ClipboardManager clipboard = clipboard();
		if (clipboard == null) return false;
		try {
			if (!clipboard.hasPrimaryClip()) return false;
			ClipDescription description = clipboard.getPrimaryClipDescription();
			return description != null && (description.hasMimeType("text/*") || description.hasMimeType(ClipDescription.MIMETYPE_TEXT_URILIST));
		} catch (RuntimeException e) {
			return false;
		}
	}
}
