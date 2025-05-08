#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <string>

// Process states
enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// String representation of process states
const std::string ProcessStateStr[] = {
    "new",
    "ready",
    "running",
    "blocked",
    "terminated"
};

// Burst types
enum class BurstType {
    CPU,
    IO
};

// Burst structure
struct Burst {
    BurstType type;
    int duration;
    int remaining;
    
    Burst(BurstType t, int d) : type(t), duration(d), remaining(d) {}
};

// Process structure
class Process {
private:
    int id;
    int arrivalTime;
    std::vector<Burst> bursts;
    int currentBurst;
    ProcessState state;
    
    // Statistics
    int serviceTime;     // Total CPU time
    int ioTime;          // Total I/O time
    int finishTime;      // Time when process terminated
    int turnaroundTime;  // Finish time - arrival time
    int waitingTime;     // Time spent in ready queue
    
public:
    Process(int pid, int arrival);
    
    // Add burst to the process
    void addCPUBurst(int duration);
    void addIOBurst(int duration);
    
    // Getters
    int getId() const { return id; }
    int getArrivalTime() const { return arrivalTime; }
    ProcessState getState() const { return state; }
    int getCurrentBurstIndex() const { return currentBurst; }
    const Burst& getCurrentBurst() const { return bursts[currentBurst]; }
    Burst& getCurrentBurstRef() { return bursts[currentBurst]; }
    bool hasNextBurst() const { return currentBurst + 1 < (int)bursts.size(); }
    const Burst& getNextBurst() const { return bursts[currentBurst + 1]; }
    int getRemainingCPUTime() const;
    int getRemainingTime() const;
    int getTotalBursts() const { return bursts.size(); }
    int getNextCPUBurstTime() const;
    
    // State transitions
    void setState(ProcessState newState);
    void advanceBurst();
    bool isCompleted() const { return state == ProcessState::TERMINATED; }
    
    // Update remaining time of current burst
    void updateRemainingTime(int time);
    
    // Statistics management
    void setFinishTime(int time);
    int getServiceTime() const { return serviceTime; }
    int getIOTime() const { return ioTime; }
    int getFinishTime() const { return finishTime; }
    int getTurnaroundTime() const { return turnaroundTime; }
    int getWaitingTime() const { return waitingTime; }
    void calculateStatistics();
    void incrementWaitingTime(int time) { waitingTime += time; }
};

#endif // PROCESS_H