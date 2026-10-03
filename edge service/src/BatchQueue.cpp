#include "BatchQueue.hpp"

void BatchQueue::push(Batch batch)
{
	    {
		            std::lock_guard<std::mutex> lock(mutex_);

			            if (stopped_)
					            {
							                return;
									        }

				            queue_.push(std::move(batch));
					        }

	        condition_.notify_one();
}

bool BatchQueue::pop(Batch& batch)
{
	    std::unique_lock<std::mutex> lock(mutex_);

	        condition_.wait(lock, [this]
				    {
				            return stopped_ || !queue_.empty();
					        });

		    if (queue_.empty() && stopped_)
			        {
					        return false;
						    }

		        batch = std::move(queue_.front());

			    queue_.pop();

			        return true;
}

void BatchQueue::shutdown()
{
	    {
		            std::lock_guard<std::mutex> lock(mutex_);

			            stopped_ = true;
				        }

	        condition_.notify_all();
}
