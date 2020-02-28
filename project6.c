#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#define NUM_CHILDREN 5
#define READ_END     0
#define WRITE_END    1

// TODO
// have children repeatedly send msgs after a random delay
// have the active child take input from stdin to send over pipe
// select to repeatedly take in messages from children
// write recieved messages to output file (with second timestamp?)
// figure out if this is everything we need to do

void execChild(int id, int * fd);
void execActiveChild(int id, int * fd);
void generateTimestamp(char * timestamp);

int main() {

    pid_t pid;
    int fd[5][2];

    // Create pipes
    int i;
    for (i = 0; i < NUM_CHILDREN; ++i) {
        if (pipe(fd[i]) == -1) {
            fprintf(stderr, "pipe() failed for %d\n", i);
            return -1;
        }
    }

    for (i = 0; i < NUM_CHILDREN; ++i) {
        pid = fork();

        if (pid == 0) {
            if (i < NUM_CHILDREN - 1) {
                execChild(i, fd[i]);
                return i;
            }
            else {
                execActiveChild(i, fd[i]);
                return i;
            }
        }

        else if (pid < 0) {
            fprintf(stderr, "fork() failed on child %d\n", i);
            return pid;
        }
    }


    // Parent code

    for (i = 0; i < NUM_CHILDREN; ++i)
        close(fd[i][WRITE_END]);

    char messageBuffer[32];
    int bytesRead = 0;
    while (bytesRead <= 0) {
        bytesRead = read(fd[0][READ_END], messageBuffer, 32);
        printf("%d bytes read\n", bytesRead);
    }
    printf("Child 1 said: %s", messageBuffer);

    for (i = 0; i < NUM_CHILDREN; ++i)
        close(fd[i][READ_END]);

    printf("Parent completed\n");
    return 0;
}


void execChild(int id, int * fd) {
    printf("Child %d created\n", id);
    close(fd[READ_END]);

    int msgCount = 1;    
    char message[32];
    char timestamp[16];

    generateTimestamp(timestamp);
    sprintf(message, "%s: Child %d message %d\n", timestamp, id, msgCount);
    write(fd[WRITE_END], message, strlen(message) + 1);
    printf("Child %d sending parent: %s", id, message);

    close(fd[WRITE_END]);
}

void execActiveChild(int id, int * fd) {
    printf("Active child %d created\n", id);
    close(fd[READ_END]);

    int msgCount = 1;
    char message[32] = "TODO: get user input\n";
    char timestamp[16];

    generateTimestamp(timestamp);
    // TODO: need to get user input to send to parent
    write(fd[WRITE_END], message, strlen(message) + 1);
    printf("Child %d sending parent: %s", id, message);

    close(fd[WRITE_END]);
}

void generateTimestamp(char * timestamp) {  
    // TODO: validate this timestamp, is it accurate and in proper form
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int sec = (int) tv.tv_sec % 60;
    int msec = (int) tv.tv_usec / 1000;

    sprintf(timestamp, "0:%02d.%03d", sec, msec);
}

