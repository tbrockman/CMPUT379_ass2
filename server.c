#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	MY_PORT	2220


// cant add new user
// only last two letters gets added
// client side freezes trying to get 3rd user

int main()
{
    char user_name[6] = "user1";

    char **users = malloc(sizeof(char*) * 100); // start with 100 users

    void *buff;

	int	sock, snew, fromlength, number, size = 100;
    
    uint16_t outnum, length, user_num = 0;

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
    
    ///////////////////////////////////////////
    // add new user
    users[0] = malloc(sizeof(user_name));
    strcpy(users[0], user_name);
    user_num++;
    /////////////////////////////////////////////////


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
    outnum = htons(user_num);
    write (snew, &outnum, sizeof (user_num));

    // iterate through all the connected users
    for (int i = 0; i < user_num; i++){

        // send length
        outnum = htons( sizeof(users[i]) + 1 );
        write (snew, &outnum, sizeof (outnum));

        // send string
        write (snew, users[i], strlen(users[i])+1);
    }

    // increase user space size if it reached limit
    if ( user_num == size){
        users = realloc(users, size * 2 * sizeof(char*));
        size = size * 2;
    }

    // ask for length and user name
    read (snew, &length, sizeof(length));
    // give mem space for user name
    users[user_num] = malloc(ntohs(length));
    read (snew, user_name, ntohs(length));
    strcpy(users[user_num], buffer);
    user_num++;

    close (snew);
    }
    // free all spaced use for user
    for (int i = 0; i < user_num; i++){
        free(users[i]);
    }
    free(users);
}
