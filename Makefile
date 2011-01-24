
CFLAGS=-Wall -g -std=gnu99
CFLAGS+=`llvm-config-2.7 --libs --ldflags --cflags` -lstdc++

lang: *.[ch] Makefile
	gcc *.c $(CFLAGS) -o lang

run: lang
	./lang test.x

debug: lang
	gdb ./lang --eval-command="run test.x"

valgrind: lang
	valgrind ./lang test.x
