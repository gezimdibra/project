#include "parser.h"
#include <iostream>
#include <sstream>
#include <string>

Parser::Parser(std::istream& in) : input(in) {
}

bool Parser::parse(std::vector<std::shared_ptr<Process>>& processes, int& contextSwitchTime) {
    int numProcesses;
    
    // Read number of processes and context switch time
    input >> numProcesses >> contextSwitchTime;
    
    if (numProcesses <= 0) {
        std::cerr << "Error: Invalid number of processes" << std::endl;
        return false;
    }
    
    // Read each process
    for (int i = 0; i < numProcesses; i++) {
        int pid, arrivalTime, numBursts;
        
        // Read process ID, arrival time, and number of bursts
        input >> pid >> arrivalTime >> numBursts;
        
        if (pid <= 0 || arrivalTime < 0 || numBursts <= 0) {
            std::cerr << "Error: Invalid process parameters for process " << i+1 << std::endl;
            return false;
        }
        
        // Create new process
        auto process = std::make_shared<Process>(pid, arrivalTime);
        
        // Read burst information
        for (int j = 0; j < numBursts; j++) {
            int burstNum, cpuTime;
            
            input >> burstNum >> cpuTime;
            
            if (burstNum != j+1 || cpuTime <= 0) {
                std::cerr << "Error: Invalid burst parameters for process " << pid << std::endl;
                return false;
            }
            
            // Add CPU burst
            process->addCPUBurst(cpuTime);
            
            // If not the last burst, read IO time
            if (j < numBursts - 1) {
                int ioTime;
                input >> ioTime;
                
                if (ioTime <= 0) {
                    std::cerr << "Error: Invalid I/O time for process " << pid << std::endl;
                    return false;
                }
                
                // Add IO burst
                process->addIOBurst(ioTime);
            }
        }
        
        // Add process to vector
        processes.push_back(process);
    }
    
    return true;
}

bool Parser::parseCommandLine(int argc, char* argv[], 
                             bool& detailedMode, bool& verboseMode, std::string& algorithm) {
    // Default values
    detailedMode = false;
    verboseMode = false;
    algorithm = "ALL";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-d") {
            detailedMode = true;
        } else if (arg == "-v") {
            verboseMode = true;
        } else if (arg == "-a" && i + 1 < argc) {
            // Get algorithm name
            algorithm = argv[++i];
            
            // Validate algorithm
            if (algorithm != "FCFS" && algorithm != "SJF" && algorithm != "SRTN" && 
                algorithm != "RR10" && algorithm != "RR50" && algorithm != "RR100") {
                std::cerr << "Error: Invalid algorithm. Must be one of: FCFS, SJF, SRTN, RR10, RR50, RR100" << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error: Invalid argument: " << arg << std::endl;
            std::cerr << "Usage: sim [-d] [-v] [-a algorithm] < input_file" << std::endl;
            return false;
        }
    }
    
    return true;
}