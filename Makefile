prefix= /home/zjz311/openresty/lua/src
include= $(prefix)
link= $(prefix)

CC= cc
CFLAGS= -pipe -O -W -Wall -Wpointer-arith -Wno-unused -Werror -g
CPP= cc -E
LINK= $(CC)

LUAINC= -I$(include)
LUALINK= -llua5.1 -Wl,-rpath,$(link)
LUAFLAGS= $(LUAINC) $(LUALINK)

name= lua_c_thread_model
target= $(name).out

all: $(target)

$(target): $(name).o
	$(LINK) -o $@ $? $(CFLAGS) $(LUAFLAGS) -Wl,-E -v
	ldd $@

$(name).o: lua_c_thread_model.c
	$(CC) -c $(CFLAGS) -o $@ $?

luaproc.o: luaproc.c
	$(CC) -c -fPIC $(LUAINC) -o $@ $? $(CFLAGS)

luaproc.so: luaproc.o
	$(CC) -shared -fPIC -ldl -lpthread -o $@ $? $(CFLAGS)

clean:
	rm -f $(name).o $(target) luaproc.o luaproc.so
