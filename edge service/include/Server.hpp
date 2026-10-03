#pragma once

#include"TaskQueue.hpp"
#include<unordered_map>
#include<string>
class Server
{
	public:
		explicit Server(int port,TaskQueue& task_queue);

		void run();
	private:
		int createListenSocket();
		void setNonBlocking(int fd);
		void setupEpoll();
		void handleEvents();
	private:
		int port_;
		int server_fd_;
		int epoll_fd_;

		std::unordered_map<int,std::string> recv_buffers_;
		TaskQueue & task_queue_;
};
