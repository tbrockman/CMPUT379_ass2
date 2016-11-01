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
    while (current->next) {
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

void notify_users_disconnect(char * user_name_ptr) {
    // fork process to non-blockingly notify
    // all users that the user disconnected
    // use linked list for users
}

void notify_users_closed_socket(int socket, char * user_name_ptr) {
    shutdown(socket, 0);
    
    // remove from users
    // decrement user_cout
    notify_users_disconnect(user_name_ptr);
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

		    if (pid == 0) {
			// we're in the child
			close(fd_child[1]); // close child write
			close(fd_parent[0]); // close parent read
			close(sock); // close listening socket

			fdmax = clientfd;
			FD_ZERO(&master_read_fds); // erase read fds
			FD_ZERO(&write_fds); // erase write fds
			FD_SET(fd_parent[1], &master_write_fds); // add parent write
			FD_SET(fd_child[0], &master_read_fds); // add child read
			FD_SET(clientfd, &master_read_fds); // add client to read

			if (fd_parent[1] > fdmax) {
			    fdmax = fd_parent[1];
			}

			if (fd_child[0] > fdmax) {
			    fdmax = fd_child[0];
			}
			printf("FDMAX: %d\n", fdmax);

			// we now need to communicate with the client
			int n;
			n = htons((0xCF << 8) + 0xA7);
			
			send(clientfd, (const void *)(&n), sizeof(n), 0);
			
		    }
		    
		    else if (pid > 0) {
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
		    
		    else {
			perror("Error creating pipe\n");
			exit(1);
		    }
		}

		// we are a child, we have data from a socket
		else if (i == clientfd) {
		    unsigned short int network_length;
		    int host_length;
		    char * buff;
		    int receive;
		    receive = recv(clientfd, &network_length, sizeof(network_length), 0);

		    if (receive == -1) {
			perror("Client disconnect.\n");
			exit(1);
		    }

		    host_length = ntohs(network_length);
		    buff = malloc((host_length + 1) * sizeof(char));
		    recv(clientfd, buff, host_length, 0);
		    buff[host_length] = '\0';

		    printf("heard: %hu\n", host_length);
		    printf("and: %s\n", buff);

		    if (select(fdmax+1, NULL, &write_fds, NULL, NULL) == -1) {
			perror("Error on write select\n");
			exit(1);
		    }

		    for(int i = 0; i <= fdmax; i++) {
		    	if (FD_ISSET(i, &write_fds)) {
			    write(i, &host_length+1, sizeof(host_length));
		    	    write(i, buff, host_length+1);
		    	}
		    }

		    // write this to master pipe

		}

		// other we have read data from a pipe
		// types:
		// user disconnect -> remove user from list, tell users
		// user connect -> add user to linked list, tell users
		// user message -> send to all users
		else {
		    int read_length;
		    char * read_buff;

		    read(i, &read_length, sizeof(read_length));
		    read_buff = malloc(read_length * sizeof(char));
		    read(i, read_buff, read_length);
		    printf("heard from pipe: %s\n", read_buff);
		}
	    }
	    
	}
	char test1[10] = "test123";
	unsigned short int length = 10;
	struct node * test;
	test = create_node(test1, length);
	free(test);
	//remove_node(&test);
    }
}
