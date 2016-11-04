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
#include "utility.h"

int GLOBAL_PORT;
struct node * user_linked_list_ptr;

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
    fd_set master_worker_pipes;
    fd_set worker_pipes;
    fd_set read_fds;
    fd_set write_fds;

    timeperiod = NULL;

    sigsegv_action.sa_handler = sigterm_handler;
    sigemptyset (&sigsegv_action.sa_mask);
    sigsegv_action.sa_flags = 0;
    sigaction(SIGTERM, &sigsegv_action, 0);
    
    FD_ZERO(&master_read_fds);
    FD_ZERO(&master_write_fds);
    FD_ZERO(&master_worker_pipes);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&worker_pipes);

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

    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
	perror("setsockopt(SO_REUSEADDR) failed");
	exit(1);
    }
	


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
			exit(1);
		    }

		    else if (pid == 0) {
			// we're in the child
			
			FD_ZERO(&master_read_fds); // erase read fds
			FD_ZERO(&master_write_fds); // erase write fds
			FD_ZERO(&master_worker_pipes);
			FD_SET(fd_parent[1], &master_write_fds); // add parent write
			FD_SET(fd_child[0], &master_read_fds); // add child read
			FD_SET(clientfd, &master_read_fds); // add client to read
			child_read = fd_child[0];
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



			// we now need to communicate with the client
			unsigned char n[2];
			unsigned short int length;
			char ** users_ptr;
			char * buffer;

			n[0] = 0xCF;
			n[1] = 0xA7;

			if (send(clientfd, n, sizeof(unsigned char), 0) == -1) {
			    perror("Error sending handshake to client.\n");
			    close(clientfd);
			    exit(1);
			}

			length = htons(count_nodes_and_return_usernames(&users_ptr, user_linked_list_ptr));
			
			if (send(clientfd, n+1, sizeof(unsigned char), 0) == -1) {
			    perror("Error sending # of users to client.\n");
			    close(clientfd);
			    exit(1);
			}
			

			// send user list
			for (int v = 0; v < ntohs(length); v++) {
			    int string_length;
			    unsigned short int network_length;
			    string_length = strlen(users_ptr[v]);
			    network_length = htons(string_length);
			    if (send(clientfd, &network_length, sizeof(unsigned short int), 0) == -1) {
				    perror("Error sending user to client.\n");
				    close(clientfd);
				    exit(1);
			    }
			    
			    printf("Sending username: %s\n to client.\n", users_ptr[v]);
			    if (send(clientfd, users_ptr[v], sizeof(char) * string_length, 0) == -1) {
				perror("Error sending user to client.\n");
				close(clientfd);
				exit(1);
			    }
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
			    int success;
			    success = write(j, &event, sizeof(int));

			    if (success == -1) {
				close(j);
				FD_CLR(j, &master_write_fds);
				exit(1);
			    }

			    success = write(j, &host_length, sizeof(host_length));

			    if (success == -1) {
				close(j);
				FD_CLR(j, &master_write_fds);
				exit(1);
			    }

		    	    success = write(j, buff, host_length);

			    if (success == -1) {
				close(j);
				FD_CLR(j, &master_write_fds);
				exit(1);
			    }
		    	}
		    }

		    free(buff);

		}

		// we're in the child and have data from the parent
		else if (i == child_read) {
		    int event;
		    printf("In child reading from parent.\n");
		    read(i, &event, sizeof(int));
		    if (event == CHILD_SUICIDE) {
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

		// otherwise we have read data from a pipe
		// types:
		// user disconnect -> remove user from list, tell users
		// user connect -> add user to linked list, tell users
		// user message -> send to all users
		else {

		    pid_t worker;
		    int worker_pipe[2];

		    // fork a process to read/write the data
		    // to all pipes, creating a pipe in case we have
		    // to modify our linked list

		    pipe(worker_pipe);
		    worker = fork();
		    
		    if (worker == -1) {
			perror("Parent fork error.\n");
			exit(1);
		    }

		    else if (worker == 0) {
			unsigned short int read_length;
			int event, success;
			char * read_buff;

			close(worker_pipe[0]); // close read side of pipe
			success = read(i, &event, sizeof(int)); // read event

			if (success == -1 || success == 0) {
			    perror("Error reading from pipe.\n");
			    close(worker_pipe[1]);
			}

			// get corresponding string (username, message, etc.);
			read_length = get_string_from_fd(i, &read_buff);
			printf("heard string: %s\n", read_buff);

			if (read_length == -1) {
			    perror("Error reading from pipe.\n");
			    exit(1);
			}
		    
			if (event == USR_CONNECT) {
			    
			    printf("Username '%s' connected.\n", read_buff);
			    struct node * exists;
			    exists = get_user(read_buff, user_linked_list_ptr);
			    if (exists) {
				int disconnect = CHILD_SUICIDE;
				write(i, &disconnect, sizeof(int));
			    }
			    // need to tell parent to create the node;
			    write(worker_pipe[1], &event, sizeof(int));
			    write(worker_pipe[1], &read_length, sizeof(unsigned short int));
			    write(worker_pipe[1], read_buff, sizeof(char) * read_length);
			}

			else if (event == USR_DISCONNECT) {
			    printf("disconnect\n");
			    // tell parent to remove the node
			    write(worker_pipe[1], &event, sizeof(int));
			    write(worker_pipe[1], &read_length, sizeof(unsigned short int));
			    write(worker_pipe[1], read_buff, sizeof(char) * read_length);
			}

			else if (event == USR_MESSAGE) {
			    // tell all children
			    for (int k = 0; k <= fdmax; k++) {
				// don't care if it's ready or not
				// we'll block until it is
				if (FD_ISSET(k, &master_write_fds)) {
				    write(k, &read_length, sizeof(int));
				    write(k, read_buff, sizeof(char) * read_length);
				}
			    }
			}

			exit(1);
		    }
		    // we're not the worker
		    else {
			close(worker_pipe[1]); // close write end of pipe;
			FD_SET(worker_pipe[0], &master_worker_pipes);
			if (worker_pipe[0] > fdmax) {
			    fdmax = worker_pipe[0];
			}
		    }
		}
	    }
	}

	worker_pipes = master_worker_pipes;
	struct timeval non_block;
	non_block.tv_sec = 0;
	non_block.tv_usec = 0;

	if (select(fdmax+1, &worker_pipes, NULL, NULL, &non_block) == -1) {
	    perror("Error reading from worker pipe.\n");
	    exit(1);
	}
       
	for (int y = 0; y <= fdmax; y++) {
	    if (FD_ISSET(y, &worker_pipes)) {

		unsigned short int read_length;
		int event, success;
		char * read_buff;

		success = read(y, &event, sizeof(int));

	        if (success <= 0) {
		    close(y);
		    FD_CLR(y, &master_worker_pipes);
		}
		
	        read_length = get_string_from_fd(y, &read_buff);

		if (read_length <= 0) {
		    close(y);
		    FD_CLR(y, &master_worker_pipes);
		}

		printf("read from worker pipe: %s\n", read_buff);
		
	    }
	}	
    }
}
