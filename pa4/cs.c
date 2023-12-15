#include "structures.h"
#include "pa2345.h"
#include "pqueue.h"

extern size_t number_of_processes;

int request_cs(const void* source) {

    ipc_message* message = (ipc_message*) source;
    message -> message_time++;

    Priority_queue_item item = (Priority_queue_item) {
            .item_timestamp = get_lamport_time(),
            .item_id    = message -> message_id,
    };

    push(item);

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

        if (waited_replies == 0 && peek().item_id == message -> message_id) break;

        Message receive_message;
        local_id any = receive_any(message, &receive_message);
        increase_latest_time(message, receive_message.s_header.s_local_time);

        if (receive_message.s_header.s_type == DONE) {
            message -> done_messages++;
        } else if (receive_message.s_header.s_type == CS_RELEASE) {
            pop();
        } else if (receive_message.s_header.s_type == CS_REPLY) {
            waited_replies = waited_replies - 1;
        } else if (receive_message.s_header.s_type == CS_REQUEST) {
            item = (Priority_queue_item) {
                    .item_id = any,
                    .item_timestamp = receive_message.s_header.s_local_time,
            };
            push(item);
            Message content = {
                    .s_header = {
                            .s_magic = MESSAGE_MAGIC,
                            .s_local_time = ++message->message_time,
                            .s_type = CS_REPLY,
                            .s_payload_len = 0,
                    }};
            send(message, any, &content);
        }
    }
    return 0;
}

int release_cs(const void* source) {
    ipc_message* message = (ipc_message*) source;
    pop();
    Message msg = {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC,
                    .s_local_time = ++message->message_time,
                    .s_type = CS_RELEASE,
                    .s_payload_len = 0,
            }
    };
    send_multicast(message, &msg);
    return 0;
}
