#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define NUM_CHILDREN 5
#define READ_END     0
#define WRITE_END    1
#define MESSAGE_SIZE 256

void execChild(int id, int * fd);
void execActiveChild(int id, int * fd);
void generateTimestamp(char * timestamp);
void writeTofile(char *msg);

int main() {
    int fds[5][2];
    pid_t c_pid[5];
    pid_t p_pid = getpid();
    int i = 0;
    srand(0);

    // Create Pipes
    for(i = 0; i < NUM_CHILDREN; ++i) {
        if (pipe(fds[i]) == -1) {
            fprintf(stderr, "pipe() failed\n");
            return -1;
        }
    }

    // Fork processes
    for (i = 0; i < NUM_CHILDREN; ++i) {
        pid_t fork_result = fork();

        if (fork_result == 0) {
            // Child code
            printf("Child %d with pid %d created\n",i,getpid());
            if (i < NUM_CHILDREN - 1) {
                execChild(i, fds[i]);
                return 0;
            } 
            else {
                execActiveChild(i, fds[i]);
                return 0;
            }
        } 
        else if (fork_result > 0) { 
            c_pid[i] = fork_result;
            close(fds[i][WRITE_END]);
        } 
        else {
            fprintf(stderr, "fork() failed on child %d\n", i);
            return -1;
        }
    }

    // Parent code
    close(0);
    time_t start_time = time(0);

    fd_set inputfds;
    struct timeval timeout;
    char read_buffer[MESSAGE_SIZE];
    int select_result = 1;

    while (select_result > 0) {
        if(time(0) > start_time + 30) {
            for(i = 0; i < NUM_CHILDREN; ++i) {
                kill(c_pid[i], SIGKILL);
            }
            return 0;
        }

        FD_ZERO(&inputfds);
        for(i = 0; i < NUM_CHILDREN; ++i) {
            FD_SET(fds[i][READ_END], &inputfds);
        }

        timeout.tv_sec = 2;
        timeout.tv_usec = 500000;

        select_result = select(FD_SETSIZE, &inputfds, 0, 0, &timeout);

        if (select_result < 0) {
            printf("Select Error\n");
        } else if (select_result > 0) {
            for(i = 0; i < NUM_CHILDREN; ++i) {
                if(FD_ISSET(fds[i][READ_END], &inputfds)) {
                    int nread = read(fds[i][READ_END], read_buffer, MESSAGE_SIZE);
                    read_buffer[nread] = 0;
                    writeTofile(read_buffer);
                }
            }
        }
    }

    for (i = 0; i < NUM_CHILDREN; ++i) {
        close(fds[i][READ_END]);
    }

    return 0;
}

void execChild(int id, int * pipe) {
    close(0);
    close(pipe[READ_END]);
    char timestamp[16] = {0};
    char msg_buffer[MESSAGE_SIZE] = {0};
    int msg_count = 1;

    while(1) {
        generateTimestamp(timestamp);
        sprintf(msg_buffer, "%s : Child %d message %d \n", timestamp, id, msg_count);
        write(pipe[WRITE_END], msg_buffer, strlen(msg_buffer) + 1);
        ++msg_count;

        int sleep_time = rand() % 3;
        sleep(sleep_time);
    }
}

void execActiveChild(int id, int * pipe) {
    char timestamp[16] = {0};
    char input[MESSAGE_SIZE - 60] = {0};
    char msg_buffer[MESSAGE_SIZE] = {0};
    char msg_count = 1;

    while(1) {
        fgets(input, sizeof(msg_buffer), stdin);
        generateTimestamp(timestamp);

        sprintf(msg_buffer, "%s : Child %d message %d: %s", timestamp, id, msg_count, input);
        write(pipe[WRITE_END], msg_buffer, strlen(msg_buffer) + 1);
        ++msg_count;

        int sleep_time = rand() % 3;
        sleep(sleep_time);
    }
}

void generateTimestamp(char * timestamp)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int sec = (int) tv.tv_sec % 60;
    int msec = (int) tv.tv_usec / 1000;

    sprintf(timestamp, "0:%02d.%03d", sec, msec);
}

void writeTofile(char *msg) {
    FILE * out_fd = NULL;
    out_fd = fopen("output.txt", "a+");
    if(out_fd == NULL) {
        printf("Error opening output.txt\n");
        return;
    }

    char timestamp[16] = {0};
    generateTimestamp(timestamp);
    fprintf(out_fd, "%s : %s\n", timestamp, msg);

    fclose(out_fd);
}

