/*
 * Raven - platform/android/java/raven/android/RavenFileProvider.java
 *
 * Hands files of the application's cache to other applications as
 * content:// URIs (androidx's FileProvider is not in an APK built without
 * Gradle):
 *
 *  - Rshare_show with a file: Intent.ACTION_SEND with EXTRA_STREAM and
 *    FLAG_GRANT_READ_URI_PERMISSION;
 *  - Rcamera_take_photo: the EXTRA_OUTPUT the camera application writes to
 *    (FLAG_GRANT_WRITE_URI_PERMISSION).
 *
 * content://<package>.raven.files/<path under getCacheDir()/raven>. The
 * provider is not exported: only the applications given a URI grant can
 * open one, and only files under getCacheDir()/raven are served (openFile
 * refuses any other path, ".." included). query answers the display name
 * and the size (OpenableColumns), what the receivers of a share ask for.
 * The manifest declares it (authority: the package name + ".raven.files").
 * See docs/device-services.md.
 */

package raven.android;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class RavenFileProvider extends ContentProvider {
	static String authority(Context context) {
		return context.getPackageName() + ".raven.files";
	}

	/** getCacheDir()/raven: everything Raven copies, and all the provider serves. */
	static File root(Context context) {
		return new File(context.getCacheDir(), "raven");
	}

	/** The URI of a file under root(). */
	static Uri uri(Context context, File file) throws IOException {
		String base = root(context).getCanonicalPath() + File.separator;
		String path = file.getCanonicalPath();
		if (!path.startsWith(base)) throw new IOException("not under " + base);
		Uri.Builder builder = new Uri.Builder().scheme("content").authority(authority(context));
		for (String segment : path.substring(base.length()).split("/")) builder.appendPath(segment);
		return builder.build();
	}

	/**
	 * A file of Raven (an absolute path, or a path of the APK's assets) as a
	 * file under root(): itself when it is there, else a copy in
	 * root()/shared/<id>/.
	 */
	static File servable(Context context, String path, int id) throws IOException {
		File file = new File(path);
		String base = root(context).getCanonicalPath() + File.separator;
		if (file.isAbsolute() && file.getCanonicalPath().startsWith(base)) return file;
		File folder = new File(new File(root(context), "shared"), Integer.toString(id));
		if (!folder.isDirectory() && !folder.mkdirs()) throw new IOException("cannot create " + folder);
		File copy = new File(folder, RavenRequestActivity.safeName(file.getName(), "file"));
		InputStream input = open(context, path);
		try {
			copy(input, copy);
		} finally {
			input.close();
		}
		return copy;
	}

	/** Reads a path Raven can read: absolute, or relative to the APK's assets (Raven's relative paths). */
	static InputStream open(Context context, String path) throws IOException {
		if (path.startsWith("/")) return new java.io.FileInputStream(path);
		String relative = path.startsWith("./") ? path.substring(2) : path;
		return context.getAssets().open(relative, AssetManager.ACCESS_STREAMING);
	}

	static long copy(InputStream input, File file) throws IOException {
		OutputStream output = new FileOutputStream(file);
		try {
			return copy(input, output);
		} finally {
			output.close();
		}
	}

	static long copy(InputStream input, OutputStream output) throws IOException {
		byte[] buffer = new byte[65536];
		long total = 0;
		int read;
		while ((read = input.read(buffer)) > 0) {
			output.write(buffer, 0, read);
			total += read;
		}
		return total;
	}

	/* the file of a URI, only under root() */
	private File file(Uri uri) throws FileNotFoundException {
		Context context = getContext();
		String path = uri.getPath();
		if (context == null || path == null) throw new FileNotFoundException(String.valueOf(uri));
		try {
			File root = root(context);
			File file = new File(root, path).getCanonicalFile();
			if (!file.getPath().startsWith(root.getCanonicalPath() + File.separator)) throw new FileNotFoundException("not served: " + uri);
			return file;
		} catch (IOException e) {
			throw new FileNotFoundException(e.getMessage());
		}
	}

	@Override
	public boolean onCreate() {
		return true;
	}

	@Override
	public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
		File file = file(uri);
		int flags = ParcelFileDescriptor.parseMode(mode);
		if ((flags & ParcelFileDescriptor.MODE_CREATE) != 0) {
			File parent = file.getParentFile();
			if (parent != null) parent.mkdirs();
		}
		return ParcelFileDescriptor.open(file, flags);
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection, String[] arguments, String order) {
		File file;
		try {
			file = file(uri);
		} catch (FileNotFoundException e) {
			return null;
		}
		if (projection == null) projection = new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE };
		String[] columns = new String[projection.length];
		Object[] values = new Object[projection.length];
		int count = 0;
		for (String column : projection) {
			if (OpenableColumns.DISPLAY_NAME.equals(column)) {
				columns[count] = column;
				values[count++] = file.getName();
			} else if (OpenableColumns.SIZE.equals(column)) {
				columns[count] = column;
				values[count++] = file.length();
			}
		}
		String[] usedColumns = new String[count];
		Object[] usedValues = new Object[count];
		System.arraycopy(columns, 0, usedColumns, 0, count);
		System.arraycopy(values, 0, usedValues, 0, count);
		MatrixCursor cursor = new MatrixCursor(usedColumns, 1);
		cursor.addRow(usedValues);
		return cursor;
	}

	@Override
	public String getType(Uri uri) {
		String path = uri.getPath();
		return path != null ? RavenDevice.mimeOfName(path) : null;
	}

	@Override
	public Uri insert(Uri uri, ContentValues values) {
		throw new UnsupportedOperationException("RavenFileProvider serves files only");
	}

	@Override
	public int delete(Uri uri, String selection, String[] arguments) {
		return 0;
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection, String[] arguments) {
		throw new UnsupportedOperationException("RavenFileProvider serves files only");
	}
}
