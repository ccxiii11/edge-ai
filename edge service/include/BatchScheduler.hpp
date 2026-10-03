#pragma once

#include "TaskQueue.hpp"
#include "Batch.hpp"

#include "BatchQueue.hpp"
class BatchScheduler
{
	public:
		BatchScheduler(
				TaskQueue& task_queue,
				size_t batch_size,
				int timeout_ms);

		bool getBatch(Batch &batch);
		void run(BatchQueue &batch_queue);
	private:
		TaskQueue &task_queue_;
		size_t batch_size_;
		int timeout_ms_;
};
