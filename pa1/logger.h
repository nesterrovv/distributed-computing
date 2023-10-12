#pragma once
#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>

static FILE*                log_file_for_pipes;
static FILE*                log_file_for_events;
static const char* const    ERROR_CANNOT_OPEN_FILE =    "ERROR: File cannot be opened%s\n";
static const char* const    ERROR_FILE_IS_BROKEN =      "ERROR: File is broken. Cannot read message.\"%s\"";

void init_logging();

void log_started();

void log_done();

void log_all_started();

void log_all_done();

void log_pipe_opened(int from, int to);

void finish_logging();

void log_formatted(const char *format, ...);

void log_formatted_to_file(FILE* file, const char *format, ...);

void log_formatted_arguments(const char *format, va_list args);

void log_formatted_arguments_to_file(FILE* file, const char *format, va_list args);

void log_formatted_errors(const char *format, ...);

void log_formatted_events(const char *format, ...);

void log_formatted_pipes(const char *format, ...);


#endif  // __LOGGER_H
