#include "banking.h"
#include "structures.h"
#include "ipc.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void transfer(void * data_from_source, local_id source_id, local_id destination_id, balance_t money) {
    TransferOrder s_transfer_order = {
        source_id,
        destination_id,
        money
    };
    Transfer* transfer = (Transfer*) data_from_source;
    transfer -> transfer_order = s_transfer_order;
    Message message_for_transfer = {
        .s_header = {
            .s_local_time = get_physical_time(),
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = sizeof(transfer),
            .s_type = TRANSFER,
        },
    };
    memcpy(
        &message_for_transfer.s_payload,
        transfer,
        sizeof(Transfer)
    );
    uint16_t message_magic_number = message_for_transfer.s_header.s_magic;
    if (message_magic_number != MESSAGE_MAGIC
        || destination_id >= MAXIMAL_PROCESSES_NUMBER
        || source_id == MAXIMAL_PROCESSES_NUMBER) {
        return;
    } else {
        local_id id_of_next_client;
        if (transfer->message_id != source_id) {
            id_of_next_client = source_id;
        }
        else {
            id_of_next_client = destination_id;
        }
        int write_result_status = write(write_matrix[transfer->message_id][id_of_next_client],
                                 &message_for_transfer,
                                 sizeof(MessageHeader) + message_for_transfer.s_header.s_payload_len);

        if (write_result_status == IPC_MESSAGE_STATUS_NOT_SENT) {
            if (errno == EPIPE) {
                return;
            }
        }
        if (message.message_id == PARENT_ID) {
            Message buffer_for_received_message;
            // we're waiting for the ACK type of message in this case
            receive(&message, destination_id, &buffer_for_received_message);
            // check the type from header
            if (buffer_for_received_message.s_header.s_type != ACK) {
                printf("Transfer process is not atomic for the process #- %u\n", message.message_id);
            }
        }
    }

}