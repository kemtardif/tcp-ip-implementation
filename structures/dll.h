#ifndef _DLL_H_
#define _DLL_H_

//////////////Structure definitions////////////////

struct doubly_linked_item {
    void *data;
    struct doubly_linked_item *prev;
    struct doubly_linked_item *next;
};

struct doubly_linked_list {
    struct doubly_linked_item *head;
};

///////////////////Methods////////////////////////

void add_to_list(struct doubly_linked_list *list, struct doubly_linked_item *node);
void remove_from_list_by_item(struct doubly_linked_list *list, struct doubly_linked_item *to_remove);
void remove_from_list_by_data(struct doubly_linked_list *list, void *data);
struct doubly_linked_list *dll_init();
void dll_free(struct doubly_linked_list *list);
#endif