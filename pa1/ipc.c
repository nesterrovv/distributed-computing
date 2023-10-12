#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "ipc.h"
#include "messaging.h"

static int read_all_content(int io_descriptor_id, void* buffer, int bytes_left) {
    assert(buffer);
    assert(bytes_left >= 0);
    int read_bytes_number = 0;
    char* pointer_to_buffer = (char*) buffer;
    while (bytes_left > 0) {
        int current_bytes_read =
                read(io_descriptor_id, pointer_to_buffer, bytes_left);
        if (current_bytes_read <= 0) {
            return read_bytes_number;
        }
        pointer_to_buffer += current_bytes_read;
        read_bytes_number += current_bytes_read;
        bytes_left -= current_bytes_read;
    }
    return read_bytes_number;
}

int send(void * self, local_id dst, const Message * msg) {
    if (self == NULL) {
        return ERROR_SOURCE_PROCESS_IS_NULL;
    }
    if (((message*) self) -> message_id >= processes_number || dst >= processes_number) {
        return ERROR_NUMBER_OF_PROCESSES_EXCEEDED;
    }
    if (msg -> s_header.s_magic != MESSAGE_MAGIC) {
        return ERROR_INCORRECT_MAGIC_MESSAGE;
    }
    int write_header_row = ((message*) self) -> message_id;
    int write_header_column = dst;
    int write_header_code_result = write(
        write_matrix[write_header_row][write_header_column],
        &msg -> s_header,
        sizeof(MessageHeader)
    );
    int write_payload_row = ((message*) self) -> message_id;
    int write_payload_column = dst;
    int write_payload_code_result = write(
            write_matrix[write_payload_row][write_payload_column],
            &msg -> s_payload,
            msg -> s_header.s_payload_len
    );
    if (write_header_code_result == -1 || write_payload_code_result == -1) {
        if (errno == EPIPE) {
            return ERROR_PIPE_IS_CLOSED;
        } else {
            return ERROR_CANNOT_WRITE;
        }
    }
    return SUCCESS;
}

int send_multicast(void* self, const Message* msg) {
    message_status status = SUCCESS;
    for (local_id dst = 0; dst < processes_number; dst++) {
        if (dst != ((message*) self) -> message_id) {
            message_status result = send(self, dst, msg);
            if (result != SUCCESS) {
                status = result;
            }
        }
    }
    return status;
}

int receive(void* self, local_id from, Message * msg) {
    if (self == NULL) {
        return ERROR_SOURCE_PROCESS_IS_NULL;
    }
    if (((message*) self) -> message_id >= processes_number ||from >= processes_number) {
        return ERROR_NUMBER_OF_PROCESSES_EXCEEDED;
    }
    int read_header_row = from;
    int read_header_column = ((message*) self) -> message_id;
    int header_descriptor_id = read_matrix[read_header_row][read_header_column];
    int read_header_bytes_number = read_all_content(
            header_descriptor_id,
            &msg -> s_header,
            sizeof(MessageHeader)
    );
    int read_payload_row = from;
    int read_payload_column = ((message*) self) -> message_id;
    int payload_descriptor_id = read_matrix[read_payload_row][read_payload_column];
    int read_payload_bytes_number = read_all_content(
            payload_descriptor_id,
            &msg -> s_payload,
            msg->s_header.s_payload_len
    );
    if (read_header_bytes_number != sizeof(MessageHeader) ||
            read_payload_bytes_number != msg -> s_header.s_payload_len) {
        return ERROR_CANNOT_READ;
    }
    if (msg -> s_header.s_magic != MESSAGE_MAGIC) {
        return ERROR_INCORRECT_MAGIC_MESSAGE;
    }
    return SUCCESS;
}

int receive_any(void * self, Message * msg) {
    return ERROR_NOT_RESOLVED;
}
