#include <iostream>
#include <list>
#include <iterator>
#include <array>
#include "engine.h"

// SECTION: Algorithms (FCFS, SJF, MLFQ).
// std::list<OS_Scheduler_Simulator::Engine::Data_Time_Point> algorithm_FCFS(std::list<OS_Scheduler_Simulator::Engine::Process> data);

#ifdef _DEBUG
void test_basic_process_structures();
void test_data_points();
#endif // _DEBUG


int main(void) {

#ifdef _DEBUG
    std::cout << "Running debugging version." << std::endl;
    test_basic_process_structures();
    test_data_points();
#endif // DEBUG

    return 0;
}

#ifdef _DEBUG
void test_basic_process_structures() {
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

// Demonstration of the FCFS algorithm.
void test_data_points() {
    std::list<OS_Scheduler_Simulator::Engine::Process_Data> starting_list;

    std::vector<unsigned> bursts = { 5, 8, 3 };
    starting_list.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P1", bursts));

    bursts = { 4, 3, 5 };
    starting_list.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P2", bursts));

    bursts = { 8, 1, 2 };
    starting_list.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P3", bursts));

    bursts = { 3, 12, 4 };
    starting_list.push_back(OS_Scheduler_Simulator::Engine::Process_Data("P4", bursts));

    OS_Scheduler_Simulator::Engine::Data_Point* current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(starting_list);

    // Breakpoint 1: Check if initial Data_Point is correct (everything is in the waiting_list).
    
    // Demonstration of a First Come First Serve algorithm.
    std::list<OS_Scheduler_Simulator::Engine::Data_Point*> timeline;

    std::list<OS_Scheduler_Simulator::Engine::Running_Process> waiting_list = current_data_point->get_waiting_list();
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> ready_list = current_data_point->get_ready_list();
    OS_Scheduler_Simulator::Engine::Running_Process running = ready_list.front();
    running.send_to_cpu();
    ready_list.pop_front();

    delete current_data_point;
    current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(0, waiting_list, ready_list, running);
    timeline.push_back(current_data_point);

    while (!current_data_point->is_done()) {
        ready_list = current_data_point->get_ready_list();
        waiting_list = current_data_point->get_waiting_list();
        running = current_data_point->get_cpu_process();

        OS_Scheduler_Simulator::Engine::Data_Point::event next_event = current_data_point->get_next_event();
        
        // Running events.
        if (running.is_valid()) running = running.get_next_process_state(next_event.time);
        for (OS_Scheduler_Simulator::Engine::Running_Process& process : waiting_list) process = process.get_next_process_state(next_event.time);
        
        // Removing process from CPU if completed.
        if (next_event.event_type == OS_Scheduler_Simulator::Engine::Data_Point::event_type::cpu) {
            if (running.get_status() == OS_Scheduler_Simulator::Engine::Running_Process::status_type::waiting)
                waiting_list.push_back(running); // It will be performing some IO operations now.

            running = OS_Scheduler_Simulator::Engine::Running_Process(nullptr); // CPU open.
        }

        // Regardless of previous case, check if any I/O operations is completed.
        for (std::list<OS_Scheduler_Simulator::Engine::Running_Process>::iterator it{ waiting_list.begin() }; it != waiting_list.end(); ) {
            if ((*it).get_status() == OS_Scheduler_Simulator::Engine::Running_Process::status_type::ready) {
                ready_list.push_back((*it));
                it = waiting_list.erase(it);
            }

            else it = std::next(it);
        }

        if (!running.is_valid() && ready_list.size() > 0) {
            running = ready_list.front();
            ready_list.pop_front();
            running.send_to_cpu();
        }

        // Adding data point.
        current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(current_data_point->get_time_since_start() + next_event.time, waiting_list, ready_list, running);
        timeline.push_back(current_data_point);
    }

    std::cout << "Relevant timestamps in the algorithm: ";
    for (auto data_point : timeline) std::cout << data_point->get_time_since_start() << " ";
    std::cout << std::endl;

    // Breakpoint 2: Testing different methods of the Data_Point class (essential for algorithms).

    // Evaluation algorithm efficiency.
    OS_Scheduler_Simulator::Engine::Evaluator eval(starting_list, &timeline);

    std::cout << "Printing all processes waiting times: " << std::endl;
    for (OS_Scheduler_Simulator::Engine::Evaluator::Process& process : eval.get_all_processes_data()) {
        std::cout << "- Process \"" << process.get_process_name() << "\"\n\tWaiting time: " << process.get_total_waiting_time() << std::endl;
        std::cout << "\tResponse time: " << process.get_response_time() << std::endl;
        std::cout << "\tTurnaround time: " << process.get_turnaround_time() << std::endl;
    }

    // Printing totals.
    std::cout << "\n\nTotals:" << std::endl;
    std::cout << "\t   Avg waiting time: " << eval.get_overall_totals().avg_waiting_time << std::endl;
    std::cout << "\t  Avg response time: " << eval.get_overall_totals().avg_response_time << std::endl;
    std::cout << "\tAvg turnaround time: " << eval.get_overall_totals().avg_turnaround_time << std::endl;

    std::cout << "\nTotal CPU utilization: " << eval.get_overall_totals().cpu_utilization * 100 << "%" << std::endl;

    // Delete all data points.
    for (auto data_point : timeline) delete data_point;
}
#endif // _DEBUG
