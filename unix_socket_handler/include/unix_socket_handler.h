#ifndef UNIX_SOCKET_HANDLER_H
#define UNIX_SOCKET_HANDLER_H

#include <amqpcpp.h>
#include <string>
#include <memory>

/* each connection handler handles a connection to a single AMQP-CPP service. 
 * it can handle multiple vhosts. */
class UnixSocketConnectionHandler : public AMQP::ConnectionHandler {
public:
    UnixSocketConnectionHandler(const std::string& hostname, int port);
    ~UnixSocketConnectionHandler();

    UnixSocketConnectionHandler(const UnixSocketConnectionHandler&) = delete;
    UnixSocketConnectionHandler& operator=(const UnixSocketConnectionHandler&) = delete;

    UnixSocketConnectionHandler(UnixSocketConnectionHandler&& other);
    UnixSocketConnectionHandler& operator=(UnixSocketConnectionHandler&& other);

    /* this function starts the event loop of the handler. It is a blocking,
     * and should be called after the handler along with the other AMQP elements are set up */
    void loop();

    void onData(AMQP::Connection *connection, const char *data, size_t size) override;
    void onReady(AMQP::Connection *connection) override;
    void onError(AMQP::Connection *connection, const char *message) override;
    void onClosed(AMQP::Connection *connection) override;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};


#endif // UNIX_SOCKET_HANDLER_H