#include <assert.h>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace enginetool {
	class Thread {
        private:
        
        bool destroying = false;
        std::thread worker;
        std::queue<std::function<void()>> jobQueue;
        std::mutex queueMutex;
        std::condition_variable condition;
    };
}