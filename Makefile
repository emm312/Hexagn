SOURCES = ${wildcard src/*.cpp src/compiler/*.cpp src/importer/*.cpp}
OBJS = ${SOURCES:.cpp=.o}

CXX = g++

CFLAGS = -I./include -O3 -Wall -std=c++20 -g

hexagn: pre-build $(OBJS)
	$(CXX) $(CFLAGS) obj/*.o -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o obj/$(notdir $@)

clean:
	rm -rf obj/

	
pre-build: clean
	rm -rf hexagn
	mkdir obj

wasm:
	-mkdir build
	cd build
	em++ ./src/main.cpp ./src/util.cpp ./src/compiler/compiler.cpp  ./src/compiler/lexer.cpp  ./src/compiler/linker.cpp  ./src/compiler/parser.cpp  ./src/compiler/string.cpp  ./src/compiler/token.cpp  ./src/importer/importHelper.cpp  ./src/importer/sourceParser.cpp -I./include/ --std=c++20 -s WASM=1 -sEXPORTED_FUNCTIONS=_compiler -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -o ./build/main.js