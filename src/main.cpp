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

    // First breakpoint for debugging. INFO: Testing the Process_Data class.
    std::list<OS_Scheduler_Simulator::Engine::Process_Data>::iterator it(processes.begin());
    
    // Running.
    OS_Scheduler_Simulator::Engine::Running_Process running(&(* it));
    running.send_to_cpu();

    // Ready list.
    it = std::next(it);
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> ready_list;
    ready_list.push_back(&(*it));

    it = std::next(it);
    ready_list.push_back(&(*it));

    it = std::next(it);
    ready_list.push_back(&(*it));

    // Second breakpoint for debugging. INFO: Testing constructor of Running_Process class.

    // After first burst completed.
    constexpr unsigned time{ 5 };

    OS_Scheduler_Simulator::Engine::Running_Process temp = running.get_next_process_state(time);
    for (OS_Scheduler_Simulator::Engine::Running_Process& process : ready_list) process.get_next_process_state(time);
    
    ready_list.push_back(temp);
    
    running = ready_list.front();
    running.send_to_cpu();
    ready_list.pop_front();

    // Third breakpoint for debugging. INFO: Testing one iteration in a FCFS algorithm fashion.
}
#endif // _DEBUG
