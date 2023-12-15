#include "structures.h"
#include "banking.h"

timestamp_t get_lamport_time() {
    return message.message_time;
}

void increase_latest_time(ipc_message* msg, timestamp_t limit_time) {
    if (msg -> message_time < limit_time) msg -> message_time = limit_time;
    msg -> message_time++;
}
