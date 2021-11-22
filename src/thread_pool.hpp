#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <mutex>
#include <future>
#include <vector>
#include <deque>
#include <condition_variable>
#include <thread>
#include <random>

class ThreadPool {
	public:
		ThreadPool(size_t threads);
		~ThreadPool();
		
		std::future<void> queue(std::function<void(std::mt19937&)>&& f);

		inline void cancel_pending() {
			std::unique_lock<std::mutex> l(mutex);
			tasks.clear();
		}

	private:
	std::mutex mutex;
	std::condition_variable cond;
	bool stop;

	std::deque<std::packaged_task<void(std::mt19937&)>> tasks;
	std::vector<std::thread> workers;
};

inline std::future<void> ThreadPool::queue(std::function<void(std::mt19937&)>&& f) {
	std::packaged_task<void(std::mt19937&)> p(f);

	auto r=p.get_future(); // get the return value before we hand off the task
	{
		std::unique_lock<std::mutex> l(mutex);
		tasks.emplace_back(std::move(p)); // store the task<R()> as a task<void()>
	}
	cond.notify_one(); // wake a thread to work on the task

	return r; // return the future result of the task
}

inline ThreadPool::ThreadPool(std::size_t n) : stop(false){
	for (std::size_t i = 0; i < n; ++i){
		workers.emplace_back([this] {
				std::mt19937 gen32;
				for(;;) {
					std::packaged_task<void(std::mt19937&)> task;
					{
						std::unique_lock<std::mutex> lock(this->mutex);
						this->cond.wait(lock,
								[this]{ return this->stop || !this->tasks.empty(); });
						if(this->stop && this->tasks.empty())
							return;
						task = std::move(this->tasks.front());
						this->tasks.pop_front();
					}
					task(gen32);
				}
			});
	}
}

inline ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(mutex);
		stop = true;
	}
	cond.notify_all();
	for(std::thread &worker: workers)
		worker.join();
}
#endif
