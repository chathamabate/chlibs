#include "chutil/list.h"
#include "chsys/mem.h"
#include <stdlib.h>
#include <string.h>


// Concrete function usages:
static const list_impl_t ARRAY_LIST_IMPL_VAL = {
    .constructor = (list_constructor_ft)new_array_list,
    .destructor = (list_destructor_ft)delete_array_list,
    .len = (list_len_ft)al_len,
    .cell_size = (list_cell_size_ft)al_cell_size,
    .get = (list_get_ft)al_get,
    .get_copy = (list_get_copy_ft)al_get_copy,
    .set = (list_set_ft)al_set,
    .push = (list_push_ft)al_push,
    .pop = (list_pop_ft)al_pop,
    .poll = (list_poll_ft)al_poll,

    .reset_iterator = (list_reset_iterator_ft)al_reset_iterator,
    .next = (list_next_ft)al_next,
};
const list_impl_t *ARRAY_LIST_IMPL = &ARRAY_LIST_IMPL_VAL;

static const list_impl_t LINKED_LIST_IMPL_VAL = {
    .constructor = (list_constructor_ft)new_linked_list,
    .destructor = (list_destructor_ft)delete_linked_list,
    .len = (list_len_ft)ll_len,
    .cell_size = (list_cell_size_ft)ll_cell_size,
    .get = (list_get_ft)ll_get,
    .get_copy = (list_get_copy_ft)ll_get_copy,
    .set = (list_set_ft)ll_set,
    .push = (list_push_ft)ll_push,
    .pop = (list_pop_ft)ll_pop,
    .poll = (list_poll_ft)ll_poll,

    .reset_iterator = (list_reset_iterator_ft)ll_reset_iterator,
    .next = (list_next_ft)ll_next,
};
const list_impl_t *LINKED_LIST_IMPL = &LINKED_LIST_IMPL_VAL;

list_t *new_list(const list_impl_t *impl, size_t cs) {
    void *list = impl->constructor(cs); 
    list_t *l = safe_malloc(sizeof(list_t));

    l->list = list;
    l->impl = impl;
    return l;
}

void delete_list(list_t *l) {
    l->impl->destructor(l->list);
    safe_free(l);
}

// Array List 

array_list_t *new_array_list(size_t cs) {
    if (cs == 0) {
        return NULL;
    }

    array_list_t *al = safe_malloc(sizeof(array_list_t));

    al->cap = 1;
    al->len = 0;
    al->cell_size = cs;

    al->arr = safe_malloc(al->cell_size * al->cap);

    return al;
}

void delete_array_list(array_list_t *al) {
    safe_free(al->arr);
    safe_free(al);
}

void *delete_and_move_array_list(array_list_t *al) {
    void *moved_table = safe_realloc(al->arr, al->len * al->cell_size);
    safe_free(al);

    return moved_table;
}

void al_push(array_list_t *al, const void *src) {
    if (al->len == al->cap) {
        // Resize time!
        size_t new_cap = al->cap * 2;
        al->arr = safe_realloc(al->arr, al->cell_size * new_cap);
        al->cap = new_cap;
    }

    al_set(al, al->len, src); 
    al->len++;
}

void al_pop(array_list_t *al, void *dest) {
    if (al->len == 0) {
        return;
    }
    
    if (dest) {
        memcpy(dest, al_get(al, al->len - 1), al->cell_size);
    }

    al->len--;
}

void al_poll(array_list_t *al, void *dest) {
    if (al->len == 0) {
        return;
    }

    if (dest) {
        memcpy(dest, al->arr, al->cell_size);
    }

    uint8_t *curr = al_get(al, 0);
    uint8_t *next = al_get(al, 1);
    const uint8_t *end = al_get(al, al->len - 1);

    while (next <= end) {
        memcpy(curr, next, al->cell_size);
        curr = next;
        next += al->cell_size;
    }

    al->len--;
}

void *al_next(array_list_t *al) {
    void *ret_ptr;
    if (al->iter_ind < al->len) {
        ret_ptr = al_get(al, al->iter_ind);
        al->iter_ind++;
        return ret_ptr;
    }

    return NULL;
}

// Linked List

linked_list_t *new_linked_list(size_t cs) {
    if (cs == 0) {
        return NULL;
    }

    linked_list_t *ll = safe_malloc(sizeof(linked_list_t));
    ll->cell_size = cs;
    ll->len = 0;
    ll->first = NULL;
    ll->last = NULL;

    return ll;
}

void delete_linked_list(linked_list_t *ll) {
    linked_list_node_hdr_t *curr = ll->first;
    linked_list_node_hdr_t *next;

    while (curr) {
        next = curr->next;
        safe_free(curr);

        curr = next;
    }

    safe_free(ll);
}

void *ll_get(linked_list_t *ll, size_t i) {
    linked_list_node_hdr_t *node = ll->first;
    size_t cnt = 0;

    while (node && i < cnt) {
        node = node->next;
        cnt++;
    }

    if (node) {
        return llnh_get_cell(node);
    }

    return NULL;
}

void ll_push(linked_list_t *ll, const void *src) {
    linked_list_node_hdr_t *node = 
        safe_malloc(sizeof(linked_list_node_hdr_t) + ll->cell_size);

    node->next = NULL;
    node->prev = ll->last;

    if (ll->last) {
        ll->last->next = node;
    } else {
        ll->first = node;
    }

    ll->last = node;
    ll->len++;

    void *cell = llnh_get_cell(node);
    memcpy(cell, src, ll->cell_size);
}

void ll_pop(linked_list_t *ll, void *dest) {
    if (ll->len == 0) {
        return;
    }

    linked_list_node_hdr_t *last = ll->last;

    // Last cannot be NULL here.
    if (dest) {
        void *cell = llnh_get_cell(last);
        memcpy(dest, cell, ll->cell_size);
    }

    ll->last = last->prev;
    if (ll->last) {
        ll->last->next = NULL;
    } else {
        // Empty list case.
        ll->first = NULL;
    }

    ll->len--;

    safe_free(last);
}

void ll_poll(linked_list_t *ll, void *dest) {
    if (ll->len == 0) {
        return;
    }

    linked_list_node_hdr_t *first = ll->first;

    if (dest) {
        void *cell = llnh_get_cell(first);
        memcpy(dest, cell, ll->cell_size);
    }

    ll->first = first->next;
    if (ll->first) {
        ll->first->prev = NULL;
    } else {
        ll->last = NULL;
    }

    ll->len--;

    safe_free(first);
}

void *ll_next(linked_list_t *ll) {
    if (ll->iter) {
        void *val_ptr = llnh_get_cell(ll->iter);
        ll->iter = ll->iter->next;

        return val_ptr;
    }

    return NULL;
}



