prefix= /home/zjz311/openresty/lua/src
include= $(prefix)
link= $(prefix)

CC= cc
CFLAGS= -I$(include) -pipe -O -W -Wall -Wpointer-arith -Wno-unused -Werror -g
CPP= cc -E
LINK= $(CC)

name= lua-c-thread-model
target= $(name).out

all: $(target)

$(target): $(name).o
	$(LINK) -o $@ $? -L$(link)/lib -llua5.1 -Wl,-rpath,$(link) -Wl,-E -v
	ldd $@

$(name).o: lua-c-thread-model.c
	$(CC) -c $(CFLAGS) -o $@ $?

clean:
	rm -f $(name).o $(target)
