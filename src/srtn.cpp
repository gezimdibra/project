#include "srtn.h"

SRTNScheduler::SRTNScheduler(int contextSwitchTime)
    : Scheduler("Shortest Remaining Time Next", contextSwitchTime) {
}

void SRTNScheduler::addProcess(std::shared_ptr<Process> process) {
    readyQueue.push_back(process);
    process->setState(ProcessState::READY);
}

std::shared_ptr<Process> SRTNScheduler::getNextProcess() {
    if (readyQueue.empty()) {
        return nullptr;
    }
    
    // Find the process with the shortest remaining time
    auto shortestIt = std::min_element(
        readyQueue.begin(), readyQueue.end(),
        [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
            return a->getRemainingTime() < b->getRemainingTime();
        }
    );
    
    std::shared_ptr<Process> shortestProcess = *shortestIt;
    readyQueue.erase(shortestIt);
    
    return shortestProcess;
}

bool SRTNScheduler::shouldPreempt(std::shared_ptr<Process> newProcess) {
    // If there's no current process, no need to preempt
    if (!hasCpuProcess()) {
        return false;
    }
    
    // Preempt if the new process has a shorter remaining time
    return newProcess->getRemainingTime() < currentProcess->getRemainingTime();
}

void SRTNScheduler::updateWaitingTime(int timeElapsed) {
    // Update waiting time for all processes in the ready queue
    for (auto& process : readyQueue) {
        process->incrementWaitingTime(timeElapsed);
    }
}