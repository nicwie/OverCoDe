# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Werror -O3 -std=c++11 -pthread
DEBUGFLAGS = -Wall -Werror -g -std=c++11 -pthread # Flags for debugging

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

# Rule to build the executable with debugging information
debug: CXXFLAGS = $(DEBUGFLAGS)
debug: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) 

# Rule to compile source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean up the build
clean:
	rm -f $(OBJS) $(TARGET)

# Rule to build and run the program
run: $(TARGET)
	./$(TARGET) $(EGOGRAPH) $(ALPHA_BETA) $(FILENAME) $(GRAPHS_RUNS) $(OVERLAPS)
