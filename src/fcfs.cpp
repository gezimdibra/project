#include "fcfs.h"

FCFSScheduler::FCFSScheduler(int contextSwitchTime)
    : Scheduler("First Come First Serve", contextSwitchTime) {
}

void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    readyQueue.push(process);
    process->setState(ProcessState::READY);
}

std::shared_ptr<Process> FCFSScheduler::getNextProcess() {
    if (readyQueue.empty()) {
        return nullptr;
    }
    
    auto process = readyQueue.front();
    readyQueue.pop();
    return process;
}

bool FCFSScheduler::shouldPreempt(std::shared_ptr<Process> newProcess) {
    // FCFS is non-preemptive, so always return false
    return false;
}

void FCFSScheduler::updateWaitingTime(int timeElapsed) {
    // In FCFS, we update waiting time for all processes in the ready queue
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