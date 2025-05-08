#ifndef RR_H
#define RR_H

#include <queue>
#include "scheduler.h"

// Round Robin Scheduler
class RRScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    int timeQuantum;
    int currentTimeSlice;
    
public:
    RRScheduler(int contextSwitchTime, int quantum);
    
    // Implementation of abstract methods
    void addProcess(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> getNextProcess() override;
    bool shouldPreempt(std::shared_ptr<Process> newProcess) override;
    bool isPreemptive() const override { return true; }
    void updateWaitingTime(int timeElapsed) override;
    
    // RR specific methods
    int getTimeQuantum() const { return timeQuantum; }
    void resetTimeSlice() { currentTimeSlice = timeQuantum; }
    void decrementTimeSlice(int time) { currentTimeSlice -= time; }
    int getCurrentTimeSlice() const { return currentTimeSlice; }
    bool isTimeSliceExpired() const { return currentTimeSlice <= 0; }
    size_t getReadyQueueSize() const { return readyQueue.size(); }
};

#endif // RR_H