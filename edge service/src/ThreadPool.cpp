#include "ThreadPool.hpp"

#include <iostream>
#include <sys/socket.h>

ThreadPool::ThreadPool(size_t thread_count)
	:inference_engine_(thread_count)
{
    workers_.reserve(thread_count);

    for (size_t i = 0; i < thread_count; ++i)
    {
        workers_.emplace_back();
    }
}

void ThreadPool::start(BatchQueue& batch_queue)
{
    batch_queue_ = &batch_queue;

    for (size_t i =0;i<workers_.size();++i)
    {
        workers_[i] = std::thread(
            &ThreadPool::worker,
            this,
            std::ref(batch_queue),
	    i
        );
    }
}

void ThreadPool::worker(BatchQueue& batch_queue
		,size_t worker_index)
{

    nvinfer1::IExecutionContext *context=
	    inference_engine_.createContext(
			    static_cast<int>(worker_index)
			    );
    if(context ==nullptr)
    {
	    std::cout
		    <<"Worker"
		    <<worker_index
		    <<"failed to create context"
		    <<std::endl;
	    return;
    }
    std::cout<<
	    worker_index
	    <<"started"
	    <<std::endl;
    while (true)
    {
        Batch batch;

        if (!batch_queue.pop(batch))
        {
            break;
        }

        std::cout
            << "[WORKER] thread_id = "
            << std::this_thread::get_id()
            << "batchsize = "
            << batch.tasks.size()
            << std::endl;
	std::vector<std::string> results=inference_engine_.infer(context,batch
			,static_cast<int>(worker_index));


        // send推理结果
        for (size_t i=0;i<batch.tasks.size();++i)
        {
            std::string response =results[i] +'\n';
         
            send(
                batch.tasks[i].client_fd,
                response.c_str(),
                response.size(),
                0
            );
        }
    }

    context->destroy();
    std::cout
        << "Worker "
        << std::this_thread::get_id()
        << " stopped."
        << std::endl;
}

ThreadPool::~ThreadPool()
{
    if (batch_queue_ != nullptr)
    {
        batch_queue_->shutdown();
    }

    for (auto& thread : workers_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}
