#ifndef C379_SOCKET_UTILITY
#define C379_SOCKET_UTILITY
#define USR_MESSAGE 0
#define USR_CONNECT 1
#define USR_DISCONNECT 2
#define CHILD_SUICIDE 3 // the most descriptive name I could think of

struct node {
    unsigned short int length;
    char * username_ptr;
    struct node * next;
    int socket_fd;
};

int count_nodes_and_return_usernames(char *** array_ptr_ptr, struct node * head);
int remove_node(char * username_ptr, struct node ** head_ptr_ptr);
int username_exists(char * username_ptr, struct node * head);
struct node * create_node(char * text, unsigned short int length, int socket_fd, struct node ** head_ptr_ptr);
unsigned short int get_string_from_fd(int fd, char ** buffer_ptr);

#endif
