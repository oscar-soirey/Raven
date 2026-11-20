/*
 * A program that embeds Lua and gives Raven to its scripts: what a game
 * engine or a tool with Lua inside does to have its interfaces written in Lua.
 *
 *     host script.lua [arguments...]
 *
 * It is built with raven_lua.c compiled in, RAVEN_LUA_LINK defined, and
 * linked with Raven and Lua (see bindings/CMakeLists.txt). The build of Raven
 * runs example.lua --test with it (binding_lua_embedded).
 */

#include <stdio.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "raven_lua.h"

static int traceback(lua_State* L) {
	const char* message = lua_tostring(L, 1);
#if LUA_VERSION_NUM >= 502
	luaL_traceback(L, L, message ? message : "(error object is not a string)", 1);
#else
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_pushstring(L, message ? message : "(error object is not a string)");
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
#endif
	return 1;
}

int main(int argc, char** argv) {
	lua_State* L;
	int i, status;
	if (argc < 2) {
		fprintf(stderr, "usage: %s script.lua [arguments...]\n", argv[0]);
		return 2;
	}
	L = luaL_newstate();
	if (!L) return 1;
	luaL_openlibs(L);

	/* require("raven") finds the module in this program */
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, luaopen_raven);
	lua_setfield(L, -2, "raven");
	lua_pop(L, 2);

	/* arg, as the interpreter sets it: arg[0] is the script */
	lua_createtable(L, argc - 2, 1);
	for (i = 1; i < argc; i++) {
		lua_pushstring(L, argv[i]);
		lua_rawseti(L, -2, i - 1);
	}
	lua_setglobal(L, "arg");

	lua_pushcfunction(L, traceback);
	status = luaL_loadfile(L, argv[1]);
	if (status == 0) status = lua_pcall(L, 0, 0, -2);
	if (status != 0) fprintf(stderr, "%s\n", lua_tostring(L, -1));
	lua_close(L);
	return status == 0 ? 0 : 1;
}
