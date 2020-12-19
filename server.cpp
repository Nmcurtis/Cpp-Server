// User library includes:
#include "utility.h"


// Standard library includes:
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <exception>
#include <thread>


// Using declarations:
using std::cout;
using std::endl;
using std::to_string;
using std::exception;
using std::string;
using std::thread;


// File level constants:
const char* const USAGE_MESSAGE_c = "Usage: ./server {port number}";
const char* const USER_SELECTED_FOLLOWING_PORT_c = "> User selected to run the server on port: ";
const char* const USER_DID_NOT_SPECIFY_PORT_c = "> User did not specify a port, picking a port for the user";
const char* const SERVER_LISTENING_ON_PORT_c = "> Server live and listening on port: ";
const char* const HANDLING_NEW_CONNECTION_c = "> Handling new connection: ";

const char* const ENCOUNTERED_AN_ERROR_OPENING_STREAM_SOCKET_c = "> Encountered a fatal error while opening stream socket";
const char* const ENCOUNTERED_ERROR_SETTING_SOCKET_OPTIONS_c = "> Encountered a fatal error while setting socket options";
const char* const ENCOUNTERED_ERROR_BINDING_STREAM_SOCKET_c = "> Encountered a fatal error while binding stream socket";
const char* const ENCOUNTERED_ERROR_FETCHING_SOCKET_PORT_c = "> Encountered a fatal error while fetching socket port";
const char* const ENCOUNTERED_ERROR_ACCEPTING_NEW_CONNECTION_c = "> Encountered a fatal error while attempting to accept a new connection";


constexpr int SERVER_SOCKET_QUEUE_SIZE_c = 30;
constexpr int MAX_MESSAGE_SIZE_IN_BYTES_c = 4096;


/* Statically linked helper functions */

// Establishes a socket connection and indefinitely listens for
// requests to the server...
//
// port_number describes the port on which the server runs
// queue_size describes the size of the socket queue to be used
//
// Throws exceptions on failures...
static void run_server(int port_number, int socket_queue_size);


// Make a server sockaddr given a port.
//
// addr: a pointer to the sockaddr to modify
// port: the port on which to listen for incoming connections.
//
// Throws exceptions on failures...
static void make_server_sockaddr(struct sockaddr_in *addr, int port);


// Return the port number that was assigned to a given socket
//
// sock_fd: A socket file descriptor
//
// Returns the port of the socket if it exists
//
// Throws exceptions on failures...
static int get_port_number(int sock_fd);


// Services a connection to the server, thread safe
//
// connection_fd: File descriptor for a socket connection
//
// Throws exceptions on failures...
static void handle_connection(int connection_fd);


int main(int argc, char** argv) {
    // Anything more than two command line args is an error:
    if (argc > 2) {
        thread_safe_print(USAGE_MESSAGE_c);
        return -1;
    }

    // Extract the port information:
    int port = 0;
    if (argc == 2) {  // Port was passed in via the command line
        port = atoi(argv[1]);
        thread_safe_print(string(USER_SELECTED_FOLLOWING_PORT_c) + to_string(port));
    }
    else {  // Let the OS decide a port
        port = 0;
        thread_safe_print(USER_DID_NOT_SPECIFY_PORT_c);
    }

    // Try to establish and run the server:
    try {
      run_server(port, SERVER_SOCKET_QUEUE_SIZE_c);
    }
    catch (exception& err) {
        thread_safe_print(err.what());
        return -1;
    }
}


// Establishes a socket connection and indefinitely listens for
// requests to the server...
//
// port_number describes the port on which the server runs
// queue_size describes the size of the socket queue to be used
//
// Throws exceptions on failures...
void run_server(int port_number, int socket_queue_size) {

  // (1.0) Establish the initial socket:
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    throw Server_Error(ENCOUNTERED_AN_ERROR_OPENING_STREAM_SOCKET_c);
  }

  // (2.0) Allow the socket to reuse its port:
  int yes_val = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes_val, sizeof(yes_val)) == -1) {  // Could not update socket to reuse port
    throw Server_Error(ENCOUNTERED_ERROR_SETTING_SOCKET_OPTIONS_c);
  }

  // (3.0) Create a sockaddr_in struct for the specified port:
  sockaddr_in addr;
  make_server_sockaddr(&addr, port_number);

  // (3.1) Bind the socket to the port:
  if (bind(socket_fd, (sockaddr *) &addr, sizeof(addr)) == -1) {
    throw Server_Error(ENCOUNTERED_ERROR_BINDING_STREAM_SOCKET_c);
  }

  // (3.2) Fetch the port number from the socket descriptor:
  port_number = get_port_number(socket_fd);
  thread_safe_print(string(SERVER_LISTENING_ON_PORT_c) + to_string(port_number));

  // (4) Begin listening for incoming connections:
  listen(socket_fd, socket_queue_size);

  // (5) Serve incoming connections one by one indefinitely:
  do {
    int connection_fd = accept(socket_fd, 0, 0);

    if (connection_fd == -1) {
      throw Server_Error(ENCOUNTERED_ERROR_ACCEPTING_NEW_CONNECTION_c);
    }

    // Spawn a thread to handle the incoming connection:
    thread service_connection(handle_connection, connection_fd);
    service_connection.detach();

  } while (true);
}


// Make a server sockaddr given a port.
//
// addr: a pointer to the sockaddr to modify
// port: the port on which to listen for incoming connections.
//
// Throws exceptions on failures...
void make_server_sockaddr(struct sockaddr_in *addr, int port) {
  // (1.0): Specify the socket family...
  // This is an internet socket:
  addr->sin_family = AF_INET;

  // (2.0): Specify socket address (hostname)...
  // The socket will be a server, so it will only be listening.
  // Let the OS map it to the correct address.
  addr->sin_addr.s_addr = INADDR_ANY;

  // (3.0): Set the port value...
  // If port is 0, the OS will choose the port for us.
  // Use htons to convert from local byte order to network byte order:
  addr->sin_port = htons(port);
}


// Return the port number that was assigned to a given socket
//
// sock_fd: A socket file descriptor
//
// Returns the port of the socket if it exists
//
// Throws exceptions on failures...
int get_port_number(int sock_fd) {
  sockaddr_in addr;
  socklen_t length = sizeof(addr);

  if (getsockname(sock_fd, (sockaddr *) &addr, &length) == -1) {  // Could not find the port of the provided socket
    throw Server_Error(ENCOUNTERED_ERROR_FETCHING_SOCKET_PORT_c);
  }
  // Use ntohs to convert from network byte order to host byte order.
  return ntohs(addr.sin_port);
}


// Services a connection to the server, thread safe
//
// connection_fd: File descriptor for a socket connection
//
// Throws exceptions on failures...
void handle_connection(int connection_fd) {
  thread_safe_print(string(HANDLING_NEW_CONNECTION_c) + to_string(connection_fd));

  // Buffer to read the client message into:
  char client_message[MAX_MESSAGE_SIZE_IN_BYTES_c];
  memset(client_message, 0, sizeof(client_message));
}
