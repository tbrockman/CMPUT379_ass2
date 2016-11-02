#ifndef C379_SOCKET_SERVER 
#define C379_SOCKET_SERVER
#define USR_MESSAGE 0
#define USR_CONNECT 1
#define USR_DISCONNECT 2
#define KILL 3

struct node {
    unsigned short int length;
    char * username_ptr;
    struct node * next;
};

#endif
