_SOURCES = ${wildcard src/*.cpp src/compiler/*.cpp src/compiler/ast/*.cpp src/importer/*.cpp}
SOURCES = ${filter-out src/compiler/ast/ast-copy.cpp, $(_SOURCES)}

OBJS = ${SOURCES:.cpp=.o}

CXX = g++

CFLAGS = -I./include -O2 -Wall -std=c++20 -g

hexagn: pre-build $(OBJS)
	$(CXX) $(CFLAGS) obj/*.o -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o obj/$(notdir $@)

clean:
	rm -rf obj/

	
pre-build: clean
	rm -rf hexagn
	mkdir obj
