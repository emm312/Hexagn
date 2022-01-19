SOURCES = ${wildcard src/*/*.cpp}
OBJS = ${SOURCES:.cpp=.o}

CXX = g++		# Change to clang++ if using clang

CFLAGS = -I/src/include -O3

hexagn: $(OBJS)
	$(CXX) $(CFLAGS) obj/*.o -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o obj/$(notdir $@)
