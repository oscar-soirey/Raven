/**
 * Raven tools - crash_report.h
 * When a tool of Raven stops on a crash, it leaves a report behind: what
 * happened, where (the functions of the stack, with their files and lines when
 * the symbols are there), the last things the tool did, and on Windows a
 * minidump that Visual Studio or WinDbg opens on the line that crashed. The
 * next start says where the report is.
 */

#ifndef RAVEN_CRASH_REPORT_H
#define RAVEN_CRASH_REPORT_H

/**
 * Catches the crashes from now on. `folder` is where the report goes (made
 * when missing), `name` the name of the tool ("raven_designer").
 */
void crash_report_install(const char* folder, const char* name);

/**
 * Something the tool does, kept in memory (the last 24): the report lists
 * them, oldest first. Costs a copy, nothing is written until a crash.
 */
void crash_report_note(const char* format, ...);

/**
 * The report left by the last run when it crashed, NULL when it did not.
 * Asked once: the next call gives NULL (the report itself stays on disk).
 */
const char* crash_report_previous(void);

#endif
