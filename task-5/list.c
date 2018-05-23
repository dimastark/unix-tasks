#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TNode {
    char* str;
    struct TNode* next;
} Node;

typedef struct TList {
    struct TNode* first;
} List;

Node* new_Node() {
    Node* node = malloc(sizeof(Node));

    if (node == NULL) {
        exit(EXIT_FAILURE);
    }

    node->str = "";
    node->next = NULL;
    
    return node;
}

List* new_List() {
    List* list = malloc(sizeof(List));

    if (list == NULL) {
        exit(EXIT_FAILURE);
    }
    
    Node *node = new_Node();
    list->first = node;
    
    return list;
}

void free_List(List* list) {
    Node* node = list->first;
    Node* next;

    while (node != NULL) {
        next = node->next;

        free(node);

        node = next;
    }
    
    free(list);
}

void push(List* list, char* str) {
    Node* node = list->first;

    while (node->next != NULL) {
        node = node->next;
    }
    
    node->str = str;
    node->next = new_Node();
}

char* item(List* list, int index) {
    Node* node = list->first;

    while (index-- > 0) {
        node = node->next;
    }

    return node->str;
}

int length(List* list) {
    Node* node = list->first;

    int length = 0;

    while (node->next != NULL) {
        length++;
        
        node = node->next;
    }
    
    return length;
}
