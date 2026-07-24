CXX ?= g++

CPPFLAGS += -Isrc
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion

TARGET := MiniVim
BUILD_DIR := build

SOURCES := \
	src/Buffer.cpp \
	src/Command.cpp \
	src/Editor.cpp \
	src/Renderer.cpp \
	src/Terminal.cpp \
	src/Window.cpp \
	src/main.cpp

OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

CORE_OBJECTS := \
	$(BUILD_DIR)/Buffer.o \
	$(BUILD_DIR)/Command.o \
	$(BUILD_DIR)/Renderer.o \
	$(BUILD_DIR)/Window.o
TEST_OBJECT := $(BUILD_DIR)/CoreTests.o
TEST_TARGET := $(BUILD_DIR)/core_tests

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST_OBJECT): tests/CoreTests.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST_TARGET): $(CORE_OBJECTS) $(TEST_OBJECT)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPENDENCIES) $(TEST_OBJECT:.o=.d)
