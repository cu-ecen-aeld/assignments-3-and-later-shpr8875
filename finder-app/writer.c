/*
* Author: Shweta Prasad
* Description: Write a string to a text file.
* Date: 09-06-2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Open a connection to the syslog 
    openlog("writer_assignment2", 0 , LOG_USER);

    // Checking the number of correct arguments
    if (argc != 3) {
        printf("Fail: Number of parameters is not valid \n");
        syslog(LOG_ERR, "Usage: %s <writefile> <writestr>", argv[0]);
        exit(1);
    }

    // Passing two arguments
    char *writefile = argv[1];
    char *writestr = argv[2];

    // Open the file for read-write access and create it with read, write, execute permissions for owner, group, and others
    int fd = open(writefile, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd == -1) 
    {
        syslog(LOG_ERR, "Failed to open file %s , Error is %s", writefile, strerror(errno));
        printf("Error: Unable to open the file \n");
        exit(1);
    }

    // Write the string to the file
    ssize_t num_bytes = write(fd, writestr, strlen(writestr));
    if (num_bytes == -1) 
    {
        syslog(LOG_ERR, "Failed to write to file: %s Error is: %s", writefile, strerror(errno));
        printf("Error: Failed to write to file %s \n", writefile);
        exit(1);
    }

    // Logging successful write operation
    printf("Writing %s to %s \n", writestr, writefile);
    syslog(LOG_DEBUG, "Writing %s to %s \n", writestr, writefile);

    close(fd);
    
    // Close the syslog connection
    closelog();

    return 0;
}
