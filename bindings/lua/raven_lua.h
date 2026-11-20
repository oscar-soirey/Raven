/*
 * Raven for Lua: what a program that embeds Lua needs to give Raven to its scripts.
 *
 * Compile raven_lua.c into the program with RAVEN_LUA_LINK defined (it then
 * calls the Raven the program links, instead of opening the shared library),
 * and register the module in the lua_State, before the scripts run:
 *
 *   #include "raven_lua.h"
 *
 *   luaL_requiref(L, "raven", luaopen_raven, 0);    (Lua 5.2 and later)
 *   lua_pop(L, 1);
 *
 * or, in any version, as a preloaded module (require("raven") then finds it):
 *
 *   lua_getglobal(L, "package");
 *   lua_getfield(L, -1, "preload");
 *   lua_pushcfunction(L, luaopen_raven);
 *   lua_setfield(L, -2, "raven");
 *   lua_pop(L, 2);
 *
 * Raven has one UI thread: the lua_State that uses it must be used from that
 * thread (the one that calls Rinit), which is where every callback runs.
 * See README.md.
 */

#ifndef RAVEN_LUA_H
#define RAVEN_LUA_H

#ifndef RAVEN_LUA_API
#if defined(_WIN32)
#define RAVEN_LUA_API __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define RAVEN_LUA_API __attribute__((visibility("default")))
#else
#define RAVEN_LUA_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;

/** Opens the module: returns (pushes) its table, as require("raven") does. */
RAVEN_LUA_API int luaopen_raven(struct lua_State* L);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_LUA_H */
