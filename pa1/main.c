#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"
#include "pa1.h"
#include "messaging.h"
#include "logger.h"

void close_pipes();

void wait_for_children(local_id);

pid_t process_pids[MAXIMUM_PROCESSES_NUMBER+1];

static const char * const help =
        "Lab work #1 for disributed systems. IPC messaging. Usage: -p NUM_PROCESSES (0 .. 10)\n";

int main(int argc, char const *argv[]) {
    size_t num_children;
    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        num_children = strtol(argv[2], NULL, 10);
    } else {
        log_formatted_errors(help);
        return 1;
    }
    if (num_children >= MAXIMUM_PROCESSES_NUMBER) {
        log_formatted_errors(help);
        return 1;
    }
    processes_number = num_children + 1;
    init_logging();
    // pipes opening
    for (size_t source = 0; source < processes_number; source++) {
        for (size_t destination = 0; destination < processes_number; destination++) {
            if (source != destination) {
                int fd[2]; pipe(fd);
                read_matrix[source][destination] = fd[0];
                write_matrix[source][destination] = fd[1];
                log_pipe_opened(source, destination);
            }
        }
    }
    // create processes for IPC messaging
    process_pids[PARENT_ID] = getpid();
    for (size_t id = 1; id <= num_children; id++) {
        int child_pid = fork();
        if (child_pid > 0) {
            ipc_message = (message) { .message_id = PARENT_ID };
            process_pids[id] = child_pid;
        } else if (child_pid == 0) {
            ipc_message = (message) { .message_id = id };
            break;
        } else {
            log_formatted_errors("Error: Cannot create process. Parrent: %d", process_pids[PARENT_ID]);
        }
    }
    // close pipes
    close_pipes();
    message_status status;
    // send STARTED message
    if (ipc_message.message_id != PARENT_ID) {
        Message msg = {.s_header = {
                                .s_magic = MESSAGE_MAGIC,
                                .s_type = STARTED,
                                    },
                       };
        sprintf(msg.s_payload, log_started_fmt, ipc_message.message_id, getpid(), getppid());
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        status = send_multicast(&ipc_message, &msg);
        if (status != SUCCESS) {
            log_formatted_errors("Error: failed to send multicast from %d. Code: %d\n", ipc_message.message_id, status);
        }
        log_started();
    }
    // get STARTED message
    for (size_t i = 1; i <= num_children; i++) {
        Message msg;
        if (i == ipc_message.message_id) continue;
        receive(&ipc_message, i, &msg);
    }
    log_all_started();
    // send final message
    if (ipc_message.message_id != PARENT_ID) {
        Message msg = {
                .s_header = {
                    .s_magic = MESSAGE_MAGIC,
                    .s_type = DONE,
                            },
        };
        sprintf(msg.s_payload, log_done_fmt, ipc_message.message_id);
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(&ipc_message, &msg);
        log_done();
    }

    // get final message
    for (size_t i = 1; i <= num_children; i++) {
        Message msg;
        if (i == ipc_message.message_id) {
            continue;
        }
        receive(&ipc_message.message_id, i, &msg);
    }
    log_all_done();
    wait_for_children(PARENT_ID);
    finish_logging();
    return 0;
}

void close_pipes() {
    local_id current_process = ipc_message.message_id;
    for (size_t source = 0; source < processes_number; source++) {
        for (size_t destination = 0; destination < processes_number; destination++) {
            if (source != current_process && destination != current_process &&
                source != destination) {
                close(write_matrix[source][destination]);
                close(read_matrix[source][destination]);
            }
            if (source == current_process && destination != current_process) {
                close(read_matrix[source][destination]);
            }
            if (destination == current_process && source != current_process) {
                close(write_matrix[source][destination]);
            }
        }
    }
}

void wait_for_children(local_id parrend_id) {
    if (ipc_message.message_id == parrend_id) {
        for (size_t i = 1; i <= processes_number; i++) {
            waitpid(process_pids[i], NULL, 0);
        }
    }
}


