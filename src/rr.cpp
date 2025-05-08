#include "rr.h"
#include <sstream>

RRScheduler::RRScheduler(int contextSwitchTime, int quantum)
    : Scheduler("Round Robin", contextSwitchTime),
      timeQuantum(quantum),
      currentTimeSlice(quantum) {
    
    // Append time quantum to the name
    std::stringstream ss;
    ss << "Round Robin (quantum=" << quantum << ")";
    name = ss.str();
}

void RRScheduler::addProcess(std::shared_ptr<Process> process) {
    readyQueue.push(process);
    process->setState(ProcessState::READY);
}

std::shared_ptr<Process> RRScheduler::getNextProcess() {
    if (readyQueue.empty()) {
        return nullptr;
    }
    
    auto process = readyQueue.front();
    readyQueue.pop();
    
    // Reset time slice for the new process
    resetTimeSlice();
    
    return process;
}

bool RRScheduler::shouldPreempt(std::shared_ptr<Process> newProcess) {
    // RR preempts when time slice expires, but arrival of new process doesn't cause preemption
    return false;
}

void RRScheduler::updateWaitingTime(int timeElapsed) {
    // In RR, we update waiting time for all processes in the ready queue
    size_t queueSize = readyQueue.size();
    std::queue<std::shared_ptr<Process>> tempQueue;
    
    // Process each element in the queue
    for (size_t i = 0; i < queueSize; i++) {
        auto process = readyQueue.front();
        readyQueue.pop();
        
        // Update waiting time
        process->incrementWaitingTime(timeElapsed);
        
        // Put it back
        tempQueue.push(process);
    }
    
    // Restore the queue
    readyQueue = tempQueue;
}