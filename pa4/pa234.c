#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "structures.h"
#include "ipc.h"
#include "pa2345.h"

int STRING_SIZE = 128;
int PRINT_VARIANTS = 5;
int MAXIMAL_QUEUE_CAPACITY = 64;

//void renew_metrics(int, timestamp_t);

pid_t pid_of_processes[MAXIMAL_PROCESSES_NUMBER + 1];

static FILE* log_file_for_pipes;
static FILE* log_file_for_info;

void log_formatted_arguments_without_file(const char* format, va_list args) {
    vprintf(format, args);
}

void log_formatted_errors_without_file(const char* format, ...) {
    va_list error_arguments;
    va_start(error_arguments, format);
    fprintf(stderr, format, error_arguments);
    va_end(error_arguments);
}

void log_formatted_arguments_to_file(FILE* file, const char *format, va_list args) {
    if (file == NULL) {
        fprintf(stderr, "Cannot write logs with format \"%s\"", format);
        return;
    }
    vfprintf(file, format, args);
}

void log_info(const char *format, ...) {
    va_list logging_arguments;
    va_start(logging_arguments, format);
    log_formatted_arguments_without_file(format, logging_arguments);
    log_formatted_arguments_to_file(log_file_for_info, format, logging_arguments);
    va_end(logging_arguments);
}

//void log_starting_info() {
//    pid_t pid = getpid();
//    pid_t parent_pid = getppid();
//    //balance_t log_balance = message.balance_history.s_history[message.balance_history.s_history_len - 1].s_balance;
//    log_info(log_started_fmt, get_physical_time(), message.message_id, pid, parent_pid, 0);
//}

void do_pre_run_logging() {
    if ((log_file_for_info = fopen(events_log, "w")) == NULL)
        log_formatted_errors_without_file("File cannot be opened.", events_log);

    if ((log_file_for_pipes = fopen(pipes_log, "w")) == NULL)
        log_formatted_errors_without_file("File cannot be opened.", pipes_log);
}

void log_running_processes() {
    timestamp_t phys_time = get_physical_time();
    log_info(log_received_all_started_fmt, phys_time, message.message_id);
}

void log_finished_processes() {
    timestamp_t phys_time = get_physical_time();
    log_info(log_received_all_done_fmt, phys_time, message.message_id);
}

void log_info_about_pipes(const char *format, ...) {
    va_list logging_arguments;
    va_start(logging_arguments, format);
    log_formatted_arguments_to_file(log_file_for_pipes, format, logging_arguments);
    log_formatted_arguments_without_file(format, logging_arguments);;
    va_end(logging_arguments);
}

void log_pipes_is_ready(int source_id, int destination_id) {
    log_info_about_pipes("Pipe from process %1d to %1d OPENED\n", source_id, destination_id);
}

//void log_transmit_info(local_id source_id, local_id destination_id, balance_t balance) {
//    log_info(log_transfer_out_fmt, get_physical_time(), source_id,balance, destination_id);
//}
//void move_to_queue(ipc_message* money_source, timestamp_t initial_time, timestamp_t time_limit, balance_t current_money) {
//    if (time_limit > initial_time) {
//        for (timestamp_t time = initial_time; time < time_limit; time = time + 1) {
//            money_source ->
//            balance_history
//            .s_history[time]
//            .s_balance_pending_in = current_money;
//        }
//    }
//}

//void renew_metrics(int state_sum, timestamp_t time) {
//    timestamp_t time_now = get_lamport_time();
//
//    if (time_now <= MAX_T) {
//
//        int history_length = message.balance_history.s_history_len;
//
//        if (history_length - time_now == 1) {
//            if (state_sum > 0)
//                move_to_queue(&message, time, time_now, state_sum);
//            message.balance_history.s_history[history_length - 1].s_balance += state_sum;
//        } else if (history_length == time_now) {
//            if (state_sum > 0)
//                move_to_queue(&message, time, time_now, state_sum);
//            balance_t new_balance = message.balance_history.s_history[history_length - 1].s_balance + state_sum;
//            history_length = history_length + 1;
//            message.balance_history.s_history[history_length - 1] = (BalanceState) {
//                    .s_balance = new_balance,
//                    .s_time = time_now,
//                    .s_balance_pending_in = 0,
//            };
//            message.balance_history.s_history_len++;
//        } else if (time_now - history_length > 0) {
//            balance_t last_balance = message.balance_history.s_history[history_length - 1].s_balance;
//            for (int index = history_length; index < time_now; index++) {
//                message.balance_history.s_history[index] = (BalanceState) {
//                        .s_balance = last_balance,
//                        .s_time = index,
//                        .s_balance_pending_in = 0,
//                };
//            }
//
//            if (state_sum > 0) move_to_queue(&message, time, time_now, state_sum);
//
//            message.balance_history.s_history[time_now] = (BalanceState) {
//                    .s_balance = last_balance + state_sum,
//                    .s_time = time_now,
//                    .s_balance_pending_in = 0,
//            };
//
//            message.balance_history.s_history_len = time_now + 1;
//        }
//    }
//}

void pipes_cleanup() {
    for (size_t pipe_i = 0; pipe_i < MAXIMAL_PROCESSES_NUMBER; pipe_i++) {
        for (size_t pipe_j = 0; pipe_j < MAXIMAL_PROCESSES_NUMBER; pipe_j++) {
            if (pipe_i != pipe_j) {
                if (pipe_j != message.message_id) {
                    if (pipe_i != message.message_id) {
                        close(write_matrix[pipe_i][pipe_j]);
                        close(read_matrix[pipe_i][pipe_j]);
                    }
                }
            }
            if (pipe_i == message.message_id) {
                if (pipe_j != message.message_id) {
                    close(read_matrix[pipe_i][pipe_j]);
                }
            }
            if (pipe_i != message.message_id) {
                if (pipe_j == message.message_id) {
                    close(write_matrix[pipe_i][pipe_j]);
                }
            }
        }
    }
}

void connect_to_children_processes(local_id parrend_process_id) {
    if (message.message_id == parrend_process_id)
        for (size_t index = 1; index <= MAXIMAL_PROCESSES_NUMBER; index = index + 1)
            waitpid(pid_of_processes[index], NULL, 0);
}


//void share_handler(ipc_message* source, Message* content) {
//    balance_t money_diff;
//    timestamp_t time_for_move = content -> s_header.s_local_time;
//    Transfer* pointer_to_transfer = (Transfer*) content -> s_payload;
//    TransferOrder transfer_order = pointer_to_transfer -> transfer_order;
//    if (message.message_id == transfer_order.s_dst) {
//        money_diff = transfer_order.s_amount;
//        log_info(log_transfer_in_fmt, transfer_order.s_src, transfer_order.s_dst, transfer_order.s_amount);
//        source -> message_time += 1;
//        Message ack_message = {
//                .s_header = {
//                        .s_magic = MESSAGE_MAGIC,
//                        .s_type = ACK,
//                        .s_local_time = get_lamport_time(),
//                        .s_payload_len = 0,
//                },
//        };
//        ack_message.s_header.s_payload_len = strlen(ack_message.s_payload);
//        send(&message, PARENT_ID, &ack_message);
//    }
//    else {
//        money_diff = -transfer_order.s_amount;
//        log_transmit_info(transfer_order.s_src, transfer_order.s_dst, transfer_order.s_amount);
//        pointer_to_transfer -> message_id = message.message_id;
//        transfer(
//                pointer_to_transfer,
//                transfer_order.s_src,
//                transfer_order.s_dst,
//                transfer_order.s_amount
//        );
//    }
    //renew_metrics(money_diff, time_for_move);
//}


int main(int argc, char * argv[]) {
    log_info_about_pipes("program started", 1, 2);
    size_t children_processes_number = -1;
    int is_common = 0;
    int not_all_args = argc < 3;
    if (not_all_args) {
        log_formatted_errors_without_file("Incorrect format of program running.");
        return 1;
    }
    int argument_index = 1; // because 0th is not necessary for us now
    while (argument_index < argc) {
        int has_p_flag = strcmp(argv[argument_index], "-p");
        int has_mutexl_flag = (strcmp(argv[argument_index], "--mutexl"));
        if (has_mutexl_flag == 0) {
            is_common = 1;
        } else if (has_p_flag == 0) {
            argument_index = argument_index + 1;
            if (argc > argument_index) {
                children_processes_number = strtol(argv[argument_index], NULL, 10);
                number_of_processes = children_processes_number + 1;
            } else {
                log_formatted_errors_without_file("Incorrect format of program running.");
                return 1;
            }
        } else {
            log_formatted_errors_without_file("Incorrect format of program running.");
            return 1;
        }
        argument_index = argument_index + 1;
    }
    if (children_processes_number == -1) {
        log_formatted_errors_without_file("Incorrect format of program running.");
        return 1;
    }
    if (children_processes_number >= MAXIMAL_PROCESSES_NUMBER) {
        log_formatted_errors_without_file("Incorrect format of program running.");
        return 1;
    }
    number_of_processes = children_processes_number + 1;

    do_pre_run_logging();

    int read_matrix_index = 0;
    int write_matrix_index = 1;

    for (size_t i = 0; i < number_of_processes; i = i + 1) {
        for (size_t j = 0; j < number_of_processes; j = j + 1) {
            if (i == j) continue;
            else {
                int pipe_file_descriptors[2];
                pipe(pipe_file_descriptors);
                if (fcntl(pipe_file_descriptors[0], F_SETFL, fcntl(pipe_file_descriptors[0], F_GETFL, 0) | O_NONBLOCK) < 0) exit(2);
                if (fcntl(pipe_file_descriptors[1], F_SETFL, fcntl(pipe_file_descriptors[1], F_GETFL, 0) | O_NONBLOCK) < 0)  exit(2);
                read_matrix[i][j] = pipe_file_descriptors[read_matrix_index];
                write_matrix[i][j] = pipe_file_descriptors[write_matrix_index];

                log_pipes_is_ready(i, j);
            }
        }
    }

    pid_of_processes[PARENT_ID] = getpid();
    for (size_t current_id = 1; current_id <= children_processes_number; current_id = current_id + 1) {
        int child_process_id = fork();
        if (child_process_id == 0) {
            message = (ipc_message) {
                .message_id = current_id,
                .message_time = 0,
            };
            break;
        } else if (child_process_id > 0) {
            message = (ipc_message) {
                    .message_id = PARENT_ID,
                    .message_time = 0,
            };
            break;
        } else {
            int invalid_parent = pid_of_processes[PARENT_ID];
            log_formatted_errors_without_file("fork() was unsuccessful for parent %d", invalid_parent);
        }
    }

    pipes_cleanup();

    if (message.message_id == PARENT_ID) {
        Message content;
        receive_from_all_children(&message, &content, children_processes_number);
        log_finished_processes();
    } else {
        Message content;
        send_started_to_all(&message);
        receive_from_all_children(&message, &content, children_processes_number);
        message.done_messages = 0;
        if (is_common == 1) request_cs(&message);
        char string[STRING_SIZE];
        int prints_enough = message.message_id * PRINT_VARIANTS;
        for (int index = 1; index <= prints_enough; index = index + 1) {
            memset(string, 0, sizeof(string));
            sprintf(string, log_loop_operation_fmt, message.message_id, index, prints_enough);
            print(string);
        }
        if (is_common == 1) release_cs(&message);
        send_done_to_all(&message);
        while (message.done_messages < children_processes_number - 1) {
            log_info_about_pipes("while", 1, 2);
            Message content;
            receive_any(&message, &content);
            MessageType message_type = content.s_header.s_type;
            switch (message_type)
            {
                case DONE:
                    message.done_messages++;
                    break;
                default:
                    break;
            }
        }
    }
    connect_to_children_processes(PARENT_ID);
    log_finished_processes();
    //wait_for_children(PARENT_ID);
    return 0;
}
