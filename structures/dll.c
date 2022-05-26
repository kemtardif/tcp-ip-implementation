#include <stdlib.h>
#include "dll.h"

void add_to_list(struct doubly_linked_list *list, struct doubly_linked_item *new_item)
{
    struct doubly_linked_item *head;

    if(!list || !new_item) return;

    head = list->head;
    list->head = new_item;
    new_item->prev = NULL;
    new_item->next = head;

    if(head)
        head->prev = new_item;
}

void remove_from_list_by_item(struct doubly_linked_list *list, struct doubly_linked_item *to_remove)
{
    struct doubly_linked_item *head, *prev, *next;

    if(!list || !to_remove) return;

    head = list->head;
    prev = to_remove->prev;
    next = to_remove->next;

    if(head == to_remove)
        list->head = next;   
    if(prev)
        prev->next = next;
    if(next)
        next->prev = prev;

    free(to_remove);
}

void remove_from_list_by_data(struct doubly_linked_list *list, void *data)
{
    struct doubly_linked_item *item;

    if(!list) 
        return;
    item = list->head;

    while(item)
    {
        if(item->data == data)
        {
            remove_from_list_by_item(list, item);
            return;
        }
        item = item->next;
    }
}

struct doubly_linked_list *dll_init()
{
    struct doubly_linked_list *dll;  
    if((dll = malloc(sizeof(struct doubly_linked_list))) == NULL)
        return NULL;

    dll->head = NULL;
    return dll;
}

void dll_free(struct doubly_linked_list *list)
{
    struct doubly_linked_item *head, *temp;
    if(!list) return;

    head = list->head;
    while(head)
    {
        temp = head;
        head = head->next;
        free(temp);
    }
    free(list);
}