
CFLAGS+=-Wall -std=gnu99

CFLAGS+=-O2

HEADERS:=$(shell find -iname "*.h")
OBJECTS:=ast.o error.o lang.o llvmgen.o operators.o parser.o parser_utils.o stream.o

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
	./lang -r test.x

dump: lang
	./lang -d test.x

debug: lang
	gdb ./lang --eval-command="run -c test.x"

clean:
	rm $(OBJECTS)

