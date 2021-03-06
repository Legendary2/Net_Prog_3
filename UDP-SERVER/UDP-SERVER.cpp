#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iomanip>
#include <thread>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "socket_headers.h"
#include "socket_wrapper.h"
#include "socket_class.h"


const int BUFFER_SZ = 256;

char buff[BUFFER_SZ];

int main(int argc, char const* argv[])
{
    using namespace std::chrono_literals;

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;

    const int port{ std::stoi(argv[1]) };

    std::cout << "Receiving messages on the port " << port << "...\n";
    
    struct sockaddr_in addr = { .sin_family = PF_INET, .sin_port = htons(port) };
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socket_wrapper::Socket sock = { AF_INET, SOCK_DGRAM, IPPROTO_UDP };

    std::cout << "Starting echo server on the port " << port << "...\n";

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&broadcast), sizeof(broadcast));

    if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr)) == -1)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    char buffer[buffer_size] = {};

    // socket address used to store client address
    struct sockaddr_in client_address = { 0 };
    socklen_t client_address_len = sizeof(sockaddr_in);
    ssize_t recv_len = 0;

    std::cout << "Running echo server...\n" << std::endl;
    char client_address_buf[INET_ADDRSTRLEN];

    while (true)
    {
        // Read content into buffer from an incoming client.
        recv_len = recvfrom(sock, buff, sizeof(buff) - 1, 0,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_len);

        if (recv_len > 0)
        {
            buff[recv_len] = '\0';
            std::cout
                << "Client with address "
                << inet_ntop(AF_INET, &client_address.sin_addr, client_address_buf, sizeof(client_address_buf) / sizeof(client_address_buf[0]))
                << ":" << ntohs(client_address.sin_port)
                << "\nClient name ";

            char buff_client_name[BUFFER_SZ];
            if (getnameinfo(reinterpret_cast<sockaddr*>(&client_address), sizeof(client_address), buff_client_name, BUFFER_SZ, nullptr, 0, NI_NAMEREQD))
            {
                std::cerr << "couldn’t resolve hostname " << std::endl;
            }
            else
            {
                std::cout << buff_client_name << std::endl;
            }


            std::cout
                << "sent datagram"
                << "[length = "
                << recv_len
                << "]:\n'''\n"
                << buff
                << "\n'''"
                << std::endl;

            sendto(sock, buff, recv_len, 0, reinterpret_cast<const sockaddr*>(&client_address), client_address_len);

            if (strcmp(buff, "exit") == 0)
            {
                std::cout << "Exit\n";
                close(sock);
                return EXIT_SUCCESS;
            }
        }
        std::cout << std::endl;

        if (recv(sock, buffer, sizeof(buffer) - 1, 0) < 0)
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << buffer << std::endl;
    }
    return EXIT_SUCCESS;
}