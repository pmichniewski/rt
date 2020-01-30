TARGET := rt
CXXFLAGS := -std=c++17
CXX := g++
SRCS := main.cpp

OBJS := $(SRCS:.cpp=.o)

.DEFAULT_GOAL := all

%.o: %.cpp
	$(CXX) -c $<

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@

all: $(TARGET)

.PHONY clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
