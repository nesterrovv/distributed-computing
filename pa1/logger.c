#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

#include "logger.h"
#include "common.h"
#include "pa1.h"
#include "messaging.h"

void init_logging() {
    if ((log_file_for_events = fopen(events_log, "w")) == NULL) {
        log_formatted_errors(ERROR_CANNOT_OPEN_FILE, events_log);
    }
    if ((log_file_for_pipes = fopen(pipes_log, "w")) == NULL) {
        log_formatted_errors(ERROR_CANNOT_OPEN_FILE, pipes_log);
    }
}

void log_started() {
    pid_t process_id = getpid();
    pid_t parent_process_id = getppid();
    log_formatted_events(log_started_fmt, ipc_message, process_id, parent_process_id);
}

void log_done() {
    log_formatted_events(log_done_fmt, ipc_message.message_id);
}

void log_all_started() {
    log_formatted_events(log_received_all_started_fmt, ipc_message.message_id);
}

void log_all_done() {
    log_formatted_events(log_received_all_done_fmt, ipc_message.message_id);
}

void log_pipe_opened(int from, int to) {
    log_formatted_events("Pipe from process %1d to %1d OPENED\n", from, to);
}

void finish_logging() {
    fclose(log_file_for_events);
    fclose(log_file_for_pipes);
}

void log_formatted(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_formatted_arguments(format, args);
    va_end(args);
}

void log_formatted_to_file(FILE* file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_formatted_arguments_to_file(file, format, args);
    va_end(args);
}

void log_formatted_arguments(const char *format, va_list args) {
    vprintf(format, args);
}

void log_formatted_arguments_to_file(FILE* file, const char *format, va_list args) {
    if (file == NULL) {
        fprintf(stderr, ERROR_FILE_IS_BROKEN, format);
        return;
    } else {
        vfprintf(file, format, args);
    }
}

void log_formatted_errors(const char *format, ...) {
    va_list args;

    va_start(args, format);
    fprintf(stderr, format, args);
    va_end(args);
}

void log_formatted_events(const char *format, ...) {
    va_list args;

    va_start(args, format);
    log_formatted_arguments(format, args);
    log_formatted_arguments_to_file(log_file_for_events, format, args);
    va_end(args);
}

void log_pipes_info(const char *format, ...) {
    va_list args;

    va_start(args, format);
    log_formatted_arguments(format, args);
    log_formatted_arguments_to_file(log_file_for_pipes, format, args);
    va_end(args);
}
