#ifndef _OS_SCHEDULER_SIMULATOR_ENGINE_
#define _OS_SCHEDULER_SIMULATOR_ENGINE_

#include <string>
#include <list>
#include <functional>

class OS_Scheduler_Simulator {
public:
	class Engine;
};

class OS_Scheduler_Simulator::Engine {
public:
	class Process;
	class Data_Time_Point;
	class Report_Resutls;

	// Setup/controller functions.

	Engine(); // Setup.

private:
	std::list<Process> initial_data;
	std::list<Data_Time_Point> timeline;
};

class OS_Scheduler_Simulator::Engine::Process {
public:
	static constexpr int CPU_BURST{ 1 };
	static constexpr int IO_BURST{ 2 };

	Process(std::string name, std::list<int> times);

private:
	struct event {
		int time;
		int type;
	};

	std::string name;
	std::list<struct event> events;
};

class OS_Scheduler_Simulator::Engine::Data_Time_Point {};

class OS_Scheduler_Simulator::Engine::Report_Resutls {
	Report_Resutls(double cpu_u, double avg_t_w, double avg_t_tr, double avg_t_r);

	double get_cpu_utilization();
	double get_avg_waiting_time();
	double get_turnaround_time();
	double avg_response_time();

private:
	double cpu_utilization;
	double avg_waiting_time;
	double avg_turnaround_time;
	double avg_response_time;
};

#endif
