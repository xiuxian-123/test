#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include <log.h>

static void parent(pid_t pid, int input_pipe[2], int output_pipe[2]) 
{    
    close(input_pipe[0]);
    close(output_pipe[1]);

    char send_buf[1024];
    char recv_buf[1024];

    
    FILE *child_out = fdopen(output_pipe[0], "r");

    printf("2 started. Type 'exit' to quit.\n");

    while (1) {
        printf("请输入你想计算的算式...\n");
        
        char *send_line = fgets(send_buf, sizeof(send_buf), stdin);
        if (send_line == NULL)
            break;

        send_buf[strcspn(send_buf, "\n")] = 0;

        int res = strcmp(send_buf, "exit");
        if ( res == 0) {
            write(input_pipe[1], "quit\n", 5);
            break;
        }

        write(input_pipe[1], send_buf, strlen(send_buf));
        write(input_pipe[1], "\n", 1);

        char *recv_line = fgets(recv_buf, sizeof(recv_buf), child_out);
        if (recv_line != NULL) {
            printf("Result:%s", recv_buf);
        } else {
            fprintf(stderr, "2 process closed unexpectedly.\n");
            break;
        }
    }

    memset(send_buf, 0, sizeof(send_buf));
    memset(recv_buf, 0, sizeof(recv_buf));

    close(input_pipe[1]);
    fclose(child_out);

    int status;
    waitpid(pid, &status, 0);

    printf("2 exited with status %d\n", WEXITSTATUS(status));
}

static void child(int input_pipe[2], int output_pipe[2]) 
{
    dup2(input_pipe[0], STDIN_FILENO);
    dup2(output_pipe[1], STDOUT_FILENO);

    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);

    execl("./calculator", "calculator", (char *)NULL);

    fprintf(stderr, "execl failed");

    exit(1);     
}

int main()
{
    int input_pipe[2] = {0}; 
    int output_pipe[2] = {0};
    pid_t pid;

    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        fprintf(stderr, "pipe failed");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed");
        exit(1);
    }

    if (pid == 0) {
        child(input_pipe, output_pipe);
    }

    if (pid == -1) {
        parent(pid, input_pipe, output_pipe);
    }
        
        return 0;
}
