#pragma once

class WorldClock { // TODO Allow only one method to modify this class
public:
    WorldClock();
    ~WorldClock();

    double totalElapsedTime = 0.0;
	double fixedTimeValue = 0.0;
	double frameTime = 0.0;
    double fps = 0.0;
};