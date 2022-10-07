#include "engine.h"

class OS_Scheduler_Simulator
{
public:
    class Engine;

    OS_Scheduler_Simulator() {

    }

    ~OS_Scheduler_Simulator() {

    }

private:

};

class OS_Scheduler_Simulator::Engine {
public:
    class Process;
    class Data_Time_Point;

    /*
    * Setup of the OS Scheduler Simulator Engine.
    */
    Engine() {

    }

    /*
    * Run a specific algorithm given a list of processes.
    */
    static std::list<OS_Scheduler_Simulator::Engine::Data_Time_Point> run_algorithm(
        std::list<OS_Scheduler_Simulator::Engine::Process> data,
        std::function<std::list<OS_Scheduler_Simulator::Engine::Data_Time_Point>(std::list<OS_Scheduler_Simulator::Engine::Process>)> algorithm
    ) {
        return algorithm(data);
    }

    /*Data_Time_Point get_dynamic_execution_data_point(int time) {
        Data_Time_Point result();

        return result;
    }*/

private:
    std::list<OS_Scheduler_Simulator::Engine::Data_Time_Point> simulation_data;
    OS_Scheduler_Simulator::Engine::Data_Time_Point initial_data;
};

class OS_Scheduler_Simulator::Engine::Process {
public:
    static constexpr int CPU_BURST = 1;
    static constexpr int IO_BURST = 2;

    struct event {
        int type;
        int time;
    };

    Process(std::string name, std::list<int> times) : name(name), events() {
        if (times.size() % 2 == 0) {
            std::cerr << "Process \"" << name << "\" has an invalid number of CPU/IO bursts." << std::endl;
        }

        else {
            for (int i{ 0 }; i < times.size(); i++)
                this->events.push_back(event{
                        .type = (.type = (true) ? 1 : 0),
                        .time = times.
                    });
        }
    }

private:
    std::string name;
    std::list<struct event> events;
};

class OS_Scheduler_Simulator::Engine::Data_Time_Point {
public:
    std::list<OS_Scheduler_Simulator::Engine::Process> ready;
    std::list<OS_Scheduler_Simulator::Engine::Process> io_queue;
    OS_Scheduler_Simulator::Engine::Process* running;

    Data_Time_Point() {}

    /*
    * Test if there is nothing else to do.
    */
    bool completed() {
        return true;
    }
};