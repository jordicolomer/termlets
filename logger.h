#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

// Log levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

// Initialize / shutdown
int  log_init(const char* filename);     // Call once at startup
void log_close(void);                    // Call at shutdown

// Set minimum log level
void log_set_level(LogLevel level);

// Main logging function
void log_message(LogLevel level, const char* file, int line, const char* fmt, ...);

// Convenience macros
#define LOG_DEBUG(...)   log_message(LOG_DEBUG,   __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)    log_message(LOG_INFO,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)    log_message(LOG_WARN,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)   log_message(LOG_ERROR,   __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...)   log_message(LOG_FATAL,   __FILE__, __LINE__, __VA_ARGS__)

#endif
