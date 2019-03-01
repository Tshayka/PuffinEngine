#include <iostream>

#include "WorldClock.hpp"
#include "ErrorCheck.hpp"


// ------- Constructors and dectructors ------------- //

WorldClock::WorldClock() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "WorldClock created\n";
#endif 
}

WorldClock::~WorldClock() {
#if BUILD_ENABLE_VULKAN_DEBUG
	std::cout << "WorldClock destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //