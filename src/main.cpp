#include <iostream>
#include <list>
#include <iterator>
#include <array>
#include "engine.h"

// SECTION: Algorithms (FCFS, SJF, MLFQ).
// std::list<OS_Scheduler_Simulator::Engine::Data_Time_Point> algorithm_FCFS(std::list<OS_Scheduler_Simulator::Engine::Process> data);

#ifdef _DEBUG
void test_routine();
#endif // _DEBUG


int main(void) {

#ifdef _DEBUG
    std::cout << "Running debugging version." << std::endl;
    test_routine();
#endif // DEBUG

    return 0;
}

#ifdef _DEBUG
void test_routine() {
    // Creating four processes.
    std::list<OS_Scheduler_Simulator::Engine::Process_Data> processes;

    std::vector<unsigned> bursts = { 5, 8, 3 };
    processes.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P1", bursts));

    bursts = { 4, 3, 5 };
    processes.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P2", bursts));

    bursts = { 8, 1, 2 };
    processes.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P3", bursts));

    bursts = { 3, 12, 4 };
    processes.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P4", bursts));

}
#endif // _DEBUG
