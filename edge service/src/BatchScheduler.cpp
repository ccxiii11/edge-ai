#include "BatchScheduler.hpp"

#include <chrono>
#include<thread>
#include<iostream>

BatchScheduler::BatchScheduler(
		 TaskQueue &task_queue,
		 size_t batch_size,
		 int timeout_ms)
	          : task_queue_(task_queue),
		  batch_size_(batch_size),
		  timeout_ms_(timeout_ms)
{
}



bool BatchScheduler::getBatch(Batch& batch)
{
    batch.tasks.clear();

    Task first_task;

    if (!task_queue_.pop(first_task))
    {
        return false;
    }

    batch.tasks.push_back(std::move(first_task));

    auto start =
        std::chrono::steady_clock::now();

    while (batch.tasks.size() < batch_size_)
    {
        Task task;

        if (task_queue_.try_pop(task))
        {
            batch.tasks.push_back(std::move(task));
            continue;
        }

        auto now =
            std::chrono::steady_clock::now();

        auto elapsed =
            std::chrono::duration_cast<
                std::chrono::milliseconds
            >(now - start).count();

        if (elapsed >= timeout_ms_)
        {
            break;
        }

        std::this_thread::yield();
    }

    return true;
}
void BatchScheduler::run(BatchQueue &batch_queue)
{
	while(1)

	{
		Batch batch;
		if(!getBatch(batch))
		{
			break;
		}
		std::cout
			<<"Scheduler created batch ,size ="
			<<batch.tasks.size()
			<<std::endl;
		batch_queue.push(std::move(batch));
	}
}
