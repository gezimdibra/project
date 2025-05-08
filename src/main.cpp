#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include "process.h"
#include "simulator.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool detailedMode = false;
    bool verboseMode = false;
    std::string algorithm = "ALL";
    
    if (!Parser::parseCommandLine(argc, argv, detailedMode, verboseMode, algorithm)) {
        return 1;
    }
    
    // Parse input file
    std::vector<std::shared_ptr<Process>> processes;
    int contextSwitchTime;
    
    Parser parser(std::cin);
    if (!parser.parse(processes, contextSwitchTime)) {
        return 1;
    }
    
    // Create simulator with context switch time
    Simulator simulator(contextSwitchTime);
    
    // Initialize simulator with processes
    simulator.initialize(processes);
    
    // Set simulation parameters
    SimulationParams params;
    params.detailedMode = detailedMode;
    params.verboseMode = verboseMode;
    params.algorithm = algorithm;
    simulator.setParams(params);
    
    if (algorithm == "ALL") {
        // Run all algorithms one after another
        // First run with the active scheduler to collect all results
        simulator.run();
        simulator.outputResults();
    } else {
        // Run specific algorithm only
        simulator.run();
        simulator.outputResults();
    }
    
    return 0;
}
