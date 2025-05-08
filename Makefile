# Makefile for CPU Scheduling Simulator

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

# Source files
SOURCES = src/main.cpp src/process.cpp src/event.cpp src/simulator.cpp src/scheduler.cpp \
          src/fcfs.cpp src/sjf.cpp src/srtn.cpp src/rr.cpp src/parser.cpp
GENERATOR_SRC = src/generator.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
GENERATOR_OBJ = $(GENERATOR_SRC:.cpp=.o)

# Executables
EXECUTABLE = sim
GENERATOR = generate

all: $(EXECUTABLE) $(GENERATOR)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

$(GENERATOR): $(GENERATOR_OBJ)
	$(CXX) $(CXXFLAGS) $(GENERATOR_OBJ) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(EXECUTABLE) $(GENERATOR)
	rm -f *.txt output/* trace/*

# Create output and trace directories if they don't exist
setup:
	mkdir -p output trace

# Run the data generator
generate: $(GENERATOR)
	./$(GENERATOR)

# Run all simulations and generate reports
run: $(EXECUTABLE) setup
	./$(EXECUTABLE) < input.txt > output/default_output.txt
	./$(EXECUTABLE) -d < input.txt > output/detailed_output.txt
	./$(EXECUTABLE) -v < input.txt > output/verbose_output.txt
	./$(EXECUTABLE) -a FCFS < input.txt > output/fcfs_output.txt
	./$(EXECUTABLE) -a SJF < input.txt > output/sjf_output.txt
	./$(EXECUTABLE) -a SRTN < input.txt > output/srtn_output.txt
	./$(EXECUTABLE) -a RR10 < input.txt > output/rr10_output.txt
	./$(EXECUTABLE) -a RR50 < input.txt > output/rr50_output.txt
	./$(EXECUTABLE) -a RR100 < input.txt > output/rr100_output.txt
	./$(EXECUTABLE) -d -v -a FCFS < input.txt > output/fcfs_detailed_verbose.txt
	./$(EXECUTABLE) -d -v -a SJF < input.txt > output/sjf_detailed_verbose.txt
	./$(EXECUTABLE) -d -v -a SRTN < input.txt > output/srtn_detailed_verbose.txt
	./$(EXECUTABLE) -d -v -a RR10 < input.txt > output/rr10_detailed_verbose.txt
	./$(EXECUTABLE) -d -v -a RR50 < input.txt > output/rr50_detailed_verbose.txt
	./$(EXECUTABLE) -d -v -a RR100 < input.txt > output/rr100_detailed_verbose.txt

.PHONY: all clean setup generate run