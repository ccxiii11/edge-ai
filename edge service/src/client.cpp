#include <iostream>
#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1)
    {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1",
                  &server_addr.sin_addr) <= 0)
    {
        std::cerr << "inet_pton failed" << std::endl;
        close(fd);
        return 1;
    }

    if (connect(fd,
                reinterpret_cast<sockaddr*>(&server_addr),
                sizeof(server_addr)) == -1)
    {
        std::cerr << "connect failed" << std::endl;
        close(fd);
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    for (int i = 0; i < 20; ++i)
    {
        std::string request =
            "request_" + std::to_string(i)+"\n";

        send(
            fd,
            request.c_str(),
            request.size(),
            0
        );
    }
    
 
    char buffer[1024];

std::string recv_buffer;

int received_count = 0;

while (received_count < 20)
{
    ssize_t n = recv(
        fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (n <= 0)
    {
        break;
    }

    recv_buffer.append(buffer, n);

    while (true)
    {
        size_t pos =
            recv_buffer.find('\n');

        if (pos == std::string::npos)
        {
            break;
        }

        std::string result =
            recv_buffer.substr(0, pos);

        recv_buffer.erase(
            0,
            pos + 1
        );

        std::cout
            << "Receiver:"
            << result
            << std::endl;

        received_count++;

        if (received_count == 20)
        {
            break;
        }
    }
}
    close(fd);

    return 0;
}
