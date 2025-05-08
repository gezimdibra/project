#include <iostream>
#include <random>
#include <ctime>
#include <vector>

// Process structure for generator
struct GenProcess {
    int id;
    int arrivalTime;
    std::vector<int> cpuBursts;
    std::vector<int> ioBursts;
};

int main() {
    // Seed random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Constants
    const int NUM_PROCESSES = 50;
    const int CONTEXT_SWITCH_TIME = 5;
    const int MEAN_ARRIVAL_INTERVAL = 50;
    const int MEAN_CPU_BURSTS = 20;
    const int MIN_CPU_BURST = 5;
    const int MAX_CPU_BURST = 500;
    const int MIN_IO_BURST = 30;
    const int MAX_IO_BURST = 1000;
    
    // Distributions
    std::exponential_distribution<> arrivalDist(1.0 / MEAN_ARRIVAL_INTERVAL);
    std::poisson_distribution<> burstCountDist(MEAN_CPU_BURSTS);
    std::uniform_int_distribution<> cpuBurstDist(MIN_CPU_BURST, MAX_CPU_BURST);
    std::uniform_int_distribution<> ioBurstDist(MIN_IO_BURST, MAX_IO_BURST);
    
    // Generate processes
    std::vector<GenProcess> processes;
    
    int currentArrivalTime = 0;
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        GenProcess process;
        process.id = i + 1;
        
        // Calculate arrival time (exponential distribution)
        int arrivalInterval = static_cast<int>(arrivalDist(gen));
        currentArrivalTime += arrivalInterval;
        process.arrivalTime = currentArrivalTime;
        
        // Generate number of CPU bursts (Poisson distribution, at least 1)
        int numBursts = std::max(1, static_cast<int>(burstCountDist(gen)));
        
        // Generate CPU and I/O bursts
        for (int j = 0; j < numBursts; j++) {
            // CPU burst
            int cpuBurst = cpuBurstDist(gen);
            process.cpuBursts.push_back(cpuBurst);
            
            // I/O burst (except for last CPU burst)
            if (j < numBursts - 1) {
                int ioBurst = ioBurstDist(gen);
                process.ioBursts.push_back(ioBurst);
            }
        }
        
        processes.push_back(process);
    }
    
    // Output in the required format
    std::cout << NUM_PROCESSES << " " << CONTEXT_SWITCH_TIME << std::endl;
    
    for (const auto& process : processes) {
        std::cout << process.id << " " << process.arrivalTime << " " << process.cpuBursts.size() << std::endl;
        
        for (size_t i = 0; i < process.cpuBursts.size(); i++) {
            std::cout << i + 1 << " " << process.cpuBursts[i];
            
            if (i < process.ioBursts.size()) {
                std::cout << " " << process.ioBursts[i];
            }
            
            std::cout << std::endl;
        }
    }
    
    return 0;
}