#include "engine.h"

#include <string>
#include <list>
#include <vector>
#include <functional>
#include <iterator>

#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

/// <summary>
/// Process_Data constructor.
/// </summary>
/// <param name="name">- Name of the process.</param>
/// <param name="operations_list">- List of unsigned numbers representing the CPU and I/O bursts.</param>
OS_Scheduler_Simulator::Engine::Process_Data::Process_Data(std::string name, std::span<unsigned> operations_list) 
    : name(name), operations(operations_list.size()) {
    for (unsigned i{ 0 }; i < operations_list.size(); i++) this->operations.at(i) = operations_list[i];

#ifdef _DEBUG
    if (operations_list.size() % 2 == 0) std::cerr << "Critial error: Process_Data for process \"" << name << "\" was initialized with even size amount of operations." << std::endl;
#endif // _DEBUG
}

OS_Scheduler_Simulator::Engine::Running_Process::Running_Process(const OS_Scheduler_Simulator::Engine::Process_Data* process) 
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
        case status_type::running:
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

    else if (this->time_to_end_current_burst() > time && (this->status == status_type::waiting || this->status == status_type::running)) {
        new_process_data = (*this);
        new_process_data.time_in_current_operation += time;
    }

#ifdef _DEBUG
    else {
        std::cerr << "Running process \"" << this->process->get_name() << "\" caused an unknown error in updating its state." << std::endl;
    }
#endif // _DEBUG

    return new_process_data;
}

OS_Scheduler_Simulator::Engine::Data_Point::Data_Point(const std::list<Process_Data>& starting_list)
    : ready_list(), waiting_list(),
    running(OS_Scheduler_Simulator::Engine::Running_Process(nullptr)), time_since_start(0) {
    for (const Process_Data& process : starting_list)
        ready_list.push_back(Running_Process(&process));
}

OS_Scheduler_Simulator::Engine::Data_Point::Data_Point(unsigned time_since_start, const std::list<Running_Process>& waiting_list, const std::list<Running_Process>& ready_list, Running_Process running_process)
    : time_since_start(time_since_start), waiting_list(waiting_list), ready_list(ready_list), running(running_process) {}
    // Note that if receiving a null pointer for the running process, it will call the Running_Process constructor with null pointer as argument.
    // If receiving an actual Running_Process for the running_process argument, then it will be called with the defaul copy constructor (not defined here).

OS_Scheduler_Simulator::Engine::Data_Point::event OS_Scheduler_Simulator::Engine::Data_Point::get_next_event() {
    unsigned shortest_time{ 0 };
    event_type ev;

    // Defaults
    ev = event_type::unresolved; // FIXME: This was done to clear the "uninitialize memory 'ev'" warning, but it must be reviewed to see its effect on the rest of the engine.

    if (this->running.is_valid()) { 
        shortest_time = this->running.time_to_end_current_burst();
        ev = event_type::cpu;
    }
    
    if (this->waiting_list.size() > 0)
        if (!this->running.is_valid()) {
            shortest_time = waiting_list.front().time_to_end_current_burst();
            ev = event_type::io;
        }
        
        for (const Running_Process& process : this->waiting_list) {
            const unsigned time = process.time_to_end_current_burst();
            if (time < shortest_time) {
                shortest_time = time;
                ev = event_type::io;
            }
        }

    if (!this->running.is_valid() && this->waiting_list.size() == 0) ev = event_type::done;

    return event{
        .event_type = ev,
        .time = shortest_time
    };
}

OS_Scheduler_Simulator::Engine::Evaluator::Evaluator(std::list<Process_Data>& processes, std::list<Data_Point*>* timeline)
    : timeline(timeline), processes_data(processes.size()), total_results({0, 0, 0, 0}) {
    unsigned i{ 0 };
    for (Process_Data& process : processes) {
        this->processes_data.at(i).set_process_addr(&process);
        i++;
    }

    this->run_evaluation();
}

OS_Scheduler_Simulator::Engine::Evaluator::Evaluator(std::vector<Process_Data>& processes, std::list<Data_Point*>* timeline)
    : timeline(timeline), processes_data(processes.size()), total_results({ 0, 0, 0, 0 }) {
    for (unsigned i{ 0 }; i < processes.size(); i++)
        this->processes_data.at(i).set_process_addr(&processes.at(i));

    this->run_evaluation();
}

OS_Scheduler_Simulator::Engine::Evaluator::Process* find_process(std::vector<OS_Scheduler_Simulator::Engine::Evaluator::Process>& list_of_processes, std::string proc_name) {
    OS_Scheduler_Simulator::Engine::Evaluator::Process* needle = nullptr;
    
    for (OS_Scheduler_Simulator::Engine::Evaluator::Process& process : list_of_processes) {
        if (process.get_process_name() == proc_name) {
            needle = &process;
            break;
        }
    }

    return needle;
}

void OS_Scheduler_Simulator::Engine::Evaluator::run_evaluation() {
    if (this->timeline != nullptr) {
        // Setup.
        std::list<Data_Point*>::iterator previous_point = this->timeline->begin();
        std::list<Data_Point*>::iterator current_point = std::next(previous_point);
        std::list<Data_Point*>::iterator last_point = std::prev(this->timeline->end());

        unsigned unused_cpu{ 0 };

        while (current_point != last_point) {
            // Time for the current block.
            unsigned diff = (* current_point)->get_time_since_start() - (* previous_point)->get_time_since_start();
            
            // Calculate the waiting times at every point adding them up.
            for (Running_Process& waiting_process : (* previous_point)->get_ready_list()) {
                Process* proc_to_update = find_process(this->processes_data, waiting_process.get_proc_name());
                if (proc_to_update != nullptr) proc_to_update->add_total_waiting_time(diff);
            }

            if ((*previous_point)->is_cpu_busy()) {
                // Calculating response time.
                Process* running_proc = find_process(this->processes_data, (*previous_point)->get_cpu_process().get_proc_name());
                if (running_proc->is_response_set() == false)
                    running_proc->set_response_time((*previous_point)->get_time_since_start());

                // Calculating turnaround time.
                // This model assumes all processes are submitted at start.
                Running_Process::status_type p_next_status = (*previous_point)->get_cpu_process().get_next_process_state(diff).get_status();
                if (p_next_status == Running_Process::status_type::done)
                    running_proc->set_turnaround_time((*current_point)->get_time_since_start());
            }
            
            else { // Add time not being utilized.
                unused_cpu += diff;
            }
            
            // The i++
            previous_point = current_point;
            current_point = std::next(current_point);
        }

        // Adding turnaround for the last process.
        Process* last_proc = find_process(this->processes_data, (*previous_point)->get_cpu_process().get_proc_name());
        last_proc->set_turnaround_time((*current_point)->get_time_since_start());

        // Calculate CPU utilization.
        this->total_results.cpu_utilization = static_cast<double>((*last_point)->get_time_since_start() - unused_cpu) / static_cast<double>((*last_point)->get_time_since_start());

        // Calculate averages.
        this->total_results.avg_response_time = this->total_results.avg_turnaround_time = this->total_results.avg_waiting_time = 0;
        
        for (const Evaluator::Process& proc : this->processes_data) {
            this->total_results.avg_response_time   += static_cast<double>(proc.get_response_time());
            this->total_results.avg_turnaround_time += static_cast<double>(proc.get_turnaround_time());
            this->total_results.avg_waiting_time    += static_cast<double>(proc.get_total_waiting_time());
        }

        this->total_results.avg_response_time   /= this->processes_data.size();
        this->total_results.avg_turnaround_time /= this->processes_data.size();
        this->total_results.avg_waiting_time    /= this->processes_data.size();
    }
}

OS_Scheduler_Simulator::Engine::Evaluator::Process::Process(OS_Scheduler_Simulator::Engine::Process_Data* process)
    : process(process), total_waiting_time(0), turnaround_time(0), response_time(0), response_time_set(false), turnaround_time_set(false) {}

OS_Scheduler_Simulator::Engine::Simulation::Simulation(const std::span<Process_Data>& processes)
    : processes(), evaluator(nullptr) {
    for (unsigned i{ 0 }; i < processes.size(); i++)
        this->processes.push_back(processes[i]);

    this->processes.shrink_to_fit();
    this->evaluator = new Evaluator(this->processes, &this->timeline);
}

OS_Scheduler_Simulator::Engine::Simulation::~Simulation() {
    delete this->evaluator;

    for (Data_Point* data_point : this->timeline)
        delete data_point;
}

void OS_Scheduler_Simulator::Engine::Simulation::register_algorithm(std::string name, std::function<void(const std::vector<Process_Data>&, std::list<Data_Point>&)> algorithm) {
    bool algorithm_exists = false;

    for (const auto& [alg_name, func]: this->algorithms)
        if (alg_name == name) {
            algorithm_exists = true;
            break;
        }
    
    if (!algorithm_exists)
        this->algorithms.push_back(std::pair(name, algorithm));

    // NOTE: For future refactoring. Could the list be changed to vector, be sorted, and improve checking time?
}