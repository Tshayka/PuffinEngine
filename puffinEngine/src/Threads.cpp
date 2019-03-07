#include <assert.h>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace enginetool {
	class Thread {
        public:
        Thread() {Init();}
		~Thread() {DeInit();}

		void AddJob(std::function<void()> function)	{
			std::lock_guard<std::mutex> lock(queueMutex);
			jobQueue.push(std::move(function));
			condition.notify_one();
		}

		pthread_t GetCurrentThreadID() {
			return pthread_self();
		}

		void Sleep() {
			std::this_thread::yield();
        }

        void Wait() {
			std::unique_lock<std::mutex> lock(queueMutex);
			condition.wait(lock, [this]() { return jobQueue.empty(); });
        }

        private:
        bool destroying = false;
        std::thread worker;
        std::queue<std::function<void()>> jobQueue;
        std::mutex queueMutex;
        std::condition_variable condition;

		void DeInit() {
			if (worker.joinable()) {
				Wait();
				queueMutex.lock();
				destroying = true;
				condition.notify_one();
				queueMutex.unlock();
				worker.join();
			}
		}

		void Init() {
			worker = std::thread(&Thread::QueueLoop, this);
		}

		void QueueLoop() {
			while (true) {
				std::function<void()> job;
				
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });
					if (destroying) {
						break;
					}
					job = jobQueue.front();
				}

				job();

				{
					std::lock_guard<std::mutex> lock(queueMutex);
					jobQueue.pop();
					condition.notify_one();
				}
			}
		}
    };
}

namespace enginetool {
	class ThreadPool {
	public:
		std::vector<std::unique_ptr<Thread>> threads;

		void SetThreadCount(uint32_t count)	{
			threads.clear();
			for (uint32_t i = 0; i < count; i++) {
				threads.push_back(std::make_unique<Thread>());
			}
		}

		void Hold()	{
			for (const std::unique_ptr<Thread> &thread : threads) {
				thread->Wait();
			}
		}

		void Sleep() {
			for (const std::unique_ptr<Thread> &thread : threads) {
				thread->Sleep();
			}
		}
	};
}