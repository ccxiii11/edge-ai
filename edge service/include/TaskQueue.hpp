#pragma once
#include "Task.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

class TaskQueue
{
	public:
		void push(Task task);

		bool try_pop(Task &task);
		bool pop(Task &task);
		void shutdown();
	private:
		std::queue<Task> queue_;
		std::mutex mutex_;
		std::condition_variable condition_;
		bool stopped_ =false;
};

