
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
char **parse_args(char *line);//切割输入
void execute(char ** argv);//execute函数执行

int main() {
    char line[1024];
    char **argv;
    while (1) {
        printf("myshell> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        argv = parse_args(line);
        execute(argv);
        printf("你输入了： %s\n",line);
    }
    return 0;
}
//函数实现
char **parse_args(char *line){
    char **argv = (char**)malloc(64 * sizeof(char *));
    int argc = 0;
    char *token=strtok(line , " \t\n");
    while (token != NULL) {
        argv[argc] = token;
        argc++;
        token=strtok(NULL , " \t\n");
    }
    argv[argc] = NULL;
    return argv;
}
void execute(char ** argv) {
    if (argv[0] == NULL) return;
    pid_t pid = fork();
    if (pid ==0) {
        execvp(argv[0],argv);
        perror("execvp");//异常处理
        exit(1);
    }else{wait(NULL);}
}
