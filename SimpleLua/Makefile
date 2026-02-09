CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Isrc

SRCS = src/main.cpp src/Lexer.cpp src/Compiler.cpp src/LuaGenerator.cpp src/VMP/OpCodeStrategy.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = simple_lua

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: src/tests/test_value.o
	$(CXX) $(CXXFLAGS) -o test_value src/tests/test_value.o
	./test_value

src/tests/%.o: src/tests/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) output.lua test_value src/tests/*.o
