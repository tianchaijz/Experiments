extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <list>
#include <cstdio>
#include <cstdlib>

using namespace ::std;

#define NGX_LUA_CORT_REF "ngx_lua_cort_ref"
#define NGX_LUA_CODE_REF "ngx_lua_code_ref"
#define NGX_LUA_RUN_CODE "ngx_lua_run_code"

int ngx_foo(lua_State *l)
{
  printf(">>> ngx_foo run in coroutine %p: top=%d\n", l, lua_gettop(l));

  /* do something... */
  lua_getglobal(l, "max_cycle");
  if(lua_isnil(l, -1)) {
    fprintf(stderr, "cort %p: can't get max_cycle!\n", l);
    return 0;
  }
  printf("ngx_foo run in coroutine %p: max_cycle=%ld\n", l, lua_tointeger(l, -1));

  /* yield with max_cycle */
  return lua_yield(l, 1);
}

int ngx_bar(lua_State *l)
{
  printf(">>> ngx_bar run in coroutine %p: top=%d\n", l, lua_gettop(l));

  /* do something... */
  lua_getglobal(l, "max_cycle");
  if(lua_isnil(l, -1)) {
    fprintf(stderr, "cort %p: can't get max_cycle!\n", l);
    return 0;
  }
  printf("ngx_bar run in coroutine %p: max_cycle=%ld\n", l, lua_tointeger(l, -1));

  /* yield with max_cycle */
  return lua_yield(l, 1);
}

void init_ngx_lua_registry(lua_State *l)
{
  /* ngx_lua_cort_ref = {} */
  /* key = (ref)[int], val = (thread) */
  lua_newtable(l);
  lua_setfield(l, LUA_REGISTRYINDEX, NGX_LUA_CORT_REF);
}

void init_ngx_lua_globals(lua_State *l)
{
  /* replace main thread's globals with new environment */
  lua_newtable(l);

  lua_newtable(l);    /* ngx.* */

  lua_pushcfunction(l, ngx_foo);
  lua_setfield(l, -2, "foo");
  lua_pushcfunction(l, ngx_bar);
  lua_setfield(l, -2, "bar");

  lua_newtable(l);    /* .status */

  lua_pushinteger(l, 200);
  lua_setfield(l, -2, "HTTP_OK");
  lua_pushinteger(l, 302);
  lua_setfield(l, -2, "HTTP_LOCATION");

  lua_setfield(l, -2, "status");

  lua_setfield(l, -2, "ngx");

  lua_replace(l, LUA_GLOBALSINDEX);
}

void init_ngx_lua_code(lua_State *l, const char *script)
{
  int rc = luaL_loadfile(l, script);
  if(rc) {
    fprintf(stderr, "failed to load script: rc=%d\n", rc);
    exit(1);
  }

  lua_setfield(l, LUA_REGISTRYINDEX, NGX_LUA_CODE_REF);
}

void load_ngx_lua_code(lua_State *l, lua_State *cr)
{
  lua_getfield(cr, LUA_REGISTRYINDEX, NGX_LUA_CODE_REF);

  int rc = lua_pcall(cr, 0, 1, 0);

  if(rc != 0) {
    fprintf(stderr, "failed to run code factory to generate new closure: rc=%d: %s\n", rc, lua_tostring(cr, -1));
    exit(1);
  }

  lua_pushvalue(cr, LUA_GLOBALSINDEX);
  lua_setfenv(cr, -2);

  lua_pushvalue(cr, -1);
  lua_setglobal(cr, NGX_LUA_RUN_CODE);
}

lua_State* new_lua_thread(lua_State *l, int *ref)
{
  int top = lua_gettop(l);

  lua_getfield(l, LUA_REGISTRYINDEX, NGX_LUA_CORT_REF);   /* sp++ */

  lua_State *cr = lua_newthread(l);                       /* sp++ */
  if(cr) {
    printf("coroutine %p created successfully\n", cr);

    lua_newtable(cr);                                     /* sp++ */

    lua_newtable(cr);
    lua_pushvalue(cr, LUA_GLOBALSINDEX);
    lua_setfield(cr, -2, "__index");
    lua_setmetatable(cr, -2);

    lua_replace(cr, LUA_GLOBALSINDEX);                    /* sp-- */

    *ref = luaL_ref(l, -2);                               /* sp-- */
    if(*ref == LUA_NOREF) {
      fprintf(stderr, "failed to make reference to the newly created coroutine\n");
      lua_settop(l, top);
      return NULL;
    }
  }

  lua_pop(l, 1);                                          /* sp-- */
  return cr;
}

void stop_lua_thread(lua_State *l, int ref, int force_quit)
{
  lua_getfield(l, LUA_REGISTRYINDEX, NGX_LUA_CORT_REF);   /* sp++ */

  lua_rawgeti(l, -1, ref);                                /* sp++ */
  lua_State *cr = lua_tothread(l, -1);
  lua_pop(l, 1);                                          /* sp-- */

  if(cr && force_quit) {
    lua_getglobal(cr, NGX_LUA_RUN_CODE);                  /* sp++ */

    lua_newtable(cr);                                     /* sp++ */
    lua_setfenv(cr, -2);                                  /* sp-- */

    do {
      lua_settop(cr, 0);                                  /* sp=0 */
      lua_pushnil(cr);                                    /* sp++ */
    } while(lua_resume(cr, 1) == LUA_YIELD);

    lua_settop(cr, 0);                                    /* sp=0 */
  }

  luaL_unref(l, -1, ref);
  lua_pop(l, 1);                                          /* sp-- */
}

typedef struct {
  lua_State *cr;
  int cr_ref;
  int curr_cycle;
  int max_cycle;
} LuaCoroutine;

int main(int argc, char *argv[])
{
  if(argc != 2) {
    fprintf(stderr, "usage: %s <lua file>\n", argv[0]);
    exit(1);
  }

  lua_State *l = luaL_newstate();
  if(!l) {
    fprintf(stderr, "failed to create lua vm instance!\n");
    exit(1);
  }

  init_ngx_lua_registry(l);
  init_ngx_lua_globals(l);
  luaL_openlibs(l);

  init_ngx_lua_code(l, argv[1]);

  list<LuaCoroutine> corts;

#define MAX_CORTS 20
#define MIN_CORTS 10
  for(int i = 0; i < 10000; ++i) {
    if(corts.size() < MIN_CORTS) {
      while(corts.size() < MAX_CORTS) {
        LuaCoroutine new_co;
        new_co.cr = new_lua_thread(l, &(new_co.cr_ref));
        if(!new_co.cr) {
          fprintf(stderr, "failed to create new coroutine to supply queue\n");
          exit(1);
        }
        load_ngx_lua_code(l, new_co.cr);                         /* load user code */
        new_co.curr_cycle = 0;
        new_co.max_cycle = random()%6 + 3;

        lua_pushinteger(new_co.cr, new_co.max_cycle);            /* sp++ */
        lua_setfield(new_co.cr, LUA_GLOBALSINDEX, "max_cycle");  /* sp-- */

        corts.push_back(new_co);
      }
    }

    LuaCoroutine curr = corts.front();
    corts.pop_front();

    if(curr.curr_cycle < curr.max_cycle) {
      int rc = lua_resume(curr.cr, 0);
      switch(rc) {
        case LUA_YIELD:
          printf("--- coroutine %p yielded\n", curr.cr);
          for(int i=1; i<=lua_gettop(curr.cr); ++i) {
            printf("\tstack[%d] = '%s'\n", i, lua_tostring(curr.cr, i));
          }
          lua_settop(curr.cr, 0); /* clear returned params */
          curr.curr_cycle++;
          corts.push_back(curr);  /* push to the end of scheduling list */
          break;
        case 0:                   /* normal finished */
          printf("*** coroutine %p finished normally\n", curr.cr);
          stop_lua_thread(l, curr.cr_ref, 0);
          break;
        default:                  /* error */
          printf("!!! coroutine %p finished for error\n", curr.cr);
          for(int i=1; i<=lua_gettop(curr.cr); ++i) {
            printf("\tstack[%d] = '%s'\n", i, lua_tostring(curr.cr, i));
          }
          stop_lua_thread(l, curr.cr_ref, 0);
          break;
      }
    } else {
      printf("!!! FORCE KILL COROUTINE %p!\n", curr.cr);
      stop_lua_thread(l, curr.cr_ref, 1);
    }
  }

  lua_close(l);
  return 0;
}

/* vi:set ft=cpp ts=2 sw=2 et fdm=marker: */
