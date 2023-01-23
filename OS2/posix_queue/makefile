CC = gcc
FL = -Wall -fsanitize=address,undefined
FL_O = $(FL) -o

all: main

main: main.c utils.c 
	$(CC) $(FL_O) main main.c utils.c

.PHONY: clean
clean:
	rm main

.PHONY: run
run:
	./main 10

.PHONY: pack
pack:
	tar -cjf package.tar.bz2 main.c utils.h utils.c