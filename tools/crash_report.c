/**
 * Raven tools - crash_report.c
 * See crash_report.h. On Windows: an unhandled exception filter (and abort,
 * and the invalid parameters of the C library), a minidump and a walk of the
 * stack with DbgHelp, loaded when the crash comes (nothing to link). Elsewhere:
 * the signals of a crash on a stack of their own, and backtrace() where the C
 * library has it.
 *
 * What runs during a crash avoids the heap as much as it can: the report is
 * built in static memory and written with the calls of the system.
 */

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "crash_report.h"

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NOTE_COUNT 24
#define NOTE_SIZE 160

static char g_folder[1024];
static char g_name[64];
static char g_report[1100];
static char g_flag[1100];
static char g_previous[1100];
static int g_has_previous;
static char g_notes[NOTE_COUNT][NOTE_SIZE];
static unsigned g_note_next;
static volatile long g_crashing;

/* the report being written, in static memory */
static char g_text[48 * 1024];
static size_t g_length;

static void add(const char* text) {
	size_t n = strlen(text);
	if (g_length + n >= sizeof(g_text)) n = sizeof(g_text) - 1 - g_length;
	memcpy(g_text + g_length, text, n);
	g_length += n;
	g_text[g_length] = '\0';
}

static void addf(const char* format, ...) {
	char line[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	add(line);
}

void crash_report_note(const char* format, ...) {
	va_list args;
	char* slot = g_notes[g_note_next % NOTE_COUNT];
	va_start(args, format);
	vsnprintf(slot, NOTE_SIZE, format, args);
	va_end(args);
	g_note_next++;
}

static void add_header(const char* what) {
	char when[64];
	time_t now = time(NULL);
	struct tm* local = localtime(&now);
	if (!local || !strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", local)) strcpy(when, "?");
	g_length = 0;
	g_text[0] = '\0';
	addf("%s crashed, %s\n", g_name, when);
	addf("What: %s\n", what);
}

static void add_notes(void) {
	unsigned i, first = g_note_next > NOTE_COUNT ? g_note_next - NOTE_COUNT : 0;
	add("\nThe last things it did (oldest first):\n");
	if (g_note_next == 0) add("  (nothing noted)\n");
	for (i = first; i < g_note_next; i++) addf("  %s\n", g_notes[i % NOTE_COUNT]);
}

const char* crash_report_previous(void) {
	if (!g_has_previous) return NULL;
	g_has_previous = 0;
	return g_previous;
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* the types of DbgHelp from its header; its functions are loaded when the crash comes (nothing to link) */
#include <dbghelp.h>
#include <process.h>
#include <stdint.h>
#include <wchar.h>

typedef BOOL(WINAPI* sym_initialize_fn)(HANDLE, PCSTR, BOOL);
typedef DWORD(WINAPI* sym_set_options_fn)(DWORD);
typedef BOOL(WINAPI* sym_from_addr_fn)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
typedef BOOL(WINAPI* sym_get_line_fn)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
typedef BOOL(WINAPI* stack_walk_fn)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
	PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
typedef BOOL(WINAPI* write_dump_fn)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
	PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

static char g_dump[1100];

static const char* exception_name(DWORD code) {
	switch (code) {
		case EXCEPTION_ACCESS_VIOLATION: return "access violation (a pointer to memory that is not there, or freed)";
		case EXCEPTION_STACK_OVERFLOW: return "stack overflow (a recursion without end?)";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "misaligned data";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "float divided by zero";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divided by zero";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
		case EXCEPTION_IN_PAGE_ERROR: return "in page error";
		case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
		case 0xC0000409: return "stack buffer overrun, or a fast fail of the C library";
		case 0xC0000374: return "heap corruption (memory written after its end, or freed twice)";
		case 0xE06D7363: return "C++ exception";
		default: return "exception";
	}
}

static void module_of(DWORD64 address, char* out, size_t size, DWORD64* offset) {
	HMODULE module = NULL;
	char path[MAX_PATH];
	const char* base;
	*offset = 0;
	out[0] = '\0';
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(ULONG_PTR)address, &module) || !module) {
		snprintf(out, size, "?");
		return;
	}
	if (!GetModuleFileNameA(module, path, sizeof(path))) path[0] = '\0';
	base = strrchr(path, '\\');
	snprintf(out, size, "%s", base ? base + 1 : path);
	*offset = address - (DWORD64)(ULONG_PTR)module;
}

static void write_file(const char* path, const char* data, size_t length) {
	HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD written = 0;
	if (file == INVALID_HANDLE_VALUE) return;
	WriteFile(file, data, (DWORD)length, &written, NULL);
	CloseHandle(file);
}

static void report(const char* what, EXCEPTION_POINTERS* pointers, CONTEXT* context_in) {
	HMODULE help = LoadLibraryA("dbghelp.dll");
	HANDLE process = GetCurrentProcess(), thread = GetCurrentThread();
	CONTEXT context;
	STACKFRAME64 frame;
	DWORD machine;
	int i, symbols = 0;
	static ULONG64 symbol_space[(sizeof(SYMBOL_INFO) + 512 + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
	PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_space;
	sym_initialize_fn sym_initialize = help ? (sym_initialize_fn)(void (*)(void))GetProcAddress(help, "SymInitialize") : NULL;
	sym_set_options_fn sym_set_options = help ? (sym_set_options_fn)(void (*)(void))GetProcAddress(help, "SymSetOptions") : NULL;
	sym_from_addr_fn sym_from_addr = help ? (sym_from_addr_fn)(void (*)(void))GetProcAddress(help, "SymFromAddr") : NULL;
	sym_get_line_fn sym_get_line = help ? (sym_get_line_fn)(void (*)(void))GetProcAddress(help, "SymGetLineFromAddr64") : NULL;
	PFUNCTION_TABLE_ACCESS_ROUTINE64 sym_function_table = help ? (PFUNCTION_TABLE_ACCESS_ROUTINE64)(void (*)(void))GetProcAddress(help, "SymFunctionTableAccess64") : NULL;
	PGET_MODULE_BASE_ROUTINE64 sym_module_base = help ? (PGET_MODULE_BASE_ROUTINE64)(void (*)(void))GetProcAddress(help, "SymGetModuleBase64") : NULL;
	stack_walk_fn stack_walk = help ? (stack_walk_fn)(void (*)(void))GetProcAddress(help, "StackWalk64") : NULL;
	write_dump_fn write_dump = help ? (write_dump_fn)(void (*)(void))GetProcAddress(help, "MiniDumpWriteDump") : NULL;

	add_header(what);
	if (pointers && pointers->ExceptionRecord) {
		EXCEPTION_RECORD* record = pointers->ExceptionRecord;
		char module[MAX_PATH];
		DWORD64 offset;
		module_of((DWORD64)(ULONG_PTR)record->ExceptionAddress, module, sizeof(module), &offset);
		addf("Code: 0x%08lX, %s\n", (unsigned long)record->ExceptionCode, exception_name(record->ExceptionCode));
		addf("At: %s+0x%llX\n", module, (unsigned long long)offset);
		if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
			addf("While %s the address 0x%llX\n", record->ExceptionInformation[0] == 0 ? "reading" : record->ExceptionInformation[0] == 1 ? "writing" : "running",
				(unsigned long long)record->ExceptionInformation[1]);
	}
	addf("Thread: %lu\n", (unsigned long)GetCurrentThreadId());

	/* the minidump first: it does not depend on the symbols */
	if (write_dump) {
		HANDLE file = CreateFileA(g_dump, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE) {
			MINIDUMP_EXCEPTION_INFORMATION info;
			info.ThreadId = GetCurrentThreadId();
			info.ExceptionPointers = pointers;
			info.ClientPointers = FALSE;
			if (write_dump(process, GetCurrentProcessId(), file,
				(MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs | MiniDumpWithThreadInfo), pointers ? &info : NULL, NULL, NULL))
				addf("Minidump: %s (open it in Visual Studio, next to the .pdb of the build)\n", g_dump);
			CloseHandle(file);
		}
	}

	add("\nThe stack (the crash first):\n");
	if (pointers && pointers->ContextRecord) context = *pointers->ContextRecord;
	else if (context_in) context = *context_in;
	else RtlCaptureContext(&context);
	memset(&frame, 0, sizeof(frame));
#if defined(_M_X64) || defined(__x86_64__)
	machine = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = context.Rip;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrStack.Offset = context.Rsp;
#elif defined(_M_ARM64) || defined(__aarch64__)
	machine = IMAGE_FILE_MACHINE_ARM64;
	frame.AddrPC.Offset = context.Pc;
	frame.AddrFrame.Offset = context.Fp;
	frame.AddrStack.Offset = context.Sp;
#else
	machine = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset = context.Eip;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrStack.Offset = context.Esp;
#endif
	frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
	if (sym_set_options) sym_set_options(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);
	if (sym_initialize) {
		/* the symbols next to the program (the .pdb of the build) */
		char search[MAX_PATH], *slash;
		GetModuleFileNameA(NULL, search, sizeof(search));
		slash = strrchr(search, '\\');
		if (slash) *slash = '\0';
		symbols = sym_initialize(process, search, TRUE);
	}
	for (i = 0; i < 64; i++) {
		char module[MAX_PATH];
		DWORD64 address, offset, displacement = 0;
		DWORD line_displacement = 0;
		IMAGEHLP_LINE64 line;
		if (!stack_walk || !stack_walk(machine, process, thread, &frame, &context, NULL, sym_function_table, sym_module_base, NULL)) break;
		address = frame.AddrPC.Offset;
		if (!address) break;
		module_of(address, module, sizeof(module), &offset);
		memset(symbol_space, 0, sizeof(symbol_space));
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = 512;
		memset(&line, 0, sizeof(line));
		line.SizeOfStruct = sizeof(line);
		if (symbols && sym_from_addr && sym_from_addr(process, address, &displacement, symbol)) {
			if (sym_get_line && sym_get_line(process, address, &line_displacement, &line) && line.FileName)
				addf("  %2d  %s  (%s:%lu)  [%s+0x%llX]\n", i, symbol->Name, line.FileName, (unsigned long)line.LineNumber, module, (unsigned long long)offset);
			else
				addf("  %2d  %s+0x%llX  [%s+0x%llX]\n", i, symbol->Name, (unsigned long long)displacement, module, (unsigned long long)offset);
		} else {
			addf("  %2d  %s+0x%llX\n", i, module, (unsigned long long)offset);
		}
	}
	if (i == 0) add("  (DbgHelp could not walk the stack: the minidump has it)\n");
	add_notes();
	write_file(g_report, g_text, g_length);
	write_file(g_flag, g_report, strlen(g_report));
}

static LONG WINAPI on_exception(EXCEPTION_POINTERS* pointers) {
	if (InterlockedExchange(&g_crashing, 1) == 0) report("an unhandled exception", pointers, NULL);
	return EXCEPTION_CONTINUE_SEARCH;
}

static void on_abort(int signal_number) {
	(void)signal_number;
	if (InterlockedExchange(&g_crashing, 1) == 0) report("abort() (an assertion that failed, or the C library gave up)", NULL, NULL);
	_exit(3);
}

static void on_purecall(void) {
	if (InterlockedExchange(&g_crashing, 1) == 0) report("a pure virtual call", NULL, NULL);
	_exit(3);
}

static void on_invalid_parameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved) {
	(void)expression; (void)function; (void)file; (void)line; (void)reserved;
	if (InterlockedExchange(&g_crashing, 1) == 0) report("an invalid parameter given to the C library (a NULL FILE*, a bad format...)", NULL, NULL);
	_exit(3);
}

static void make_folder(const char* path) {
	char copy[1024];
	size_t i;
	snprintf(copy, sizeof(copy), "%s", path);
	for (i = 1; copy[i]; i++) {
		if (copy[i] == '/' || copy[i] == '\\') {
			char c = copy[i];
			copy[i] = '\0';
			CreateDirectoryA(copy, NULL);
			copy[i] = c;
		}
	}
	CreateDirectoryA(copy, NULL);
}

static int read_small(const char* path, char* out, size_t size) {
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD got = 0;
	if (file == INVALID_HANDLE_VALUE) return 0;
	ReadFile(file, out, (DWORD)(size - 1), &got, NULL);
	CloseHandle(file);
	out[got] = '\0';
	return got > 0;
}

void crash_report_install(const char* folder, const char* name) {
	ULONG guarantee = 64 * 1024;
	snprintf(g_folder, sizeof(g_folder), "%s", folder && folder[0] ? folder : ".");
	snprintf(g_name, sizeof(g_name), "%s", name ? name : "raven");
	make_folder(g_folder);
	snprintf(g_report, sizeof(g_report), "%s/%s-crash.txt", g_folder, g_name);
	snprintf(g_dump, sizeof(g_dump), "%s/%s-crash.dmp", g_folder, g_name);
	snprintf(g_flag, sizeof(g_flag), "%s/%s-crashed", g_folder, g_name);
	for (char* c = g_report; *c; c++) if (*c == '/') *c = '\\';
	for (char* c = g_dump; *c; c++) if (*c == '/') *c = '\\';
	if (read_small(g_flag, g_previous, sizeof(g_previous))) {
		g_has_previous = 1;
		DeleteFileA(g_flag);
	}
	/* room to write the report when the stack itself overflowed */
	SetThreadStackGuarantee(&guarantee);
	SetUnhandledExceptionFilter(on_exception);
	signal(SIGABRT, on_abort);
#if defined(_MSC_VER)
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	_set_purecall_handler(on_purecall);
	_set_invalid_parameter_handler(on_invalid_parameter);
#else
	(void)on_purecall;
	(void)on_invalid_parameter;
#endif
}

/* ---------------------------------------------------------------------------
 *  Linux, macOS and the others
 * ------------------------------------------------------------------------- */

#else

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__GLIBC__) || defined(__APPLE__)
#include <execinfo.h>
#define HAVE_BACKTRACE 1
#endif

static const int g_signals[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT };

static const char* signal_name(int number) {
	switch (number) {
		case SIGSEGV: return "SIGSEGV (a pointer to memory that is not there, or freed)";
		case SIGBUS: return "SIGBUS (misaligned or missing memory)";
		case SIGFPE: return "SIGFPE (a division by zero)";
		case SIGILL: return "SIGILL (illegal instruction)";
		case SIGABRT: return "SIGABRT (abort: an assertion that failed, or the C library gave up)";
		default: return "a signal";
	}
}

static void write_all(int fd, const char* data, size_t length) {
	while (length > 0) {
		ssize_t n = write(fd, data, length);
		if (n <= 0) {
			if (n < 0 && errno == EINTR) continue;
			return;
		}
		data += n;
		length -= (size_t)n;
	}
}

static void on_signal(int number, siginfo_t* info, void* context) {
	int fd;
	(void)context;
	if (__sync_lock_test_and_set(&g_crashing, 1) == 0) {
		add_header(signal_name(number));
		if (info && (number == SIGSEGV || number == SIGBUS)) addf("Address: %p\n", info->si_addr);
		add_notes();
		add("\nThe stack (the crash first; addr2line -f -e the program, the addresses):\n");
		fd = open(g_report, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd >= 0) {
			write_all(fd, g_text, g_length);
#ifdef HAVE_BACKTRACE
			{
				void* frames[64];
				int count = backtrace(frames, 64);
				backtrace_symbols_fd(frames, count, fd);
			}
#else
			write_all(fd, "  (no backtrace on this system)\n", 32);
#endif
			close(fd);
		}
		fd = open(g_flag, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd >= 0) {
			write_all(fd, g_report, strlen(g_report));
			close(fd);
		}
		write_all(2, "\nA crash report is in ", 22);
		write_all(2, g_report, strlen(g_report));
		write_all(2, "\n", 1);
	}
	/* the handler is gone (SA_RESETHAND): the signal again, as it would have been */
	raise(number);
}

void crash_report_install(const char* folder, const char* name) {
	static char alternate[64 * 1024];
	stack_t stack;
	struct sigaction action;
	char copy[1024];
	size_t i;
	int fd;
	snprintf(g_folder, sizeof(g_folder), "%s", folder && folder[0] ? folder : ".");
	snprintf(g_name, sizeof(g_name), "%s", name ? name : "raven");
	snprintf(copy, sizeof(copy), "%s", g_folder);
	for (i = 1; copy[i]; i++) {
		if (copy[i] == '/') {
			copy[i] = '\0';
			mkdir(copy, 0755);
			copy[i] = '/';
		}
	}
	mkdir(copy, 0755);
	snprintf(g_report, sizeof(g_report), "%s/%s-crash.txt", g_folder, g_name);
	snprintf(g_flag, sizeof(g_flag), "%s/%s-crashed", g_folder, g_name);
	fd = open(g_flag, O_RDONLY);
	if (fd >= 0) {
		ssize_t got = read(fd, g_previous, sizeof(g_previous) - 1);
		close(fd);
		if (got > 0) {
			g_previous[got] = '\0';
			g_has_previous = 1;
		}
		unlink(g_flag);
	}
	/* a stack of its own: the report is written even when the stack overflowed */
	memset(&stack, 0, sizeof(stack));
	stack.ss_sp = alternate;
	stack.ss_size = sizeof(alternate);
	sigaltstack(&stack, NULL);
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = on_signal;
	action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
	sigemptyset(&action.sa_mask);
	for (i = 0; i < sizeof(g_signals) / sizeof(g_signals[0]); i++) sigaction(g_signals[i], &action, NULL);
#ifdef HAVE_BACKTRACE
	{
		/* (backtrace loads libgcc the first time: not during a crash) */
		void* frame[1];
		backtrace(frame, 1);
	}
#endif
}

#endif
