# DisPing Makefile for MinGW-w64 (x86_64)

CXX = C:/msys64/ucrt64/bin/g++.exe
NASM = C:/msys64/ucrt64/bin/nasm.exe

CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Iinclude
LDFLAGS = -static -static-libgcc -static-libstdc++ -lws2_32 -liphlpapi -lwinmm -lntdll -lavrt -lpsapi -ladvapi32

BUILD_DIR = build
SRC_DIR = src
ASM_DIR = src/asm
TEST_DIR = tests

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))
ASM_OBJECT = $(BUILD_DIR)/disping_routines.obj

all: $(BUILD_DIR)/disping.exe $(BUILD_DIR)/disping_tests.exe

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ASM_OBJECT): $(ASM_DIR)/disping_routines.asm | $(BUILD_DIR)
	$(NASM) -f win64 $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/disping.exe: $(OBJECTS) $(ASM_OBJECT)
	$(CXX) $(OBJECTS) $(ASM_OBJECT) -o $@ $(LDFLAGS)

$(BUILD_DIR)/disping_tests.exe: $(TEST_DIR)/test_main.cpp $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) $(ASM_OBJECT)
	$(CXX) $(CXXFLAGS) $< $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) $(ASM_OBJECT) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
