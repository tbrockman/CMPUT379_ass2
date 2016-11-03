#include <stdlib.h>
#include <string.h>
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

int remove_node(char * username_ptr) {
    struct node * current;
    struct node * last;
    current = user_linked_list_ptr;
    while (current) {
	if (strcmp(current->username_ptr, username_ptr) == 0) {
	    if (last) {
		last->next = current->next;
	    }

	    else {
		user_linked_list_ptr = current->next;
	    }

	    free(current);
	    return 1;
	}
	last = current;
	current = current->next;
    }
    return 0;
}

int username_exists(char * username_ptr) {
    struct node * current;
    current = user_linked_list_ptr;

    while (current) {
	if (strcmp(current->username_ptr, username_ptr) == 0) {
	    return 1;
	}
	current = current->next;
    }
    return 0;
}

struct node * create_node(char * text, unsigned short int length) {
    struct node * new_node;

    new_node = (struct node *)malloc(sizeof(struct node));
    new_node->username_ptr = text;
    new_node->length = length;
    new_node->next = NULL;

    if (!user_linked_list_ptr) {
	user_linked_list_ptr = new_node;
    }
    else {
	struct node * current;
	current = user_linked_list_ptr;
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
    if (!success) {
	return -1;
    }

    length = ntohs(length);

    *buffer_ptr = malloc(length * sizeof(char));
    
    success = read(fd, *buffer_ptr, length * sizeof(char));
    if (!success) {
	return -1;
    }
    return length;
}
