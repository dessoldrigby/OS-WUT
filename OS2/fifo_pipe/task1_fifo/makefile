CC = gcc
FL = -Wall -fsanitize=address,undefined
FL_O = $(FL) -o

all: main

main: main.c utils.c 
	$(CC) $(FL_O) main main.c utils.c
	$(CC) $(FL_O) main_c main_c.c utils.c

.PHONY: clean
clean:
	rm main

.PHONY: run
run:
	./main_c a makefile & ./main_c a main.c & ./main_c a main_c.c & sleep 1; ./main a

.PHONY: pack
pack:
	tar -cjf package.tar.bz2 main.c utils.h utils.c