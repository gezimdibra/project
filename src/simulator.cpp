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

Simulator::~Simulator() {
    if (verboseOutput.is_open()) {
        verboseOutput.close();
    }
}

void Simulator::initialize(const std::vector<std::shared_ptr<Process>>& processList) {
    // Sort processes by arrival time for FCFS
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

void Simulator::runScheduler(std::shared_ptr<Scheduler> scheduler) {
    // Reset simulation state
    currentTime = 0;
    scheduler->setTotalTime(0);
    scheduler->clearCurrentProcess();
    
    // Clear event queue
    while (!eventQueue.empty()) {
        eventQueue.pop();
    }
    
    // Reset all processes and add initial events
    for (auto& process : processes) {
        process->setState(ProcessState::NEW);
        int arrivalTime = process->getArrivalTime();
        Event arrivalEvent(EventType::PROCESS_ARRIVAL, arrivalTime, process);
        eventQueue.push(arrivalEvent);
        scheduler->addToAllProcesses(process);
    }
    
    int totalCpuTime = 0;
    
    // Main event loop
    while (!eventQueue.empty()) {
        Event event = eventQueue.top();
        eventQueue.pop();
        
        // Update current time
        if (event.getTime() > currentTime) {
            int timeElapsed = event.getTime() - currentTime;
            
            // Update CPU busy time if a process is running
            if (scheduler->hasCpuProcess()) {
                scheduler->incrementCpuBusyTime(timeElapsed);
                totalCpuTime += timeElapsed;
                
                // Update RR time slice if applicable
                if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(scheduler)) {
                    rrScheduler->decrementTimeSlice(timeElapsed);
                }
            }
            
            // Update waiting time for processes in ready queue
            scheduler->updateWaitingTime(timeElapsed);
            currentTime = event.getTime();
        }
        
        // Process the event
        switch (event.getType()) {
            case EventType::PROCESS_ARRIVAL:
                processArrival(event, scheduler);
                break;
                
            case EventType::CPU_BURST_COMPLETION:
                processCPUBurstCompletion(event, scheduler);
                break;
                
            case EventType::IO_COMPLETION:
                processIOCompletion(event, scheduler);
                break;
                
            case EventType::TIMER_INTERRUPT:
                processTimerInterrupt(event, scheduler);
                break;
                
            case EventType::CONTEXT_SWITCH_COMPLETE:
                processContextSwitchComplete(event, scheduler);
                break;
        }
        
        // Check for RR time slice expiration
        if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(scheduler)) {
            if (rrScheduler->hasCpuProcess() && rrScheduler->isTimeSliceExpired()) {
                Event timerEvent(EventType::TIMER_INTERRUPT, currentTime, 
                               rrScheduler->getCurrentProcess());
                eventQueue.push(timerEvent);
            }
        }
    }
    
    // Update final statistics
    scheduler->setTotalTime(currentTime);
    
    // Calculate CPU utilization
    if (currentTime > 0) {
        double utilization = (static_cast<double>(totalCpuTime) / currentTime) * 100.0;
        scheduler->setCpuUtilization(utilization);
    }
}

void Simulator::processArrival(const Event& event, std::shared_ptr<Scheduler> scheduler) {
    auto process = event.getProcess();
    
    if (params.verboseMode) {
        logStateTransition(process, process->getState(), ProcessState::READY);
    }
    
    scheduler->addProcess(process);
    
    if (!scheduler->hasCpuProcess()) {
        scheduleNextEvent(scheduler);
    } else if (scheduler->isPreemptive()) {
        checkPreemption(process, scheduler);
    }
}

void Simulator::processCPUBurstCompletion(const Event& event, std::shared_ptr<Scheduler> scheduler) {
    auto process = event.getProcess();
    process->advanceBurst();
    
    if (process->getCurrentBurstIndex() >= process->getTotalBursts()) {
        // Process has completed all bursts
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::TERMINATED);
        }
        
        process->setState(ProcessState::TERMINATED);
        process->setFinishTime(currentTime);
        scheduler->clearCurrentProcess();
        scheduleNextEvent(scheduler);
    } else if (process->getCurrentBurst().type == BurstType::IO) {
        // Process needs I/O
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::BLOCKED);
        }
        
        process->setState(ProcessState::BLOCKED);
        int ioCompletionTime = currentTime + process->getCurrentBurst().duration;
        Event ioCompletionEvent(EventType::IO_COMPLETION, ioCompletionTime, process);
        eventQueue.push(ioCompletionEvent);
        
        scheduler->clearCurrentProcess();
        scheduleNextEvent(scheduler);
    }
}

void Simulator::processIOCompletion(const Event& event, std::shared_ptr<Scheduler> scheduler) {
    auto process = event.getProcess();
    process->advanceBurst();
    
    if (params.verboseMode) {
        logStateTransition(process, ProcessState::BLOCKED, ProcessState::READY);
    }
    
    scheduler->addProcess(process);
    
    if (!scheduler->hasCpuProcess()) {
        scheduleNextEvent(scheduler);
    } else if (scheduler->isPreemptive()) {
        checkPreemption(process, scheduler);
    }
}

void Simulator::processTimerInterrupt(const Event& event, std::shared_ptr<Scheduler> scheduler) {
    auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(scheduler);
    if (!rrScheduler) return;
    
    auto process = event.getProcess();
    
    if (rrScheduler->getCurrentProcess() == process) {
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::READY);
        }
        
        rrScheduler->addProcess(process);
        rrScheduler->clearCurrentProcess();
        contextSwitch(process, nullptr, scheduler);
    }
}

void Simulator::processContextSwitchComplete(const Event& event, std::shared_ptr<Scheduler> scheduler) {
    scheduleNextEvent(scheduler);
}

void Simulator::scheduleNextEvent(std::shared_ptr<Scheduler> scheduler) {
    if (scheduler->hasCpuProcess()) return;
    
    auto nextProcess = scheduler->getNextProcess();
    if (!nextProcess) return;
    
    contextSwitch(nullptr, nextProcess, scheduler);
}

void Simulator::scheduleProcess(std::shared_ptr<Process> process, std::shared_ptr<Scheduler> scheduler) {
    if (!process) return;
    
    if (params.verboseMode) {
        logStateTransition(process, ProcessState::READY, ProcessState::RUNNING);
    }
    
    process->setState(ProcessState::RUNNING);
    scheduler->setCurrentProcess(process);
    
    int remaining = process->getCurrentBurst().remaining;
    
    if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(scheduler)) {
        int timeSlice = rrScheduler->getCurrentTimeSlice();
        if (timeSlice < remaining) {
            remaining = timeSlice;
        }
    }
    
    int completionTime = currentTime + remaining;
    Event completionEvent(EventType::CPU_BURST_COMPLETION, completionTime, process);
    eventQueue.push(completionEvent);
    
    process->updateRemainingTime(remaining);
}

void Simulator::checkPreemption(std::shared_ptr<Process> newProcess, std::shared_ptr<Scheduler> scheduler) {
    if (!scheduler->isPreemptive()) return;
    
    auto currentProcess = scheduler->getCurrentProcess();
    
    if (scheduler->shouldPreempt(newProcess)) {
        if (params.verboseMode) {
            logStateTransition(currentProcess, ProcessState::RUNNING, ProcessState::READY);
        }
        
        scheduler->addProcess(currentProcess);
        contextSwitch(currentProcess, newProcess, scheduler);
    }
}

void Simulator::contextSwitch(std::shared_ptr<Process> oldProcess, std::shared_ptr<Process> newProcess,
                            std::shared_ptr<Scheduler> scheduler) {
    scheduler->clearCurrentProcess();
    scheduler->incrementContextSwitchCount();
    
    int completionTime = currentTime + processSwitchTime;
    Event completionEvent(EventType::CONTEXT_SWITCH_COMPLETE, completionTime, newProcess);
    eventQueue.push(completionEvent);
    
    if (newProcess) {
        scheduleProcess(newProcess, scheduler);
    }
}

void Simulator::logStateTransition(std::shared_ptr<Process> process, 
                                 ProcessState oldState, ProcessState newState) {
    if (!process) return;
    
    std::string message = "At time " + std::to_string(currentTime) + ": Process " + 
                         std::to_string(process->getId()) + " moves from " + 
                         ProcessStateStr[static_cast<int>(oldState)] + " to " + 
                         ProcessStateStr[static_cast<int>(newState)];
    
    std::cout << message << std::endl;
    
    if (verboseOutput.is_open()) {
        verboseOutput << message << std::endl;
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
