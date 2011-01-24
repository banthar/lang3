
CFLAGS+=-Wall -std=gnu99

CFLAGS+=-g -O0

LLVM_CONFIG:=/usr/local/lib/llvm-2.8/bin/llvm-config

CFLAGS+=`$(LLVM_CONFIG) --cflags`
LIBS+=`$(LLVM_CONFIG) --libs --ldflags` -lstdc++

SOURCES=error.c llvmgen.c ast.c parser.c parser_utils.c lang.c stream.c operators.c

.DEFAULT: lang
.PHONY: run debug

lang: *.[ch] Makefile
	gcc $(SOURCES) $(CFLAGS) $(LIBS) -o lang

run: lang
	./lang -c test.x

dump: lang
	./lang -d test.x


debug: lang
	gdb ./lang --eval-command="run -c test.x"
