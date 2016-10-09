#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	MY_PORT	2220

// user part needs to be fixed

int main()
{
    char* users;

	int	sock, snew, fromlength, number;
    
    uint16_t outnum, user_num = 1;

	struct	sockaddr_in	master, from;

    struct users * curr_users;

	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("Server: cannot open master socket");
		exit (1);
	}

	master.sin_family = AF_INET;
	master.sin_addr.s_addr = INADDR_ANY;
	master.sin_port = htons (MY_PORT);

	if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
		perror ("Server: cannot bind master socket");
		exit (1);
	}

	number = 0;

	listen (sock, 5);

    printf("Listening \n");

    while(1){

    // wait and listen to connection
	fromlength = sizeof (from);
	snew = accept (sock, (struct sockaddr*) & from, & fromlength);
	
    // drop if server accept failed
    if (snew < 0) {
		perror ("Server: accept failed");
		exit (1);
    }

    // when connectioned
    // send first confirmation byte
    outnum = htons (0xCF);
	write (snew, &outnum, sizeof (outnum));

    // send second confirmation byte
    outnum = htons (0xA7);
	write (snew, &outnum, sizeof (outnum));

    // send number of users
    user_num = htons(user_num);
    write (snew, &user_num, sizeof (user_num));

    // iterate through all the connected users
    for (int i = 0; i < user_num; i++){

        // send length
        outnum = htons( sizeof(users[i]) + 1 );
        write (snew, &outnum, sizeof (outnum));

        // send string
        write (snew, &users[i], strlen(&users[i])+1);
    }

    close (snew);

    }
}

