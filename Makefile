
CFLAGS+=-Wall -std=gnu99

CFLAGS+=-g -O2

OBJECTS:=ast.o error.o lang.o llvmgen.o operators.o parser.o parser_utils.o stream.o

LLVM_CONFIG:=/usr/local/lib/llvm-2.8/bin/llvm-config
CFLAGS+=`$(LLVM_CONFIG) --cflags`
LIBS+=`$(LLVM_CONFIG) --libs --ldflags` -lstdc++


.DEFAULT: lang
.PHONY: run debug clean

lang: $(OBJECTS) Makefile
	gcc $(OBJECTS) $(LIBS) -o lang

run: lang
	./lang -c test.x

dump: lang
	./lang -d test.x


debug: lang
	gdb ./lang --eval-command="run -c test.x"

clean:
	rm $(OBJECTS)
