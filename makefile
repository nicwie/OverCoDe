# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -fopenmp -Wall -O2

# Target executable
TARGET = main

# Source files
SRCS = main.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header files
HEADERS = OverCoDe.h DistributedProcess.h ClusteredGraph.h

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean up the build
clean:
	rm -f $(OBJS) $(TARGET)
