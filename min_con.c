#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>

#define CORO_NUM 2
#define SCRIPT "return function() while true do print 'before yield'; coroutine.yield(); print 'after yield'; end end"

int main(int argc, char *argv[])
{
  int i, cnt;
  int rc;
  lua_State *l;
  lua_State *cc[CORO_NUM];

  /* create main thread */
  l = luaL_newstate();
  luaL_openlibs(l);

  /* load user code */
  luaL_loadstring(l, SCRIPT);
  lua_setfield(l, LUA_REGISTRYINDEX, "code");

  /* create new thread */
  for(i = 0; i < CORO_NUM; ++i) {
    cc[i] = lua_newthread(l);

    /* load user code into new thread */
    lua_getfield(cc[i], LUA_REGISTRYINDEX, "code");
    /* create new closure */
    lua_pcall(cc[i], 0, 1, 0);
  }

  cnt = 0;
  for(i = 0; i < 10000; ++i) {
    rc = lua_resume(cc[cnt], 0);
    switch(rc) {
      case LUA_YIELD:
        printf("*** coroutine %p yielded ***\n", cc[cnt]);
        break;
      case 0:
        printf("--- coroutine %p finished normally ---\n", cc[cnt]);
        exit(1);
      default:
        printf("!!! coroutine %p finished for error !!!\n", cc[cnt]);
        for(i = 1; i <= lua_gettop(cc[cnt]); ++i) {
          printf("\tstack[%d] = '%s'\n", i, lua_tostring(cc[cnt], i));
        }
        exit(1);
    }
    cnt = (cnt + 1) % CORO_NUM;
  }

  return 0;
}

/* vi:set ft=c ts=2 sw=2 et fdm=marker: */
