#include "process.h"

Process::Process(int pid, int arrival) :
    id(pid),
    arrivalTime(arrival),
    currentBurst(0),
    state(ProcessState::NEW),
    serviceTime(0),
    ioTime(0),
    finishTime(0),
    turnaroundTime(0),
    waitingTime(0) {
}

void Process::addCPUBurst(int duration) {
    bursts.push_back(Burst(BurstType::CPU, duration));
    serviceTime += duration;
}

void Process::addIOBurst(int duration) {
    bursts.push_back(Burst(BurstType::IO, duration));
    ioTime += duration;
}

void Process::setState(ProcessState newState) {
    state = newState;
}

void Process::advanceBurst() {
    if (currentBurst < static_cast<int>(bursts.size()) - 1) {
        currentBurst++;
    }
}

void Process::updateRemainingTime(int time) {
    if (currentBurst < static_cast<int>(bursts.size())) {
        bursts[currentBurst].remaining -= time;
        if (bursts[currentBurst].remaining < 0) {
            bursts[currentBurst].remaining = 0;
        }
    }
}

int Process::getRemainingCPUTime() const {
    int remaining = 0;
    for (size_t i = currentBurst; i < bursts.size(); i++) {
        if (bursts[i].type == BurstType::CPU) {
            remaining += (i == static_cast<size_t>(currentBurst)) ? bursts[i].remaining : bursts[i].duration;
        }
    }
    return remaining;
}

int Process::getRemainingTime() const {
    if (currentBurst >= static_cast<int>(bursts.size())) {
        return 0;
    }
    return bursts[currentBurst].remaining;
}

int Process::getNextCPUBurstTime() const {
    if (currentBurst < static_cast<int>(bursts.size()) && 
        bursts[currentBurst].type == BurstType::CPU) {
        return bursts[currentBurst].remaining;
    }
    
    for (size_t i = currentBurst + 1; i < bursts.size(); i++) {
        if (bursts[i].type == BurstType::CPU) {
            return bursts[i].duration;
        }
    }
    
    return 0; // No more CPU bursts
}

void Process::setFinishTime(int time) {
    // Only set finish time if we haven't already
    if (finishTime == 0) {
        finishTime = time;
        calculateStatistics();
    }
}

void Process::calculateStatistics() {
    // Only calculate if we have a valid finish time
    if (finishTime > 0) {
        turnaroundTime = finishTime - arrivalTime;
        waitingTime = turnaroundTime - (serviceTime + ioTime);
        
        if (waitingTime < 0) {
            waitingTime = 0;
        }
    }
}

void Process::incrementWaitingTime(int time) {
    if (state == ProcessState::READY) {
        waitingTime += time;
    }
}
