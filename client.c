#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define	 MY_PORT  2222

int main()
{
    char user_name[256];  // 256 max character user name

	int	s, size = 10;

    char ** users;

    uint16_t verify1, verify2, user_num, length, outnum;

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

        // inital verification read
		read (s, &verify1, sizeof (verify1));
		read (s, &verify2, sizeof (verify2));
        if ((ntohs(verify1) == 207)&&(ntohs(verify2) == 167)){
		    fprintf (stderr, "Connected to the correct server \n");
        }else{close(s);}

        // get number of users connected
        read (s, &user_num, sizeof (user_num));
        user_num = ntohs(user_num);
		fprintf (stderr, "number of user are: %d \n", user_num);

        // give mem space for all the users
        users = malloc( sizeof(char*) * user_num);

        // get all the connected uers
        for(int i = 0; i < user_num; i++){
            read(s, &length, sizeof (length));
            // give mem space for each uer name
            users[i] = malloc(ntohs(length));

            // get user name and add to array
            read(s, users[i], ntohs(length));
            fprintf ( stderr, "User %d: %s \n",i+1, users[i]);          
        }
       
        
        strcpy(user_name, "user"); 


        // send length of user name
        outnum = htons(strlen(user_name) + 1);
        write (s, &outnum, sizeof (outnum));
        // send string
        write (s, user_name, strlen(user_name)+1);
    
        close (s);
        
        break;
	}

    // free all spaced use for user
    for (int i = 0; i < user_num; i++){
        free(users[i]);
    }
    free(users);
    
}

