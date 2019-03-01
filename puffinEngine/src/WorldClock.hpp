#pragma once

class WorldClock { // TODO Allow only one method to modify this class
        public:
       	WorldClock();
    	~WorldClock();

        double totalTime = 0.0;
	    const double fixedTimeValue = 0.016666;
	    double currentTime = 0.0;
	    double accumulator = 0.0;
};