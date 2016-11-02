#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[])
{
    if (argc < 4) {
	printf("Not enough specified arguments.\n");
	exit(1);
    }

    char * hostname;
    char * port_no;
    char * username;
    int username_length;
    hostname = argv[1];
    port_no = argv[2];
    username = argv[3];

    username_length = htons(strlen(username));
    
    // Error checking omitted for expository purposes
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < -1) {
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

    //0xCF 0xA7 portion of the handshake
    
    int receive;
    unsigned char buff[2];
    receive = recv(sockfd, buff, sizeof(buff), 0);

    if (!receive) {
	perror("Error receiving handshake from server.");
	exit(-1);
    }

    if ((buff[0] != 0xcf) || (buff[1] != 0xa7)) {
	perror("Incorrect handshake from server.");
	exit(-1);
    }

    // Numbers of users

    unsigned short int num_connected;
    recv(sockfd, &num_connected, sizeof(num_connected), 0);
    num_connected = ntohs(num_connected);

    printf("Number of users: %hu\n", num_connected);

    send(sockfd, &username_length, sizeof(unsigned short int), 0);
    send(sockfd, username, username_length * sizeof(char), 0);
    printf("Sent username length: %hu", ntohs(username_length));

    // List of usernames
    
    char *buffer;
    int read;
    size_t message_length = 0;
    unsigned short int network_order;
    while (1) {
	//printf("%s> ", username);
	read = getline(&buffer, &message_length, stdin);
	network_order = htons(read-1);
	printf("sent: %hu\n", ntohs(network_order));
	send(sockfd, &network_order, sizeof(network_order), 0);
	send(sockfd, buffer, read-1, 0);
    }
    return 0;
}

