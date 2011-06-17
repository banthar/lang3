
CFLAGS+=-Wall -std=gnu99

CFLAGS+=-O2

HEADERS:=$(shell find -iname "*.h")
OBJECTS:=ptree.o error.o lang.o llvmgen.o operators.o parser.o parser_utils.o stream.o 

#LLVM_CONFIG:=/usr/local/bin/llvm-config
LLVM_CONFIG:=llvm-config-2.9
CFLAGS+=`$(LLVM_CONFIG) --cflags`
CXXFLAGS+=`$(LLVM_CONFIG) --cxxflags`
LIBS+=`$(LLVM_CONFIG) --libs --ldflags` -lstdc++ -lpthread -lm -ldl

.DEFAULT: lang
.PHONY: run debug clean

lang3i: $(OBJECTS) llvm_utils.o
	$(CC) $(CFLAGS) $(OBJECTS) llvm_utils.o $(LIBS) -o lang3i

$(OBJECTS): %.o: %.c Makefile $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

llvm_utils.o: llvm_utils.cpp Makefile $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
run: lang3i
	./lang3i --run test.l3 1 2 3 4 5 6

debug: lang3i
	gdb ./lang3i --eval-command="run test.l3 1 2 3 4 5 6"

clean:
	rm $(OBJECTS)

