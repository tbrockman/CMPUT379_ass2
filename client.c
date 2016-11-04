#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"

struct node * user_linked_list_ptr;

void read_users_into_linked_list(int socket_fd, unsigned short int user_count, struct node ** user_linked_list_ptr) {
    for (int i = 0; i < user_count; i++) {
	char * username_buffer;
	unsigned short int length;
	length = get_string_from_fd(socket_fd, &username_buffer);
	create_node(username_buffer, length, 0, user_linked_list_ptr);
    }
    return;
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

    int fd_pipe[2];
    pid_t background;
    
    pipe(fd_pipe);
    background = fork();
    
    if (background == -1) {
	perror("Error forking.\n");
	exit(1);
    }

    // Child prints stuff from socket in server

    else if (background == 0) {

	if (getppid() == 1) {
	    printf("Parent closed.\n");
	    exit(1);
	}


	//0xCF 0xA7 portion of the handshake
	int receive, fdmax;
	unsigned char buff[2];
	fd_set master_read_fds;
	fd_set read_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&master_read_fds);
	close(fd_pipe[1]); // close write end of the pipe
	close(fileno(stdin));

	receive = recv(sockfd, buff, sizeof(buff), 0);

	if (receive == -1) {
	    perror("Error receiving handshake from server.");
	    exit(1);
	}

	if ((buff[0] != 0xcf) || (buff[1] != 0xa7)) {
	    perror("Incorrect handshake from server.");
	    exit(1);
	}

	fdmax = sockfd;

	unsigned short int num_connected;
	if (recv(sockfd, &num_connected, sizeof(num_connected), 0) == -1) {
	    perror("Error retrieving number of users.\n");
	    exit(1);
	}

	num_connected = ntohs(num_connected);

	printf("Number of users: %hu\n", num_connected);

	read_users_into_linked_list(sockfd, num_connected, &user_linked_list_ptr);
	if (send(sockfd, &username_length, sizeof(unsigned short int), 0) == -1) {
	    perror("Error sending username.\n");
	    exit(1);
	}

	if (send(sockfd, username, ntohs(username_length) * sizeof(char), 0) == -1) {
	    perror("Error sending username.\n");
	    exit(1);
	}

	FD_SET(sockfd, &master_read_fds);
	FD_SET(fd_pipe[0], &master_read_fds);

	if (fd_pipe[0] > sockfd) {
	    fdmax = fd_pipe[0];
	}
	else {
	    fdmax = sockfd; // if they're the same who cares
	}

	while(1) {
	    read_fds = master_read_fds;
	    int select_status;

	    select_status = select(fdmax+1, &read_fds, NULL, NULL, NULL          );

	    if (select_status == -1) {
		perror("Error on slect\n");
		exit(1);
	    }

	    else {
		for (int i =0; i <= fdmax; i++) {
		    if (FD_ISSET(i, &read_fds)) {
			// info from server;
			char translation;
			int type;
			unsigned short int length;
			char * text_buffer;

			if (i == sockfd) {

			    if (recv(sockfd, &translation, sizeof(char), 0) == -1) {
				perror("Error reading from socket.\n");
				exit(1);
			    }

			    if (translation == 0x00) {
				type = 0;
			    }
			    
			    else if (translation == 0x01) {
				type = 1;
			    }

			    else if (translation == 0x02) {
				type = 2;
			    }

			    if ((length = get_string_from_fd(sockfd, &text_buffer)) == -1) {
				perror("Error reading from socket.\n");
				exit(1);
			    }

			    if (type == USR_MESSAGE) {
				printf("User message: %s\n", text_buffer);
			    }

			    else if (type == USR_CONNECT) {
				printf("< User \"%s\" has connected to the chat. > \n", text_buffer);
				create_node(text_buffer, length, 0, &user_linked_list_ptr);
			    }

			    else if (type == USR_DISCONNECT) {
				printf("< User \"%s\" has left the chat.\n", text_buffer);
				remove_node(text_buffer, &user_linked_list_ptr);
			    }
			}

			// pipe info from stdin, process commands
			else if (i == fd_pipe[0]) {
			    int read_status;
			    read_status = read(fd_pipe[0], &type, sizeof(int));
			    if (read_status == -1 || read_status == 0) {
				close(fd_pipe[0]);
				perror("Error reading from parent pipe.\n");
				exit(1);
			    }

			    // could allow for multiple commands, currently
			    // only the one

			    if (type == LIST_USERS) {
				int node_count;
				char ** usernames;
				node_count = count_nodes_and_return_usernames(&usernames, user_linked_list_ptr);
				printf("Users currently in chat:\n");
				for (int i = 0; i < node_count; i++) {
				    printf("%s\n", usernames[i]);
				}
			    }
			}
			else {
			    exit(1);
			}
		    }
		}
	    }
	}
    }

    // parent reads from stdin and writes to socket
    else {

	char * buffer = NULL;
	int read, error, fdmax;
	size_t message_length = 0;
	unsigned short int network_order;
        fd_set read_stdin, master;

	FD_ZERO(&read_stdin);
	FD_ZERO(&master);
	FD_SET(fileno(stdin), &master); 
	fdmax = fileno(stdin);

	close(fd_pipe[0]); // close read end

	while (1) {
	    //printf("%s> ", username);
	    int select_status;
	    struct timeval thirty_seconds;
	    read_stdin = master;
	    thirty_seconds.tv_sec = 30;
	    thirty_seconds.tv_usec = 0;
	    select_status = select(fdmax+1, &read_stdin, NULL, NULL, &thirty_seconds);
	    
	    if (select_status == -1) {
		perror("Waiting on STDIN error.\n");
		exit(1);
	    }
	    
	    else if (select_status == 0) {
		printf("Sending dummy message.\n");
		network_order = 0;
		if (send(sockfd, &network_order, sizeof(unsigned short int), 0) == -1) {
		    perror("Error sending dummy message.\n");
		    exit(1);
		}
	    }

	    else {
		read = getline(&buffer, &message_length, stdin);
		if (strcmp(buffer, "/list\n") == 0) {
		    int command;
		    command = LIST_USERS;
		    if (write(fd_pipe[1], &command, sizeof(int)) == -1) {
			perror("Socket write error.\n");
			exit(1);
		    }
		}

		else {

		    network_order = htons(read-1);
		    error = send(sockfd, &network_order, sizeof(network_order), 0);
		    if (error == -1) {
			close(sockfd);
			perror("Client socket write error.\n");
			exit(1);
		    }

		    error = write(sockfd, buffer, sizeof(char) * (read-1));
		    if (error == -1) {
			close(sockfd);
			perror("Client socket write error.\n");
			exit(1);
		    }
		}
	    }

	}
	return 0;
    }

}

