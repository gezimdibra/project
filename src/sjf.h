#ifndef SJF_H
#define SJF_H

#include <vector>
#include <algorithm>
#include "scheduler.h"

// Shortest Job First Scheduler (Non-preemptive)
class SJFScheduler : public Scheduler {
private:
    std::vector<std::shared_ptr<Process>> readyQueue;
    
public:
    SJFScheduler(int contextSwitchTime);
    
    // Implementation of abstract methods
    void addProcess(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> getNextProcess() override;
    bool shouldPreempt(std::shared_ptr<Process> newProcess) override;
    bool isPreemptive() const override { return false; }
    void updateWaitingTime(int timeElapsed) override;
    
    // SJF specific methods
    size_t getReadyQueueSize() const { return readyQueue.size(); }
};

#endif // SJF_H