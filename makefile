VALGRIND_FLAGS=--leak-check=full --track-origins=yes --show-reachable=yes --error-exitcode=2 --show-leak-kinds=all --trace-children=yes
CFLAGS =-std=c99 -Wall -Wconversion -Wtype-limits -pedantic -Werror -O0 -g
CC = gcc
PUERTO = 8080

all: clean valgrind-demo

valgrind-demo: demo
	valgrind $(VALGRIND_FLAGS) ./demo $(PUERTO)

demo: main.c extra/engine.c extra/engine.h
	$(CC) $(CFLAGS) main.c extra/engine.c -o demo

clean:
	rm -f demo
