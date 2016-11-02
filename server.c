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

int username_exists(char * username_ptr) {
    struct node * current;
    current = user_linked_list_ptr;
    if (current && *(current->username_ptr) == *username_ptr) {
	return 1;
    }

    while (current && current->next) {
	current = current->next;
	if (*(current->username_ptr) == *username_ptr) {
	    return 1;
	}
    }
    return 0;
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

    int sock, fdmax, clientfd, child_read, needs_to_connect;
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
			    if (x != fd_child[0] && x != fd_parent[1] &&
				x != clientfd && x != fileno(stdout)) {
				if (close(x) == -1) {
				    perror("Error closing child FD\n");
				}
			    }
			}

			FD_ZERO(&master_read_fds); // erase read fds
			FD_ZERO(&master_write_fds); // erase write fds
			FD_SET(fd_parent[1], &master_write_fds); // add parent write
			FD_SET(fd_child[0], &master_read_fds); // add child read
			FD_SET(clientfd, &master_read_fds); // add client to read
			child_read = child[0];

			// we now need to communicate with the client
			int n;
			unsigned short int length;
			char * buffer;
			n = htons((0xCF << 8) + 0xA7);

			if (send(clientfd, (const void *)(&n), sizeof(n), 0) == -1) {
			    perror("Error sending handshake to client.\n");
			    exit(-1);
			}
			n = htons(count_nodes());
			
			if (send(clientfd, (const void *)(&n), sizeof(n), 0) == -1) {
			    perror("Error sending # of users to client.\n");
			    exit(-1);
			}
			
			needs_to_connect = 1;
		    }
		    
		    else {
			// we're in the parent
			close(fd_child[0]); // close child read;
			close(fd_parent[1]); // close parent write;
			close(clientfd); // close accepted socket, 
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

		// we are a child and we have data from a socket
		else if (i == clientfd) {
		    unsigned short int host_length;
		    char event;
		    char * buff;
		    pid_t pid_here;
		    
		    pid_here = getpid();
		    printf("here: pid %d\n", pid_here);
		    host_length = get_string_from_fd(clientfd, &buff);

		    if (needs_to_connect) {
			event = 1;
			needs_to_connect = 0;
		    }

		    else {
			event = 0;
		    }

		    if (select(fdmax+1, NULL, &write_fds, NULL, NULL) == -1) {
			perror("Error on write select\n");
			exit(-1);
		    }

		    // write to parents pipe
		    for(int j = 0; j <= fdmax; j++) {
		    	if (FD_ISSET(j, &write_fds)) {
			    printf("writing length:'%hu' and string:'%s' to j: %d\n", host_length, buff, j);
			    write(j, &event, sizeof(int));
			    write(j, &host_length, sizeof(host_length));
		    	    write(j, buff, host_length);
		    	}
		    }

		    free(buff);

		}

		// we're in the child and have data from the parent
		else if (i == child_read) {
		    int event;
		    read(i, &event, sizeof(int));
		    if (event == USR_INVALID) {
			exit(1);
		    }

		    else if (event == USR_DISCONNECT) {
			// send to client fd that user disconnected
		    }

		    else if (event == USR_CONNECT) {
			// send to client fd that user connected
		    }

		    else if (event == USR_MESSAGE) {
			// send to client fd user message
		    }
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
			int event;
			char * read_buff;

			read(i, &event, sizeof(int));
		    
			if (event == USR_CONNECT) {
			    read_length = get_string_from_fd(i, &read_buff);
			    printf("Username '%s' connected.\n", read_buff);
			    int exists;
			    exists = username_exists(read_buff);
			    if (exists) {
				int disconnect = USR_INVALID;
				write(i, &disconnect, sizeof(int));
			    }
			    create_node(read_buff, read_length);
			}

			else if (event == USR_DISCONNECT) {
			    printf("disconnect\n");
			}

			else if (event == USR_MESSAGE) {
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
