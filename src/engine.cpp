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

OS_Scheduler_Simulator::Engine::Data_Point::Data_Point(const std::vector<Process_Data>& starting_list)
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
    // Clear evaluator data if any.
    for (auto& proc : this->processes_data) proc.reset();

    if (this->timeline != nullptr && this->timeline->size() > 0) {
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

    // Registering default algorithms.
    this->register_algorithm("FCFS", OS_SS_Algorithms::FCFS);
    this->register_algorithm("SJF", OS_SS_Algorithms::SJF);
    this->register_algorithm("MLFQ", OS_SS_Algorithms::MLFQ);
}

OS_Scheduler_Simulator::Engine::Simulation::~Simulation() {
    delete this->evaluator;

    for (Data_Point* data_point : this->timeline)
        delete data_point;
}

void OS_Scheduler_Simulator::Engine::Simulation::register_algorithm(std::string name, std::function<void(const std::vector<Process_Data>&, std::list<Data_Point*>&)> algorithm) {
    bool algorithm_exists = false;

    for (const auto& [alg_name, func] : this->algorithms)
        if (alg_name == name) {
            algorithm_exists = true;
            break;
        }
    
    if (!algorithm_exists)
        this->algorithms.push_back(std::pair(name, algorithm));

    // NOTE: For future refactoring. Could the list be changed to vector, be sorted, and improve checking time?
}

OS_Scheduler_Simulator::Engine::Evaluator::results_table OS_Scheduler_Simulator::Engine::Simulation::execute_algorithm(std::string name_identifier) {
    // Clear the timeline before doing anything else.
    for (Data_Point* data_point : this->timeline)
        delete data_point;

    this->timeline.clear();

    // Run function if it exists.
    for (const auto& [alg_name, func] : this->algorithms)
        if (alg_name == name_identifier) {
            func(this->processes, this->timeline);
            this->evaluator->run_evaluation();
        }

    return this->evaluator->get_overall_totals();
}

/// <summary>
/// First Come, First Serve scheduling algorithm.
/// </summary>
/// <param name="processes">- List of processes for this algorithm.</param>
/// <param name="timeline">- Blank timeline to populate.</param>
void OS_SS_Algorithms::FCFS(const std::vector<OS_Scheduler_Simulator::Engine::Process_Data>& processes, std::list<OS_Scheduler_Simulator::Engine::Data_Point*>& timeline) {
    OS_Scheduler_Simulator::Engine::Data_Point* current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(processes);

    // Sending the first process to the CPU before commiting to the timeline.
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
}

/// <summary>
/// Shortest Job First algorithm. Note: This algorithm could not be implemented in real life because the scheduler would need to know the CPU bursts beforehand; however, the ASJF is a real world implementation of this algorithm.
/// </summary>
/// <param name="processes">- List of processes for this algorithm.</param>
/// <param name="timeline">- Blank timeline to populate.</param>
void OS_SS_Algorithms::SJF(const std::vector<OS_Scheduler_Simulator::Engine::Process_Data>& processes, std::list<OS_Scheduler_Simulator::Engine::Data_Point*>& timeline) {
    OS_Scheduler_Simulator::Engine::Data_Point* current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(processes);

    // Sending the shortest process first to the CPU before commiting to the timeline.
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> waiting_list = current_data_point->get_waiting_list();
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> ready_list = current_data_point->get_ready_list();

    // Get the shortest job in the ready list.
    auto shortest = [&ready_list]() -> std::list<OS_Scheduler_Simulator::Engine::Running_Process>::const_iterator {
        std::list<OS_Scheduler_Simulator::Engine::Running_Process>::const_iterator shortest_job = ready_list.begin();

        for (auto it{ ready_list.begin() }; it != ready_list.end(); it = std::next(it))
            if (it->time_to_end_current_burst() < shortest_job->time_to_end_current_burst())
                shortest_job = it;

        return shortest_job;
    };

    // Send the shortest job to the CPU.
    std::list<OS_Scheduler_Simulator::Engine::Running_Process>::const_iterator temp = shortest();
    OS_Scheduler_Simulator::Engine::Running_Process running = *temp;
    ready_list.erase(temp);

    running.send_to_cpu();

    // Commit to timeline.
    delete current_data_point;
    current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(0, waiting_list, ready_list, running);
    timeline.push_back(current_data_point);

    // Start loop for the timeline.
    while (!current_data_point->is_done()) {
        // Update lists.
        ready_list = current_data_point->get_ready_list();
        waiting_list = current_data_point->get_waiting_list();
        running = current_data_point->get_cpu_process();

        // Get next event.
        OS_Scheduler_Simulator::Engine::Data_Point::event next_event = current_data_point->get_next_event();

        // Running processes.
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
            // Getting the next shortest job first if CPU is open.
            temp = shortest();
            running = *temp;
            ready_list.erase(temp);

            running.send_to_cpu();
        }

        // Adding data point.
        current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(current_data_point->get_time_since_start() + next_event.time, waiting_list, ready_list, running);
        timeline.push_back(current_data_point);
    }
}

/// <summary>
/// This Implementation of MLFQ has three levels.
/// 
/// - First level uses Round Robin with time quantum = 5.
/// - Second level uses Round Robin with time quantum = 10.
/// - Third level uses Round Robin with time quantum = 10.
/// </summary>
/// <param name="processes">- List of processes for this algorithm.</param>
/// <param name="timeline">- Blank timeline to populate.</param>
void OS_SS_Algorithms::MLFQ(const std::vector<OS_Scheduler_Simulator::Engine::Process_Data>& processes, std::list<OS_Scheduler_Simulator::Engine::Data_Point*>& timeline) {
    // All the ready queues.
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> round_robin_1;
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> round_robin_2;
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> FCFS;
    
    std::list<OS_Scheduler_Simulator::Engine::Running_Process> IO_list; // waiting_list in other algorithms here.
    OS_Scheduler_Simulator::Engine::Running_Process running(nullptr);

    // Preparing the first commit.
    for (const auto& proc : processes) // Initially adding all of them to the level 1.
        round_robin_1.push_back(OS_Scheduler_Simulator::Engine::Running_Process(&proc));
    
    // Send the first process to CPU.
    running = round_robin_1.front();
    round_robin_1.pop_front();
    running.send_to_cpu();

    // What level is running?
    enum levels { level_1, level_2, level_3 };
    levels level_running = levels::level_1;

    auto get_time_quantum = [](levels level) -> unsigned {
        unsigned time;
        switch (level)
        {
        case levels::level_1:
            time = 5;
            break;
        case levels::level_2:
            time = 10;
            break;
        case levels::level_3: // FCFS has no time quantum.
        default:
            time = 0;
            break;
        }

        return time;
    };

    // Function to combine all queues to one for easy commit.
    // This could be improved by changing the architecture of a Data_Point to hold multiple ready_queues (optimization for MLFQ).
    auto prepare_ready_queue = [](
            const std::list<OS_Scheduler_Simulator::Engine::Running_Process>& round_robin_1, 
            const std::list<OS_Scheduler_Simulator::Engine::Running_Process>& round_robin_2,
            const std::list<OS_Scheduler_Simulator::Engine::Running_Process>& FCFS
        ) -> std::list<OS_Scheduler_Simulator::Engine::Running_Process> {
        std::list<OS_Scheduler_Simulator::Engine::Running_Process> ready_queue;
        ready_queue.insert(ready_queue.end(), round_robin_1.begin(), round_robin_1.end());
        ready_queue.insert(ready_queue.end(), round_robin_2.begin(), round_robin_2.end());
        ready_queue.insert(ready_queue.end(), FCFS.begin(),    FCFS.end());
        return ready_queue;
    };

    // First commit to the timeline.
    OS_Scheduler_Simulator::Engine::Data_Point* current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(0, IO_list, prepare_ready_queue(round_robin_1, round_robin_2, FCFS), running);
    timeline.push_back(current_data_point);

    // The loop.
    while (!current_data_point->is_done()) {
        // All lists should stay the same as in the previous iteration.

        // Get the next event.
        OS_Scheduler_Simulator::Engine::Data_Point::event next_event = current_data_point->get_next_event();
        unsigned current_time_quantum{ 0 };

        // Check if interrupted by time quantum.
        current_time_quantum = get_time_quantum(level_running);
        
        if (current_data_point->is_cpu_busy() && level_running == levels::level_1 && current_time_quantum < running.time_in_operation() + next_event.time) {
            next_event.event_type = OS_Scheduler_Simulator::Engine::Data_Point::event_type::cpu;
            next_event.time = current_time_quantum - running.time_in_operation();
        }

        else if (current_data_point->is_cpu_busy() && level_running == levels::level_2 && current_time_quantum < (running.time_in_operation() - get_time_quantum(levels::level_1)) + next_event.time) {
            next_event.event_type = OS_Scheduler_Simulator::Engine::Data_Point::event_type::cpu;
            next_event.time = current_time_quantum - (running.time_in_operation() - get_time_quantum(levels::level_1));
        }
        
        // Running processes.
        if (running.is_valid()) running = running.get_next_process_state(next_event.time);
        for (OS_Scheduler_Simulator::Engine::Running_Process& process : IO_list) process = process.get_next_process_state(next_event.time);

        // Removing process from CPU if completed or time quantum interrupted.
        if (next_event.event_type == OS_Scheduler_Simulator::Engine::Data_Point::event_type::cpu) {
            if (running.get_status() == OS_Scheduler_Simulator::Engine::Running_Process::status_type::waiting)
                IO_list.push_back(running); // It will be performing some IO operations now.

            // If an even in CPU was not caused by burst completion, it must have been a quantum interruption.
            else if (running.get_status() != OS_Scheduler_Simulator::Engine::Running_Process::status_type::done) {
                switch (level_running)
                {
                case levels::level_1:
                    round_robin_2.push_back(running);
                    break;
                case levels::level_2:
                    FCFS.push_back(running);
                    break;
                }
            }
            
            // Removing process completely if done.
            running = OS_Scheduler_Simulator::Engine::Running_Process(nullptr); // CPU open.
        }

        // Regardless of previous case, check if any I/O operations is completed.
        for (std::list<OS_Scheduler_Simulator::Engine::Running_Process>::iterator it{ IO_list.begin() }; it != IO_list.end(); ) {
            if ((*it).get_status() == OS_Scheduler_Simulator::Engine::Running_Process::status_type::ready) {
                round_robin_1.push_back((*it)); // Send to level 1 if done.
                it = IO_list.erase(it);
            }

            else it = std::next(it);
        }

        // Put something in the CPU if empty.
        if (!running.is_valid() && (round_robin_1.size() > 0 || round_robin_2.size() > 0 || FCFS.size() > 0)) {
            if (round_robin_1.size() > 0) {
                running = round_robin_1.front();
                round_robin_1.pop_front();
                level_running = levels::level_1;
            }

            else if (round_robin_2.size() > 0) {
                running = round_robin_2.front();
                round_robin_2.pop_front();
                level_running = levels::level_2;
            }

            else {
                running = FCFS.front();
                FCFS.pop_front();
                level_running = levels::level_3;
            }
            
            running.send_to_cpu();
        }

        // Commit to timeline.
        current_data_point = new OS_Scheduler_Simulator::Engine::Data_Point(current_data_point->get_time_since_start() + next_event.time, IO_list, prepare_ready_queue(round_robin_1, round_robin_2, FCFS), running);
        timeline.push_back(current_data_point);
    }
}