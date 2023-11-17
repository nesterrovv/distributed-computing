#include "banking.h"
#include "structures.h"

timestamp_t get_lamport_time(){
    return message.message_time;
}

void increase_latest_time(ipc_message* message, timestamp_t limit_time) {
    if (message -> message_time < limit_time) {
        message -> message_time = limit_time;
    }
    message -> message_time++;
}
