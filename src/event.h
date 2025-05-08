#ifndef EVENT_H
#define EVENT_H

#include <queue>
#include <vector>
#include <memory>
#include "process.h"

// Event types
enum class EventType {
    PROCESS_ARRIVAL,
    CPU_BURST_COMPLETION,
    IO_COMPLETION,
    TIMER_INTERRUPT,
    CONTEXT_SWITCH_COMPLETE
};

// String representation of event types
const std::string EventTypeStr[] = {
    "Process Arrival",
    "CPU Burst Completion",
    "IO Completion",
    "Timer Interrupt",
    "Context Switch Complete"
};

// Event structure
class Event {
private:
    EventType type;
    int time;
    std::shared_ptr<Process> process;
    
public:
    Event(EventType t, int timeStamp, std::shared_ptr<Process> p = nullptr)
        : type(t), time(timeStamp), process(p) {}
    
    // Getters
    EventType getType() const { return type; }
    int getTime() const { return time; }
    std::shared_ptr<Process> getProcess() const { return process; }
    
    // Compare events for priority queue (based on time)
    bool operator>(const Event& other) const {
        return time > other.time;
    }
};

// Event queue - priority queue based on event time
typedef std::priority_queue<Event, std::vector<Event>, std::greater<Event>> EventQueue;

#endif // EVENT_H