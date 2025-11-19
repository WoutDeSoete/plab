#include <stdio.h>     
#include <unistd.h>     
#include <string.h>     
#include <sys/types.h>
#include <ctype.h>



#define SIZE 25
#define READ_END 0
#define WRITE_END 1
int main(void){
    char wmsg[SIZE] = "GREETings";
    char rmsg[SIZE];
    pid_t pid;
    int fd[2];

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

    if (pid > 0){ // parent process
        close(fd[READ_END]);
        write(fd[WRITE_END], wmsg, strlen(wmsg)+1);
        close(fd[WRITE_END]);
    }

    else{ // child process
        close(fd[WRITE_END]);
        read(fd[READ_END], rmsg, SIZE);

        int i = 0;
        while(i < strlen(rmsg))
        {
            if(islower(rmsg[i]))
            {
                rmsg[i] = toupper(rmsg[i]);
            }else{
                rmsg[i] = tolower(rmsg[i]);
            }
            i++;
        }
        printf("%s\n", rmsg);
        close(fd[READ_END]);
    }

    return 0;
}
