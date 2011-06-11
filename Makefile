
CFLAGS+=-Wall -std=gnu99

CFLAGS+=-O2

HEADERS:=$(shell find -iname "*.h")
OBJECTS:=ast.o error.o lang.o llvmgen.o operators.o parser.o parser_utils.o stream.o

#LLVM_CONFIG:=/usr/local/bin/llvm-config
LLVM_CONFIG:=llvm-config-2.9
CFLAGS+=`$(LLVM_CONFIG) --cflags`
LIBS+=`$(LLVM_CONFIG) --libs --ldflags` -lstdc++ -lpthread -lm -ldl

.DEFAULT: lang
.PHONY: run debug clean

lang: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o lang

$(OBJECTS): %.o: %.c Makefile $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: lang
	./lang test.x 1 2 3 4 5 6

dump: lang
	./lang test.x 1 2 3 4 5 6

debug: lang
	gdb ./lang --eval-command="run test.x 1 2 3 4 5 6"

clean:
	rm $(OBJECTS)

