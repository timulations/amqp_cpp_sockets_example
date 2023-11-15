#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <amqpcpp.h>
#include <map>

#include "unix_socket_handler.h"


struct UnixSocketConnectionHandler::Impl {

    /* create socket and connect on start */
    Impl(const std::string& hostname, int port): _hostname{hostname}, _port{port} {}

    ~Impl() {
        for (const auto& entry: _connection_to_sockfd) {
            close(entry.second);
        }
    }

    /* open a new tcp socket for a new connection, and connect(). */
    int openNewTcpSocket() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_port);

        if (inet_pton(AF_INET, _hostname.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            throw std::runtime_error("Invalid address / Address not supported");
        }

        if (connect(sockfd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Connection failed");
        }

        return sockfd;
    }

    void onData(AMQP::Connection *connection, const char *data, size_t size) {
        if (!_connection_to_sockfd.count(connection)) {
            _connection_to_sockfd[connection] = openNewTcpSocket();
            _connection_buffer[connection] = {};
            _sockfd_to_connection[_connection_to_sockfd[connection]] = connection;
        }

        send(_connection_to_sockfd[connection], data, size, 0);
    }

    void onReady(AMQP::Connection *connection) {
        if (!_connection_to_sockfd.count(connection)) {
            _connection_to_sockfd[connection] = openNewTcpSocket();
            _connection_buffer[connection] = {};
            _sockfd_to_connection[_connection_to_sockfd[connection]] = connection;
        }
    }

    void onError(AMQP::Connection *connection, const char *message) {
        if (!_connection_to_sockfd.count(connection)) {
            _connection_to_sockfd[connection] = openNewTcpSocket();
            _connection_buffer[connection] = {};
            _sockfd_to_connection[_connection_to_sockfd[connection]] = connection;
        }

        std::cerr << "Error: " << message << std::endl;
        // Error logic
    }

    void onClosed(AMQP::Connection *connection) {
        if (_connection_to_sockfd.count(connection)) {
            _sockfd_to_connection.erase(_connection_to_sockfd[connection]);
            _connection_buffer.erase(connection);
            close(_connection_to_sockfd[connection]);
            _connection_to_sockfd.erase(connection);
        }
    }

    void loop() {

        std::vector<char> buffer;
        buffer.reserve(66536); 

        while (true) {
            fd_set readfds;
            FD_ZERO(&readfds);

            int max_fd = _sockfd_to_connection.rbegin()->first;
            for (const auto& entry : _sockfd_to_connection) {
                FD_SET(entry.first, &readfds);
            }

            struct timeval timeout;
            timeout.tv_sec = 3600;
            timeout.tv_usec = 0;

            int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

            if (activity < 0) {
                std::cerr << "Error on select()" << std::endl;
                break;
            } else if (activity == 0) {
                std::cerr << "Timeout on select()" << std::endl;
                continue;
            }

            for (const auto& entry: _sockfd_to_connection) {
                auto sockfd = entry.first;
                if (FD_ISSET(sockfd, &readfds)) {
                    char buf[2048];
                    size_t read = recv(sockfd, buf, sizeof(buf) - 1, 0);

                    auto& buffer = _connection_buffer[_sockfd_to_connection[sockfd]];
                    buffer.insert(buffer.end(), buf, buf + read);

                    /* check if parseable */
                    size_t parsed = _sockfd_to_connection[sockfd]->parse(buffer.data(), buffer.size());

                    if (parsed > 0) {
                        /* restart */
                        buffer.erase(buffer.begin(), buffer.begin() + parsed);
                    }
                }
            }
        }
    }

private:
    std::string _hostname;
    int _port;
    std::map<AMQP::Connection*, int> _connection_to_sockfd;
    std::map<int, AMQP::Connection*> _sockfd_to_connection;
    std::map<AMQP::Connection*, std::vector<char>> _connection_buffer;
};


UnixSocketConnectionHandler::UnixSocketConnectionHandler(const std::string& hostname, int port)
    :pImpl{std::make_unique<Impl>(hostname, port)} {}

UnixSocketConnectionHandler::~UnixSocketConnectionHandler() {
    pImpl.reset();
}

UnixSocketConnectionHandler::UnixSocketConnectionHandler(UnixSocketConnectionHandler&& other)
    :pImpl{std::move(other.pImpl)} {}

UnixSocketConnectionHandler& UnixSocketConnectionHandler::operator=(UnixSocketConnectionHandler&& other) {
    pImpl = std::move(other.pImpl);
    return *this;
}

void UnixSocketConnectionHandler::loop() {
    pImpl->loop();
}

void UnixSocketConnectionHandler::onData(AMQP::Connection *connection, const char *data, size_t size) {
    pImpl->onData(connection, data, size);
}

void UnixSocketConnectionHandler::onReady(AMQP::Connection *connection) {
    pImpl->onReady(connection);
}

void UnixSocketConnectionHandler::onError(AMQP::Connection *connection, const char *message) {
    pImpl->onError(connection, message);
}

void UnixSocketConnectionHandler::onClosed(AMQP::Connection *connection) {
    pImpl->onClosed(connection);
}