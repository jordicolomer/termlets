#include <string.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#ifndef _WIN32
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>

void write_stacktrace_to_fd(int fd) {
    void *array[50];
    int size = backtrace(array, 50);

    backtrace_symbols_fd(array, size, fd);
}

void crash_handler(int sig) {
    int fd = open("crash.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd >= 0) {
        dprintf(fd, "\n--- Crash detected (signal %d) ---\n", sig);
        write_stacktrace_to_fd(fd);
        close(fd);
    }

    _exit(1);
}
#else
/* Windows crash handler - simplified version without backtrace */
void crash_handler(int sig) {
    FILE *f = fopen("crash.log", "a");
    if (f) {
        fprintf(f, "\n--- Crash detected (signal %d) ---\n", sig);
        fclose(f);
    }
    _exit(1);
}
#endif
void setup_crash_handler() {
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
}
