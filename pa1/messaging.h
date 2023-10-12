#pragma once
#ifndef __MESSAGING_H
#define __MESSAGING_H

#include "ipc.h"

#define MINIMAL_PROCESSES_NUMBER 1
#define MAXIMUM_PROCESSES_NUMBER 10

size_t processes_number;

int read_matrix[MAXIMUM_PROCESSES_NUMBER][MAXIMUM_PROCESSES_NUMBER];
int write_matrix[MAXIMUM_PROCESSES_NUMBER][MAXIMUM_PROCESSES_NUMBER];

typedef struct __attribute__((packed)) {
    local_id message_id;
} message;

message ipc_message;

typedef enum {
    SUCCESS                             = 0,
    ERROR_INCORRECT_PEER                = 1,
    ERROR_INCORRECT_MAGIC_MESSAGE       = 2,
    ERROR_SOURCE_PROCESS_IS_NULL        = 3,
    ERROR_NUMBER_OF_PROCESSES_EXCEEDED  = 4,
    ERROR_PIPE_IS_CLOSED                = 5,
    ERROR_CANNOT_READ                   = 6,
    ERROR_CANNOT_WRITE                  = 7,
    ERROR_NOT_RESOLVED                  = 8,
} message_status;

#endif  // __MESSAGING_H
