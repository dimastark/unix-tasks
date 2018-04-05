#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct node {
    char* val;
    struct node* next;
} node_t;

node_t* root = NULL;

void init(node_t** node, char* val) {
    (*node) = malloc(sizeof(node_t));
    (*node)->val = val;
    (*node)->next = NULL;
}

void push(char* val) {
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

char* concat(const char* s1, const char* s2) {
    char* result = malloc(strlen(s1) + strlen(s2) + 1);

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}

char* substring(char* string, int position, int length) {
    char* pointer = malloc(length + 1);
   
    for (int c = 0; c < position - 1 ; c++)
        string++; 
 
    for (int c = 0; c < length; c++) {
        *(pointer + c) = *string;      
        string++;
    }
 
    *(pointer + length) = '\0';
 
    return pointer;
}

int cmp_str_as_number(const void* a, const void* b) {
    char* ac = *(char**)a;
    char* bc = *(char**)b;

    if (ac[0] == bc[0] && ac[0] == '-')
        return -strcmp(ac, bc);
    
    return strcmp(ac, bc);
}

char* read_all(FILE* file) {
    size_t read_size;
    long string_size;

    char* buffer = NULL;
    
    fseek(file, 0, SEEK_END);
    string_size = ftell(file);
    rewind(file);

    buffer = (char*) malloc(sizeof(char) * (string_size + 1));
    read_size = fread(buffer, sizeof(char), (size_t) string_size, file);

    buffer[string_size] = '\0';

    if (string_size != read_size) {
        free(buffer);
        buffer = NULL;
    }

    fclose(file);

    return buffer;
}

void read_numbers_from_file(const char* path) {
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        printf("Error while open '%s'. Skipping...\n", path);
        return;
    }

    char* content = read_all(file);

    if (content == NULL) {
        printf("Error while reading '%s'. Skipping...\n", path);
        return;
    }

    long length = strlen(content);

    char buffer[BUFFER_SIZE];

    for (int i = 0; i < length; i++) {
        if (!isdigit(content[i]) && content[i] != '-')
            continue;
        
        while (content[i] == '-')
            i++;
        
        int substring_len = 0;

        while (isdigit(content[i + substring_len]))
            substring_len++;
        
        printf("'%c' %d\n", content[i], substring_len);
        push(substring(content, i, substring_len));
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        perror("Wrong count of arguments");
        return 1;
    }

    for (int i = 1; i < argc - 1; i++)
        read_numbers_from_file(argv[i]);
    
    long count = size();
    char* strs[count];
    long index = 0;

    node_t * current = root;

    while (current != NULL) {
        strs[index++] = current->val;
        current = current->next;
    }

    qsort(strs, count, sizeof(char*), cmp_str_as_number);

    int fdo = open(argv[argc - 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fdo < 0) {
        perror("Error while output open");
        return 1;
    }
    
    printf("%ld\n", count);
    for (int i = 0; i < count; i++)
        write(fdo, concat(strs[i], " "), strlen(strs[i]) + 1);

    return 0;
}
