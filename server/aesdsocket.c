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
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <sys/queue.h>
#include <netdb.h>

#define PORT "9000"
//#define FILE_PATH "/var/tmp/aesdsocketdata"
#define FILE_PATH "/dev/aesdchar"
#define CONNECTIONS 20

#define USE_AESD_CHAR_DEVICE 1


// Thread elements in structure
typedef struct {                    
    pthread_t   thread_id;                
    bool        conn_build;
    int         conn_socket;
} thread_info_t;

pthread_mutex_t mutex;
//pthread_t timestamp_threadId;


// Singly linked list node to hold thread information
typedef struct slist_data_s slist_data_t;
struct slist_data_s
{
    thread_info_t* threadInfo;
    SLIST_ENTRY(slist_data_s) entries;
};

SLIST_HEAD(slisthead, slist_data_s) head;
struct addrinfo* addrinfo_s;

// Global variables
int sockfd = -1;
FILE* fp = 0;
unsigned int sendSizeTotal = 0;
char* conn_ip;
bool callTimeStampthread = false;

// Signal handler for cleaning up child process
static void signal_handler_child(int signal_number)
{
    if (signal_number == SIGCHLD)
    {
        int saved_errno = errno;
        syslog(LOG_ERR, "In signal_handler_child");
        while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
        errno = saved_errno;
    }
}


// Function to clean up the linked list and join threads
static void cleaup_slist(void)
{
    slist_data_t *datap;
    while (!SLIST_EMPTY(&head))
    {
        datap = SLIST_FIRST(&head);
        if (datap->threadInfo && datap->threadInfo->conn_socket > 0)
        {
         	close(datap->threadInfo->conn_socket);
        	datap->threadInfo->conn_socket = 0;
        	pthread_join(datap->threadInfo->thread_id, NULL);
        	free(datap->threadInfo);
        }
        SLIST_REMOVE_HEAD(&head, entries);
        free(datap);
    }
}


// Signal handler for SIGINT and SIGTERM
static void signal_handler(int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        syslog(LOG_ERR, "Caught signal \n");
       
       // Free allocated resources and cleanup
        if (addrinfo_s)
        {
         	freeaddrinfo(addrinfo_s);
                addrinfo_s = 0;
        }
        
        if (fp)
        {
        	remove(FILE_PATH);
        	fclose(fp);
        }
        cleaup_slist();
      //  pthread_cancel(timestamp_threadId);
      //  pthread_join(timestamp_threadId, NULL);
        exit(0);
    }
}

/*
// Timestamp thread that writes current timestamp to the file every 10 seconds
static void timestamp_thread(void* thread)
{
    while (1)
    {
        sleep(10);      // in seconds
        time_t rawtime;
        struct tm *info;
        time( &rawtime );
        info = localtime( &rawtime );
        
        //Time in RFC 2822-compliant data format
        char buffer1[128] = " ";
        char buffer2[128] = "timestamp: ";
        strftime(buffer1, sizeof(buffer1), "%a, %d %b %Y %T %z", info);
        buffer1[strlen(buffer1)] = '\n';
        strcat( buffer2, buffer1 );
        //printf("%s", buffer2);
       
        // Write timestamp to file, synchronized with mutex
        pthread_mutex_lock(&mutex);
        fseek(fp, 0, SEEK_END);
        fwrite(buffer2, sizeof(char), strlen(buffer2), fp);
        pthread_mutex_unlock(&mutex);
    }
}
*/

// Thread function to handle individual client connections
static void connection_thread(void* thread)
{
    thread_info_t* threadInfo = thread;
    syslog(LOG_DEBUG, "Start connection_thread %ld\n", threadInfo->thread_id);
    while (threadInfo->conn_build == 0)
    {
        int receiveResult = 0;
        const unsigned int DataSize = 150;
        char* receiveData = malloc(DataSize+1);
        memset(receiveData, 0, DataSize+1);
        
        // Lock mutex to ensure only one thread accesses the file at a time
        pthread_mutex_lock(&mutex);
        bool newlineFound = false;
        receiveResult = recv(threadInfo->conn_socket, receiveData, DataSize, 0);
        
        if (receiveResult == -1)
        {
            syslog(LOG_DEBUG, "Receive unsuccessful \n");
        }
        
        else if (receiveResult == 0)
        {
        // connection not made
            syslog(LOG_DEBUG, "Closed connection from %s\n", conn_ip);
            syslog(LOG_DEBUG, "Receive successful \n");
            threadInfo->conn_build = true;
        }
        
        else
        {
        // Write recieved data to the file
            fseek(fp, 0, SEEK_END);
            fwrite(receiveData, sizeof(char), receiveResult, fp);
        }
        
        // Check for newline in received data
        if ( strchr(receiveData, '\n') )
        {
            newlineFound = true;
        }
        
        free(receiveData);
        fflush(fp);
        pthread_mutex_unlock(&mutex);
        
        // If newline found, start timestamp thread and send contents to the client
        if (newlineFound)
        {
            if (callTimeStampthread == false)
            {
                callTimeStampthread = true;
            //    int error = pthread_create(&timestamp_threadId,
            //                NULL,
            //                (void*) &timestamp_thread,
             //               NULL);
               // if (error != 0)
               // {
               //     exit(-1);
               // }    
            }
           
            threadInfo->conn_build = true;
            
            // Send file contents back to the client
            FILE * fp_send;
            uint8_t byte;
            fp_send = fopen(FILE_PATH, "r");
            
            while (!feof(fp_send))
            {
                byte = fgetc(fp_send);
                if (feof(fp_send))
                {
                    break;  
                }
                send(threadInfo->conn_socket, &byte, 1, 0);
            }
            
            fclose(fp_send);
        
        }
    }
    
    syslog(LOG_DEBUG, "Closing connection thread %ld\n", threadInfo->thread_id);
    close(threadInfo->conn_socket);
    threadInfo->conn_build = true;
    threadInfo->conn_socket = 0;
    
    syslog(LOG_DEBUG, "Done with connection thread %ld\n", threadInfo->thread_id);
}

// function for cleaning up and exiting
void exit_procedure(void)
{
    if (addrinfo_s)
    {
        freeaddrinfo(addrinfo_s);
        addrinfo_s = 0;
    }
    remove(FILE_PATH);
    fclose(fp);
    cleaup_slist();
    //pthread_cancel(timestamp_threadId);
    //pthread_join(timestamp_threadId, NULL);
    exit(-1);
}


int main(int argc, char **argv)
{
    // Initilise the linked list
    SLIST_INIT(&head);
    
    // Startting aesd socket
    syslog(LOG_DEBUG, "Starting AESD Socket");
    openlog("aesdsocket", LOG_PID, LOG_USER);
    
    
    //run the aesdsocket application as a daemon
    if (argc >= 2 && strcmp(argv[1], "-d") == 0)
    {
        // fork
        pid_t pid = fork();
        if (pid == -1)
        {
            syslog(LOG_ERR, "Failed in forking");
            exit(-1);
        }
        else if (pid)
        {
            syslog(LOG_ERR, "Fork successful");
            exit(0);
        }
    }
    
    // Initilise mutex
    pthread_mutex_init(&mutex, NULL);
    
    
    // Signal - for SIGTERM and SIGINT signals
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGINT, &new_action, NULL);
    
    // Setup the Socket
    struct addrinfo hints;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(NULL, PORT, &hints, &addrinfo_s);  
    if (status != 0)
    {
        syslog(LOG_ERR, "Failed to getAddrStatus");
        exit_procedure();
    }
    
    // socket creation
    sockfd = socket(hints.ai_family, hints.ai_socktype, 0);
    
    if (sockfd == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        exit_procedure();
    }
    
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        syslog(LOG_ERR, "Failed");
        exit_procedure();
    }
    
    // Binding socket to the port
    status = bind(sockfd, addrinfo_s->ai_addr, addrinfo_s->ai_addrlen);
    

    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to bind");
        exit_procedure();
    }
    
    // cleanup
    if (addrinfo_s)
    {
        freeaddrinfo(addrinfo_s);
        addrinfo_s = 0;
    }
   
    syslog(LOG_DEBUG, "Listen");
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGINT, &new_action, NULL);
    new_action.sa_handler = signal_handler_child;
    sigaction(SIGCHLD, &new_action, NULL);
      
    // Start listening for connections
    status = listen(sockfd, CONNECTIONS);
    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to listen");
        exit_procedure();
    }
    
    // open file
    fp = fopen(FILE_PATH,"a+");
    
    if (fp == NULL)
    {
        syslog(LOG_ERR, "Failed: Unable to open file.\n");
        exit(-1);
    }
    
    int new_conn_socket = 0;
    slist_data_t *datap = NULL;
    sendSizeTotal = 0;
    
    while(1)
    {
        // accept - accept a connection on a socket
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        new_conn_socket = accept(sockfd, (struct sockaddr*) &client_addr, (socklen_t*) &addrlen);
        
        if (new_conn_socket == -1)
        {
            syslog(LOG_ERR, "Failed to accept");
            exit_procedure();
        }
        
        // Get client IP address
        conn_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_DEBUG, "Accepted connection from %s\n", conn_ip);
        
        // Create connection thread with slist
        syslog(LOG_DEBUG, "Malloc start \n");
        datap = malloc(sizeof(slist_data_t));
        datap->threadInfo = malloc(sizeof(thread_info_t));
        syslog(LOG_DEBUG, "Malloc end \n");
        datap->threadInfo->conn_socket = new_conn_socket;
        datap->threadInfo->conn_build = false;
        SLIST_INSERT_HEAD(&head, datap, entries);
        
        int error = pthread_create(&datap->threadInfo->thread_id,
                    NULL,
                    (void*) &connection_thread,
                    datap->threadInfo);
                    
        if (error != 0)
        {
            exit(-1);
        }
        
        // Cleaning up the singly linked list
        SLIST_FOREACH(datap, &head, entries)
        {
            pthread_join(datap->threadInfo->thread_id, NULL);
            free(datap->threadInfo);
            SLIST_REMOVE(&head, datap, slist_data_s, entries);
        }
        
    }  
    
    syslog(LOG_DEBUG, "Ending \n");
    pthread_mutex_destroy(&mutex);
   // pthread_cancel(timestamp_threadId);
   // pthread_join(timestamp_threadId, NULL);
    fclose(fp);
    remove(FILE_PATH);
    syslog(LOG_DEBUG, "Ending \n");
    
    return 0;
}
