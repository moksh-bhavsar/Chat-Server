#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>

#include "RobustIO.h"

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void *send_message(void* client_socket);
void *recieving_message(void* client_socket);
string username;


int main(int argc, char *argv[]) {
	struct addrinfo hints;
	struct addrinfo *addr;
	struct sockaddr_in *addrinfo;
	int rc;
	int sock;
	int len;

    // Clear the data structure to hold address parameters
    memset(&hints, 0, sizeof(hints));

    // TCP socket, IPv4/IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    // Get address info for local host
    rc = getaddrinfo("localhost", NULL, &hints, &addr);
    if (rc != 0) {
        // Note how we grab errors here
        printf("Hostname lookup failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    // Copy the address info, use it to create a socket
    addrinfo = (struct sockaddr_in *) addr->ai_addr;

    sock = socket(addrinfo->sin_family, addr->ai_socktype, addr->ai_protocol);
    if (sock < 0) {
        printf("Can't connect to server\n");
		exit(1);
    }

    // Make sure port is in network order
    addrinfo->sin_port = htons(4321);

    // Connect to the server using the socket and address we resolved
    rc = connect(sock, (struct sockaddr *) addrinfo, addr->ai_addrlen);
    if (rc != 0) {
        printf("Connection failed\n");
        exit(1);
    }

    // Clear the address struct
    freeaddrinfo(addr);

    // using count variable to keep count of number of users to create two threads per user
    int count = 0; 
    if (argc < 2){
        cout << "Enter username: ";
        getline(cin, username);
    }else{
        username = argv[1];
    }

    RobustIO::write_string(sock, username);
    count++;

    //Threads to handle sending and reciving the message to and from the server
    pthread_t message_sender_handler;
    pthread_t message_recieve_handler;
    pthread_create(&message_sender_handler, NULL, send_message, &sock);
    pthread_create(&message_recieve_handler, NULL, recieving_message, &sock);

    pthread_join(message_recieve_handler, NULL);
    pthread_join(message_sender_handler, NULL);
	close(sock);
}

// function created to use pthreads to handle recieving of messages from server
void *recieving_message(void* clientSocket) { 
    int client_socket = *((int *)clientSocket);
    char buf[512];
    
    while (true)
    {
        auto s = RobustIO::read_string(client_socket); 
        strcpy(buf, s.c_str());
        printf("%s\n", buf); 
        memset(&buf, 0, 512);
    }
    pthread_exit(NULL); 
}

// function created to use pthreads to handle sending of messages to server
void *send_message(void* clientSocket) {
    int client_socket = *((int *)clientSocket);
    char tempBuf[64];
    char buf[512];

    while (true) {
        string message = username + ": ";
        string msg;
        getline(cin, msg);
        if (msg == "exit" || msg == "Exit"){
            RobustIO::write_string(client_socket, msg);
            pthread_exit(NULL);
            exit(1);
        }else{ 
            message = message + msg;
            RobustIO::write_string(client_socket, message); 
        }
    }

    pthread_exit(NULL);
}
