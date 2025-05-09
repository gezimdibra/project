#include "simulator.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <sstream>
#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"

Simulator::Simulator(int switchTime)
    : currentTime(0),
      processSwitchTime(switchTime) {
    
    // Initialize schedulers
    fcfsScheduler = std::make_shared<FCFSScheduler>(processSwitchTime);
    sjfScheduler = std::make_shared<SJFScheduler>(processSwitchTime);
    srtnScheduler = std::make_shared<SRTNScheduler>(processSwitchTime);
    rr10Scheduler = std::make_shared<RRScheduler>(processSwitchTime, 10);
    rr50Scheduler = std::make_shared<RRScheduler>(processSwitchTime, 50);
    rr100Scheduler = std::make_shared<RRScheduler>(processSwitchTime, 100);
    
    // Default to FCFS
    activeScheduler = fcfsScheduler;
}

void Simulator::initialize(const std::vector<std::shared_ptr<Process>>& processList) {
    processes = processList;
    std::sort(processes.begin(), processes.end(),
              [](const auto& a, const auto& b) { return a->getArrivalTime() < b->getArrivalTime(); });
    
    // Reset all processes to NEW state
    for (auto& process : processes) {
        process->setState(ProcessState::NEW);
    }
}

void Simulator::setParams(const SimulationParams& simulationParams) {
    params = simulationParams;
    
    // Set active scheduler based on algorithm parameter
    if (params.algorithm == "FCFS") {
        activeScheduler = fcfsScheduler;
    } else if (params.algorithm == "SJF") {
        activeScheduler = sjfScheduler;
    } else if (params.algorithm == "SRTN") {
        activeScheduler = srtnScheduler;
    } else if (params.algorithm == "RR10") {
        activeScheduler = rr10Scheduler;
    } else if (params.algorithm == "RR50") {
        activeScheduler = rr50Scheduler;
    } else if (params.algorithm == "RR100") {
        activeScheduler = rr100Scheduler;
    }
    
    // Open verbose output file if in verbose mode
    if (params.verboseMode) {
        std::string filename = "trace/";
        if (params.algorithm != "ALL") {
            filename += params.algorithm + "_trace.txt";
        } else {
            filename += "all_trace.txt";
        }
        verboseOutput.open(filename);
    }
}

void Simulator::run() {
    if (params.algorithm == "ALL") {
        // Run each scheduler separately
        runScheduler(fcfsScheduler);
        runScheduler(sjfScheduler);
        runScheduler(srtnScheduler);
        runScheduler(rr10Scheduler);
        runScheduler(rr50Scheduler);
        runScheduler(rr100Scheduler);
    } else {
        // Run only the active scheduler
        runScheduler(activeScheduler);
    }
}

void Simulator::outputResults() const {
    if (params.algorithm == "ALL") {
        outputSchedulerResults(fcfsScheduler);
        outputSchedulerResults(sjfScheduler);
        outputSchedulerResults(srtnScheduler);
        outputSchedulerResults(rr10Scheduler);
        outputSchedulerResults(rr50Scheduler);
        outputSchedulerResults(rr100Scheduler);
    } else {
        outputSchedulerResults(activeScheduler);
    }
}

void Simulator::outputSchedulerResults(std::shared_ptr<Scheduler> scheduler) const {
    std::cout << "\n" << scheduler->getName() << " Results:\n"
              << "Total Time: " << scheduler->getTotalTime() << " time units\n"
              << "CPU Utilization: " << std::fixed << std::setprecision(2) 
              << scheduler->getCpuUtilization() << "%\n"
              << "Context Switches: " << scheduler->getContextSwitchCount() << "\n\n";
    
    if (params.detailedMode) {
        std::cout << "Process Details:\n";
        for (const auto& process : scheduler->getAllProcesses()) {
            std::cout << "Process " << process->getId() << ":\n"
                      << "  Arrival Time: " << process->getArrivalTime() << "\n"
                      << "  Service Time: " << process->getServiceTime() << "\n"
                      << "  I/O Time: " << process->getIOTime() << "\n"
                      << "  Finish Time: " << process->getFinishTime() << "\n"
                      << "  Turnaround Time: " << process->getTurnaroundTime() << "\n"
                      << "  Waiting Time: " << process->getWaitingTime() << "\n\n";
        }
    }
}

Simulator::~Simulator() {
    if (verboseOutput.is_open()) {
        verboseOutput.close();
    }
}
