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

#include "banking.h"
#include "structures.h"
#include "ipc.h"
#include "pa2345.h"

void renew_metrics(int, timestamp_t);

pid_t pid_of_processes[MAXIMAL_PROCESSES_NUMBER + 1];
balance_t initial_balances_of_clients[MAXIMAL_PROCESSES_NUMBER + 1];

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

void log_starting_info() {
    pid_t pid = getpid();
    pid_t parent_pid = getppid();
    balance_t log_balance = message.balance_history.s_history[message.balance_history.s_history_len - 1].s_balance;
    log_info(log_started_fmt, get_physical_time(), message.message_id, pid, parent_pid, log_balance);
}

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

void log_transmit_info(local_id source_id, local_id destination_id, balance_t balance) {
    log_info(log_transfer_out_fmt, get_physical_time(), source_id,balance, destination_id);
}
void move_to_queue(ipc_message* money_source, timestamp_t initial_time, timestamp_t time_limit, balance_t current_money) {
    if (time_limit > initial_time) {
        for (timestamp_t time = initial_time; time < time_limit; time = time + 1) {
            money_source ->
            balance_history
            .s_history[time]
            .s_balance_pending_in = current_money;
        }
    }
}

void renew_metrics(int state_sum, timestamp_t time) {
    timestamp_t time_now = get_lamport_time();

    if (time_now <= MAX_T) {

        int history_length = message.balance_history.s_history_len;

        if (history_length - time_now == 1) {
            if (state_sum > 0)
                move_to_queue(&message, time, time_now, state_sum);
            message.balance_history.s_history[history_length - 1].s_balance += state_sum;
        } else if (history_length == time_now) {
            if (state_sum > 0)
                move_to_queue(&message, time, time_now, state_sum);
            balance_t new_balance = message.balance_history.s_history[history_length - 1].s_balance + state_sum;
            history_length = history_length + 1;
            message.balance_history.s_history[history_length - 1] = (BalanceState) {
                    .s_balance = new_balance,
                    .s_time = time_now,
                    .s_balance_pending_in = 0,
            };
            message.balance_history.s_history_len++;
        } else if (time_now - history_length > 0) {
            balance_t last_balance = message.balance_history.s_history[history_length - 1].s_balance;
            for (int index = history_length; index < time_now; index++) {
                message.balance_history.s_history[index] = (BalanceState) {
                        .s_balance = last_balance,
                        .s_time = index,
                        .s_balance_pending_in = 0,
                };
            }

            if (state_sum > 0) move_to_queue(&message, time, time_now, state_sum);

            message.balance_history.s_history[time_now] = (BalanceState) {
                    .s_balance = last_balance + state_sum,
                    .s_time = time_now,
                    .s_balance_pending_in = 0,
            };

            message.balance_history.s_history_len = time_now + 1;
        }
    }
}

void pipes_cleanup() {
    for (size_t pipe_i = 0; pipe_i < MAXIMAL_PROCESSES_NUMBER; pipe_i++) {
        for (size_t pipe_j = 0; pipe_j < MAXIMAL_PROCESSES_NUMBER; pipe_j++) {
            if (pipe_i != pipe_j) {
                if (pipe_j != message.message_id) {
                    close(write_matrix[pipe_i][pipe_j]);
                    close(read_matrix[pipe_i][pipe_j]);
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


void share_handler(ipc_message* source, Message* content) {
    balance_t money_diff;
    timestamp_t time_for_move = content -> s_header.s_local_time;
    Transfer* pointer_to_transfer = (Transfer*) content -> s_payload;
    TransferOrder transfer_order = pointer_to_transfer -> transfer_order;
    if (message.message_id == transfer_order.s_dst) {
        money_diff = transfer_order.s_amount;
        log_info(log_transfer_in_fmt, transfer_order.s_src, transfer_order.s_dst, transfer_order.s_amount);
        source -> message_time += 1;
        Message ack_message = {
                .s_header = {
                        .s_magic = MESSAGE_MAGIC,
                        .s_type = ACK,
                        .s_local_time = get_lamport_time(),
                        .s_payload_len = 0,
                },
        };
        ack_message.s_header.s_payload_len = strlen(ack_message.s_payload);
        send(&message, PARENT_ID, &ack_message);
    }
    else {
        money_diff = -transfer_order.s_amount;
        log_transmit_info(transfer_order.s_src, transfer_order.s_dst, transfer_order.s_amount);
        pointer_to_transfer -> message_id = message.message_id;
        transfer(
                pointer_to_transfer,
                transfer_order.s_src,
                transfer_order.s_dst,
                transfer_order.s_amount
        );
    }
    renew_metrics(money_diff, time_for_move);
}

int release_cs(const void* source) {
    int shift = 1;
    timestamp_t latest_time = 0;
    AllHistory* history = (AllHistory*) source;
    AllHistory pa3_update_history = (AllHistory) {.s_history_len = history->s_history_len,};
    source = &pa3_update_history;
    Message release_message;
    for (size_t current_source = shift; current_source <= history->s_history_len; current_source += 1) {
        message.message_time += shift;
        receive(&message, current_source, &release_message);
        increase_latest_time(&message, release_message.s_header.s_local_time);
        if (release_message.s_header.s_type == BALANCE_HISTORY) {
            BalanceHistory* balance_history = (BalanceHistory*) release_message.s_payload;
            history -> s_history[balance_history -> s_id - shift] = *balance_history;
            if (latest_time < release_message.s_header.s_local_time) latest_time =
                    release_message.s_header.s_local_time;
        }
        else return IPC_MESSAGE_STATUS_ERROR_CANNOT_READ;
    }
    for (size_t current_child_id = shift; current_child_id <= history -> s_history_len; current_child_id += shift) {
        BalanceHistory balance_history = history -> s_history[current_child_id - shift];
        BalanceHistory new_balance_history = (BalanceHistory) {.s_history_len = latest_time + shift,.s_id = balance_history.s_id,};
        for (int iterable = 0; iterable < balance_history.s_history_len; iterable++) {
            new_balance_history.s_history[iterable].s_time = iterable;
            new_balance_history.s_history[iterable].s_balance_pending_in = balance_history.s_history[iterable].s_balance_pending_in;
            new_balance_history.s_history[iterable].s_balance = balance_history.s_history[iterable].s_balance;
        }

        balance_t last_balance = balance_history.s_history[balance_history.s_history_len - shift].s_balance;
        for (int iterable2 = balance_history.s_history_len; iterable2<=latest_time; iterable2++) {
            new_balance_history.s_history[iterable2].s_time = iterable2;
            new_balance_history.s_history[iterable2].s_balance_pending_in = 0;
            new_balance_history.s_history[iterable2].s_balance = last_balance;
        }

        pa3_update_history.s_history[current_child_id - shift] = new_balance_history;

    }
    print_history(&pa3_update_history); // print updated history in pa3 comparing with pa2
    return IPC_MESSAGE_STATUS_SUCCESS;
}

int main(int argc, char * argv[]) {
    int limit_arg_number = 3;

    int has_enough_args = argc >= limit_arg_number;
    int has_flag = strcmp(argv[1], "-p") == 0;
    int strtol_is_valid = strtol(argv[2], NULL, 10) == argc - limit_arg_number;

    size_t children_process_number;
    if (has_enough_args && has_flag && strtol_is_valid) {
        children_process_number = strtol(argv[2], NULL, 10);
        if (children_process_number >= MAXIMAL_PROCESSES_NUMBER) {
            log_formatted_errors_without_file("Processes number is out of bounds.");
            return 1;
        } else {
            for (int current_child = 1; current_child <= children_process_number; current_child += 1) {
                initial_balances_of_clients[current_child] = strtol(
                        argv[current_child + 2],
                        NULL,
                        10
                );
            }
        }
    } else {
        log_formatted_errors_without_file("Invalid program running options.");
        return IPC_MESSAGE_STATUS_ERROR_PIPE_IS_CLOSED;
    }
    if (children_process_number >= MAXIMAL_PROCESSES_NUMBER) {
        log_formatted_errors_without_file("Processes number is out of bounds.");
        return IPC_MESSAGE_STATUS_ERROR_PIPE_IS_CLOSED;
    }
    number_of_processes = children_process_number + 1;
    do_pre_run_logging();
    int failed_to_open_pipes_exit_code = 2;
    for (size_t i = 0; i < number_of_processes; i += 1) {
        for (size_t j = 0; j < number_of_processes; j += 1) {
            if (i == j) {
                continue;
            } else {
                int fd[2]; pipe(fd);
                if (fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL, 0) | O_NONBLOCK) < 0
                    || fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL, 0) | O_NONBLOCK)) {
                    exit(failed_to_open_pipes_exit_code);
                }
                read_matrix[i][j] = fd[0];
                write_matrix[i][j] = fd[1];
                log_pipes_is_ready(i, j);
            }
        }
    }

    pid_of_processes[PARENT_ID] = getpid();

    for (size_t current_id = 1; current_id <= children_process_number; current_id += 1) {
        int child_pid = fork();
        if (child_pid == 0) {
            message = (ipc_message) {
                    .balance_history = (BalanceHistory) {
                            .s_id = current_id,
                            .s_history[0] = (BalanceState) {
                                .s_balance_pending_in = 0,
                                .s_time = 0,
                                .s_balance = initial_balances_of_clients[current_id],
                            },
                            .s_history_len = 1,
                    },
                    .message_id = current_id,
            };
            break;
        } else if (child_pid > 0) {
            message = (ipc_message) {.message_id = PARENT_ID,};
            pid_of_processes[current_id] = child_pid;
        } else {
            log_formatted_errors_without_file("Process from parent %d not created.", pid_of_processes[PARENT_ID]);
        }
    }

    pipes_cleanup();
    ipc_message_status status;

    message.message_time = 0;

    if (message.message_id == PARENT_ID) {
        log_starting_info();
        Message content;
        receive_from_all_children(&message, &content, children_process_number);
        log_running_processes();
        Transfer msg_transfer = {.message_id = message.message_id,};
        bank_robbery(&msg_transfer, children_process_number);
        status = send_stop_to_all(&message);
        if (status != IPC_MESSAGE_STATUS_SUCCESS) {
            log_formatted_errors_without_file(" Multicast sending from %d. finished with no-zero code: %d\n", message.message_id, status);
        }
        receive_from_all_children(&message, &content, children_process_number);
        log_finished_processes();
        AllHistory all_history = {.s_history_len = children_process_number,};
        status = release_cs(&all_history);
        if (status != IPC_MESSAGE_STATUS_SUCCESS) {
            print_history(&all_history);
        } else {
            log_formatted_errors_without_file("Message receiving failed with no-zero code: %d\n", status);
        }
    }
    else {
        Message content;
        send_started_to_all(&message);
        log_starting_info();
        receive_from_all_children(&message, &content, children_process_number);
        log_running_processes();
        int16_t type = -1;
        local_id children_processes_left = children_process_number - 1;
        for (;;) {
            if (type == STOP) {
                break;
            } else {
                message.message_time += 1;
                type = receive_any(&message, &content);
                increase_latest_time(&message, content.s_header.s_local_time);
                if (type == TRANSFER) {
                    share_handler(&message, &content);
                } else if (type == DONE) {
                    children_processes_left--;
                } else if (type == STOP) {
                    send_done_to_all(&message);
                }
            }
        }
        log_finished_processes();
        type = -1;
        while(children_processes_left > 0) {
            message.message_time += 1;
            type = receive_any(&message, &content);
            increase_latest_time(&message, content.s_header.s_local_time);
            if (type == TRANSFER) {
                share_handler(&message, &content);
            } else if (type == DONE) {
                children_processes_left--;
            }
        }
        log_finished_processes();
        message.message_time++;
        Message msg_hisory = {
            .s_header = {
                .s_type = BALANCE_HISTORY,
                .s_local_time = message.message_time,
                .s_magic = MESSAGE_MAGIC,
                .s_payload_len = sizeof(BalanceHistory),
            },
        };
        memcpy(
            &msg_hisory.s_payload,
            &message.balance_history,
            sizeof(BalanceHistory)
        );
        status = send(&message, PARENT_ID, &msg_hisory);
        //if (status != IPC_MESSAGE_STATUS_SUCCESS) {
            //log_formatted_errors_without_file("Multicast sending from %d. finished with no-zero code: %d\n", message.message_id, status);
        //}
    }
    connect_to_children_processes(PARENT_ID);
    return 0;
}
