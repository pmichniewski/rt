TARGET := rt
CXXFLAGS := -std=c++17
CXX := g++
SRCS := main.cpp

OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

.DEFAULT_GOAL := all

-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $<

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

all: $(TARGET)

.PHONY clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(TARGET)
