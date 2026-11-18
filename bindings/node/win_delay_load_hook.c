/*
 * Windows, Visual C++: the addon imports Node-API from node.exe (node.lib names
 * it), and those imports are delay-loaded (/DELAYLOAD:node.exe). When the first
 * one is called, this hook answers the load of node.exe with the program that
 * is running, so the addon also works in an executable that has another name:
 * Electron, a node.exe renamed or embedded. node-gyp and cmake-js do the same.
 */
#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <delayimp.h>
#include <string.h>

static FARPROC WINAPI raven_node_load_hook(unsigned event, PDelayLoadInfo info)
{
	if (event != dliNotePreLoadLibrary || _stricmp(info->szDll, "node.exe") != 0)
		return NULL;
	return (FARPROC)GetModuleHandleW(NULL);
}

const PfnDliHook __pfnDliNotifyHook2 = raven_node_load_hook;

#endif
