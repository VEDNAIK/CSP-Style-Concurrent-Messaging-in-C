#include <stdlib.h>
#include "linked_list.h"

// Creates and returns a new list
list_t* list_create()
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_t* list = (list_t *)malloc(sizeof(list_t));
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    return list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t* curr = list->head;
    while (curr != NULL){
      list_node_t* next = curr->next;
      free(curr);
      curr = next;
    }
    free(list);
}

// Returns head of the list
list_node_t* list_head(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->head;
}

// Returns tail of the list
list_node_t* list_tail(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->tail;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (node!= NULL)
      return node->next;
    else
      return NULL;
}

// Returns prev element in the list
list_node_t* list_prev(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (node!= NULL)
      return node->prev;
    else
      return NULL;
}

// Returns end of the list marker
list_node_t* list_end(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->tail;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (node!= NULL)
      return node->data;
    else
      return NULL;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t* curr = list->head;
    while (curr != NULL){
      if (curr->data == data)
        return curr;
      curr = curr->next;
    }
    return NULL;
}

// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t* list_insert(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (!list) {
        return NULL;
    }
    list_node_t* node = (list_node_t*)malloc(sizeof(list_node_t));
    node->next = NULL;
    node->data = data;

    if (!list->head) {
        list->head = node;
        list->tail = node;
        node->prev = NULL;
        list->count = list->count + 1;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
        list->count = list->count + 1;
    }
    return node;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (!list || !node) {
        return; // Handle null list or node
    }
    if (node == list->head) {
        list->head = node->next;
        if (list->head) {
            list->head->prev = NULL;
        } else {
            // The list is now empty
            list->tail = NULL;
        }
        list->count = list->count - 1;
    }
    else if (node == list->tail) {
        list->tail = node->prev;
        if (list->tail) {
            list->tail->next = NULL;
        } else {
            // The list is now empty
            list->head = NULL;
        }
        list->count = list->count - 1;
    }
    else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        list->count = list->count - 1;
    }

    free(node);
}
