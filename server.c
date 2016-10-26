#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

int GLOBAL_PORT;
int * user_count_ptr;
char ** users_ptr;

void notify_users_disconnect(char * user_name_ptr) {
    // fork process to non-blockingly notify
    // all users that the user disconnected
    // use linked list for users
}

void notify_users_closed_socket(int socket, char * user_name_ptr) {
    shutdown(socket, 0);
    user_count_ptr--;
    
    // remove from users
    // decrement user_cout
    notify_users_diconnect(user_name_ptr);
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

    int sock;
    pid_t current_pid;
    struct sockaddr_in master;
    struct sigaction sigsegv_action;

    sigsegv_action.sa_handler = sigterm_handler;
    sigemptyset (&sigsegv_action.sa_mask);
    sigsegv_action.sa_flags = 0;
    sigaction(SIGTERM, &sigsegv_action, 0);
    
    current_pid = getpid();
    printf("Process ID: %d\n", (int)(current_pid));

    GLOBAL_PORT = atoi(argv[1]);

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror ("Server: cannot open master socket");
	exit(1);
    }

    master.sin_family = AF_INET;
    master.sin_addr.s_addr = INADDR_ANY;
    master.sin_port = htons (GLOBAL_PORT);

    if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
	perror ("Server: cannot bind master socket");
	exit(1);
    }

    if (listen(sock, 5) == -1) {
	perror("Error listening on master socket.\n");
	exit(1);
    }

    printf("Listening \n");

    while(1) {
    }
}
