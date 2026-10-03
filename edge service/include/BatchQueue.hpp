#pragma once

#include "Batch.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>

class BatchQueue
{
public:
    void push(Batch batch);

    bool pop(Batch& batch);

    void shutdown();

private:
    std::queue<Batch> queue_;

    std::mutex mutex_;

    std::condition_variable condition_;

    bool stopped_ = false;
};
