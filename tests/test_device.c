/*
 * The services of the device (raven_device.h): permissions, pickers, the
 * camera, sharing, haptics, URLs and downloads. The host platform
 * (raven_host.h) plays the phone and its user; then the null platform shows
 * what a desktop without these services answers, and the desktop file
 * dialogs answer the asynchronous pickers.
 */
#include "test.h"
#include <stdint.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 *  The phone: what the host platform was asked, and how it answers
 * ------------------------------------------------------------------------- */

static struct {
	RPermissionStatus status[5];
	int permission_requests;
	RPermission last_permission;
	int service_requests;
	RHostServiceRequest last;
	char last_text[256];
	char last_file_name[128];
	char last_data[64];
	size_t last_size;
	char last_filters[128];
	int haptics;
	RHaptic last_haptic;
	int urls;
	char last_url[256];
	int downloads;
	int download_fails;
} g_phone;

/* What the application was answered. */
static struct {
	int permission_answers;
	RPermission permission;
	RPermissionStatus status;
	int device_answers;
	RDeviceResult result;
	int count;
	char names[4][64];
	char paths[4][512];
	uint64_t sizes[4];
	int width, height;
} g_answer;

static RPermissionStatus phone_permission_status(RPermission permission, void* user) {
	(void)user;
	return g_phone.status[permission - R_PERMISSION_CAMERA];
}

static void phone_permission_request(RPermission permission, void* user) {
	(void)user;
	g_phone.permission_requests++;
	g_phone.last_permission = permission;
}

static void phone_service_request(const RHostServiceRequest* request, void* user) {
	(void)user;
	g_phone.service_requests++;
	g_phone.last = *request;
	/* (valid during the call only) */
	snprintf(g_phone.last_text, sizeof(g_phone.last_text), "%s", request->text ? request->text : "");
	snprintf(g_phone.last_file_name, sizeof(g_phone.last_file_name), "%s", request->file_name ? request->file_name : "");
	g_phone.last_size = request->size;
	memset(g_phone.last_data, 0, sizeof(g_phone.last_data));
	if (request->data && request->size < sizeof(g_phone.last_data)) memcpy(g_phone.last_data, request->data, request->size);
	g_phone.last_filters[0] = '\0';
	if (request->filter_count > 0) snprintf(g_phone.last_filters, sizeof(g_phone.last_filters), "%s", request->filters[0].patterns);
	g_phone.last.text = NULL;
	g_phone.last.file_name = NULL;
	g_phone.last.data = NULL;
	g_phone.last.filters = NULL;
	g_phone.last.title = NULL;
	g_phone.last.path = NULL;
	g_phone.last.mime_type = NULL;
	g_phone.last.share_title = NULL;
	g_phone.last.default_extension = NULL;
}

static int phone_open_url(const char* url, void* user) {
	(void)user;
	g_phone.urls++;
	snprintf(g_phone.last_url, sizeof(g_phone.last_url), "%s", url);
	return R_TRUE;
}

static int phone_haptic(RHaptic haptic, void* user) {
	(void)user;
	g_phone.haptics++;
	g_phone.last_haptic = haptic;
	return R_TRUE;
}

/* A 2 x 2 red PNG. */
static const unsigned char g_png[] = {
	0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,
	0x08,0x06,0x00,0x00,0x00,0x72,0xb6,0x0d,0x24,0x00,0x00,0x00,0x11,0x49,0x44,0x41,0x54,0x78,0x9c,0x63,0xf8,0xcf,0xc0,0xf0,
	0x1f,0x84,0x19,0x60,0x0c,0x00,0x47,0xca,0x07,0xf9,0x67,0x59,0x6e,0xb7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,
	0x60,0x82
};

/* Worker threads. */
static void* phone_download(const char* url, size_t* size, void* user) {
	void* data;
	(void)user;
	g_phone.downloads++;
	if (strstr(url, "missing")) {
		g_phone.download_fails++;
		return NULL;
	}
	data = Rmemory_alloc(sizeof(g_png));
	memcpy(data, g_png, sizeof(g_png));
	*size = sizeof(g_png);
	return data;
}

static void on_permission(RPermission permission, RPermissionStatus status, void* user) {
	g_answer.permission_answers += (int)(intptr_t)user;
	g_answer.permission = permission;
	g_answer.status = status;
}

static void on_device(RDeviceResult result, const RPickedFile* files, int count, void* user) {
	int i;
	(void)user;
	g_answer.device_answers++;
	g_answer.result = result;
	g_answer.count = count;
	for (i = 0; i < count && i < 4; i++) {
		snprintf(g_answer.names[i], sizeof(g_answer.names[i]), "%s", files[i].name ? files[i].name : "");
		snprintf(g_answer.paths[i], sizeof(g_answer.paths[i]), "%s", files[i].path ? files[i].path : "");
		g_answer.sizes[i] = files[i].size;
	}
	if (count > 0) {
		g_answer.width = files[0].width;
		g_answer.height = files[0].height;
	}
}

static void reset(void) {
	int i;
	memset(&g_answer, 0, sizeof(g_answer));
	g_phone.permission_requests = 0;
	g_phone.service_requests = 0;
	for (i = 0; i < 5; i++) g_phone.status[i] = R_PERMISSION_NOT_ASKED;
}

static int write_file(const char* path, const char* text) {
	return Rfile_write(path, text, strlen(text));
}

/* ---------------------------------------------------------------------------
 *  Permissions
 * ------------------------------------------------------------------------- */

static void test_permission_request(void) {
	reset();
	CHECK(Rpermission_get_status(R_PERMISSION_CAMERA) == R_PERMISSION_NOT_ASKED);
	CHECK_STR(Rpermission_status_to_string(R_PERMISSION_NOT_ASKED), "not asked");
	CHECK_STR(Rpermission_to_string(R_PERMISSION_NOTIFICATIONS), "notifications");

	/* two requests, one question; nothing answered inside the call */
	Rpermission_request(R_PERMISSION_CAMERA, on_permission, (void*)(intptr_t)1);
	Rpermission_request(R_PERMISSION_CAMERA, on_permission, (void*)(intptr_t)10);
	CHECK(g_phone.permission_requests == 1);
	CHECK(g_phone.last_permission == R_PERMISSION_CAMERA);
	CHECK(g_answer.permission_answers == 0);
	Rdraw_frame();
	CHECK(g_answer.permission_answers == 0);

	/* the user says yes: both hear it at the next frame */
	g_phone.status[0] = R_PERMISSION_GRANTED;
	Rhost_permission_answer(R_PERMISSION_CAMERA, R_PERMISSION_GRANTED);
	CHECK(g_answer.permission_answers == 0);
	Rdraw_frame();
	CHECK(g_answer.permission_answers == 11);
	CHECK(g_answer.permission == R_PERMISSION_CAMERA);
	CHECK(g_answer.status == R_PERMISSION_GRANTED);

	/* already granted: no question, the status at the next frame */
	reset();
	g_phone.status[0] = R_PERMISSION_GRANTED;
	Rpermission_request(R_PERMISSION_CAMERA, on_permission, (void*)(intptr_t)1);
	CHECK(g_phone.permission_requests == 0);
	CHECK(g_answer.permission_answers == 0);
	Rdraw_frame();
	CHECK(g_answer.permission_answers == 1);
	CHECK(g_answer.status == R_PERMISSION_GRANTED);

	/* refused for good: no question either */
	reset();
	g_phone.status[1] = R_PERMISSION_BLOCKED;
	Rpermission_request(R_PERMISSION_NOTIFICATIONS, on_permission, (void*)(intptr_t)1);
	CHECK(g_phone.permission_requests == 0);
	Rdraw_frame();
	CHECK(g_answer.status == R_PERMISSION_BLOCKED);

	/* refused once: asked again */
	reset();
	g_phone.status[1] = R_PERMISSION_DENIED;
	Rpermission_request(R_PERMISSION_NOTIFICATIONS, NULL, NULL);
	CHECK(g_phone.permission_requests == 1);
	Rhost_permission_answer(R_PERMISSION_NOTIFICATIONS, R_PERMISSION_BLOCKED);
	Rdraw_frame();

	EXPECT_ERRORS_BEGIN();
	Rpermission_request((RPermission)0x7777, on_permission, NULL);
	CHECK(Rpermission_get_status((RPermission)0x7777) == R_PERMISSION_UNSUPPORTED);
	Rhost_permission_answer((RPermission)0x7777, R_PERMISSION_GRANTED);
	EXPECT_ERRORS_END(3);
	Rdraw_frame();
	CHECK(g_answer.permission_answers == 0);
}

static void answer_from_worker(void* user) {
	(void)user;
	Rhost_permission_answer(R_PERMISSION_MICROPHONE, R_PERMISSION_DENIED);
}

/* The system answers on its own thread (Android's main thread, a completion block of UIKit). */
static void test_permission_answer_from_another_thread(void) {
	int i;
	reset();
	Rpermission_request(R_PERMISSION_MICROPHONE, on_permission, (void*)(intptr_t)1);
	CHECK(g_phone.permission_requests == 1);
	Rtask_run_async(answer_from_worker, NULL);
	for (i = 0; i < 200 && g_answer.permission_answers == 0; i++) Rwait_frame(0.01);
	CHECK(g_answer.permission_answers == 1);
	CHECK(g_answer.permission == R_PERMISSION_MICROPHONE);
	CHECK(g_answer.status == R_PERMISSION_DENIED);
}

/* ---------------------------------------------------------------------------
 *  Pickers, camera, share
 * ------------------------------------------------------------------------- */

static void test_file_open(void) {
	RFileDialogDesc desc;
	RFileFilter filter = { "Text", "*.txt;*.MD" };
	RPickedFile files[2];
	char* content;
	size_t size = 0;

	reset();
	CHECK(write_file("raven_test_device_a.txt", "alpha"));
	CHECK(write_file("raven_test_device_b.md", "beta!"));
	memset(&desc, 0, sizeof(desc));
	desc.filters = &filter;
	desc.filter_count = 1;
	desc.flags = R_FILE_DIALOG_MULTIPLE;
	Rfile_open_async(&desc, on_device, NULL);
	CHECK(g_phone.service_requests == 1);
	CHECK(g_phone.last.service == R_HOST_SERVICE_OPEN_FILES);
	CHECK(g_phone.last.flags & R_FILE_DIALOG_MULTIPLE);
	CHECK(g_phone.last.filter_count == 1);
	CHECK_STR(g_phone.last_filters, "*.txt;*.MD");
	CHECK(g_phone.last.id != 0);
	CHECK(g_phone.last.window != R_INVALID_ID);

	/* the user picks two files: the copies of the platform, by path */
	memset(files, 0, sizeof(files));
	files[0].path = "raven_test_device_a.txt";
	files[0].name = "Alpha.txt";
	files[1].path = "raven_test_device_b.md";
	Rhost_service_complete(g_phone.last.id, R_DEVICE_DONE, files, 2);
	CHECK(g_answer.device_answers == 0);
	Rdraw_frame();
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_DONE);
	CHECK(g_answer.count == 2);
	CHECK_STR(g_answer.names[0], "Alpha.txt");
	/* (no name given: the name of the file) */
	CHECK_STR(g_answer.names[1], "raven_test_device_b.md");
	/* (size 0: read from the file) */
	CHECK(g_answer.sizes[0] == 5);
	content = Rfile_read(g_answer.paths[0], &size);
	CHECK(content && size == 5 && strcmp(content, "alpha") == 0);
	Rmemory_free(content);

	/* answered twice, or for a request that is not open: ignored with a warning */
	EXPECT_ERRORS_BEGIN();
	Rhost_service_complete(g_phone.last.id, R_DEVICE_DONE, NULL, 0);
	Rdraw_frame();
	EXPECT_ERRORS_END(1);
	CHECK(g_answer.device_answers == 1);

	/* cancelled */
	reset();
	Rfile_open_async(NULL, on_device, NULL);
	CHECK(g_phone.last.filter_count == 0);
	CHECK(!(g_phone.last.flags & R_FILE_DIALOG_MULTIPLE));
	Rhost_service_complete(g_phone.last.id, R_DEVICE_CANCELLED, NULL, 0);
	Rdraw_frame();
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_CANCELLED);
	CHECK(g_answer.count == 0);
	remove("raven_test_device_a.txt");
	remove("raven_test_device_b.md");
}

static void test_one_sheet_at_a_time(void) {
	Ruint first;
	reset();
	Rphoto_pick(3, on_device, NULL);
	CHECK(g_phone.service_requests == 1);
	CHECK(g_phone.last.service == R_HOST_SERVICE_PICK_PHOTOS);
	CHECK(g_phone.last.max_count == 3);
	first = g_phone.last.id;
	/* a picker is open: busy */
	Rfile_open_async(NULL, on_device, NULL);
	CHECK(g_phone.service_requests == 1);
	Rdraw_frame();
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_BUSY);
	Rhost_service_complete(first, R_DEVICE_CANCELLED, NULL, 0);
	Rdraw_frame();
	CHECK(g_answer.device_answers == 2);
	CHECK(g_answer.result == R_DEVICE_CANCELLED);
	/* closed: the next one is shown */
	Rfile_open_async(NULL, on_device, NULL);
	CHECK(g_phone.service_requests == 2);
	Rhost_service_complete(g_phone.last.id, R_DEVICE_CANCELLED, NULL, 0);
	Rdraw_frame();
}

static void test_save(void) {
	RFileDialogDesc desc;
	RPickedFile saved;
	char bytes[] = "hello phone";
	reset();
	memset(&desc, 0, sizeof(desc));
	desc.file_name = "note.txt";
	Rfile_save_async(&desc, bytes, strlen(bytes), on_device, NULL);
	/* (a copy: the application may change its buffer at once) */
	bytes[0] = 'J';
	CHECK(g_phone.service_requests == 1);
	CHECK(g_phone.last.service == R_HOST_SERVICE_SAVE_FILE);
	CHECK_STR(g_phone.last_file_name, "note.txt");
	CHECK(g_phone.last_size == 11);
	CHECK_STR(g_phone.last_data, "hello phone");
	/* saved where the user chose: a name, no path the application could write to */
	memset(&saved, 0, sizeof(saved));
	saved.name = "note (2).txt";
	saved.size = 11;
	Rhost_service_complete(g_phone.last.id, R_DEVICE_DONE, &saved, 1);
	Rdraw_frame();
	CHECK(g_answer.result == R_DEVICE_DONE);
	CHECK(g_answer.count == 1);
	CHECK_STR(g_answer.names[0], "note (2).txt");
	CHECK_STR(g_answer.paths[0], "");
	CHECK(g_answer.sizes[0] == 11);

	/* exporting a file: its name is proposed */
	reset();
	Rfile_export_async(NULL, "some/folder/report.pdf", on_device, NULL);
	CHECK_STR(g_phone.last_file_name, "report.pdf");
	CHECK(g_phone.last_size == 0);
	Rhost_service_complete(g_phone.last.id, R_DEVICE_CANCELLED, NULL, 0);
	Rdraw_frame();
	CHECK(g_answer.result == R_DEVICE_CANCELLED);
}

static void test_camera(Rid window) {
	RPickedFile photo;
	int i;
	(void)window;
	CHECK(Rcamera_is_available());

	/* never asked: the camera permission first, then the camera */
	reset();
	Rcamera_take_photo(on_device, NULL);
	CHECK(g_phone.permission_requests == 1);
	CHECK(g_phone.last_permission == R_PERMISSION_CAMERA);
	CHECK(g_phone.service_requests == 0);
	/* (the permission is a screen of the system: busy meanwhile) */
	Rfile_open_async(NULL, on_device, NULL);
	g_phone.status[0] = R_PERMISSION_GRANTED;
	Rhost_permission_answer(R_PERMISSION_CAMERA, R_PERMISSION_GRANTED);
	Rdraw_frame();
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_BUSY);
	CHECK(g_phone.service_requests == 1);
	CHECK(g_phone.last.service == R_HOST_SERVICE_TAKE_PHOTO);
	CHECK(write_file("raven_test_device_photo.jpg", "not really a jpeg"));
	memset(&photo, 0, sizeof(photo));
	photo.path = "raven_test_device_photo.jpg";
	photo.mime_type = "image/jpeg";
	photo.width = 4032;
	photo.height = 3024;
	Rhost_service_complete(g_phone.last.id, R_DEVICE_DONE, &photo, 1);
	for (i = 0; i < 3; i++) Rdraw_frame();
	CHECK(g_answer.device_answers == 2);
	CHECK(g_answer.result == R_DEVICE_DONE);
	CHECK(g_answer.width == 4032 && g_answer.height == 3024);
	CHECK(g_answer.sizes[0] == 17);
	remove("raven_test_device_photo.jpg");

	/* refused: no camera */
	reset();
	Rcamera_take_photo(on_device, NULL);
	Rhost_permission_answer(R_PERMISSION_CAMERA, R_PERMISSION_DENIED);
	for (i = 0; i < 3; i++) Rdraw_frame();
	CHECK(g_phone.service_requests == 0);
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_DENIED);

	/* refused for good: not even asked */
	reset();
	g_phone.status[0] = R_PERMISSION_BLOCKED;
	Rcamera_take_photo(on_device, NULL);
	CHECK(g_phone.permission_requests == 0);
	Rdraw_frame();
	CHECK(g_answer.result == R_DEVICE_DENIED);
}

static void test_share(Rid window) {
	RShareDesc share;
	Rid button = Rbutton_create("Share");
	float x, y, w, h;
	Rwidget_add_child(Rwindow_get_content(window), button);
	Rdraw_frame();
	Rwidget_get_rect(button, &x, &y, &w, &h);

	reset();
	memset(&share, 0, sizeof(share));
	share.text = "https://example.com/raven";
	share.anchor = button;
	Rshare_show(&share, on_device, NULL);
	CHECK(g_phone.service_requests == 1);
	CHECK(g_phone.last.service == R_HOST_SERVICE_SHARE);
	CHECK_STR(g_phone.last_text, "https://example.com/raven");
	CHECK(g_phone.last.window == window);
	/* (pixels: the window has a scale of 2) */
	CHECK_NEAR(g_phone.last.anchor_x, x * 2.0f);
	CHECK_NEAR(g_phone.last.anchor_width, w * 2.0f);
	CHECK(g_phone.last.anchor_width > 0.0f);
	Rhost_service_complete(g_phone.last.id, R_DEVICE_DONE, NULL, 0);
	Rdraw_frame();
	CHECK(g_answer.result == R_DEVICE_DONE);

	EXPECT_ERRORS_BEGIN();
	memset(&share, 0, sizeof(share));
	Rshare_show(&share, on_device, NULL);
	EXPECT_ERRORS_END(1);
	Rwidget_destroy(button);
}

/* ---------------------------------------------------------------------------
 *  Haptics, URLs, downloads, clipboard
 * ------------------------------------------------------------------------- */

static void test_haptics_and_urls(void) {
	CHECK(Rhaptic_is_supported());
	CHECK(Rhaptic_play(R_HAPTIC_SUCCESS));
	CHECK(g_phone.haptics == 1);
	CHECK(g_phone.last_haptic == R_HAPTIC_SUCCESS);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rhaptic_play((RHaptic)3));
	EXPECT_ERRORS_END(1);
	CHECK(g_phone.haptics == 1);

	CHECK(Ropen_url("https://example.com/"));
	CHECK(g_phone.urls == 1);
	CHECK_STR(g_phone.last_url, "https://example.com/");
	CHECK(!Ropen_url(""));
	CHECK(g_phone.urls == 1);
}

static void test_download(void) {
	Rid texture, missing;
	int width = 0, height = 0;
	g_phone.downloads = 0;
	texture = Rtexture_create_from_url_async("https://example.test/red.png");
	CHECK(texture != R_INVALID_ID);
	Rtexture_wait(texture);
	CHECK(g_phone.downloads == 1);
	CHECK(Rtexture_is_ready(texture));
#ifndef RAVEN_TEST_NO_IMAGE_DECODER
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 2 && height == 2);
#endif
	Rtexture_destroy(texture);

	/* the platform could not: reported, and the texture is ready and empty */
	EXPECT_ERRORS_BEGIN();
	missing = Rtexture_create_from_url_async("https://example.test/missing.png");
	Rtexture_wait(missing);
	Rdraw_frame();
	EXPECT_ERRORS_END(1);
	CHECK(g_phone.download_fails == 1);
	Rtexture_get_size(missing, &width, &height);
	CHECK(width == 0 && height == 0);
	Rtexture_destroy(missing);

	EXPECT_ERRORS_BEGIN();
	CHECK(Rtexture_create_from_url_async("ftp://example.test/red.png") == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
}

static void test_clipboard_has_text(void) {
	Rclipboard_set_text("");
	CHECK(!Rclipboard_has_text());
	Rclipboard_set_text("paste me");
	CHECK(Rclipboard_has_text());
}

/* Raven stops while a picker is open: the late answer is dropped, nothing leaks (test_report). */
static Ruint g_open_at_shutdown;
static void test_open_at_shutdown(void) {
	reset();
	Rfile_open_async(NULL, on_device, NULL);
	g_open_at_shutdown = g_phone.last.id;
	CHECK(g_open_at_shutdown != 0);
}

/* ---------------------------------------------------------------------------
 *  A desktop: the null platform, with the file dialogs of Raven
 * ------------------------------------------------------------------------- */

static Rid g_desktop_window;

static void choose_in_browser(Rid timer, void* user_data) {
	Rid browser = Rwindow_get_focused_widget(g_desktop_window);
	(void)timer;
	CHECK(Rwidget_get_type(browser) == R_WIDGET_FILE_BROWSER);
	Rfile_browser_select(browser, (const char*)user_data);
	test_key(g_desktop_window, R_KEY_ENTER, 0);
}

/* The Save button of the browser (its 6th part), with the name proposed by the application. */
static void accept_browser(Rid timer, void* user_data) {
	Rid browser = Rwindow_get_focused_widget(g_desktop_window);
	(void)timer;
	(void)user_data;
	CHECK(Rwidget_get_type(browser) == R_WIDGET_FILE_BROWSER);
	test_click(g_desktop_window, Rwidget_get_child(browser, 5));
}

static void cancel_browser(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	test_key(g_desktop_window, R_KEY_ESCAPE, 0);
}

static void test_desktop(void) {
	RFileDialogDesc desc;
	RShareDesc share;
	int i;

	/* nothing to ask for, no camera, no share sheet, no haptics */
	CHECK(Rpermission_get_status(R_PERMISSION_CAMERA) == R_PERMISSION_UNSUPPORTED);
	CHECK(Rpermission_get_status(R_PERMISSION_NOTIFICATIONS) == (Rnotification_is_supported() ? R_PERMISSION_GRANTED : R_PERMISSION_UNSUPPORTED));
	CHECK(!Rpermission_open_settings());
	memset(&g_answer, 0, sizeof(g_answer));
	Rpermission_request(R_PERMISSION_CAMERA, on_permission, (void*)(intptr_t)1);
	CHECK(g_answer.permission_answers == 0);
	Rdraw_frame();
	CHECK(g_answer.permission_answers == 1);
	CHECK(g_answer.status == R_PERMISSION_UNSUPPORTED);
	CHECK(!Rhaptic_is_supported());
	CHECK(!Rhaptic_play(R_HAPTIC_LIGHT));
	CHECK(!Rcamera_is_available());

	Rcamera_take_photo(on_device, NULL);
	memset(&share, 0, sizeof(share));
	share.text = "hello";
	Rshare_show(&share, on_device, NULL);
	CHECK(g_answer.device_answers == 0);
	Rdraw_frame();
	CHECK(g_answer.device_answers == 2);
	CHECK(g_answer.result == R_DEVICE_UNSUPPORTED);

	/* the asynchronous picker is the file dialog, from the next frame */
	g_desktop_window = Rwindow_create("desktop", 900, 600, 0);
	Rwindow_set_content(g_desktop_window, Rwidget_create(R_WIDGET_VBOX));
	Rwindow_show(g_desktop_window);
	CHECK(write_file("raven_test_device_pick.txt", "picked"));
	memset(&desc, 0, sizeof(desc));
	desc.parent = g_desktop_window;
	desc.directory = ".";
	desc.flags = R_FILE_DIALOG_RAVEN;
	memset(&g_answer, 0, sizeof(g_answer));
	Rtimer_create(0.01, R_FALSE, choose_in_browser, (void*)"raven_test_device_pick.txt");
	Rfile_open_async(&desc, on_device, NULL);
	CHECK(g_answer.device_answers == 0);
	for (i = 0; i < 5 && g_answer.device_answers == 0; i++) Rdraw_frame();
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_DONE);
	CHECK(g_answer.count == 1);
	CHECK_STR(g_answer.names[0], "raven_test_device_pick.txt");
	CHECK(g_answer.sizes[0] == 6);

	/* saving: written by Raven where the user chose */
	memset(&g_answer, 0, sizeof(g_answer));
	desc.file_name = "raven_test_device_saved.txt";
	Rtimer_create(0.01, R_FALSE, accept_browser, NULL);
	Rfile_save_async(&desc, "saved!", 6, on_device, NULL);
	for (i = 0; i < 5 && g_answer.device_answers == 0; i++) Rdraw_frame();
	CHECK(g_answer.result == R_DEVICE_DONE);
	{
		size_t size = 0;
		char* content = Rfile_read("raven_test_device_saved.txt", &size);
		CHECK(content && size == 6 && strcmp(content, "saved!") == 0);
		Rmemory_free(content);
	}

	/* cancelled */
	memset(&g_answer, 0, sizeof(g_answer));
	Rtimer_create(0.01, R_FALSE, cancel_browser, NULL);
	Rphoto_pick(1, on_device, NULL);
	for (i = 0; i < 5 && g_answer.device_answers == 0; i++) Rdraw_frame();
	CHECK(g_answer.device_answers == 1);
	CHECK(g_answer.result == R_DEVICE_CANCELLED);

	remove("raven_test_device_pick.txt");
	remove("raven_test_device_saved.txt");
	Rwindow_destroy(g_desktop_window);
}

int main(void) {
	RHostPlatformDesc host;
	Rid window;

	memset(&host, 0, sizeof(host));
	host.permission_status = phone_permission_status;
	host.permission_request = phone_permission_request;
	host.service_request = phone_service_request;
	host.has_camera = R_TRUE;
	host.open_url = phone_open_url;
	host.haptic = phone_haptic;
	host.download = phone_download;
	Rhost_platform_register(&host);
	if (!test_init(4)) {
		printf("Raven did not start with the host platform\n");
		return 1;
	}
	window = Rwindow_create("phone", 390, 844, 0);
	Rwindow_set_content(window, Rwidget_create(R_WIDGET_VBOX));
	Rwindow_show(window);
	Rhost_window_set_size(window, 780, 1688, 2.0f);
	Rdraw_frame();

	RUN(test_permission_request);
	RUN(test_permission_answer_from_another_thread);
	RUN(test_file_open);
	RUN(test_one_sheet_at_a_time);
	RUN(test_save);
	test_camera(window);
	printf("test_camera\n");
	test_share(window);
	printf("test_share\n");
	RUN(test_haptics_and_urls);
	RUN(test_download);
	RUN(test_clipboard_has_text);
	RUN(test_open_at_shutdown);
	Rshutdown();
	/* the system answers after Raven stopped: ignored */
	Rhost_service_complete(g_open_at_shutdown, R_DEVICE_CANCELLED, NULL, 0);
	Rhost_permission_answer(R_PERMISSION_CAMERA, R_PERMISSION_GRANTED);
	CHECK(g_answer.device_answers == 0);

	Rhost_platform_register(NULL);
	if (!test_init(2)) {
		printf("Raven did not start headless\n");
		return 1;
	}
	RUN(test_desktop);
	Rshutdown();
	return test_report("test_device");
}
