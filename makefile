COMPILER=g++
CFLAGS=-Wall -std=c++17 -Iheaders

SRC_DIR = src
COMPILED_DIR = build
HEADER_DIR = headers
TARGET_MAIN = $(COMPILED_DIR)/main

all: $(TARGET_MAIN)

$(TARGET_MAIN): $(COMPILED_DIR)/main.o $(COMPILED_DIR)/server.o 
	$(COMPILER) $(CFLAGS) -o $@ $^

$(COMPILED_DIR)/main.o: $(SRC_DIR)/main.cpp $(HEADER_DIR)/server.h | $(COMPILED_DIR)
	$(COMPILER) $(CFLAGS) -c $< -o $@

$(COMPILED_DIR)/server.o: $(SRC_DIR)/server.cpp $(HEADER_DIR)/server.h | $(COMPILED_DIR)
	$(COMPILER) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

run: $(COMPILED_DIR)/main
	./$(COMPILED_DIR)/main

$(COMPILED_DIR):
	mkdir -p $(COMPILED_DIR)