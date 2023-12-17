#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "ipc.h"
#include "structures.h"
#include "stdio.h"
#include "pa2345.h"


int validate_sending_args(void* source, local_id destination, const Message* sending_message) {
    if (source == NULL || ((ipc_message*) source) -> message_id >= MAXIMAL_PROCESSES_NUMBER) {
        return IPC_MESSAGE_STATUS_ERROR_INVALID_IPC_MESSAGE_ID;
    }
    if (destination >= MAXIMAL_PROCESSES_NUMBER) {
        return IPC_MESSAGE_STATUS_ERROR_NUMBER_OF_PROCESSES_EXCEEDED;
    }
    if (sending_message -> s_header.s_magic != MESSAGE_MAGIC) {
        return IPC_MESSAGE_STATUS_ERROR_INVALID_MAGIC_NUMBER;
    }
    return IPC_MESSAGE_STATUS_SUCCESS;
}

int send(void* source, local_id destination, const Message* sending_message) {
    int validation_result = validate_sending_args(source, destination, sending_message);
    if (validation_result == IPC_MESSAGE_STATUS_SUCCESS) {
        int result_of_sending =
                write(
                write_matrix[((ipc_message*) source) -> message_id][destination],
                sending_message,
                sizeof(MessageHeader) + sending_message -> s_header.s_payload_len
                );
        if (result_of_sending == -1) {
            if (errno == EPIPE) {
                return IPC_MESSAGE_STATUS_ERROR_PIPE_IS_CLOSED;
            } else {
                return -1;
            }
        }
        return IPC_MESSAGE_STATUS_SUCCESS;
    } else {
        return IPC_MESSAGE_STATUS_NOT_SENT;
    }
}

int send_multicast(void* self, const Message* sending_message) {
    // for each loop for sending 'unicast' message for all addresses excluding itself
    for (local_id destination_id = 0; destination_id < MAXIMAL_PROCESSES_NUMBER; destination_id += 1) {
        // excluding sending to itself
        if (destination_id == ((ipc_message*) self) -> message_id) {
            continue;
        } else {
            ipc_message_status sending_status = send(self, destination_id, sending_message);
            // signalize if some failure happens
            if (sending_status != IPC_MESSAGE_STATUS_SUCCESS) {
                return sending_status;
            }
        }
    }
    return IPC_MESSAGE_STATUS_SUCCESS;
}

int validate_receiving_args(void* source, local_id destination, Message* receiving_message) {
    if (source == NULL || ((ipc_message*) source) -> message_id >= MAXIMAL_PROCESSES_NUMBER) {
        return IPC_MESSAGE_STATUS_ERROR_INVALID_IPC_MESSAGE_ID;
    }
    if (destination >= MAXIMAL_PROCESSES_NUMBER) {
        return IPC_MESSAGE_STATUS_ERROR_INVALID_IPC_MESSAGE_ID;
    }
    return IPC_MESSAGE_STATUS_SUCCESS;
}

int receive(void* source, local_id destination, Message* receiving_message) {
    int validation_result = validate_receiving_args(source, destination, receiving_message);
    if (validation_result == IPC_MESSAGE_STATUS_SUCCESS) {
        for (;;) {
            int temporary_receiving_result =
                    read(
                    read_matrix[destination][((ipc_message*)source) -> message_id],
                    &receiving_message -> s_header,
                    sizeof(MessageHeader)
                    );
            if (temporary_receiving_result == IPC_MESSAGE_STATUS_SUCCESS
            || temporary_receiving_result == IPC_MESSAGE_STATUS_NOT_SENT) {
                continue;
            }
            if (receiving_message -> s_header.s_payload_len > 0) {
                for (;;) {
                    temporary_receiving_result =
                        read(
                        read_matrix[destination][((ipc_message *)source)->message_id],
                        &receiving_message->s_payload,
                        receiving_message->s_header.s_payload_len
                        );
                    if (!(temporary_receiving_result == IPC_MESSAGE_STATUS_SUCCESS
                    || temporary_receiving_result == IPC_MESSAGE_STATUS_NOT_SENT)) {
                        break;
                    }
                }
            }
            return receiving_message -> s_header.s_type;
        }
    } else {
        return validation_result;
    }
}

int receive_any(void* source, Message* receiving_message) {
    for (;;) {
        for (int current_receiver = 0; current_receiver < MAXIMAL_PROCESSES_NUMBER; current_receiver += 1) {
            if (current_receiver == ((ipc_message*) source) -> message_id) {
                continue;
            }
            int temporary_receiving_result =
                read(
                read_matrix[current_receiver][((ipc_message*) source) -> message_id],
                &receiving_message -> s_header,
                sizeof(MessageHeader)
                );
            if (temporary_receiving_result == IPC_MESSAGE_STATUS_SUCCESS
            || temporary_receiving_result == IPC_MESSAGE_STATUS_NOT_SENT) {
                continue;
            }
            if (receiving_message -> s_header.s_payload_len > 0) {
                for (;;) {
                    temporary_receiving_result =
                        read(
                        read_matrix[current_receiver][((ipc_message*) source) -> message_id],
                        &receiving_message -> s_payload,
                        receiving_message -> s_header.s_payload_len
                        );
                    if (!(temporary_receiving_result == IPC_MESSAGE_STATUS_SUCCESS
                    || temporary_receiving_result == IPC_MESSAGE_STATUS_NOT_SENT)) {
                        break;
                    }
                }
            }
            return receiving_message -> s_header.s_type;
        }
    }
}

int receive_from_all_children(ipc_message* source, Message* receiving_message, int children_processes_number) {
    for (int current_children = 1; current_children <= children_processes_number; current_children += 1) {
        if (current_children == source -> message_id) {
            continue;
        }
        receive(source, current_children, receiving_message);
    }
    return receiving_message -> s_header.s_type;
}

int send_started_to_all(ipc_message* start_source) {
    Message started_message = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type = STARTED,
        },
    };
    int shift = 1;
    int payload_len = sprintf(
        started_message.s_payload,
        log_started_fmt,
        get_physical_time(),
        start_source -> message_id,
        getpid(),
        getppid(),
        start_source -> balance_history.s_history[start_source -> balance_history.s_history_len - shift].s_balance
    );
    started_message.s_header.s_payload_len = payload_len;
    return send_multicast(start_source, &started_message);
}

int send_done_to_all(ipc_message* done_source) {
    Message done_message = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type = DONE,
        },
    };
    int shift = 1;
    int payload_len = sprintf(
            done_message.s_payload,
        log_done_fmt,
        get_physical_time(),
        done_source -> message_id,
        done_source -> balance_history.s_history[done_source->balance_history.s_history_len - shift].s_balance
    );
    done_message.s_header.s_payload_len = payload_len;
    return send_multicast(done_source, &done_message);
}

int send_stop_to_all(ipc_message* stop_source) {
    Message stop_message = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type = STOP,
            .s_payload_len = 0,
            .s_local_time = get_physical_time(),
        },
    };
    return send_multicast(stop_source, &stop_message);
}