#include "sjf.h"

SJFScheduler::SJFScheduler(int contextSwitchTime)
    : Scheduler("Shortest Job First", contextSwitchTime) {
}

void SJFScheduler::addProcess(std::shared_ptr<Process> process) {
    readyQueue.push_back(process);
    process->setState(ProcessState::READY);
}

std::shared_ptr<Process> SJFScheduler::getNextProcess() {
    if (readyQueue.empty()) {
        return nullptr;
    }
    
    // Find the process with the shortest next CPU burst
    auto shortestIt = std::min_element(
        readyQueue.begin(), readyQueue.end(),
        [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
            return a->getNextCPUBurstTime() < b->getNextCPUBurstTime();
        }
    );
    
    std::shared_ptr<Process> shortestProcess = *shortestIt;
    readyQueue.erase(shortestIt);
    
    return shortestProcess;
}

bool SJFScheduler::shouldPreempt(std::shared_ptr<Process> newProcess) {
    // SJF is non-preemptive, so always return false
    return false;
}

void SJFScheduler::updateWaitingTime(int timeElapsed) {
    // Update waiting time for all processes in the ready queue
    for (auto& process : readyQueue) {
        process->incrementWaitingTime(timeElapsed);
    }
}