#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define	 MY_PORT  2220

/* ---------------------------------------------------------------------
 This is a sample client program for the number server. The client and
 the server need not run on the same machine.				 
 --------------------------------------------------------------------- */

int main()
{
	int	s;

    char* users;

    uint16_t verify1, verify2, user_num, length;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	host = gethostbyname ("localhost");

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit (1);
	}

	while (1) {

		s = socket (AF_INET, SOCK_STREAM, 0);

		if (s < 0) {
			perror ("Client: cannot open socket");
			exit (1);
		}

		bzero (&server, sizeof (server));
		bcopy (host->h_addr, & (server.sin_addr), host->h_length);
		server.sin_family = host->h_addrtype;
		server.sin_port = htons (MY_PORT);

		if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
			perror ("Client: cannot connect to server");
			exit (1);
		}

		read (s, &verify1, sizeof (verify1));
		read (s, &verify2, sizeof (verify2));
        if ((ntohs(verify1) == 207)&&(ntohs(verify2) == 167)){
		    fprintf (stderr, "Connected to the correct server \n");
        }else{close(s);}

        read (s, &user_num, sizeof (user_num));
		fprintf (stderr, "number of user are: %d \n", ntohs(user_num));
        
        for(int i = 0; i < user_num; i++){
            read(s, &length, sizeof (length));
            read(s, &users[i], ntohs(length));
                        
        }
        close (s);

        break;
	}
}

