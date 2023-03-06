#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifndef LOG_LOCATION
 #define LOG_LOCATION "/tmp/command_log"
#endif

int arglog(int argc, char **argv, char **env) {
    int index;
    FILE* log;

    log = fopen(LOG_LOCATION, "a");
    if (log == NULL) {
        return 1;
    }

    for (index = 0; index < argc; index++) {
        fwrite(argv[index], 1, strlen(argv[index]), log);
        fwrite(" ", 1, 1, log);
    }
    fwrite("\n", 1, 1, log);
    fclose(log);
}

__attribute__((section(".init_array"))) static void* arglog_constructor = &arglog;