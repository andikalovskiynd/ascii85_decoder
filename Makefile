GTEST_INC_DIR ?= /opt/homebrew/opt/googletest/include
GTEST_LIB_DIR ?= /opt/homebrew/opt/googletest/lib

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic
TEST_CXXFLAGS = $(CXXFLAGS) -Iinclude -I$(GTEST_INC_DIR)
LDFLAGS = -L$(GTEST_LIB_DIR) -lgtest_main -lgtest -pthread -fsanitize=address

MAIN_TARGET = ascii85
TEST_TARGET = test_ascii85

MAIN_SOURCE = main.cpp
TEST_SRC_DIR = test
TEST_SOURCES = $(wildcard $(TEST_SRC_DIR)/*.cpp)
TEST_OBJECTS = $(patsubst %.cpp,%.o,$(TEST_SOURCES))

all: $(MAIN_TARGET)

$(MAIN_TARGET): $(MAIN_SOURCE)
	$(CXX) $(CXXFLAGS) $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(TEST_SRC_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(MAIN_TARGET) $(TEST_TARGET) $(TEST_OBJECTS)

help:
	@echo "Available targets:"
	@echo "  all      - Build the main ASCII85 encoder/decoder program"
	@echo "  test     - Build and run unit tests"
	@echo "  clean    - Remove all built files"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  echo 'Hello World' | ./ascii85           # Encode (default)"
	@echo "  echo 'Hello World' | ./ascii85 -e        # Encode explicitly"
	@echo "  echo '87cURD]j7BEbo80~>' | ./ascii85 -d  # Decode"
	@echo "  ./ascii85 --stream < input.txt > output.txt  # Stream mode"

.PHONY: all test clean help