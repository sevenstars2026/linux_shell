#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
enum TokenType {
    TOK_WORD,
    TOK_PIPE,
    TOK_REDIR_IN ,
    TOK_REDIR_OUT,
    TOK_REDIR_APP,
    TOK_BG,
    TOK_END
};//枚举类型
typedef struct {
    enum TokenType type;
    char *val;
}Token;//创建结构体存储字符类型以及字符指针
int tokenize(char *line,Token *token){
    int argc = 0;
    char *word=strtok(line , " \t\n");
    while (word != NULL) {
        if (strcmp(word,"|")==0) {
            token[argc].type=TOK_PIPE;
            token[argc].val=NULL;
        }else if (strcmp(word,"&")==0) {
            token[argc].type=TOK_BG;
            token[argc].val=NULL;
        }else if (strcmp(word,"<")==0) {
            token[argc].type=TOK_REDIR_IN;
            token[argc].val=NULL;
        }else if (strcmp(word,">>")==0) {
            token[argc].type=TOK_REDIR_APP;
            token[argc].val=NULL;
        }else if (strcmp(word,">")==0) {
            token[argc].type=TOK_REDIR_OUT;
            token[argc].val=NULL;
        }else {
            token[argc].type=TOK_WORD;
            token[argc].val=word;
        }
        argc++;
        word=strtok(NULL , " \t\n");
    }
    token[argc].type=TOK_END;
    token[argc].val=NULL;
    return argc;
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
