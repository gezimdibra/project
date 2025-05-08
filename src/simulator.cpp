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
    processes = processList;
    
    // Schedule process arrivals
    for (const auto& process : processes) {
        Event arrivalEvent(EventType::PROCESS_ARRIVAL, process->getArrivalTime(), process);
        eventQueue.push(arrivalEvent);
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
    // Reset for new run
    currentTime = 0;
    
    // Clear event queue for new run
    while (!eventQueue.empty()) {
        eventQueue.pop();
    }
    
    // Re-initialize with process arrivals
    for (const auto& process : processes) {
        // Reset process state
        process->setState(ProcessState::NEW);
        
        // Schedule arrival event
        Event arrivalEvent(EventType::PROCESS_ARRIVAL, process->getArrivalTime(), process);
        eventQueue.push(arrivalEvent);
        
        // Add to active scheduler's process list
        activeScheduler->addToAllProcesses(process);
    }
    
    // Main event loop
    while (!eventQueue.empty()) {
        // Get next event
        Event event = eventQueue.top();
        eventQueue.pop();
        
        // Update current time
        int timeElapsed = event.getTime() - currentTime;
        if (timeElapsed > 0) {
            // Update waiting time for processes in ready queue
            activeScheduler->updateWaitingTime(timeElapsed);
            
            // If CPU is busy, update CPU busy time
            if (activeScheduler->hasCpuProcess()) {
                activeScheduler->incrementCpuBusyTime(timeElapsed);
                
                // If using Round Robin, update time slice
                if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(activeScheduler)) {
                    rrScheduler->decrementTimeSlice(timeElapsed);
                }
            }
        }
        
        currentTime = event.getTime();
        
        // Process the event based on its type
        switch (event.getType()) {
            case EventType::PROCESS_ARRIVAL:
                processArrival(event);
                break;
            case EventType::CPU_BURST_COMPLETION:
                processCPUBurstCompletion(event);
                break;
            case EventType::IO_COMPLETION:
                processIOCompletion(event);
                break;
            case EventType::TIMER_INTERRUPT:
                processTimerInterrupt(event);
                break;
            case EventType::CONTEXT_SWITCH_COMPLETE:
                processContextSwitchComplete(event);
                break;
        }
        
        // Check for timer interrupt for Round Robin
        if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(activeScheduler)) {
            if (rrScheduler->hasCpuProcess() && rrScheduler->isTimeSliceExpired()) {
                // Schedule a timer interrupt
                Event timerEvent(EventType::TIMER_INTERRUPT, currentTime, 
                                rrScheduler->getCurrentProcess());
                eventQueue.push(timerEvent);
            }
        }
    }
    
    // Set total time for the scheduler
    activeScheduler->setTotalTime(currentTime);
}

void Simulator::processArrival(const Event& event) {
    auto process = event.getProcess();
    
    if (params.verboseMode) {
        logStateTransition(process, process->getState(), ProcessState::READY);
    }
    
    // Add process to scheduler
    activeScheduler->addProcess(process);
    
    // If CPU is idle, schedule the process
    if (!activeScheduler->hasCpuProcess()) {
        scheduleNextEvent();
    } else if (activeScheduler->isPreemptive()) {
        // Check for preemption
        checkPreemption(process);
    }
}

void Simulator::processCPUBurstCompletion(const Event& event) {
    auto process = event.getProcess();
    
    // Update process state
    process->advanceBurst();
    
    if (process->getCurrentBurstIndex() >= process->getTotalBursts()) {
        // Process has completed
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::TERMINATED);
        }
        
        process->setState(ProcessState::TERMINATED);
        process->setFinishTime(currentTime);
        
        // Clear current process
        activeScheduler->clearCurrentProcess();
        
        // Schedule next process
        scheduleNextEvent();
    } else if (process->getCurrentBurst().type == BurstType::IO) {
        // Process is moving to IO
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::BLOCKED);
        }
        
        process->setState(ProcessState::BLOCKED);
        
        // Schedule IO completion
        int ioCompletionTime = currentTime + process->getCurrentBurst().duration;
        Event ioCompletionEvent(EventType::IO_COMPLETION, ioCompletionTime, process);
        eventQueue.push(ioCompletionEvent);
        
        // Clear current process
        activeScheduler->clearCurrentProcess();
        
        // Schedule next process
        scheduleNextEvent();
    }
}

void Simulator::processIOCompletion(const Event& event) {
    auto process = event.getProcess();
    
    // Update process state
    process->advanceBurst();
    
    if (params.verboseMode) {
        logStateTransition(process, ProcessState::BLOCKED, ProcessState::READY);
    }
    
    // Add process back to scheduler
    activeScheduler->addProcess(process);
    
    // If CPU is idle, schedule the process
    if (!activeScheduler->hasCpuProcess()) {
        scheduleNextEvent();
    } else if (activeScheduler->isPreemptive()) {
        // Check for preemption
        checkPreemption(process);
    }
}

void Simulator::processTimerInterrupt(const Event& event) {
    // Only applies to Round Robin
    auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(activeScheduler);
    if (!rrScheduler) {
        return;
    }
    
    auto process = event.getProcess();
    
    // Only preempt if this process is still running
    if (rrScheduler->getCurrentProcess() == process) {
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::READY);
        }
        
        // Add process back to scheduler
        rrScheduler->addProcess(process);
        
        // Clear current process
        rrScheduler->clearCurrentProcess();
        
        // Schedule context switch
        contextSwitch(process, nullptr);
    }
}

void Simulator::processContextSwitchComplete(const Event& event) {
    // Schedule next process
    scheduleNextEvent();
}

void Simulator::scheduleNextEvent() {
    // If already have a running process, nothing to do
    if (activeScheduler->hasCpuProcess()) {
        return;
    }
    
    // Get next process from scheduler
    auto nextProcess = activeScheduler->getNextProcess();
    if (!nextProcess) {
        return; // No process available
    }
    
    // Schedule context switch
    contextSwitch(nullptr, nextProcess);
}

void Simulator::scheduleProcess(std::shared_ptr<Process> process) {
    if (!process) {
        return;
    }
    
    if (params.verboseMode) {
        logStateTransition(process, ProcessState::READY, ProcessState::RUNNING);
    }
    
    // Set process state to running
    process->setState(ProcessState::RUNNING);
    
    // Set as current process
    activeScheduler->setCurrentProcess(process);
    
    // Schedule CPU burst completion
    int remaining = process->getCurrentBurst().remaining;
    
    // For Round Robin, we need to consider the time slice
    if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(activeScheduler)) {
        int timeSlice = rrScheduler->getCurrentTimeSlice();
        if (timeSlice < remaining) {
            // Time slice will expire before burst completes
            remaining = timeSlice;
        }
    }
    
    // Schedule the event
    int completionTime = currentTime + remaining;
    Event completionEvent(EventType::CPU_BURST_COMPLETION, completionTime, process);
    eventQueue.push(completionEvent);
    
    // Update remaining time
    process->updateRemainingTime(remaining);
}

void Simulator::checkPreemption(std::shared_ptr<Process> newProcess) {
    if (!activeScheduler->isPreemptive()) {
        return;
    }
    
    auto currentProcess = activeScheduler->getCurrentProcess();
    
    // Check if we should preempt
    if (activeScheduler->shouldPreempt(newProcess)) {
        // Find and remove the completion event for the current process
        // (This is a simplification - in a real implementation, we'd need to
        // search the event queue and remove the event, but that's complex in C++)
        
        // Update remaining time for current process
        // (again, this is simplified)
        
        if (params.verboseMode) {
            logStateTransition(currentProcess, ProcessState::RUNNING, ProcessState::READY);
        }
        
        // Add current process back to scheduler
        activeScheduler->addProcess(currentProcess);
        
        // Schedule context switch
        contextSwitch(currentProcess, newProcess);
    }
}

void Simulator::contextSwitch(std::shared_ptr<Process> oldProcess, std::shared_ptr<Process> newProcess) {
    // Clear current process
    activeScheduler->clearCurrentProcess();
    
    // Increment context switch count
    activeScheduler->incrementContextSwitchCount();
    
    // Schedule context switch completion
    int completionTime = currentTime + processSwitchTime;
    Event completionEvent(EventType::CONTEXT_SWITCH_COMPLETE, completionTime, newProcess);
    eventQueue.push(completionEvent);
    
    // When context switch completes, the new process will be scheduled
    if (newProcess) {
        // Pre-schedule the process
        scheduleProcess(newProcess);
    }
}

void Simulator::logStateTransition(std::shared_ptr<Process> process, 
                                  ProcessState oldState, ProcessState newState) {
    if (!process) {
        return;
    }
    
    // Log to console
    if (params.verboseMode) {
        std::cout << "At time " << currentTime << ": Process " << process->getId()
                  << " moves from " << ProcessStateStr[static_cast<int>(oldState)]
                  << " to " << ProcessStateStr[static_cast<int>(newState)] << std::endl;
        
        // Log to file if open
        if (verboseOutput.is_open()) {
            verboseOutput << "At time " << currentTime << ": Process " << process->getId()
                          << " moves from " << ProcessStateStr[static_cast<int>(oldState)]
                          << " to " << ProcessStateStr[static_cast<int>(newState)] << std::endl;
        }
    }
}

void Simulator::outputResults() const {
    // Output based on mode
    if (params.algorithm == "ALL") {
        // Default mode - show all algorithms
        std::cout << "First Come First Serve: Total Time required is " 
                  << fcfsScheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << fcfsScheduler->getCpuUtilization() << "%\n";
        
        std::cout << "Shortest Job First: Total Time required is " 
                  << sjfScheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << sjfScheduler->getCpuUtilization() << "%\n";
        
        std::cout << "Shortest Remaining Time Next: Total Time required is " 
                  << srtnScheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << srtnScheduler->getCpuUtilization() << "%\n";
        
        std::cout << "Round Robin (quantum=10): Total Time required is " 
                  << rr10Scheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << rr10Scheduler->getCpuUtilization() << "%\n";
        
        std::cout << "Round Robin (quantum=50): Total Time required is " 
                  << rr50Scheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << rr50Scheduler->getCpuUtilization() << "%\n";
        
        std::cout << "Round Robin (quantum=100): Total Time required is " 
                  << rr100Scheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << rr100Scheduler->getCpuUtilization() << "%\n";
        
        if (params.detailedMode) {
            // Detailed mode for all algorithms
            // (This would be too verbose, so we'd likely want to output to files)
        }
    } else {
        // Single algorithm mode
        std::cout << activeScheduler->getName() << ": Total Time required is " 
                  << activeScheduler->getTotalTime() << " time units\n"
                  << "CPU Utilization is " 
                  << std::fixed << std::setprecision(0) << activeScheduler->getCpuUtilization() << "%\n";
        
        if (params.detailedMode) {
            // Detailed information mode
            for (const auto& process : activeScheduler->getAllProcesses()) {
                std::cout << "Process " << process->getId() << ":\n"
                          << "  arrival time: " << process->getArrivalTime() << "\n"
                          << "  service time: " << process->getServiceTime() << " units\n"
                          << "  I/O time: " << process->getIOTime() << " units\n"
                          << "  turnaround time: " << process->getTurnaroundTime() << " units\n"
                          << "  finish time: " << process->getFinishTime() << " units\n";
            }
        }
    }
}