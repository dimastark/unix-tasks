#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct node {
    long val;
    struct node* next;
} node_t;

node_t* root = NULL;

void init(node_t** node, long val) {
    (*node) = malloc(sizeof(node_t));
    (*node)->val = val;
    (*node)->next = NULL;
}

void push(long val) {
    if (root == NULL) {
        init(&root, val);
    } else {
        node_t * current = root;

        while (current->next != NULL)
            current = current->next;

        init(&(current->next), val);
    }
}

long size() {
    long count = 0;
    node_t* current = root;

    while (current != NULL) {
        current = current->next;
        count++;
    }

    return count;
}

int cmpnums(const void* a, const void* b) {
    return *(long*)a - *(long*) b;
}

void read_numbers_from_file(const char* path) {
    char buffer[BUFFER_SIZE];
    int fd = open(path, O_RDONLY, 0444);

    if (fd < 0) {
        printf("Error while open '%s'. Skipping...\n", path);
        return;
    }

    int count = read(fd, buffer, BUFFER_SIZE);

    while (count != 0) {
        char* lim_pointer = buffer;
        char* end_pointer = buffer + count;

        while (lim_pointer != end_pointer) {
            if (!isdigit(lim_pointer[0])) {
                lim_pointer++;
                continue;
            }
            
            push(strtol(lim_pointer, &end_pointer, 10));
            lim_pointer = end_pointer;
            end_pointer = buffer + count;
        }

        count = read(fd, buffer, BUFFER_SIZE);
    }
}

int main (int argc, char **argv) {
    if (argc < 3) {
        perror("Wrong count of arguments");
        return 1;
    }

    for (int i = 1; i < argc - 1; i++)
        read_numbers_from_file(argv[i]);
    
    long count = size();
    long numbers[count];
    long index = 0;

    node_t * current = root;

    while (current != NULL) {
        numbers[index++] = current->val;
        current = current->next;
    }

    qsort(numbers, count, sizeof(long), cmpnums);

    int fdo = open(argv[argc - 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fdo < 0) {
        perror("Error while output open");
        return 1;
    }
    
    char s[10];
    for (int i = 0; i < count; i++) {
        sprintf(s, "%ld ", numbers[i]);
        write(fdo, s, strlen(s));
    }

    return 0;
}
