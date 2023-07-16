#pragma once

namespace puffinengine {
    namespace tool {
        class WorldClock { // TODO Allow only one method to modify this class
        public:
            WorldClock();
            ~WorldClock();

            void init();
            void deinit();

            double totalElapsedTime;
            double fixedTimeValue;
            double frameTime;
            double fps;
        };
    }
}