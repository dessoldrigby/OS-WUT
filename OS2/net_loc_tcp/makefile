CC = gcc
FL = -Wall -fsanitize=address,undefined
FL_O = $(FL) -o

all: server_main client_local_main client_tcp_main

server_main: main_s.c utils.c
	$(CC) $(FL_O) server main_s.c utils.c

client_local_main: main_c_local.c utils.c
	$(CC) $(FL_O) client_local main_c_local.c utils.c

client_tcp_main: main_c_tcp.c utils.c
	$(CC) $(FL_O) client_tcp main_c_tcp.c utils.c

.PHONY: clean
clean:
	rm server client

.PHONY: run
run:
	./server a 2000&

.PHONY: pack
pack:
	tar -cjf package.tar.bz2 main.c utils.h utils.c