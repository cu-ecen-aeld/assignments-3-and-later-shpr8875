#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/queue.h>
#include <netdb.h>

#define PORT "9000"
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define CONNECTIONS 20

pthread_mutex_t mutex;

// Global variables
int sockfd = -1;
FILE* fp = NULL;
struct addrinfo* addrinfo_s;
char* conn_ip;

// Signal handler for SIGINT and SIGTERM
static void signal_handler(int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        
        // Cleanup and close
        if (sockfd != -1) 
        {
        	close(sockfd);
        }
        
        if (fp) 
        {
        	fclose(fp);
        	remove(FILE_PATH);
	}
        if (addrinfo_s)
        { 
        	freeaddrinfo(addrinfo_s);
        }

        pthread_mutex_destroy(&mutex);
        closelog();
        exit(0);
    }
}

// Function to handle connection
void handle_connection(int client_socket)
{
    syslog(LOG_DEBUG, "Accepted connection from %s", conn_ip);

    const int buf_size = 1024;
    char buf[buf_size];
    ssize_t recv_size;

    // Open the file
    fp = fopen(FILE_PATH, "a+");
    if (fp == NULL)
    {
        syslog(LOG_ERR, "Failed to open file");
        close(client_socket);
        return;
    }

    while ((recv_size = recv(client_socket, buf, buf_size, 0)) > 0)
    {
        pthread_mutex_lock(&mutex);
        
        // Write received data to file
        fwrite(buf, sizeof(char), recv_size, fp);
        fflush(fp);

        
        if (strchr(buf, '\n'))
        {
            rewind(fp);
            char file_buf[buf_size];
            while (fgets(file_buf, sizeof(file_buf), fp) != NULL)
            {
                send(client_socket, file_buf, strlen(file_buf), 0);
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    syslog(LOG_DEBUG, "Closed connection from %s", conn_ip);
    close(client_socket);
}


int main(int argc, char **argv)
{
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Daemon mode
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        pid_t pid = fork();
        if (pid < 0)
        { 
        	exit(EXIT_FAILURE);  
        }
        if (pid > 0)
        { 
        	exit(EXIT_SUCCESS);  
        }
        if (setsid() < 0)
        { 
        	exit(EXIT_FAILURE); 
        }
    }

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Signal handling for SIGINT and SIGTERM
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Set up the socket
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, PORT, &hints, &addrinfo_s);
    if (status != 0)
    {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    sockfd = socket(addrinfo_s->ai_family, addrinfo_s->ai_socktype, addrinfo_s->ai_protocol);
    if (sockfd == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    if (bind(sockfd, addrinfo_s->ai_addr, addrinfo_s->ai_addrlen) == -1)
    {
        syslog(LOG_ERR, "Failed to bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(addrinfo_s);

    // Start listening for connections
    if (listen(sockfd, CONNECTIONS) == -1)
    {
        syslog(LOG_ERR, "Failed to listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == -1)
        {
            syslog(LOG_ERR, "Failed to accept connection");
            continue;
        }

        // Get client IP address
        conn_ip = inet_ntoa(client_addr.sin_addr);

        // Handle connection 
        handle_connection(client_socket);
    }

    // Cleanup
    close(sockfd);
    pthread_mutex_destroy(&mutex);
    closelog();

    return 0;
}
