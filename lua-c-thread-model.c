#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>  /* definition of opening Lua libary, eg. <string, table, math etc> */

static void stack_dump(lua_State *L);
static int fake_socket_connect_read(lua_State *L);

char *LUA_CODE_test = "                                  \
print('[start]')                                         \
say_hi()                                                 \
local a, b = fake_socket_connect_read('127.0.0.1', 8080) \
print(a .. b)                                            \
print('[end]')                                           \
";

char *LUA_CODE_global_funcs = " \
function say_hi()               \
  print('hi')                   \
end                             \
";


int
main(int argc, char *argv[])
{
  int error;
  lua_State *L = luaL_newstate();  /* main VM */
  luaL_openlibs(L);
  lua_pushcfunction(L, fake_socket_connect_read);
  lua_setglobal(L, "fake_socket_connect_read");
  error = luaL_loadbuffer(L, LUA_CODE_global_funcs,
                          strlen(LUA_CODE_global_funcs), "funcs")
    || lua_pcall(L, 0, 0, 0);
  if (error) {
    fprintf(stderr, "err:%d, %s\n", error, lua_tostring(L, -1));
    lua_pop(L, 1);  /* pop error message from the stack */
  }

  lua_State *co = lua_newthread(L);  /* coroutine */
  error = luaL_loadbuffer(co, LUA_CODE_test,
                          strlen(LUA_CODE_test), "test") || lua_resume(co, 0);
  if (error != LUA_YIELD) {
    fprintf(stderr, "err:%d, %s\n", error, lua_tostring(co, -1));
    lua_pop(co, 1);  /* pop error message from the stack */
  }

  lua_pushstring(co, "this is what I read from remote asyncly :)");
  lua_pushstring(co, ", next");
  lua_resume(co, 2);
  stack_dump(co);
  lua_pop(L, 1);  /* after done with the thread, pop it from VM, so it'll be GCed. */
  lua_close(L);
  return 0;
}


static int
fake_socket_connect_read(lua_State *L)
{
  /* return string read from remote. */
  /*
    IMPLEMENTATION:
    Read from socket in non-blocking way in Lua,
    that is,
    1. create socket using SOCK_NONBLOCK option
    2. connect the socket
    3. read from the socket, if success, done. or goto 4
    4. read return EAGAIN, so put a handler in EPOLL, waiting EPOLLIN event
    5. about the handler,
       it mainly do the socket again. and put the result in the coroutine
       stack, and call lua_resume(L, 1).
   */

  /* 0 means nothing on the stack is useful outside */
  return lua_yield(L, 0);
}


static void
stack_dump (lua_State *L)
{
  int i;
  int top = lua_gettop(L);

  printf("STACK DUMP[%d]: ", top);

  for(i = 1; i <= top; i++) {
    int t = lua_type(L, i);
    switch (t) {
      case LUA_TSTRING:
        printf("'%s'", lua_tostring(L, i));
        break;
      case LUA_TBOOLEAN:
        printf(lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:
        printf("%g", lua_tonumber(L, i));
        break;
      default:
        printf("%s", lua_typename(L, t));
        break;
    }

    printf("  ");
  }

  printf("\n");
}

/* vi:set ft=c ts=2 sw=2 et fdm=marker: */
