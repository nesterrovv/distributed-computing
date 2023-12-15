#include "structures.h"
#include "banking.h"

extern ipc_message message;
extern size_t number_of_processes;

extern int read_matrix[MAXIMAL_PROCESSES_NUMBER][MAXIMAL_PROCESSES_NUMBER];
extern int write_matrix[MAXIMAL_PROCESSES_NUMBER][MAXIMAL_PROCESSES_NUMBER];

timestamp_t get_lamport_time() {
    return message.message_time;
}

void increase_latest_time(ipc_message* msg, timestamp_t limit_time) {
    if (msg -> message_time < limit_time) msg -> message_time = limit_time;
    msg -> message_time++;
}
