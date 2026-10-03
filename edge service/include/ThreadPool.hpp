#pragma once
#include "BatchQueue.hpp"
#include<vector>
#include<thread>
#include "InferenceEngine.hpp"

class ThreadPool
{
	public:
		explicit ThreadPool(size_t thread_count);

		~ThreadPool();
		void start(BatchQueue &batch_queue);
	private:
		void worker(BatchQueue &batch_queue,size_t worker_index);
	private:
		std::vector<std::thread> workers_;

		InferenceEngine inference_engine_;
		BatchQueue *batch_queue_ =nullptr;
};
