#pragma once
#ifndef __PQUEUE_H
#define __PQUEUE_H

#include "ipc.h"

typedef struct {
    local_id item_id;
    timestamp_t item_timestamp;
} __attribute__((packed)) Priority_queue_item;

typedef struct {
    Priority_queue_item items[64];
    int capacity;
} __attribute__((packed)) Priority_queue;

Priority_queue priority_queue;

Priority_queue_item    peek();
Priority_queue_item    pop();
void                   push(Priority_queue_item element);

#endif //__PQUEUE_H
