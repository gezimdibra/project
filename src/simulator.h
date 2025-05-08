#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <memory>
#include <vector>
#include <fstream>
#include "process.h"
#include "event.h"
#include "scheduler.h"

// Forward declarations
class FCFSScheduler;
class SJFScheduler;
class SRTNScheduler;
class RRScheduler;

// Simulation parameters
struct SimulationParams {
    bool detailedMode;
    bool verboseMode;
    std::string algorithm;
    
    SimulationParams() 
        : detailedMode(false), verboseMode(false), algorithm("ALL") {}
};

// Simulator class
class Simulator {
private:
    // Current simulation time
    int currentTime;
    
    // Event queue
    EventQueue eventQueue;
    
    // Schedulers
    std::shared_ptr<FCFSScheduler> fcfsScheduler;
    std::shared_ptr<SJFScheduler> sjfScheduler;
    std::shared_ptr<SRTNScheduler> srtnScheduler;
    std::shared_ptr<RRScheduler> rr10Scheduler;
    std::shared_ptr<RRScheduler> rr50Scheduler;
    std::shared_ptr<RRScheduler> rr100Scheduler;
    
    // Active scheduler (based on selected algorithm)
    std::shared_ptr<Scheduler> activeScheduler;
    
    // Processes
    std::vector<std::shared_ptr<Process>> processes;
    
    // Process switch time (context switch overhead)
    int processSwitchTime;
    
    // Simulation parameters
    SimulationParams params;
    
    // Verbose output stream
    std::ofstream verboseOutput;
    
    // Helper methods
    void runScheduler(std::shared_ptr<Scheduler> scheduler);
    void processArrival(const Event& event, std::shared_ptr<Scheduler> scheduler);
    void processCPUBurstCompletion(const Event& event, std::shared_ptr<Scheduler> scheduler);
    void processIOCompletion(const Event& event, std::shared_ptr<Scheduler> scheduler);
    void processTimerInterrupt(const Event& event, std::shared_ptr<Scheduler> scheduler);
    void processContextSwitchComplete(const Event& event, std::shared_ptr<Scheduler> scheduler);
    void scheduleNextEvent(std::shared_ptr<Scheduler> scheduler);
    void scheduleProcess(std::shared_ptr<Process> process, std::shared_ptr<Scheduler> scheduler);
    void checkPreemption(std::shared_ptr<Process> newProcess, std::shared_ptr<Scheduler> scheduler);
    void contextSwitch(std::shared_ptr<Process> oldProcess, std::shared_ptr<Process> newProcess,
                      std::shared_ptr<Scheduler> scheduler);
    void logStateTransition(std::shared_ptr<Process> process, ProcessState oldState, ProcessState newState);
    void outputSchedulerResults(std::shared_ptr<Scheduler> scheduler) const;
    
public:
    Simulator(int switchTime);
    ~Simulator();
    
    // Initialize the simulator with processes
    void initialize(const std::vector<std::shared_ptr<Process>>& processList);
    
    // Set simulation parameters
    void setParams(const SimulationParams& simulationParams);
    
    // Run the simulation
    void run();
    
    // Output results
    void outputResults() const;
    
    // Get active scheduler (for testing/debugging)
    std::shared_ptr<Scheduler> getActiveScheduler() const { return activeScheduler; }
};

#endif // SIMULATOR_H
