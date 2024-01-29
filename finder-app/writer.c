/*
 * File: writer.c
 * Author: Kanin McGuire
 * Date: 1/28/2024
 * Description: C application for writing content to a file with syslog logging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Check the number of arguments
    if (argc != 3) {
        // Print an error message to syslog
        openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_ERR, "Error: Usage: %s <writefile> <writestr>", argv[0]);
        closelog();

        // Print an error message to standard error
        fprintf(stderr, "Error: Usage: %s <writefile> <writestr>\n", argv[0]);
        return 1;
    }

    // Get command line arguments for the filepath and string
    char *writefile = argv[1];
    char *writestr = argv[2];

    // Create the file and write/overwrite string using write() system call
    int file = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        // Print an error message to syslog
        openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_ERR, "Error: Could not open or create file %s", writefile);
        closelog();

        // Print an error message to standard error
        fprintf(stderr, "Error: Could not open or create file %s\n", writefile);
        return 1;
    }

    ssize_t bytes_written = write(file, writestr, strlen(writestr));
    close(file);

    if (bytes_written == -1) {
        // Print an error message to syslog
        openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_ERR, "Error: Could not write to file %s", writefile);
        closelog();

        // Print an error message to standard error
        fprintf(stderr, "Error: Could not write to file %s\n", writefile);
        return 1;
    }

    // Log the message using syslog
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_DEBUG, "Writing '%s' to '%s'", writestr, writefile);
    closelog();

    printf("File '%s' created successfully with content: '%s'\n", writefile, writestr);

    return 0;
}

