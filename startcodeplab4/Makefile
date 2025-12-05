all:
	gcc -Wall -Wextra -std=c11 -pthread -O2 main.c sbuffer.c -o main

run: all
	./main

zip:
	zip milestone3.zip *.c *.h Makefile
