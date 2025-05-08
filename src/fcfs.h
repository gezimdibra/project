#ifndef FCFS_H
#define FCFS_H

#include <queue>
#include "scheduler.h"

// First Come First Serve Scheduler
class FCFSScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    
public:
    FCFSScheduler(int contextSwitchTime);
    
    // Implementation of abstract methods
    void addProcess(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> getNextProcess() override;
    bool shouldPreempt(std::shared_ptr<Process> newProcess) override;
    bool isPreemptive() const override { return false; }
    void updateWaitingTime(int timeElapsed) override;
    
    // FCFS specific methods
    size_t getReadyQueueSize() const { return readyQueue.size(); }
};

#endif // FCFS_H