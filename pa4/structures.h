#pragma once

#ifndef __STRUCTURES_H
#define __STRUCTURES_H

#include "banking.h"
#include "ipc.h"

#define MINIMAL_PROCESSES_NUMBER 1
#define MAXIMAL_PROCESSES_NUMBER 10

typedef enum {
    IPC_MESSAGE_STATUS_NOT_SENT                             = -1,
    IPC_MESSAGE_STATUS_SUCCESS                              = 0,
    IPC_MESSAGE_STATUS_ERROR_PIPE_IS_CLOSED                 = 1,
    IPC_MESSAGE_STATUS_ERROR_NOT_IMPLEMENTED                = 2,
    IPC_MESSAGE_STATUS_ERROR_CANNOT_READ                    = 3,
    IPC_MESSAGE_STATUS_ERROR_NUMBER_OF_PROCESSES_EXCEEDED   = 4,
    IPC_MESSAGE_STATUS_ERROR_INVALID_MAGIC_NUMBER           = 5,
    IPC_MESSAGE_STATUS_ERROR_INVALID_IPC_MESSAGE_ID         = 6,
} ipc_message_status;


typedef struct {
    local_id       message_id;
    int            done_messages;
    timestamp_t    message_time;
} __attribute__((packed)) ipc_message;

ipc_message message;
size_t number_of_processes;

int read_matrix[MAXIMAL_PROCESSES_NUMBER][MAXIMAL_PROCESSES_NUMBER];
int write_matrix[MAXIMAL_PROCESSES_NUMBER][MAXIMAL_PROCESSES_NUMBER];

int receive_from_all_children(ipc_message* self, Message* msg, int max_count_children_proc);
int send_started_to_all      (ipc_message* self);
int send_done_to_all         (ipc_message* self);
int send_stop_to_all         (ipc_message* self);

void increase_latest_time(ipc_message* message, timestamp_t limit_time);

#endif  // __STRUCTURES_H
