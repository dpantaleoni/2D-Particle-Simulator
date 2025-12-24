CXX = clang++
CXXFLAGS = -Wall -Wextra -O2 -std=c++20 -I/opt/homebrew/include -I.
LDFLAGS = -lm -pthread -L/opt/homebrew/lib -lraylib -framework Metal -framework Foundation

SRC_CPP = basic/main.cpp basic/Init.cpp basic/Physics.cpp
SRC_MM = metal/MetalPhysics.mm
OBJ = $(SRC_CPP:.cpp=.o) $(SRC_MM:.mm=.o)

TARGET = nbody_cpp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
