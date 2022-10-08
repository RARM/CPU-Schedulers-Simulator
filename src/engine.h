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
	unsigned get_operation(unsigned i) const { return this->operations.at(i); }

private:
	std::string name;
	std::vector<unsigned> operations;
};

class OS_Scheduler_Simulator::Engine::Running_Process {
public:
	typedef enum { runnig, waiting, ready, done } status_type;

	Running_Process(Process_Data* process);
	
	Running_Process get_next_process_state(unsigned time) const;
	unsigned time_to_end_current_burst() const;
	
	status_type get_status() const { return this->status; }
	bool is_valid() const { return (this->process != nullptr) ? true : false; }
	
private:
	Process_Data* process;
	status_type status;
	std::list<unsigned>::iterator current_operation;
	unsigned time_in_current_operation;
};

class OS_Scheduler_Simulator::Engine::Data_Point {
public:
	typedef struct {
		enum { cpu, io } event_type;
		unsigned time;
	} event;
	
	Data_Point(const std::list<Running_Process>& starting_list);
	Data_Point(unsigned time_since_start, const std::list<Running_Process>& waiting_list, const std::list<Running_Process>& ready_list, Running_Process running_process = nullptr);

	event get_next_event();
	bool is_cpu_busy() { return this->running.is_valid(); }
	
	Running_Process get_cpu_process() { return this->running; }
	std::list<Running_Process> get_waiting_list() { return this->waiting_list; }
	std::list<Running_Process> get_ready_list() { return this->ready_list; }
	unsigned get_time_since_start() { return this->time_since_start; }

private:
	std::list<Running_Process> ready_list;
	std::list<Running_Process> waiting_list;
	Running_Process running;
	unsigned time_since_start;
};

class OS_Scheduler_Simulator::Engine::Simulation {
public:
	Simulation(std::span<Process_Data> processes);

	Evaluator::results_table execute_algorithm(std::function<void(const std::vector<Process_Data>&, std::list<Data_Point>&)> algorithm);

	Data_Point get_latest_data_point() { return this->timeline.back(); }
	unsigned get_execution_time() { return this->timeline.back().get_time_since_start(); }

	// FIXME: Add evaluator functions.

private:
	std::vector<Process_Data> processes;
	std::list<Data_Point> timeline;
	Evaluator evaluator;
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

	Evaluator(std::list<Data_Point>* timeline = nullptr);

    void run_evaluation(std::list<Data_Point>* timeline = nullptr);
	results_table get_overall_totals();
	
	std::vector<Evaluator::Process> get_all_processes_data() { return this->processes_data; }

private:
	std::list<Data_Point>* timeline;
	std::vector<Evaluator::Process> processes_data;
	results_table total_results;
};

class OS_Scheduler_Simulator::Engine::Evaluator::Process {
public:
	Process(OS_Scheduler_Simulator::Engine::Process_Data* process);

	auto get_process_name() { this->process->get_name(); }

	void set_total_waiting_time(unsigned val) { this->total_waiting_time = val; }
	void set_total_turnaround_time(unsigned val) { this->total_turnaround_time = val; }
	void set_total_response_time(unsigned val) { this->total_response_time = val; }

	void add_total_waiting_time(unsigned val) { this->total_waiting_time += val; }
	void add_total_turnaround_time(unsigned val) { this->total_turnaround_time += val; }
	void add_total_response_time(unsigned val) { this->total_response_time += val; }

	unsigned get_total_waiting_time() { return this->total_waiting_time; }
	unsigned get_total_turnaround_time() { return this->total_turnaround_time; }
	unsigned get_total_response_time() { return this->total_response_time; }

private:
	OS_Scheduler_Simulator::Engine::Process_Data* process;
	unsigned total_waiting_time;
	unsigned total_turnaround_time;
	unsigned total_response_time;
};

#endif
