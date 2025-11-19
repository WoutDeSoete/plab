//
// Created by wout on 11/19/25.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <time.h>



#define READ_END 0
#define WRITE_END 1
char rmsg[];
FILE *logfile;
int counter = 0;
pid_t pid;
int fd[2];

int create_log_process(){
    // create the pipe
    if (pipe(fd) == -1){
        printf("Pipe failed\n");
        return 1;
    }

    // fork the child
    pid = fork();
    if (pid < 0){ // fork error
        printf("fork failed\n");
        return 1;
    }
    logfile = fopen("gateway.log", "a");
    return 0;
}


int write_to_log_process(char *msg)
{
    if (pid > 0){ // parent process
        close(fd[READ_END]);
        write(fd[WRITE_END], msg, strlen(msg)+1);
        close(fd[WRITE_END]);
    }

    else{ // child process
        close(fd[WRITE_END]);
        read(fd[READ_END], rmsg, 25);

        time_t currentTime;
        time(&currentTime);

        fprintf(logfile, "%d - %s - %s\n", counter, ctime(currentTime), rmsg);
        counter++;
        close(fd[READ_END]);
    }
    return 0;
}

int end_log_process(){
    return 0;
}