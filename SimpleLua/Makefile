CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Isrc

SRCS = src/main.cpp src/Lexer.cpp src/Compiler.cpp src/LuaGenerator.cpp src/VMP/OpCodeStrategy.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = simple_lua

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) output.lua
