#include "engine.h"

#include <string>
#include <list>
#include <vector>
#include <functional>
#include <iterator>

#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

OS_Scheduler_Simulator::Engine::Process_Data::Process_Data(std::string name, std::span<unsigned> operations_list) 
    : name(name), operations(operations_list.size()) {
    for (unsigned i{ 0 }; i < operations_list.size(); i++) this->operations.at(i) = operations_list[i];

#ifdef _DEBUG
    if (operations_list.size() % 2 == 0) std::cerr << "Critial error: Process_Data for process \"" << name << "\" was initialized with even size amount of operations." << std::endl;
#endif // _DEBUG

}

OS_Scheduler_Simulator::Engine::Running_Process::Running_Process(OS_Scheduler_Simulator::Engine::Process_Data* process) 
    : process(process), status(OS_Scheduler_Simulator::Engine::Running_Process::status_type::ready), 
    current_operation(0), time_in_current_operation(0) {}

unsigned OS_Scheduler_Simulator::Engine::Running_Process::time_to_end_current_burst() const {
    unsigned time{ 0 };
    
    if (this->process != nullptr && this->current_operation < this->process->get_operations_size())
        time = this->process->get_operation(this->current_operation) - time_in_current_operation;

    return time;
}

OS_Scheduler_Simulator::Engine::Running_Process OS_Scheduler_Simulator::Engine::Running_Process::get_next_process_state(unsigned time) const {
    OS_Scheduler_Simulator::Engine::Running_Process new_process_data(this->process);

    if (this->time_to_end_current_burst() < time || this->status == status_type::done || this->status == status_type::ready) {
        // The current status and current operation do not change.
        new_process_data = (* this); // Shallow copy.
        new_process_data.time_in_current_operation += (this->status != status_type::ready && this->status != status_type::done) ? time : 0;
    }

    else if (this->time_to_end_current_burst() == time) {
        // Find out the status first.
        switch (this->status)
        {
        case status_type::runnig:
            new_process_data.status = (this->current_operation + 1 < this->process->get_operations_size()) ? status_type::waiting : status_type::done;
            break;

        case status_type::waiting: // Processes never end in IO.
            new_process_data.status = status_type::ready;
            break;

#ifdef _DEBUG
        default:
            std::cerr << "Unhandled status while calculating next burst time for \"" << this->process->get_name() << "\"." << std::endl;
            break;
#endif // _DEBUG

        }

        new_process_data.current_operation = this->current_operation + 1;
        new_process_data.time_in_current_operation = 0;
    }

#ifdef _DEBUG
    else {
        std::cerr << "Running process \"" << this->process->get_name() << "\" was updated with a time higher than its current burst." << std::endl;
    }
#endif // _DEBUG


    return new_process_data;
}