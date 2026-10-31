/**
 * Raven - core/fs_watch.c
 * Native file-change notifications for Windows, macOS and Linux.
 *
 * The OS monitors containing directories rather than repeatedly calling stat
 * on every file. A directory event is narrowed back to the paths Raven has
 * registered; consumers compare stamps only after a notification arrives.
 */
#include "fs_watch.h"
#include "fs.h"
#include "thread.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#elif defined(__linux__)
#include <sys/inotify.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

typedef struct rvn_fs_watch_path_t {
    char* path;
    char* directory;
    struct rvn_fs_watch_path_t* next;
} rvn_fs_watch_path_t;

typedef struct rvn_fs_watch_dir_t {
    /* path is the directory whose registered files must be checked. On macOS,
       watch_path is either that directory or one watched file within it. */
    char* path;
#if defined(__APPLE__)
    char* watch_path;
    int is_file_watch;
#endif
#if defined(_WIN32)
    HANDLE handle;
    HANDLE event;
    HANDLE stop_event;
    OVERLAPPED overlapped;
    uint8_t buffer[8192];
    rvn_thread_t thread;
    int thread_started;
#elif defined(__APPLE__)
    int fd;
    struct kevent event;
#elif defined(__linux__)
    int descriptor;
#endif
    struct rvn_fs_watch_dir_t* next;
} rvn_fs_watch_dir_t;

static struct {
    rvn_mutex_t mutex;
    int mutex_ready;
    int started;
    rvn_atomic_t stopping; /* (read by the watching threads) */
    rvn_fs_watch_callback_t callback;
    void* user;
    rvn_fs_watch_path_t* paths;
    rvn_fs_watch_dir_t* directories;
#if defined(__APPLE__)
    int queue_fd;
    rvn_thread_t thread;
#elif defined(__linux__)
    int inotify_fd;
    rvn_thread_t thread;
#endif
} g_fs_watch;

#if defined(__APPLE__)
static void rvn_fs_watch_mac_rearm_files(const char* directory) {
    rvn_fs_watch_dir_t* item;
    rvn_mutex_lock(&g_fs_watch.mutex);
    for (item = g_fs_watch.directories; item; item = item->next) {
        int fd;
        struct kevent change;
        if (!item->is_file_watch || item->fd >= 0 || strcmp(item->path, directory) != 0) continue;
        fd = open(item->watch_path, O_EVTONLY);
        if (fd < 0) continue;
        EV_SET(&change, (uintptr_t)fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
            NOTE_WRITE | NOTE_DELETE | NOTE_RENAME | NOTE_ATTRIB | NOTE_EXTEND, 0, item);
        if (kevent(g_fs_watch.queue_fd, &change, 1, NULL, 0, NULL) == 0) item->fd = fd;
        else close(fd);
    }
    rvn_mutex_unlock(&g_fs_watch.mutex);
}
#endif

static void rvn_fs_watch_notify_directory(const char* directory) {
    rvn_fs_watch_path_t* item;
    char** paths = NULL;
    uint32_t count = 0, cap = 0, i;
    rvn_fs_watch_callback_t callback;
    void* user;

    if (!g_fs_watch.started || rvn_atomic_load(&g_fs_watch.stopping)) return;
    rvn_mutex_lock(&g_fs_watch.mutex);
    callback = g_fs_watch.callback;
    user = g_fs_watch.user;
    for (item = g_fs_watch.paths; item; item = item->next) {
        if (strcmp(item->directory, directory) != 0) continue;
        RVN_ARRAY_RESERVE(paths, cap, count + 1);
        paths[count++] = rvn_strdup(item->path);
    }
    rvn_mutex_unlock(&g_fs_watch.mutex);

    for (i = 0; i < count; i++) {
        if (callback) callback(paths[i], user);
        rvn_free(paths[i]);
    }
    rvn_free(paths);
}

#if defined(_WIN32)
static WCHAR* rvn_fs_watch_wide(const char* text) {
    int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    WCHAR* result;
    if (n <= 0) return NULL;
    result = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)n);
    if (MultiByteToWideChar(CP_UTF8, 0, text, -1, result, n) <= 0) {
        rvn_free(result);
        return NULL;
    }
    return result;
}

static void rvn_fs_watch_directory_thread(void* arg) {
    rvn_fs_watch_dir_t* dir = (rvn_fs_watch_dir_t*)arg;
    for (;;) {
        DWORD ignored = 0;
        BOOL ok;
        HANDLE waits[2];
        if (rvn_atomic_load(&g_fs_watch.stopping)) break;
        memset(&dir->overlapped, 0, sizeof(dir->overlapped));
        dir->overlapped.hEvent = dir->event;
        ResetEvent(dir->event);
        ok = ReadDirectoryChangesW(dir->handle, dir->buffer, (DWORD)sizeof(dir->buffer), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES, &ignored,
            &dir->overlapped, NULL);
        if (!ok && GetLastError() != ERROR_IO_PENDING) break;
        waits[0] = dir->stop_event;
        waits[1] = dir->event;
        if (WaitForMultipleObjects(2, waits, FALSE, INFINITE) == WAIT_OBJECT_0) {
            CancelIoEx(dir->handle, &dir->overlapped);
            break;
        }
        if (GetOverlappedResult(dir->handle, &dir->overlapped, &ignored, FALSE)) {
            /* We watch a directory, then validate the specific file's stamp on the UI thread. */
            rvn_fs_watch_notify_directory(dir->path);
        }
    }
}

#elif defined(__APPLE__)
static void rvn_fs_watch_thread(void* unused) {
    RVN_UNUSED(unused);
    while (!rvn_atomic_load(&g_fs_watch.stopping)) {
        struct kevent events[16];
        struct timespec timeout;
        int n, i;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 250000000L;
        n = kevent(g_fs_watch.queue_fd, NULL, 0, events, (int)RVN_COUNT_OF(events), &timeout);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (i = 0; i < n && !rvn_atomic_load(&g_fs_watch.stopping); i++) {
            rvn_fs_watch_dir_t* dir = (rvn_fs_watch_dir_t*)events[i].udata;
            if (!dir || !(events[i].fflags & (NOTE_WRITE | NOTE_DELETE | NOTE_RENAME | NOTE_ATTRIB | NOTE_EXTEND))) continue;
            if (dir->is_file_watch && (events[i].fflags & (NOTE_DELETE | NOTE_RENAME))) {
                /* Drop the old inode watch; the parent directory event will
                   re-arm it once the replacement file becomes visible. */
                if (dir->fd >= 0) close(dir->fd);
                dir->fd = -1;
            }
            rvn_fs_watch_notify_directory(dir->path);
            if (!dir->is_file_watch) rvn_fs_watch_mac_rearm_files(dir->path);
        }
    }
}

#elif defined(__linux__)
static void rvn_fs_watch_thread(void* unused) {
    uint8_t buffer[16384];
    RVN_UNUSED(unused);
    while (!rvn_atomic_load(&g_fs_watch.stopping)) {
        struct pollfd pfd;
        int ready;
        pfd.fd = g_fs_watch.inotify_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        ready = poll(&pfd, 1, 250);
        if (ready < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ready == 0 || !(pfd.revents & POLLIN)) continue;
        for (;;) {
            ssize_t bytes = read(g_fs_watch.inotify_fd, buffer, sizeof(buffer));
            size_t offset = 0;
            if (bytes < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (bytes == 0) break;
            while (offset < (size_t)bytes) {
                const struct inotify_event* event = (const struct inotify_event*)(buffer + offset);
                rvn_fs_watch_dir_t* dir;
                char* directory = NULL;
                rvn_mutex_lock(&g_fs_watch.mutex);
                for (dir = g_fs_watch.directories; dir; dir = dir->next) {
                    if (dir->descriptor == event->wd) {
                        directory = rvn_strdup(dir->path);
                        break;
                    }
                }
                rvn_mutex_unlock(&g_fs_watch.mutex);
                if (directory) {
                    rvn_fs_watch_notify_directory(directory);
                    rvn_free(directory);
                }
                offset += sizeof(struct inotify_event) + event->len;
            }
            break;
        }
    }
}
#else
static void rvn_fs_watch_thread(void* unused) { RVN_UNUSED(unused); }
#endif

int rvn_fs_watch_start(rvn_fs_watch_callback_t callback, void* user) {
    if (!callback) return R_FALSE;
    if (g_fs_watch.started) {
        g_fs_watch.callback = callback;
        g_fs_watch.user = user;
        return R_TRUE;
    }
    memset(&g_fs_watch, 0, sizeof(g_fs_watch));
    rvn_mutex_init(&g_fs_watch.mutex);
    g_fs_watch.mutex_ready = R_TRUE;
    g_fs_watch.callback = callback;
    g_fs_watch.user = user;
#if defined(__linux__)
    g_fs_watch.inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    if (g_fs_watch.inotify_fd < 0) {
        rvn_mutex_destroy(&g_fs_watch.mutex);
        memset(&g_fs_watch, 0, sizeof(g_fs_watch));
        return R_FALSE;
    }
    if (!rvn_thread_create(&g_fs_watch.thread, rvn_fs_watch_thread, NULL, "raven-fs-watch")) {
        close(g_fs_watch.inotify_fd);
        rvn_mutex_destroy(&g_fs_watch.mutex);
        memset(&g_fs_watch, 0, sizeof(g_fs_watch));
        return R_FALSE;
    }
#elif defined(__APPLE__)
    g_fs_watch.queue_fd = kqueue();
    if (g_fs_watch.queue_fd < 0 || !rvn_thread_create(&g_fs_watch.thread, rvn_fs_watch_thread, NULL, "raven-fs-watch")) {
        if (g_fs_watch.queue_fd >= 0) close(g_fs_watch.queue_fd);
        rvn_mutex_destroy(&g_fs_watch.mutex);
        memset(&g_fs_watch, 0, sizeof(g_fs_watch));
        return R_FALSE;
    }
#endif
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__linux__)
    /* Not every POSIX OS has inotify or Apple's vnode extensions. Tell the
       caller to use its compatibility fallback instead of silently missing changes. */
    rvn_mutex_destroy(&g_fs_watch.mutex);
    memset(&g_fs_watch, 0, sizeof(g_fs_watch));
    return R_FALSE;
#endif
    g_fs_watch.started = R_TRUE;
    return R_TRUE;
}

int rvn_fs_watch_add(const char* path) {
    char* absolute;
    char* directory;
    rvn_fs_watch_path_t* item;
    rvn_fs_watch_dir_t* dir;
    if (!path || !path[0] || !g_fs_watch.started || rvn_atomic_load(&g_fs_watch.stopping)) return R_FALSE;
    absolute = rvn_path_absolute(path);
    if (!absolute) return R_FALSE;
    directory = rvn_path_parent(absolute);
    if (!directory) directory = rvn_strdup(absolute);

    rvn_mutex_lock(&g_fs_watch.mutex);
    for (item = g_fs_watch.paths; item; item = item->next) {
        if (strcmp(item->path, absolute) == 0) {
            rvn_mutex_unlock(&g_fs_watch.mutex);
            rvn_free(directory);
            rvn_free(absolute);
            return R_TRUE;
        }
    }
    for (dir = g_fs_watch.directories; dir; dir = dir->next) {
#if defined(__APPLE__)
        if (dir->is_file_watch) continue;
#endif
        if (strcmp(dir->path, directory) == 0) break;
    }
    if (!dir) {
        dir = (rvn_fs_watch_dir_t*)rvn_calloc(sizeof(*dir));
        dir->path = rvn_strdup(directory);
#if defined(__APPLE__)
        dir->watch_path = rvn_strdup(directory);
#endif
#if defined(_WIN32)
        {
            WCHAR* wide = rvn_fs_watch_wide(directory);
            if (wide) {
                dir->handle = CreateFileW(wide, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
                rvn_free(wide);
            } else dir->handle = INVALID_HANDLE_VALUE;
            dir->event = CreateEventW(NULL, TRUE, FALSE, NULL);
            dir->stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
            if (dir->handle == INVALID_HANDLE_VALUE || !dir->event || !dir->stop_event) {
                if (dir->handle != INVALID_HANDLE_VALUE && dir->handle) CloseHandle(dir->handle);
                if (dir->event) CloseHandle(dir->event);
                if (dir->stop_event) CloseHandle(dir->stop_event);
                rvn_free(dir->path); rvn_free(dir);
                rvn_mutex_unlock(&g_fs_watch.mutex);
                rvn_free(directory); rvn_free(absolute);
                return R_FALSE;
            }
        }
#elif defined(__APPLE__)
        dir->fd = open(directory, O_EVTONLY);
        if (dir->fd < 0) {
            rvn_free(dir->watch_path); rvn_free(dir->path); rvn_free(dir);
            rvn_mutex_unlock(&g_fs_watch.mutex);
            rvn_free(directory); rvn_free(absolute);
            return R_FALSE;
        }
        {
            struct kevent change;
            EV_SET(&change, (uintptr_t)dir->fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
                NOTE_WRITE | NOTE_DELETE | NOTE_RENAME | NOTE_ATTRIB | NOTE_EXTEND, 0, dir);
            if (kevent(g_fs_watch.queue_fd, &change, 1, NULL, 0, NULL) != 0) {
                close(dir->fd);
                rvn_free(dir->watch_path); rvn_free(dir->path); rvn_free(dir);
                rvn_mutex_unlock(&g_fs_watch.mutex);
                rvn_free(directory); rvn_free(absolute);
                return R_FALSE;
            }
        }
#elif defined(__linux__)
        dir->descriptor = inotify_add_watch(g_fs_watch.inotify_fd, directory,
            IN_CLOSE_WRITE | IN_MODIFY | IN_ATTRIB | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO |
            IN_DELETE_SELF | IN_MOVE_SELF);
        if (dir->descriptor < 0) {
            rvn_free(dir->path); rvn_free(dir);
            rvn_mutex_unlock(&g_fs_watch.mutex);
            rvn_free(directory); rvn_free(absolute);
            return R_FALSE;
        }
#endif
        dir->next = g_fs_watch.directories;
        g_fs_watch.directories = dir;
#if defined(_WIN32)
        if (!rvn_thread_create(&dir->thread, rvn_fs_watch_directory_thread, dir, "raven-fs-watch")) {
            g_fs_watch.directories = dir->next;
            CloseHandle(dir->handle); CloseHandle(dir->event); CloseHandle(dir->stop_event);
            rvn_free(dir->path); rvn_free(dir);
            rvn_mutex_unlock(&g_fs_watch.mutex);
            rvn_free(directory); rvn_free(absolute);
            return R_FALSE;
        }
        dir->thread_started = R_TRUE;
#endif
    }
#if defined(__APPLE__)
    /* Watch the file vnode too: writes to file contents need not change the
       containing directory. Keep the directory vnode to catch atomic replaces. */
    {
        int found_file = R_FALSE;
        for (dir = g_fs_watch.directories; dir; dir = dir->next) {
            if (dir->is_file_watch && strcmp(dir->watch_path, absolute) == 0) { found_file = R_TRUE; break; }
        }
        if (!found_file) {
            rvn_fs_watch_dir_t* file_watch = (rvn_fs_watch_dir_t*)rvn_calloc(sizeof(*file_watch));
            file_watch->path = rvn_strdup(directory);
            file_watch->watch_path = rvn_strdup(absolute);
            file_watch->is_file_watch = R_TRUE;
            file_watch->fd = -1;
            {
                int file_fd = open(absolute, O_EVTONLY);
                if (file_fd >= 0) {
                    struct kevent change;
                    EV_SET(&change, (uintptr_t)file_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
                        NOTE_WRITE | NOTE_DELETE | NOTE_RENAME | NOTE_ATTRIB | NOTE_EXTEND, 0, file_watch);
                    if (kevent(g_fs_watch.queue_fd, &change, 1, NULL, 0, NULL) == 0) file_watch->fd = file_fd;
                    else close(file_fd);
                }
                /* Keep even an unarmed vnode entry. The parent directory watch
                   can re-arm it when a missing file is created or atomically replaced. */
                file_watch->next = g_fs_watch.directories;
                g_fs_watch.directories = file_watch;
            }
        }
    }
#endif
    item = (rvn_fs_watch_path_t*)rvn_calloc(sizeof(*item));
    item->path = absolute;
    item->directory = directory;
    item->next = g_fs_watch.paths;
    g_fs_watch.paths = item;
    rvn_mutex_unlock(&g_fs_watch.mutex);
    return R_TRUE;
}

void rvn_fs_watch_stop(void) {
    rvn_fs_watch_path_t* path;
    rvn_fs_watch_dir_t* dir;
    if (!g_fs_watch.started && !g_fs_watch.mutex_ready) return;
    rvn_atomic_store(&g_fs_watch.stopping, R_TRUE);
#if defined(_WIN32)
    for (dir = g_fs_watch.directories; dir; dir = dir->next) SetEvent(dir->stop_event);
    for (dir = g_fs_watch.directories; dir; dir = dir->next) {
        if (dir->thread_started) rvn_thread_join(&dir->thread);
    }
#else
#if defined(__APPLE__)
    if (g_fs_watch.queue_fd >= 0) {
        struct kevent wake_event;
        EV_SET(&wake_event, 1, EVFILT_USER, EV_ADD | EV_ENABLE, 0, 0, NULL);
        kevent(g_fs_watch.queue_fd, &wake_event, 1, NULL, 0, NULL);
        EV_SET(&wake_event, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
        kevent(g_fs_watch.queue_fd, &wake_event, 1, NULL, 0, NULL);
    }
    if (g_fs_watch.started) rvn_thread_join(&g_fs_watch.thread);
#elif defined(__linux__)
    /* (the thread sees the flag within a quarter of a second: its descriptor is closed once it is gone) */
    if (g_fs_watch.started) rvn_thread_join(&g_fs_watch.thread);
    if (g_fs_watch.inotify_fd >= 0) close(g_fs_watch.inotify_fd);
#endif
#endif
    path = g_fs_watch.paths;
    while (path) {
        rvn_fs_watch_path_t* next = path->next;
        rvn_free(path->path);
        rvn_free(path->directory);
        rvn_free(path);
        path = next;
    }
    dir = g_fs_watch.directories;
    while (dir) {
        rvn_fs_watch_dir_t* next = dir->next;
#if defined(_WIN32)
        if (dir->handle && dir->handle != INVALID_HANDLE_VALUE) CloseHandle(dir->handle);
        if (dir->event) CloseHandle(dir->event);
        if (dir->stop_event) CloseHandle(dir->stop_event);
#elif defined(__APPLE__)
        if (dir->fd >= 0) close(dir->fd);
        rvn_free(dir->watch_path);
#elif defined(__linux__)
        /* All inotify descriptors belong to the shared fd, closed above. */
        RVN_UNUSED(dir->descriptor);
#endif
        rvn_free(dir->path);
        rvn_free(dir);
        dir = next;
    }
#if defined(__APPLE__)
    if (g_fs_watch.queue_fd >= 0) close(g_fs_watch.queue_fd);
#elif defined(__linux__)
    /* fd closed above */
#endif
    if (g_fs_watch.mutex_ready) rvn_mutex_destroy(&g_fs_watch.mutex);
    memset(&g_fs_watch, 0, sizeof(g_fs_watch));
}
