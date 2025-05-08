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
    
    // Reset all processes and add to scheduler
    for (auto& process : processes) {
        process->setState(ProcessState::NEW);
        Event arrivalEvent(EventType::PROCESS_ARRIVAL, process->getArrivalTime(), process);
        eventQueue.push(arrivalEvent);
        scheduler->addToAllProcesses(process);
    }
    
    // Main event loop
    while (!eventQueue.empty()) {
        Event event = eventQueue.top();
        eventQueue.pop();
        
        // Update time and statistics
        int timeElapsed = event.getTime() - currentTime;
        if (timeElapsed > 0) {
            scheduler->updateWaitingTime(timeElapsed);
            
            if (scheduler->hasCpuProcess()) {
                scheduler->incrementCpuBusyTime(timeElapsed);
                
                // For Round Robin, update time slice
                if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(scheduler)) {
                    rrScheduler->decrementTimeSlice(timeElapsed);
                }
            }
        }
        
        currentTime = event.getTime();
        
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
        
        // Check for timer interrupt in Round Robin
        if (auto rrScheduler = std::dynamic_pointer_cast<RRScheduler>(scheduler)) {
            if (rrScheduler->hasCpuProcess() && rrScheduler->isTimeSliceExpired()) {
                Event timerEvent(EventType::TIMER_INTERRUPT, currentTime, 
                               rrScheduler->getCurrentProcess());
                eventQueue.push(timerEvent);
            }
        }
    }
    
    // Set final statistics
    scheduler->setTotalTime(currentTime);
    
    // Update finish times for all processes
    for (auto& process : scheduler->getAllProcesses()) {
        if (!process->isCompleted()) {
            process->setState(ProcessState::TERMINATED);
        }
        // Always set finish time to ensure statistics are calculated
        process->setFinishTime(currentTime);
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
        if (params.verboseMode) {
            logStateTransition(process, ProcessState::RUNNING, ProcessState::TERMINATED);
        }
        
        process->setState(ProcessState::TERMINATED);
        process->setFinishTime(currentTime);
        scheduler->clearCurrentProcess();
        scheduleNextEvent(scheduler);
    } else if (process->getCurrentBurst().type == BurstType::IO) {
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
    std::cout << scheduler->getName() << ": Total Time required is " 
              << scheduler->getTotalTime() << " time units\n"
              << "CPU Utilization is " 
              << std::fixed << std::setprecision(0) << scheduler->getCpuUtilization() << "%\n";
    
    if (params.detailedMode) {
        for (const auto& process : scheduler->getAllProcesses()) {
            std::cout << "Process " << process->getId() << ":\n"
                      << "  arrival time: " << process->getArrivalTime() << "\n"
                      << "  service time: " << process->getServiceTime() << " units\n"
                      << "  I/O time: " << process->getIOTime() << " units\n"
                      << "  turnaround time: " << process->getTurnaroundTime() << " units\n"
                      << "  finish time: " << process->getFinishTime() << " units\n";
        }
    }
}
