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
int * user_count_ptr;
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
    *user_count_ptr--;
    
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

    int sock, fdmax, newfd;
    pid_t current_pid;
    struct sockaddr_in sa;
    struct sockaddr_in remoteaddr;
    struct sigaction sigsegv_action;
    socklen_t addrlen;
    fd_set read_fds;
    fd_set write_fds;

    sigsegv_action.sa_handler = sigterm_handler;
    sigemptyset (&sigsegv_action.sa_mask);
    sigsegv_action.sa_flags = 0;
    sigaction(SIGTERM, &sigsegv_action, 0);
    
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

    FD_SET(sock, &read_fds);
    fdmax = sock;

    printf("Listening \n");

    while(1) {
	if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
	    printf("Select error.\n");
	    perror("Error on select\n");
	    exit(1);
	}

	for(int i = 0; i <= fdmax; i++) {
	    if (FD_ISSET(i, &read_fds)) {
		// is this the listener
		if (i == sock) {
		    printf("CLIENT CONNECTION\n");
		    addrlen = sizeof remoteaddr;
                    newfd = accept(sock, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1) {
                        perror("Error accepting client connection.");
                    } else {
                        FD_SET(newfd, &read_fds); // add to reading fds
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
		    }
	    
		    int fd_parent[2];
		    int fd_child[2];
		    pid_t pid = fork();
		    if (pid == 0) {
			// we're in the child
			close(fd_child[1]); // close child write
			close(fd_parent[0]); // close parent read
			// we now need to communicate with the client
			int n;
			n = htons(0xCF + 0xA7);
			send(newfd, (const void *)(&n), sizeof(n), 0);
			printf("SENT DATA TO CLIENT.\n");
		    }
		    
		    else if (pid > 0) {
			// we're in the parent
			close(fd_child[0]); // close child read;
			close(fd_parent[1]); // close parent write;
			shutdown(newfd, 2); // close parent socket, 
			                    // socket is only important to the child
			printf("Shutdown socket FD in parent.\n");

		    }
		    
		    else {
			perror("Error creating pipe\n");
			exit(1);
		    }
		}
		// we have read data from a pipe
		// types:
		// user disconnect -> remove user from list, tell users
		// user connect -> add user to linked list, tell users
		// user message -> send to all users
		else {
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
