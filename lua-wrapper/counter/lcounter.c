#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "compat.h"
#include "counter.h"

/* Userdata object that will hold the counter and name. */
typedef struct {
  counter_t *c;
  char *name;
} lcounter_userdata_t;

static int lcounter_new(lua_State *L) {
  lcounter_userdata_t *cu;
  const char *name;
  int start;

  /* Check the arguments are valid. */
  start = luaL_checkint(L, 1);
  name = luaL_checkstring(L, 2);
  if (name == NULL)
    luaL_error(L, "name cannot be empty");

  /* Create the user data pushing it onto the stack. We also pre-initialize
   * the member of the userdata in case initialization fails in some way. If
   * that happens we want the userdata to be in a consistent state for __gc.
   */
  cu = (lcounter_userdata_t *)lua_newuserdata(L, sizeof(*cu));
  cu->c = NULL;
  cu->name = NULL;

  /* Add the metatable to the stack. */
  luaL_getmetatable(L, "LCounter");
  /* Set the metatable on the userdata. */
  lua_setmetatable(L, -2);

  /* Create the data that comprises the userdata (the counter). */
  cu->c = counter_create(start);
  cu->name = strdup(name);

  return 1;
}

static int lcounter_add(lua_State *L) {
  lcounter_userdata_t *cu;
  int amount;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");
  amount = luaL_checkint(L, 2);
  counter_add(cu->c, amount);

  return 0;
}

static int lcounter_subtract(lua_State *L) {
  lcounter_userdata_t *cu;
  int amount;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");
  amount = luaL_checkint(L, 2);
  counter_subtract(cu->c, amount);

  return 0;
}

static int lcounter_increment(lua_State *L) {
  lcounter_userdata_t *cu;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");
  counter_increment(cu->c);

  return 0;
}

static int lcounter_decrement(lua_State *L) {
  lcounter_userdata_t *cu;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");
  counter_decrement(cu->c);

  return 0;
}

static int lcounter_getval(lua_State *L) {
  lcounter_userdata_t *cu;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");
  lua_pushinteger(L, counter_getval(cu->c));

  return 1;
}

static int lcounter_getname(lua_State *L) {
  lcounter_userdata_t *cu;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");
  lua_pushstring(L, cu->name);

  return 1;
}

static int lcounter_destroy(lua_State *L) {
  lcounter_userdata_t *cu;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");

  if (cu->c != NULL)
    counter_destroy(cu->c);
  cu->c = NULL;

  if (cu->name != NULL)
    free(cu->name);
  cu->name = NULL;

  return 0;
}

static int lcounter_tostring(lua_State *L) {
  lcounter_userdata_t *cu;

  cu = (lcounter_userdata_t *)luaL_checkudata(L, 1, "LCounter");

  lua_pushfstring(L, "%s(%d)", cu->name, counter_getval(cu->c));

  return 1;
}

static const struct luaL_Reg lcounter_methods[] = {
    {"add", lcounter_add},
    {"subtract", lcounter_subtract},
    {"increment", lcounter_increment},
    {"decrement", lcounter_decrement},
    {"getval", lcounter_getval},
    {"getname", lcounter_getname},
    {"__gc", lcounter_destroy},
    {"__tostring", lcounter_tostring},
    {NULL, NULL},
};

static const struct luaL_Reg lcounter_functions[] = {{"new", lcounter_new},
                                                     {NULL, NULL}};

int luaopen_lcounter(lua_State *L) {
  /* Create the metatable and put it on the stack. */
  luaL_newmetatable(L, "LCounter");
  /* Duplicate the metatable on the stack (We know have 2). */
  lua_pushvalue(L, -1);
  /* Pop the first metatable off the stack and assign it to __index
   * of the second one. We set the metatable for the table to itself.
   * This is equivalent to the following in lua:
   * metatable = {}
   * metatable.__index = metatable
   */
  lua_setfield(L, -2, "__index");

  /* Set the methods to the metatable that should be accessed via object:func
   */
  luaL_setfuncs(L, lcounter_methods, 0);

/* Register the object.func functions into the table that is at the top of
 * the
 * stack. */
#if LUA_VERSION_NUM < 502
  luaL_register(L, "lcounter", lcounter_functions);
#else
  luaL_newlib(L, lcounter_functions);
#endif

  return 1;
}

/* vi:set ft=c ts=2 sw=2 et fdm=marker: */
