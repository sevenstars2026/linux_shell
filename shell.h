#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
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
typedef struct Redir {
    enum TokenType type;
    char *file;
    struct Redir *next;//c有自引用限制
}Redir;//重定向链表
typedef struct {
    char *argv[128];
    int argc;
    struct Redir *redirs;  // 重定向链表头
} Cmd;//cmd结构体（内含头指针）

typedef struct {
    enum TokenType type;
    char *val;
}Token;//创建结构体存储字符类型以及字符指针
static inline int tokenize(char *line,Token *token){
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
}//输入切割分类

static inline void execute(char ** argv) {
    if (argv[0] == NULL) return;
    pid_t pid = fork();
    if (pid ==0) {
        execvp(argv[0],argv);
        perror("execvp");//异常处理
        exit(1);
    }else{wait(NULL);}
}

static inline Cmd* parse_tokens_to_cmd(Token *toks) {
    Cmd *cmd=malloc(sizeof(Cmd));
    cmd->argc=0;
    cmd->redirs=NULL;
    for (int i=0;toks[i].type!=TOK_END;++i) {
        if (toks[i].type == TOK_WORD) {
            cmd->argv[cmd->argc]=strdup(toks[i].val);
            cmd->argc++;
        }else if (toks[i].type == TOK_REDIR_APP||
            toks[i].type == TOK_REDIR_IN||
            toks[i].type == TOK_REDIR_OUT) {
                    if (toks[i+1].type == TOK_WORD) {
                        struct Redir *new_node=malloc(sizeof(struct Redir));
                        new_node->next=cmd->redirs;
                        new_node->file=strdup(toks[i+1].val);
                        new_node->type=toks[i].type;
                        cmd->redirs=new_node;
                        ++i;
                    }else {
                        fprintf(stderr,"syntax error: > without filename\n");
                    }
        }
    }
    cmd->argv[cmd->argc]=NULL;
    return cmd;
}//重定向，构建Redir链表

static inline void apply_redirs(Redir *redirs) {
    Redir *r = redirs;
    while (r != NULL) {
        int fd=-1;
        if (r->type == TOK_REDIR_IN) {
            fd=open(r->file,O_RDONLY);
            if (fd < 0) { perror(r->file); exit(1); }
            dup2(fd,STDIN_FILENO);
        }else if (r->type == TOK_REDIR_OUT) {
            fd=open(r->file,O_WRONLY|O_CREAT|O_TRUNC,0644);
            if (fd < 0) { perror(r->file); exit(1); }
            dup2(fd,STDOUT_FILENO);
        }else if (r->type == TOK_REDIR_APP) {
            fd=open(r->file,O_WRONLY|O_CREAT|O_APPEND,0644);
            if (fd < 0) { perror(r->file); exit(1); }
            dup2(fd,STDOUT_FILENO);
        }
        if (fd>=0) close(fd);
        r=r->next;
    }
}//dup2应用重定向

static inline void execute_cmd(Cmd *cmd) {
    if (cmd->argc==0) return;
    pid_t pid = fork();
    if (pid <0) {perror("fork");exit(1);}
    if (pid ==0) {
        apply_redirs(cmd->redirs);
        execvp(cmd->argv[0],cmd->argv);
        fprintf(stderr,"myshell: %s: command not found\n",cmd->argv[0]);
        exit(127);
    }else {
        wait(NULL);
    }
}//调用apply_redirs

static inline void free_cmd(Cmd *cmd) {
    if (cmd==NULL) return;
    for (int i=0;i<cmd->argc;++i) {
        free(cmd->argv[i]);
    }
    Redir *r = cmd->redirs;
    while (r != NULL) {
        Redir *next = r->next;
        free(r->file);
        free(r);
        r=next;
    }
    free(cmd);
}