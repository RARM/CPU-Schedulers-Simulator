#ifndef _OS_SCHEDULER_SIMULATOR_ENGINE_
#define _OS_SCHEDULER_SIMULATOR_ENGINE_

#include <string>
#include <list>
#include <vector>
#include <functional>
#include <span>

class OS_Scheduler_Simulator {
public:
	class Engine;
};

class OS_Scheduler_Simulator::Engine {
public:
	class Process_Data;
	class Running_Process;
	class Data_Point;
	class Simulation;
	class Evaluator;
};

class OS_Scheduler_Simulator::Engine::Process_Data {
public:
	Process_Data(std::string name, std::span<unsigned> operations_list);
	
	std::string get_name() const { return this->name; }
	size_t get_operations_size() const { return this->operations.size(); }
	unsigned get_operation(size_t i) const { return this->operations.at(i); }

private:
	std::string name;
	std::vector<unsigned> operations;
};

class OS_Scheduler_Simulator::Engine::Running_Process {
public:
	typedef enum { running, waiting, ready, done } status_type;

	Running_Process(const Process_Data* process);
	
	Running_Process get_next_process_state(unsigned time) const;
	unsigned time_to_end_current_burst() const;
	
	status_type get_status() const { return this->status; }
	void send_to_ready() { this->status = status_type::ready; }
	void send_to_cpu() { this->status = status_type::running; }
	bool is_valid() const { return (this->process != nullptr) ? true : false; }
	std::string get_proc_name() const { return this->process->get_name(); }
	
private:
	const Process_Data* process;
	status_type status;
	size_t current_operation;
	unsigned time_in_current_operation;
};

class OS_Scheduler_Simulator::Engine::Data_Point {
public:
	typedef enum { cpu, io, done } event_type;
	
	typedef struct {
		event_type event_type;
		unsigned time;
	} event;

	Data_Point(const std::list<Process_Data>& starting_list);
	Data_Point(unsigned time_since_start, const std::list<Running_Process>& waiting_list, const std::list<Running_Process>& ready_list, Running_Process running_process = nullptr);

	event get_next_event();
	bool is_cpu_busy() const { return this->running.is_valid(); }
	
	Running_Process get_cpu_process() const { return this->running; }
	std::list<Running_Process> get_waiting_list() const { return this->waiting_list; }
	std::list<Running_Process> get_ready_list() const { return this->ready_list; }
	unsigned get_time_since_start() const { return this->time_since_start; }
	bool is_done() { return (this->ready_list.size() == 0 && this->waiting_list.size() == 0 && !this->running.is_valid()); }

private:
	std::list<Running_Process> ready_list;
	std::list<Running_Process> waiting_list;
	Running_Process running;
	unsigned time_since_start;
};

class OS_Scheduler_Simulator::Engine::Evaluator {
public:
	typedef struct {
		double cpu_utilization;
		double avg_waiting_time;
		double avg_turnaround_time;
		double avg_response_time;
	} results_table;
	
	class Process;

	Evaluator(std::list<Process_Data>& processes, std::list<Data_Point*>* timeline = nullptr);

    void run_evaluation();
	
	results_table get_overall_totals() { return this->total_results; }
	std::vector<Evaluator::Process> get_all_processes_data() { return this->processes_data; }

private:
	std::list<Data_Point*>* timeline;
	std::vector<Evaluator::Process> processes_data;
	results_table total_results;
};

class OS_Scheduler_Simulator::Engine::Simulation {
public:
	Simulation(std::span<Process_Data> processes);
	~Simulation(); // Destructor needed to deallocate timeline.

	Evaluator::results_table execute_algorithm(std::function<void(const std::vector<Process_Data>&, std::list<Data_Point>&)> algorithm);

	Data_Point get_latest_data_point() { return *this->timeline.back(); }
	unsigned get_execution_time() { return (*this->timeline.back()).get_time_since_start(); }

	Evaluator::results_table get_total_results() { return this->evaluator.get_overall_totals(); }
	std::vector<Evaluator::Process> get_per_process_evaluation() { return this->evaluator.get_all_processes_data(); }

private:
	std::vector<Process_Data> processes;
	std::list<Data_Point*> timeline;
	Evaluator evaluator;
};

class OS_Scheduler_Simulator::Engine::Evaluator::Process {
public:
	Process(OS_Scheduler_Simulator::Engine::Process_Data* process = nullptr);

	std::string get_process_name() const { return this->process->get_name(); }
	OS_Scheduler_Simulator::Engine::Process_Data* get_process_addr() const { return this->process; }
	void set_process_addr(OS_Scheduler_Simulator::Engine::Process_Data* proc) { this->process = proc; }

	void set_total_waiting_time(unsigned val) { this->total_waiting_time = val; }
	void set_total_turnaround_time(unsigned val) { this->total_turnaround_time = val; }
	void set_total_response_time(unsigned val) { this->total_response_time = val; }

	void add_total_waiting_time(unsigned val) { this->total_waiting_time += val; }
	void add_total_turnaround_time(unsigned val) { this->total_turnaround_time += val; }
	void add_total_response_time(unsigned val) { this->total_response_time += val; }

	unsigned get_total_waiting_time() const { return this->total_waiting_time; }
	unsigned get_total_turnaround_time() const { return this->total_turnaround_time; }
	unsigned get_total_response_time() const { return this->total_response_time; }

private:
	OS_Scheduler_Simulator::Engine::Process_Data* process;
	unsigned total_waiting_time;
	unsigned total_turnaround_time;
	unsigned total_response_time;
};

#endif
