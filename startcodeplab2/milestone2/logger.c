//
// Created by wout on 11/19/25.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>



#define READ_END 0
#define WRITE_END 1
pid_t pid;
int fd[2];

int child_loop()
{
    close(fd[WRITE_END]);  // child does NOT write

    FILE *logfile = fopen("gateway.log", "a");
    if (!logfile) {
        perror("logger: cannot open gateway.log");
        exit(1);
    }

    char msg_buffer[100];
    time_t currentTime;
    int count = 0;

    while (1) {
        memset(msg_buffer, 0, sizeof(msg_buffer));

        ssize_t n = read(fd[0], msg_buffer, sizeof(msg_buffer) - 1);
        if (n <= 0) break; // pipe closed → shutdown

        msg_buffer[n] = '\0';

        // Check for shutdown command
        if (strcmp(msg_buffer, "END\n") == 0) {
            break;
        }

        time(&currentTime);

        fprintf(logfile, "%d - %s - %s \n", count++, ctime(&currentTime), msg_buffer);
        fflush(logfile);
    }

    fclose(logfile);
    close(fd[0]);
    exit(0);
}



int create_log_process(){
    // create the pipe
    if (pipe(fd) == -1){
        printf("Pipe failed\n");
        return -1;
    }

    // fork the child
    pid = fork();
    if (pid < 0){ // fork error
        printf("fork failed\n");
        return -1;
    }
    if (pid == 0)
    {
        child_loop();
    }
    close(fd[READ_END]); // parent doesn't read
    return 0;
}


int write_to_log_process(char *msg)
{
    if (pid <= 0) return -1;
    if (!msg) return -1;

    write(fd[WRITE_END], msg, strlen(msg)+1);
    return 0;
}

int end_log_process()
{
    if (pid <= 0) return -1;

    write(fd[WRITE_END], "END\n", 5);
    close(fd[WRITE_END]);
    wait(NULL);

    pid = -1;
    return 0;
}