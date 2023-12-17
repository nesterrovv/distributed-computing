#include "structures.h"
#include "pa2345.h"

int waited_processes[MAXIMAL_PROCESSES_NUMBER];

int request_cs(const void* source) {

    ipc_message* message = (ipc_message*) source;
    message -> message_time++;
    timestamp_t time_of_request = message -> message_time;
    Message request_message = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type = CS_REQUEST,
            .s_local_time = get_lamport_time(),
            .s_payload_len = 0,
        }
    };
    send_multicast(message, &request_message);
    int waited_replies = number_of_processes - 2;
    for (;;) {
        if (waited_replies <= 0) break; 
        Message receive_message;
        local_id any = receive_any(message, &receive_message);
        increase_latest_time(message, receive_message.s_header.s_local_time);
        if (receive_message.s_header.s_type == DONE) {
            message -> done_messages++;
        }
        else if (receive_message.s_header.s_type == CS_REPLY) {
            waited_replies = waited_replies - 1;
        } else if (receive_message.s_header.s_type == CS_REQUEST) {
            int ids_are_valid = message -> message_id < any;
            int time_is_equal = time_of_request == receive_message.s_header.s_local_time && ids_are_valid;
            int time_is_greater = time_of_request > receive_message.s_header.s_local_time;
            if (time_is_equal || time_is_greater) {
                waited_processes[any] = 0;
                Message msg = {
                    .s_header = {
                        .s_local_time = ++message -> message_time,
                        .s_type = CS_REPLY,
                        .s_payload_len = 0,
                        .s_magic = MESSAGE_MAGIC,
                    }
                };
                send(message, any, &msg);
            } else {
                waited_processes[any] = 1;
            }
            break;
        }
    }
    return 0;
}

int release_cs(const void* source) {
    ipc_message* message = (ipc_message*) source;
    int start_any = 1;
    int stop_any = number_of_processes - 1;
    for (local_id any = start_any; any <= stop_any; any++) {
        int id_is_not_same = any != message -> message_id;
        int is_waited = waited_processes[any];
        if (is_waited && id_is_not_same) {
            Message msg = {
                .s_header = {
                    .s_local_time = ++message -> message_time,
                    .s_type = CS_REPLY,
                    .s_payload_len = 0,
                    .s_magic = MESSAGE_MAGIC,
                }
            };
            send(message, any, &msg);
            waited_processes[any] = 0;
        }
    }
    return 0;
}
