#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utility.h"

int count_nodes_and_return_usernames(char *** array_ptr_ptr, struct node * head) {
    int count = 0;
    int array_size = 1;
    struct node * current;
    current = head;
    *array_ptr_ptr = malloc(sizeof (char * ) * 1);
    while (current) {
	// is our array full?
	if (array_size == count+1) {
	    // double size of array;
	    array_size *= 2;
	    *array_ptr_ptr = realloc(*array_ptr_ptr, sizeof(char *) * array_size);
	}
	// allocate memory for username
	(*array_ptr_ptr)[count] = (current->username_ptr);
	current = current->next;
	count++;
    }
    return count;
}

int remove_node(char * username_ptr, struct node ** head_ptr_ptr) {
    struct node * current;
    struct node * last_node;
    current = *head_ptr_ptr;
    last_node = current;
    while (current) {
	if (strcmp(current->username_ptr, username_ptr) == 0) {
	    if (last_node && last_node != current) {
		last_node->next = current->next;
	    }

	    else {
		*head_ptr_ptr = current->next;
	    }
	    free(current);
	    return 1;
	}
	last_node = current;
	current = current->next;
    }
    return 0;
}

struct node * get_user(char * username_ptr, struct node * head) {
    struct node * current;
    current = head;

    while (current) {
	if (strcmp(current->username_ptr, username_ptr) == 0) {
	    return current;
	}
	current = current->next;
    }
    return 0;
}

struct node * create_node(char * text, unsigned short int length, int socket_fd, struct node ** head_ptr_ptr) {
    struct node * new_node;
    
    new_node = (struct node *)malloc(sizeof(struct node));
    new_node->username_ptr = text;
    new_node->length = length;
    new_node->next = NULL;
    if (!(*head_ptr_ptr)) {
	*head_ptr_ptr = new_node;
    }
    else {
	struct node * current;
	current = *head_ptr_ptr;
	while (current->next) {
	    current = current->next;
	}
	current->next = new_node;
    }

    return new_node;
}

// Returns the length of a string, sets buffer to point to received string
unsigned short int get_string_from_fd(int fd, char ** buffer_ptr) {
    unsigned short int length;
    int success;

    success = read(fd, &length, sizeof(length));
    if (success <= 0) {
	return success;
    }

    length = ntohs(length);

    *buffer_ptr = malloc(length * sizeof(char));
    
    success = read(fd, *buffer_ptr, length * sizeof(char));
    if (success <= 0) {
	return success;
    }
    return length;
}
