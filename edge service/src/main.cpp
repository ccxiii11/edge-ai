#include "Server.hpp"
#include "TaskQueue.hpp"
#include "BatchQueue.hpp"
#include "BatchScheduler.hpp"
#include "ThreadPool.hpp"

#include <iostream>
#include <exception>
#include <thread>

int main()
{
    try
    {
        TaskQueue task_queue;

        BatchQueue batch_queue;

        BatchScheduler scheduler(
            task_queue,
            4,
            10
        );

        ThreadPool thread_pool(3);

        std::thread scheduler_thread(
            &BatchScheduler::run,
            &scheduler,
            std::ref(batch_queue)
        );

        thread_pool.start(batch_queue);

        Server server(
            8080,
            task_queue
        );

        server.run();

        scheduler_thread.join();
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Server error: "
            << e.what()
            << std::endl;

        return 1;
    }

    return 0;
}
