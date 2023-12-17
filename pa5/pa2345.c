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

int TOKENS_SIZE = 128;
int TOKENS_COEF = 5;
int MAXIMAL_QUEUE_CAPACITY = 64;

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

int main(int argc, char* argv[]) {
    int argc_not_enough = argc < 3;
    int is_common = 0;
    size_t num_children = -1;
    if (argc_not_enough) {
        log_formatted_errors_without_file("Incorrect format of run: use ./pa4 -p N --mutexl [N = 1...10]\n");
        return 1;
    } else {
        int argument_index = 1; // because 1th isn't necessary for us
        while (argument_index < argc) {
            int has_mutexl_flag = strcmp(argv[argument_index], "--mutexl");
            int has_p_flag = strcmp(argv[argument_index], "-p");
            if (has_mutexl_flag == 0) is_common = 1;
            else if (has_p_flag == 0) {
                argument_index++;
                if (argc <= argument_index) {
                    log_formatted_errors_without_file("Incorrect format of run: use ./pa4 -p N --mutexl [N = 1...10]\n");
                    return 1;
                }
                else {
                    num_children = strtol(argv[argument_index], NULL, 10);
                    number_of_processes = num_children + 1;
                }
            } else {
                log_formatted_errors_without_file("Incorrect format of run: use ./pa4 -p N --mutexl [N = 1...10]\n");
                return 1;
            }
            argument_index++;
        }
        if (num_children == -1) {
            log_formatted_errors_without_file("Incorrect format of run: use ./pa4 -p N --mutexl [N = 1...10]\n");
            return 1;
        }
        if (num_children >= MAXIMAL_PROCESSES_NUMBER) {
            log_formatted_errors_without_file("Incorrect format of run: use ./pa4 -p N --mutexl [N = 1...10]\n");
            return 1;
        }

        number_of_processes = num_children + 1;

        do_pre_run_logging();

        for (size_t i = 0; i < number_of_processes; i++) {
            for (size_t j = 0; j < number_of_processes; j++) {
                if (i == j) continue;
                else {
                    int file_descriptors[2];
                    pipe(file_descriptors);
                    if (fcntl(file_descriptors[0], F_SETFL, fcntl(file_descriptors[0], F_GETFL, 0) | O_NONBLOCK) < 0) exit(2);
                    if (fcntl(file_descriptors[1], F_SETFL, fcntl(file_descriptors[1], F_GETFL, 0) | O_NONBLOCK) < 0) exit(2);
                    read_matrix[i][j] = file_descriptors[0];
                    write_matrix[i][j] = file_descriptors[1];
                    log_pipes_is_ready(i, j);
                }
            }
        }

        pid_of_processes[PARENT_ID] = getpid();

        for (size_t current_id = 1; current_id <= num_children; current_id++) {

            int child_process_id = fork();

            if (child_process_id == 0) {
                message = (ipc_message) {.message_id = current_id, .message_time = 0,};
                break;
            } else if (child_process_id > 0) {
                pid_of_processes[current_id] = child_process_id;
                message = (ipc_message) {.message_time = 0, .message_id = PARENT_ID,};
            } else if (child_process_id < 0) {
                log_formatted_errors_without_file("Error: Cannot create process. Parrent: %d", pid_of_processes[PARENT_ID]);
            } else {
                break;
            }
        }

        pipes_cleanup();

        if (message.message_id != PARENT_ID) {
            Message msg;
            send_started_to_all(&message);
            receive_from_all_children(&message, &msg, num_children);
            message.done_messages = 0;
            int num_prints = message.message_id * TOKENS_COEF;
            char tokens[TOKENS_SIZE];
            for (int token_index = 1; token_index <= num_prints; token_index++) {
                if (is_common) {
                    request_cs(&message);
                    memset(tokens, 0, sizeof(tokens));
                    sprintf(tokens, log_loop_operation_fmt, message.message_id, token_index, num_prints);
                    print(tokens);
                    release_cs(&message);
                }
            }
            send_done_to_all(&message);
            while (message.done_messages < num_children - 1) {
                Message msg;
                receive_any(&message, &msg);
                MessageType message_type = msg.s_header.s_type;
                if (message_type == DONE) {
                    message.done_messages++;
                }
            }
        } else {
            Message msg;
            receive_from_all_children(&message, &msg, num_children);
            log_finished_processes();
        }
        connect_to_children_processes(PARENT_ID);
        log_finished_processes();
        return 0;
    }
}
