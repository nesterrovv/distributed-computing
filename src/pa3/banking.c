#include "structures.h"
#include "ipc.h"
#include "banking.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void transfer(void * data_from_source, local_id source_id, local_id destination_id, balance_t money) {
    message.message_time = message.message_time + 1;

    Transfer* transfer = (Transfer*) data_from_source;

    TransferOrder s_transfer_order = {source_id,destination_id,money};
    transfer -> transfer_order = s_transfer_order;

    timestamp_t lamport_time = get_lamport_time();
    size_t size_of_transfer = sizeof(Transfer);

    Message message_for_transfer = {
        .s_header = {
            .s_local_time = lamport_time,
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = size_of_transfer,
            .s_type = TRANSFER,
        },
    };

    memcpy(&message_for_transfer.s_payload, transfer,size_of_transfer);

    uint16_t message_magic_number = message_for_transfer.s_header.s_magic;

    if (message_magic_number != MESSAGE_MAGIC) return;
    if (destination_id >= MAXIMAL_PROCESSES_NUMBER) return;
    if (source_id == MAXIMAL_PROCESSES_NUMBER) return;

    local_id id_of_next_client = (transfer -> message_id != source_id) ? source_id : destination_id;
    int write_result_status = write(write_matrix[transfer -> message_id][id_of_next_client],
                             &message_for_transfer,
                             sizeof(MessageHeader) + message_for_transfer.s_header.s_payload_len);
    if (write_result_status == IPC_MESSAGE_STATUS_NOT_SENT && errno == EPIPE) return;

    if (message.message_id != PARENT_ID) {
        return;
    } else {
        Message buffer_for_received_message;
        // we're waiting for the ACK type of message in this case
        message.message_time = message.message_time + 1;
        receive(&message, destination_id, &buffer_for_received_message);
        // check the type from header
        if (buffer_for_received_message.s_header.s_type != ACK) printf("Transfer process is not atomic for the process #- %u\n", message.message_id);
        increase_latest_time(&message, buffer_for_received_message.s_header.s_local_time);
    }

}
