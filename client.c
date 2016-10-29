#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int main(int argc, char * argv[])
{
    if (argc < 4) {
	printf("Not enough specified arguments.\n");
	exit(1);
    }

    char * hostname;
    char * port_no;
    char * username;
    hostname = argv[1];
    port_no = argv[2];
    username = argv[3];
    
    // Error checking omitted for expository purposes
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < -1) {
	printf("Error here\n.");
	perror("Client socket error.\n");
	exit(1);
    }

    // Bind to a specific network interface (and optionally a specific local port)
    struct sockaddr_in localaddr;
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = inet_addr("192.168.1.100");
    localaddr.sin_port = 0;  // Any local port will do
    bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr));

    // Connect to the remote server
    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_addr.s_addr = inet_addr(hostname);
    remoteaddr.sin_port = htons(atoi(port_no));
    connect(sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
    
}

