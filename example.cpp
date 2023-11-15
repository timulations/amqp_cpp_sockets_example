#include <amqpcpp.h>
#include "unix_socket_handler.h"

int main() {
    // create an instance of your own connection handler
    UnixSocketConnectionHandler myHandler("127.0.0.1", 5672);

    // create a AMQP connection object
    AMQP::Connection connection(&myHandler, AMQP::Login("guest","guest"), "/");

    // and create a channel
    AMQP::Channel channel(&connection);

    // Declare a queue
    channel.declareQueue("hello");

    // Send a message
    channel.publish("", "hello", "Hello World!");

    // Run the event loop
    myHandler.loop();
}