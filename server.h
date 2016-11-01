#ifndef C379_SOCKET_SERVER 
#define C379_SOCKET_SERVER
#define MESSAGE 0
#define CONNECT 1
#define DISCONNECT 2

struct node {
    unsigned short int length;
    char * username_ptr;
    struct node * next;
};

#endif
