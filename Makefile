CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic 
LDFLAGS = -pthread 

TARGET = main

SRC_DIR = src
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
SOURCES += $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)