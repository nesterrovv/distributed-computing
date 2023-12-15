#include "structures.h"
#include "pqueue.h"

extern ipc_message message;
extern size_t number_of_processes;

extern int read_matrix[10][10];
extern int write_matrix[10][10];

int FIRST_GREATER   =  1;
int SECOND_GREATER  = -1;
int ITEMS_EQUAL     =  0;

int compare_to(Priority_queue_item item1, Priority_queue_item item2) {
    if (item1.item_timestamp <= item2.item_timestamp) {
        if (item1.item_timestamp == item2.item_timestamp) {
            if (item1.item_id > item2.item_id)          return FIRST_GREATER;
            else if (item1.item_id == item2.item_id)    return ITEMS_EQUAL;
        }
    } else                                              return FIRST_GREATER;
    return SECOND_GREATER;
}

int get_max_index_from_queue() {
    if (priority_queue.capacity <= 0) return -1;
    int maximal_index = 0;
    for (int current_index = 1; current_index < priority_queue.capacity; current_index = current_index + 1) {
        if (compare_to(priority_queue.items[current_index], priority_queue.items[maximal_index]) > 0) {
            maximal_index = current_index;
        }
    }
    return maximal_index;
}

Priority_queue_item peek() {
    int maximal_index = get_max_index_from_queue();
    return priority_queue.items[maximal_index];
}

Priority_queue_item pop() {
    int maximal_index = get_max_index_from_queue();

    Priority_queue_item element_with_max_id = priority_queue.items[maximal_index];
    priority_queue.items[maximal_index] = priority_queue.items[priority_queue.capacity - 1];
    priority_queue.capacity--;

    return element_with_max_id;
}

void push(Priority_queue_item new_item) {
    priority_queue.items[priority_queue.capacity] = new_item;
    priority_queue.capacity++;
}
