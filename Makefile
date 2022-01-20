SOURCES = ${wildcard src/*.cpp src/*/*.cpp}
OBJS = ${SOURCES:.cpp=.o}

CXX = g++		# Change to clang++ if using clang

CFLAGS = -Isrc/include -O2 -Wall -std=c++20

hexagn: pre-build $(OBJS)
	$(CXX) $(CFLAGS) obj/*.o -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o obj/$(notdir $@)

clean:
	rm -rf obj

pre-build: clean
	mkdir obj
