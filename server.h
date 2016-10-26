#ifndef C379_SOCKET_SERVER 
#define C379_SOCKET_SERVER

struct node {
    unsigned short int length;
    char * username_ptr;
    struct node * next;
};

#endif
