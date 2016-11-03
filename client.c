#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"

// Returns the length of a string, sets buffer to point to received string
unsigned short int get_string_from_fd(int fd, char ** buffer_ptr) {
    unsigned short int length;
    int success;

    success = read(fd, &length, sizeof(length));
    if (!success) {
	return -1;
    }

    length = ntohs(length);

    *buffer_ptr = malloc(length * sizeof(char));
    
    success = read(fd, *buffer_ptr, length * sizeof(char));
    if (!success) {
	return -1;
    }
    return length;
}

int main(int argc, char * argv[])
{
    if (argc < 4) {
	printf("Not enough specified arguments.\n");
	exit(1);
    }

    char * hostname;
    char * port_no;
    char * username;
    int username_length;
    hostname = argv[1];
    port_no = argv[2];
    username = argv[3];

    username_length = htons(strlen(username));
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < -1) {
	perror("Client socket error.\n");
	exit(1);
    }

    struct sockaddr_in localaddr;
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    localaddr.sin_port = 0;
    if (bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr))==-1){
	perror("Error binding.\n");
	exit(1);
    }

    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_addr.s_addr = inet_addr(hostname);
    remoteaddr.sin_port = htons(atoi(port_no));
    if (connect(sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr))==-1){
	perror("Error connecting.\n");
	exit(1);
    }

    pid_t background;
    int fd[2];

    pipe(fd);
    background = fork();
    
    if (background == -1) {
	perror("Error forking.\n");
	exit(1);
    }

    // child reads from server and parent
    else if (background == 0) {
	// close write end
	close(fd[1]);

	//0xCF 0xA7 portion of the handshake
	int receive, fdmax;
	unsigned char buff[2];
	fd_set master_read_fds;
	fd_set read_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&master_read_fds);

	receive = recv(sockfd, buff, sizeof(buff), 0);

	if (!receive) {
	    perror("Error receiving handshake from server.");
	    exit(1);
	}

	if ((buff[0] != 0xcf) || (buff[1] != 0xa7)) {
	    perror("Incorrect handshake from server.");
	    exit(1);
	}

	// Numbers of users

	unsigned short int num_connected;
	recv(sockfd, &num_connected, sizeof(num_connected), 0);
	num_connected = ntohs(num_connected);

	printf("Number of users: %hu\n", num_connected);

	// Username list (malloc num_connected * sizeof(char*))

	send(sockfd, &username_length, sizeof(unsigned short int), 0);
	send(sockfd, username, username_length * sizeof(char), 0);
	printf("Sent username length: %hu", ntohs(username_length));

	FD_SET(sockfd, &master_read_fds);
	FD_SET(fd[0], &master_read_fds);
	
	if (sockfd > fd[0]) {
	    fdmax = sockfd;
	}
	else {
	    fdmax = fd[0];
	}
	while(1) {
	    read_fds = master_read_fds;
	    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
		perror("Error on slect\n");
		exit(1);
	    }

	    for (int i =0; i <= fdmax; i++) {
		if (FD_ISSET(i, &read_fds)) {
		    // info from server;
		    if (i == sockfd) {
			char type;
			if (recv(sockfd, &type, sizeof(char), 0) == -1) {
			    perror("Error reading from socket.\n");
			    exit(1);
			}
			// 0x00, 0x01, or 0x02
			
		    }
		    // stuff from pipe
		    else {
			char * buffer;
			unsigned short int length;

			if ((length = get_string_from_fd(fd[0], &buffer)) == -1) {
			    perror("Error reading from pipe.\n");
			    exit(1);
			}
			printf("heard from buffer: %s\n", buffer);
			send(sockfd, buffer, sizeof(char) * length, 0);
		    }
		}
	    }
	}
    }

    // parent reads from stdin and writes to pipe
    else {
	close(sockfd); // close socket
	close(fd[0]); // close pipe read

	char * buffer;
	int read, error;
	size_t message_length = 0;
	unsigned short int network_order;
	while (1) {
	    //printf("%s> ", username);
	    read = getline(&buffer, &message_length, stdin);
	    network_order = htons(read-1);
	    printf("sent: %hu\n", ntohs(network_order));
	    error = write(fd[1], &network_order, sizeof(network_order));
	    if (error == -1) {
		perror("Pipe write error.\n");
		exit(1);
	    }
	    error = write(fd[1], buffer, read-1);
	    if (error == -1) {
		perror("Pipe write error.\n");
		exit(1);
	    }
	}
	return 0;
    }

}

