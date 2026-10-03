#include "Server.hpp"
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include<sys/epoll.h>
#include <netinet/in.h>
#include<fcntl.h>
#include <unistd.h>
#include<cerrno>
#include<string>
Server::Server(int port,TaskQueue &task_queue)
    : port_(port),
      server_fd_(-1),
      epoll_fd_(-1),
	task_queue_(task_queue)
{
}

int Server::createListenSocket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1)
    {
        throw std::runtime_error("socket failed");
    }

    int opt=1;
    if(setsockopt(fd,
		   SOL_SOCKET,
		   SO_REUSEADDR,
		   &opt,
		   sizeof(opt))==-1)
    {
	    close(fd);
	    throw std::runtime_error("setsockopt fail");
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(fd,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) == -1)
    {
        close(fd);
        throw std::runtime_error("bind failed");
    }

    if (listen(fd, 10) == -1)
    {
        close(fd);
        throw std::runtime_error("listen failed");
    }
    setNonBlocking(fd);

    return fd;
}
void Server::setNonBlocking(int fd)
{
	int flags=fcntl(fd,F_GETFL,0);
	if(flags==-1)
	{
		throw std::runtime_error("fcntl F_GETFL failed");

	}
	if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)==-1)
	{
		throw std::runtime_error("fcntl FSETFL failed");
	}
	std::cout<<"fd"
		<<fd
		<<"set to non-blocking"
		<<std::endl;


}

void Server::run()
{
    server_fd_ = createListenSocket();

    std::cout << "Server listening on port "
              << port_ << std::endl;
    
    
    setupEpoll();

    std::cout << "Epoll statr.....!" << std::endl;

    handleEvents();
}

void Server::setupEpoll()
{
    epoll_fd_ = epoll_create1(0);

    if (epoll_fd_ == -1)
    {
        throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event event{};

    event.events = EPOLLIN;
    event.data.fd = server_fd_;

    if (epoll_ctl(epoll_fd_,
                  EPOLL_CTL_ADD,
                  server_fd_,
                  &event) == -1)
    {
        close(epoll_fd_);
        throw std::runtime_error("epoll_ctl failed");
    }
}



void Server::handleEvents()
{
    constexpr int MAX_EVENTS = 10;

    epoll_event events[MAX_EVENTS];

    while (true)
    {
        int count = epoll_wait(epoll_fd_,
                               events,
                               MAX_EVENTS,
                               -1);

        if (count == -1)
        {
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd_)
            {
		while(1){
                int client_fd = accept(server_fd_,
                                        nullptr,
                                        nullptr);

                if (client_fd == -1)
                {
		    if(errno==EAGAIN||errno==EWOULDBLOCK){
                    break;
		    }

		    throw std::runtime_error("accept failed");
                }
		setNonBlocking(client_fd);

                epoll_event client_event{};

                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;

                if (epoll_ctl(epoll_fd_,
                              EPOLL_CTL_ADD,
                              client_fd,
                              &client_event) == -1)
                {
                    close(client_fd);
                    continue;
                }

                std::cout << "New client connected, fd = "
                          << client_fd
                          << std::endl;
		}
            }
            else
            {
                char buffer[1024];

                ssize_t n = read(fd, buffer, sizeof(buffer)-1);

                if (n == 0)
                {
                    std::cout << "Client disconnected, fd = "
                              << fd
                              << std::endl;

                    epoll_ctl(epoll_fd_,
                              EPOLL_CTL_DEL,
                              fd,
			      nullptr);

		    recv_buffers_.erase(fd);
                    close(fd);
                }
		else if(n>0)
		{
		    recv_buffers_[fd].append(buffer,n);
		    std::string &recv_buffer=recv_buffers_[fd];
		    while(1)
		    {
			    size_t pos =recv_buffer.find('\n');
			    if(pos==std::string::npos)
			    {
				    break;
			    }
			    std::string data =recv_buffer.substr(0,pos);
			    recv_buffer.erase(0,pos+1);
			    if(data.empty())
			    {
				    continue;
			    }
			    std::cout<<"Received complete request :"
				    <<data
				    <<std::endl;
		            Task task;
		            task.client_fd =fd;
		            task.data=data;

		    task_queue_.push(std::move(task));
		    }
                }
		else
		{
			if(errno==EAGAIN||errno==EWOULDBLOCK)
			{
				continue;
			}
			std::cerr<<"read failed\n";
			epoll_ctl
				(
				 epoll_fd_,
				 EPOLL_CTL_DEL,
				 fd,
				 nullptr
				 );
			close(fd);
		}
            }
        }
    }
}

