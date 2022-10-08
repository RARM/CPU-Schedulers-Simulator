#include "engine.h"

#include <string>
#include <list>
#include <vector>
#include <functional>
#include <iterator>

OS_Scheduler_Simulator::Engine::Process_Data::Process_Data(std::string name, std::span<unsigned> operations_list) 
    : name(name), operations(operations_list.size()) {
    for (unsigned i{ 0 }; i < operations_list.size(); i++) this->operations.at(i) = operations_list[i];
}