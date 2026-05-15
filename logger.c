#include "logger.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

static LogLevel min_level = LOG_INFO;
static FILE* log_file = NULL;

static const char* level_strings[] = {
    "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"
};

int log_init(const char* filename)
{
    if (log_file) {
        fclose(log_file);
    }
    
    log_file = fopen(filename, "a");  // append mode
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", filename);
        return 0;
    }
    return 1;
}

void log_close(void)
{
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_set_level(LogLevel level)
{
    min_level = level;
}

void log_message(LogLevel level, const char* file, int line, const char* fmt, ...)
{
    if (level < min_level)
        return;

    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);

    va_list args;

    // Log to file
    if (log_file) {
        fprintf(log_file, "[%s] %s %s:%d: ", 
                time_buf, level_strings[level], file, line);
        
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        
        fprintf(log_file, "\n");
        fflush(log_file);                    // Ensure it's written immediately
    }

    // Also print to console (stderr)
	/*
    fprintf(stderr, "[%s] %s %s:%d: ", 
            time_buf, level_strings[level], file, line);
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    if (level >= LOG_ERROR)
        fflush(stderr);
	*/
}
