#include <iostream>

#include "headers/WorldClock.hpp"
#include "headers/ErrorCheck.hpp"

using namespace puffinengine::tool;

// ------- Constructors and dectructors ------------- //

WorldClock::WorldClock() {
    totalElapsedTime = 0.0;
    fixedTimeValue = 0.0;
	frameTime = 0.0;
	fps = 0.0;

#if DEBUG_VERSION
	std::cout << "WorldClock created\n";
#endif 
}

WorldClock::~WorldClock() {
#if DEBUG_VERSION
	std::cout << "WorldClock destroyed\n";
#endif 
}

// ---------------- Main functions ------------------ //

void WorldClock::init() {
	totalElapsedTime = 0.0;
	fixedTimeValue = 0.0;
	frameTime = 0.0;
	fps = 0.0;
}

void WorldClock::deinit() {
	totalElapsedTime = 0.0;
	fixedTimeValue = 0.0;
	frameTime = 0.0;
	fps = 0.0;
}