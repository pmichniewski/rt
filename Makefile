TARGET := rt
CXXFLAGS := -std=c++17
CXX := g++
SRCS := \
	main.cpp \
	math.cpp \
	sphere.cpp \
	scene.cpp \
	ray.cpp \
	plane.cpp \
	primitive.cpp

OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

OUTFILE := out.ppm
TARGET_FILE := /mnt/e/Projects/$(OUTFILE)

.DEFAULT_GOAL := all

-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $<

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(OUTFILE): $(TARGET)
	./$(TARGET)

$(TARGET_FILE): $(OUTFILE)
	cp $(OUTFILE) $(TARGET_FILE)

.PHONY test: $(TARGET_FILE)

all: $(TARGET)

.PHONY clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(TARGET)
