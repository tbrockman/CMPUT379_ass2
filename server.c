#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "server.h"

int GLOBAL_PORT;
struct node * user_linked_list_ptr;

int count_nodes() {
    int count = 0;
    struct node * current;
    current = user_linked_list_ptr;
    while (current && current->next) {
	current = current->next;
	count++;
    }
    return count;
}

struct node * create_node(char * text, unsigned short int length) {
    struct node * new_node;

    new_node = (struct node *)malloc(sizeof(struct node));
    new_node->username_ptr = text;
    new_node->length = length;
    new_node->next = NULL;

    if (!user_linked_list_ptr) {
	user_linked_list_ptr = new_node;
    }
    else {
	struct node * current;
	current = user_linked_list_ptr;
	while (current->next) {
	    current = current->next;
	}
	current->next = new_node;
    }

    return new_node;
}

// Returns the length of a string, sets buffer to point to received string
unsigned short int get_string_from_socket(int socket, char ** buffer_ptr) {
    unsigned short int length;
    int success;

    success = recv(socket, &length, sizeof(length));
    if (!success) {
	return -1
    }

    *buffer = malloc(length * sizeof(char));
    
    success = recv(socket, *buffer, length * sizeof(char));
    if (!success) {
	return -1
    }
    return length;
}

void sigterm_handler(int signo) {
    printf("Terminating...\n");
    //for each user, disconnect their sockets
    exit(0);
}

int main(int argc, char * argv[])
{
    if (!argv[1]) {
	printf("Invalid port specified.\n");
	exit(0);
    }

    daemon(0, 1); // change 1 to 0 when no longer testing

    int sock, fdmax, clientfd;
    int * timeperiod;
    pid_t current_pid;
    struct sockaddr_in sa;
    struct sockaddr_in remoteaddr;
    struct sigaction sigsegv_action;
    socklen_t addrlen;
    fd_set master_read_fds; // because select modifies read_fds
    fd_set master_write_fds;
    fd_set read_fds;
    fd_set write_fds;

    timeperiod = NULL;

    sigsegv_action.sa_handler = sigterm_handler;
    sigemptyset (&sigsegv_action.sa_mask);
    sigsegv_action.sa_flags = 0;
    sigaction(SIGTERM, &sigsegv_action, 0);
    
    FD_ZERO(&master_read_fds);
    FD_ZERO(&master_write_fds);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    current_pid = getpid();
    printf("Process ID: %d\n", (int)(current_pid));

    GLOBAL_PORT = atoi(argv[1]);

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror ("Server: cannot open master socket");
	exit(1);
    }

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons (GLOBAL_PORT);

    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
	perror("setsockopt(SO_REUSEADDR) failed");


    if (bind (sock, (struct sockaddr*) &sa, sizeof (sa))) {
	perror ("Server: cannot bind master socket\n");
	exit(1);
     }

    if (listen(sock, 5) == -1) {
	perror("Error listening on master socket\n");
	exit(1);
    }

    FD_SET(sock, &master_read_fds);
    fdmax = sock;

    printf("Listening \n");

    while(1) {
	read_fds = master_read_fds;
	write_fds = master_write_fds;

	// note for later: select returns 0 on timeout
	if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
	    perror("Error on select\n");
	    exit(1);
	}

	for(int i = 0; i <= fdmax; i++) {
	    if (FD_ISSET(i, &read_fds)) {
		// is this the listener
		if (i == sock) {
		    printf("CLIENT CONNECTION\n");
		    addrlen = sizeof remoteaddr;
                    clientfd = accept(sock, (struct sockaddr *)&remoteaddr, &addrlen);
                    if (clientfd == -1) {
                        perror("Error accepting client connection.");
                    } 

		    int fd_parent[2];
		    int fd_child[2];
		    pipe(fd_parent);
		    pipe(fd_child);
		    pid_t pid = fork();

		    if (pid == -1) {
			perror("Error forking.");
			exit(-1);
		    }

		    else if (pid == 0) {
			// we're in the child
			
			fdmax = clientfd;

			if (fd_parent[1] > fdmax) {
			    fdmax = fd_parent[1];
			}

			if (fd_child[0] > fdmax) {
			    fdmax = fd_child[0];
			}

			for (int x = 0; x <= fdmax; x++) {
			    if (!(x == fd_child[0] || x == fd_parent[1] ||
				  x == clientfd)) {
				close(x); // close all fds we don't need
			    }
			}

			FD_ZERO(&master_read_fds); // erase read fds
			FD_ZERO(&master_write_fds); // erase write fds
			FD_SET(fd_parent[1], &master_write_fds); // add parent write
			FD_SET(fd_child[0], &master_read_fds); // add child read
			FD_SET(clientfd, &master_read_fds); // add client to read

			// we now need to communicate with the client
			int n;
			unsigned short int length;
			char * buffer;
			n = htons((0xCF << 8) + 0xA7);
			printf("sending to clientfd: %d\n", clientfd);
			send(clientfd, (const void *)(&n), sizeof(n), 0);
			n = htons(count_nodes());
			send(clientfd, (const void *)(&n), sizeof(n), 0);
			if (get_string_from_socket(clientfd, &buffer) == -1) {
			    // tell server to disconnect
			    exit(1);
			}
			else {
			    printf("heard username: %s\n", buffer);
			    exit(1);
			    // inspect username, does it exit
			    // add to node
			    // tell parent to send user update
			}
		    }
		    
		    else {
			// we're in the parent
			close(fd_child[0]); // close child read;
			close(fd_parent[1]); // close parent write;
			close(clientfd); // close accepted socket, 
			printf("Shutdown socket FD in parent.\n");
			FD_SET(fd_parent[0], &master_read_fds); // add parent read
			FD_SET(fd_child[1], &master_write_fds); // add child write

			if (fd_parent[0] > fdmax) {
			    fdmax = fd_parent[0];
			}

			if (fd_child[1] > fdmax) {
			    fdmax = fd_child[1];
			}
		    }
		}

		// we are a child, we have data from a socket
		else if (i == clientfd) {
		    unsigned short int network_length;
		    unsigned short int host_length;
		    char * buff;
		    int receive;
		    receive = recv(clientfd, &network_length, sizeof(network_length), 0);
		    if (!receive) {
			perror("Client disconnect.\n");
			exit(1);
		    }

		    host_length = ntohs(network_length);
		    buff = malloc((host_length + 1) * sizeof(char));
		    receive = recv(clientfd, buff, host_length, 0);
		    if (!receive) {
			perror("Client disconnect.\n");
			exit(1);
		    }

		    if (select(fdmax+1, NULL, &write_fds, NULL, NULL) == -1) {
			perror("Error on write select\n");
			exit(1);
		    }

		    // write to parents pipe
		    for(int j = 0; j <= fdmax; j++) {
		    	if (FD_ISSET(j, &write_fds)) {
			    printf("writing length:'%d' and string:'%s' to j: %d\n", host_length+1, buff, j);
			    write(j, &host_length, sizeof(host_length));
		    	    write(j, buff, host_length);
		    	}
		    }

		    free(buff);

		}

		// other we have read data from a pipe
		// types:
		// user disconnect -> remove user from list, tell users
		// user connect -> add user to linked list, tell users
		// user message -> send to all users
		else {

		    pid_t worker;

		    // fork a process to read and then write the data
		    // to all pipes
		    
		    worker = fork();
		    
		    if (worker == -1) {
			perror("Parent fork error.\n");
			exit(-1);
		    }

		    else if (worker == 0) {
			unsigned short int read_length;
			char event;
			char * read_buff;

			read(i, &event, sizeof(char));
		    
			if (event == CONNECT) {
			    printf("connect\n");
			}
			else if (event == DISCONNECT) {
			    printf("disconnect\n");
			}
			else if (event == MESSAGE) {
			    printf("message\n");
			}

			exit(1);
		    }

		    //read(i, &read_length, sizeof(read_length));
		    //read_buff = malloc(read_length * sizeof(char));
		    //read(i, read_buff, sizeof(char) * read_length);
		    //printf("heard from pipe: %s\n", read_buff);
		    exit(1);
			//}


		}
	    }
	    
	}
    }
}
