#ifndef SRTN_H
#define SRTN_H

#include <vector>
#include <algorithm>
#include "scheduler.h"

// Shortest Remaining Time Next Scheduler (Preemptive)
class SRTNScheduler : public Scheduler {
private:
    std::vector<std::shared_ptr<Process>> readyQueue;
    
public:
    SRTNScheduler(int contextSwitchTime);
    
    // Implementation of abstract methods
    void addProcess(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> getNextProcess() override;
    bool shouldPreempt(std::shared_ptr<Process> newProcess) override;
    bool isPreemptive() const override { return true; }
    void updateWaitingTime(int timeElapsed) override;
    
    // SRTN specific methods
    size_t getReadyQueueSize() const { return readyQueue.size(); }
};

#endif // SRTN_H