#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <deque>

#include "RobustIO.h"


// creating structure to store client id via socket number and the username of client
struct client_info{
        int sock; 
        std::string userid; 
};


//Using deque to store client information in a fifo format
std::deque<struct client_info> client_log;

//Using deque to store the chat history in a fifo format
std::deque<std::string> chat_log;

void remove_client(int sock){
	for (auto it = client_log.begin(); it != client_log.end(); it++){
		if (it->sock == sock){
			client_log.erase(it);
			break;
		}
	}
}


/*
*	Function to handle the message sent by each client using pthreads
*
*/
void *handleMessage(void *clientSocket){
	client_info *newClient = new client_info(); 

	int client_socket = *((int *)clientSocket); // type casting the void * parameter to int * for use
	char buffer[512];

	std::string s = RobustIO::read_string(client_socket); //Read the message sent by the client
	strcpy(buffer, s.c_str()); // copy the message to the buffer


	// initialize the struct the values
	newClient->sock = client_socket; 
	newClient->userid = buffer;

	// add the new client to the client log
	client_log.push_back(*newClient); 

	// clear the memory after the use
	memset(&buffer, 0, 512); 

	// use for loop to send the chat history to the new user
	if (chat_log.empty()){
		RobustIO::write_string(client_socket, "This is the beginning of the converstation");
	}else{
		for (auto iterator = chat_log.begin(); iterator != chat_log.end(); ++iterator){
			RobustIO::write_string(client_socket, *iterator);
		}
	}

	while (true) {
		std::string s = RobustIO::read_string(client_socket);
		if (s == "exit" || s == "Exit"){
			pthread_exit(NULL);
			remove_client(client_socket);
			close(client_socket);
		}else{
			strcpy(buffer, s.c_str());
		}
		


		//Send new message to the users other than the sender
		for (auto iterator = client_log.begin(); iterator != client_log.end(); ++iterator){
			// checking if the user denoted by the user is not the sender itself
			if (strcmp(newClient->userid.c_str(), iterator->userid.c_str()) == 0){
				continue;
			} else{
				RobustIO::write_string(iterator->sock, buffer);
			}
		}

		// outputting the new message recieved
		printf("%s\n", buffer); 

		// keep chat history of 12 messages/strings
		if (chat_log.size() >= 12){
			chat_log.pop_front();
		}
		chat_log.push_back(buffer);
		
		memset(&buffer, 0, 512);

	}

	pthread_exit(NULL);
	close(client_socket);
	
}

int main(int argc, char **argv)
{
	int sock, conn;
	int i;
	int rc;
	struct sockaddr address;
	socklen_t addrLength = sizeof(address);
	struct addrinfo hints;
	struct addrinfo *addr;
	char buffer[512];
	int len;

	// Clear the address hints structure
	memset(&hints, 0, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM;			 // TCP
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // IPv4/6, socket is for binding
	
	
	//Listening on port 4321 as port 55555 does not connect server and client on mac
	if ((rc = getaddrinfo(NULL, "4321", &hints, &addr)))
	{
		printf("host name lookup failed: %s\n", gai_strerror(rc));
		exit(1);
	}

	// Create a socket
	sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (sock < 0)
	{
		printf("Can't create socket\n");
		exit(1);
	}

	// Set the socket for address reuse, so it doesn't complain about
	// other servers on the machine.
	i = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

	// Bind the socket
	rc = bind(sock, addr->ai_addr, addr->ai_addrlen);
	if (rc < 0)
	{
		printf("Can't bind socket\n");
		exit(1);
	}

	// Clear up the address data
	freeaddrinfo(addr);

	// Listen for new connections, wait on up to five of them
	rc = listen(sock, 5);
	if (rc < 0)
	{
		printf("listen failed\n");
		exit(1);
	}

	int ret;
	//Declares thread to handle message sent by the client
	pthread_t handleReadThread;

	// When we get a new connection, try reading some data from it!
	while ((conn = accept(sock, (struct sockaddr *) &address, &addrLength)) >= 0)
	{
		// Create one threads for recieving
		ret = pthread_create(&handleReadThread, NULL, handleMessage, &conn);
		if (ret != 0) {
			printf("Cannot create the thread.");
		}
	}
	ret = pthread_join(handleReadThread, NULL);
}

