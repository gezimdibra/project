#include "scheduler.h"

Scheduler::Scheduler(const std::string& schedulerName, int switchTime)
    : totalTime(0),
      cpuBusyTime(0),
      contextSwitchTime(switchTime),
      contextSwitchCount(0),
      isCpuBusy(false),
      name(schedulerName) {
}

void Scheduler::setCurrentProcess(std::shared_ptr<Process> process) {
    currentProcess = process;
    isCpuBusy = (process != nullptr);
}

std::shared_ptr<Process> Scheduler::getCurrentProcess() const {
    return currentProcess;
}

bool Scheduler::hasCpuProcess() const {
    return isCpuBusy && currentProcess != nullptr;
}

void Scheduler::clearCurrentProcess() {
    currentProcess = nullptr;
    isCpuBusy = false;
}

double Scheduler::getCpuUtilization() const {
    if (totalTime == 0) return 0.0;
    return static_cast<double>(cpuBusyTime) / totalTime * 100.0;
}

void Scheduler::addToAllProcesses(std::shared_ptr<Process> process) {
    allProcesses.push_back(process);
}