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

TEST_NAMES := \
	KeyTests \
	TextLayoutTests \
	BufferTests \
	CommandTests \
	WindowTests \
	RendererTests
TEST_OBJECTS := $(addprefix $(BUILD_DIR)/test/,$(addsuffix .o,$(TEST_NAMES)))
TEST_TARGETS := $(addprefix $(BUILD_DIR)/,$(TEST_NAMES))

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/test/%Tests.o: test/%Tests.cpp test/TestSupport.hpp | $(BUILD_DIR)/test
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/KeyTests: $(BUILD_DIR)/test/KeyTests.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/TextLayoutTests: $(BUILD_DIR)/test/TextLayoutTests.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/BufferTests: $(BUILD_DIR)/test/BufferTests.o $(BUILD_DIR)/Buffer.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/CommandTests: $(BUILD_DIR)/test/CommandTests.o $(BUILD_DIR)/Command.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/WindowTests: $(BUILD_DIR)/test/WindowTests.o $(BUILD_DIR)/Buffer.o $(BUILD_DIR)/Window.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/RendererTests: $(BUILD_DIR)/test/RendererTests.o $(BUILD_DIR)/Buffer.o $(BUILD_DIR)/Window.o $(BUILD_DIR)/Renderer.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: $(TEST_TARGETS)
	@status=0; for test_binary in $(TEST_TARGETS); do \
		echo "Running $$test_binary"; \
		./$$test_binary || status=$$?; \
	done; exit $$status

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/test:
	mkdir -p $(BUILD_DIR)/test

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPENDENCIES) $(TEST_OBJECTS:.o=.d)
