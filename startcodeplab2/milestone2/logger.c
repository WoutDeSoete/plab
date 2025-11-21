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
    printf("child loop entered\n");
    close(fd[WRITE_END]);  // child does NOT write

    FILE *logfile = fopen("gateway.log", "a");
    if (!logfile) {
        perror("logger: cannot open gateway.log");
        exit(1);
    }

    char *msg_buffer = NULL;
    size_t buf_size = 0;
    time_t currentTime;
    int count = 0;
    ssize_t n;


    FILE *log_stream = fdopen(fd[0], "r");
    if (!log_stream) {
        perror("fdopen");
        exit(1);
    }

    while ((n = getline(&msg_buffer, &buf_size, log_stream)) != -1)
    {
        if (n > 1)
        {
            if (strcmp(msg_buffer, "END\n") == 0) break;

            printf("child: logging message: %s\n", msg_buffer);

            time(&currentTime);
            char *time = ctime(&currentTime);
            time[strcspn(time, "\n")] = '\0';
            fprintf(logfile, "%d - %s - %s", count++, time, msg_buffer);
            fflush(logfile);
        }

    }


    fclose(logfile);
    fclose(log_stream);
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
        printf("child loop skipped\n");
    }
    close(fd[READ_END]); // parent doesn't read
    return 0;
}


int write_to_log_process(char *msg)
{
    if (pid <= 0) return -1;
    if (!msg) return -1;
    printf("[LOG WRITE] %s", msg);

    write(fd[WRITE_END], msg, strlen(msg));
    return 0;
}

int end_log_process()
{
    if (pid <= 0) return -1;
    printf("ending process\n");

    write(fd[WRITE_END], "END\n", 5);
    close(fd[WRITE_END]);
    wait(NULL);

    pid = -1;
    return 0;
}