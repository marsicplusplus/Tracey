#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <mutex>
#include <future>
#include <vector>
#include <deque>
#include <condition_variable>
#include <thread>

class ThreadPool {
	public:
		ThreadPool();
		~ThreadPool();

		void init(size_t threads);
		
		std::future<void> queue(std::function<void(uint32_t&)>&& f);

		inline void cancel_pending() {
			std::unique_lock<std::mutex> l(mutex);
			tasks.clear();
		}

	private:
	std::mutex mutex;
	std::condition_variable cond;
	bool stop;

	std::deque<std::packaged_task<void(uint32_t&)>> tasks;
	std::vector<std::thread> workers;
};

inline std::future<void> ThreadPool::queue(std::function<void(uint32_t&)>&& f) {
	std::packaged_task<void(uint32_t&)> p(f);

	auto r=p.get_future(); 
	{
		std::unique_lock<std::mutex> l(mutex);
		tasks.emplace_back(std::move(p)); 
	}
	cond.notify_one();

	return r;
}

inline void ThreadPool::init(std::size_t n){
	for (std::size_t i = 0; i < n; ++i){
		workers.emplace_back([this] {
				uint32_t gen32 = uint32_t(time(NULL));
				for(;;) {
					std::packaged_task<void(uint32_t&)> task;
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
};

inline ThreadPool::ThreadPool() : stop(false){}

inline ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(mutex);
		stop = true;
	}
	cond.notify_all();
	for(std::thread &worker: workers)
		worker.join();
}

extern ThreadPool pool;
#endif
