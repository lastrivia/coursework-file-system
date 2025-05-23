#ifndef SOCKET_H
#define SOCKET_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "except.h"

namespace cs2313 {

    class socket_handle {
    public:
        virtual ~socket_handle() {
            if (!closed_)
                close(sockfd_);
        }

        int fd() const { return sockfd_; }

        socket_handle(const socket_handle &) = delete;

        socket_handle &operator=(const socket_handle &) = delete;

        socket_handle(socket_handle &&other) noexcept {
            sockfd_ = other.sockfd_;
            addr_ = other.addr_;
            closed_ = other.closed_;
            other.closed_ = true;
        }

        socket_handle &operator=(socket_handle &&other) noexcept {
            if (!closed_)
                close(sockfd_);
            sockfd_ = other.sockfd_;
            addr_ = other.addr_;
            closed_ = other.closed_;
            other.closed_ = true;
            return *this;
        }

        void send_raw(const char *buf, const size_t len) const {
            size_t total_sent = 0;
            while (total_sent < len) {
                ssize_t sent = ::send(sockfd_, buf + total_sent, len - total_sent, MSG_NOSIGNAL);
                if (sent < 0) {
                    if (errno == EINTR)
                        continue;
                    if (errno == EPIPE)
                        throw except(ERROR_SOCKET_CLOSED_BY_REMOTE, "Socket disconnected");
                    if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK)
                        throw except(errno, ERROR_SOCKET_TERMINATED, "Socket terminated");
                    throw except(errno, ERROR_SOCKET_SEND_FAIL, "Socket send failed");
                }
                total_sent += sent;
            }
        }

        void recv_raw(char *buf, const size_t len) const {
            size_t total_received = 0;
            while (total_received < len) {
                ssize_t received = ::recv(sockfd_, buf + total_received, len - total_received, 0);
                if (received == 0)
                    throw except(ERROR_SOCKET_CLOSED_BY_REMOTE, "Socket disconnected");
                if (received < 0) {
                    if (errno == EINTR)
                        continue;
                    if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK)
                        throw except(errno, ERROR_SOCKET_TERMINATED, "Socket terminated");
                    throw except(errno, ERROR_SOCKET_RECV_FAIL, "Socket receive failed");
                }
                total_received += received;
            }
        }

        template<typename T>
        void send(const T &t) {
            send_raw(reinterpret_cast<const char *>(&t), sizeof(T));
        }

        template<typename T>
        void recv(T &t) {
            recv_raw(reinterpret_cast<char *>(&t), sizeof(T));
        }

        void send_str(const std::string &str) {
            send(str.length());
            send_raw(str.data(), str.length());
        }

        void recv_str(std::string &str) {
            size_t len;
            recv(len);
            str.resize(len);
            recv_raw(str.data(), len);
        }

        void terminate() {
            if (!closed_) {
                close(sockfd_);
                closed_ = true;
            }
        }

    protected:
        int sockfd_;
        sockaddr_in addr_;
        bool closed_;

        explicit socket_handle() : closed_(true) {}
    };

    class server_connection_socket_handle : public socket_handle {

        friend class server_socket_handle;

    public:
        server_connection_socket_handle() : socket_handle() {}

        bool connected() const { return !closed_; }
    };

    class server_socket_handle : public socket_handle {
    public:
        explicit server_socket_handle(const uint16_t port) : socket_handle(), block_(true) {
            sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_ < 0)
                throw except(errno, ERROR_SOCKET_CREATE_FAIL, "Socket creation failed");
            closed_ = false;

            addr_.sin_family = AF_INET;
            addr_.sin_addr.s_addr = htonl(INADDR_ANY);
            addr_.sin_port = htons(port);
            if (bind(sockfd_, reinterpret_cast<sockaddr *>(&addr_), sizeof(sockaddr_in)) != 0) {
                close(sockfd_);
                throw except(errno, ERROR_SOCKET_CREATE_FAIL, "Socket creation failed");
            }

            listen(sockfd_, BACKLOG_MAX);
        }

        server_connection_socket_handle accept(bool block = true) {
            if (block != block_) {
                if (block)
                    fcntl(sockfd_, F_SETFL, fcntl(sockfd_, F_GETFL, 0) & (~O_NONBLOCK));
                else
                    fcntl(sockfd_, F_SETFL, fcntl(sockfd_, F_GETFL, 0) | O_NONBLOCK);
                block_ = block;
            }
            server_connection_socket_handle connection;
            socklen_t len = sizeof(sockaddr_in);
            connection.sockfd_ = ::accept(sockfd_, reinterpret_cast<sockaddr *>(&connection.addr_), &len);
            if (connection.sockfd_ < 0) {
                if (!block && (errno == EAGAIN || errno == EWOULDBLOCK))
                    return /* closed */ connection;
                if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK)
                    throw except(errno, ERROR_SOCKET_TERMINATED, "Socket terminated");
                throw except(errno, ERROR_SOCKET_CONNECT_FAIL, "Connection failed");
            }
            connection.closed_ = false;
            return connection;
        }

    private:
        bool block_;
        static constexpr int BACKLOG_MAX = 5;
    };

    class client_socket_handle : public socket_handle {
    public:
        client_socket_handle(const std::string &server_addr, const uint16_t server_port) : socket_handle() {
            sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_ < 0)
                throw except(errno, ERROR_SOCKET_CREATE_FAIL, "Socket creation failed");

            closed_ = false;

            addr_.sin_family = AF_INET;
            addr_.sin_port = htons(server_port);
            if (inet_pton(AF_INET, server_addr.c_str(), &addr_.sin_addr) <= 0) {
                close(sockfd_);
                throw except(errno, ERROR_SOCKET_CREATE_FAIL, "Invalid address");
            }

            if (connect(sockfd_, reinterpret_cast<sockaddr *>(&addr_), sizeof(addr_)) < 0) {
                close(sockfd_);
                throw except(errno, ERROR_SOCKET_CONNECT_FAIL, "Connection failed");
            }
        }
    };


}

#endif
