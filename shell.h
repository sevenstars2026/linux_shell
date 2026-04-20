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
typedef struct ArgNode {
    char *val;
    struct ArgNode *next;
} ArgNode;  // 参数链表

typedef struct Redir {
    enum TokenType type;
    char *file;
    struct Redir *next;//c有自引用限制
}Redir;//重定向链表

typedef struct {
    ArgNode *args;  // 参数链表头
    int argc;
    struct Redir *redirs;  // 重定向链表头
} Cmd;//cmd结构体（内含两个头指针）

typedef struct PipelineNode {
    Cmd *cmd;
    struct PipelineNode *next;
} PipelineNode;  // 管道链表

typedef struct {
    PipelineNode *head;
    int bg;  // 是否后台执行
} Pipeline;  //管道结构体（内含头指针）

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

static inline Cmd* parse_tokens_to_cmd(Token *toks, int start, int end) {
    Cmd *cmd=malloc(sizeof(Cmd));
    cmd->argc=0;
    cmd->args=NULL;
    cmd->redirs=NULL;
    
    ArgNode *last_arg=NULL;
    
    for (int i=start;i<end;++i) {
        if (toks[i].type == TOK_WORD) {
            ArgNode *new_node=malloc(sizeof(ArgNode));
            new_node->val=strdup(toks[i].val);
            new_node->next=NULL;
            
            if (cmd->args==NULL) {
                cmd->args=new_node;
            }else {
                last_arg->next=new_node;
            }
            last_arg=new_node;
            cmd->argc++;
        }else if (toks[i].type == TOK_REDIR_APP||
            toks[i].type == TOK_REDIR_IN||
            toks[i].type == TOK_REDIR_OUT) {
                    if (i+1<end && toks[i+1].type == TOK_WORD) {
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
    return cmd;
}//重定向，构建Redir链表和ArgNode链表

static inline Pipeline* parse_tokens_to_pipeline(Token *toks, int n) {
    Pipeline *pipe = malloc(sizeof(Pipeline));
    pipe->head = NULL;
    pipe->bg = 0;
    
    // 检查是否有后台执行符号
    if (n > 0 && toks[n-1].type == TOK_BG) {
        pipe->bg = 1;
        n--;  // 忽略最后的 &
    }
    
    PipelineNode *last_node = NULL;  // 尾插指针
    int cmd_start = 0;
    for (int i = 0; i <= n; ++i) {
        if (i == n || toks[i].type == TOK_PIPE) {
            // 解析一个命令
            Cmd *cmd = parse_tokens_to_cmd(toks, cmd_start, i);
            
            // 添加到管道链表
            PipelineNode *node = malloc(sizeof(PipelineNode));
            node->cmd = cmd;
            node->next = NULL;
            
            if (pipe->head == NULL) {
                pipe->head = node;
            } else {
                last_node->next = node;  // 直接接到末尾，O(1)
            }
            last_node = node;  // 更新尾指针
            
            cmd_start = i + 1;
        }
    }
    
    return pipe;
}//解析为管道结构

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
        
        // 转换链表为数组
        char **argv = malloc((cmd->argc + 1) * sizeof(char*));
        ArgNode *node = cmd->args;
        int idx = 0;
        while (node != NULL) {
            argv[idx++] = node->val;
            node = node->next;
        }
        argv[idx] = NULL;
        
        execvp(argv[0],argv);
        fprintf(stderr,"myshell: %s: command not found\n",argv[0]);
        free(argv);
        exit(127);
    }else {
        wait(NULL);
    }
}//调用apply_redirs

static inline void free_cmd(Cmd *cmd) {
    if (cmd==NULL) return;
    
    // 释放参数链表
    ArgNode *arg = cmd->args;
    while (arg != NULL) {
        ArgNode *next = arg->next;
        free(arg->val);
        free(arg);
        arg = next;
    }
    
    // 释放重定向链表
    Redir *r = cmd->redirs;
    while (r != NULL) {
        Redir *next = r->next;
        free(r->file);
        free(r);
        r=next;
    }
    free(cmd);
}

static inline void free_pipeline(Pipeline *pipe) {
    if (pipe == NULL) return;
    PipelineNode *node = pipe->head;
    while (node != NULL) {
        PipelineNode *next = node->next;
        free_cmd(node->cmd);
        free(node);
        node = next;
    }
    free(pipe);
}

static inline void execute_pipeline(Pipeline *pipeline) {
    if (pipeline == NULL || pipeline->head == NULL) return;
    
    // 计算管道中有多少个命令
    int cmd_count = 0;
    PipelineNode *node = pipeline->head;
    while (node != NULL) {
        cmd_count++;
        node = node->next;
    }
    
    // 如果只有一个命令，直接执行
    if (cmd_count == 1) {
        execute_cmd(pipeline->head->cmd);
        return;
    }
    
    // 多个命令：创建管道连接
    pid_t pids[cmd_count];
    int pipes[cmd_count-1][2];  // 存储管道的读写端
    
    // 创建所有的管道
    for (int i = 0; i < cmd_count - 1; ++i) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }
    
    // fork 并 exec 每个命令
    int cmd_idx = 0;
    node = pipeline->head;
    while (node != NULL) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }
        
        if (pid == 0) {
            // 子进程
            
            // 处理输入重定向：如果不是第一个命令，从前一个管道读
            if (cmd_idx > 0) {
                dup2(pipes[cmd_idx-1][0], STDIN_FILENO);
            }
            
            // 处理输出重定向：如果不是最后一个命令，写入下一个管道
            if (cmd_idx < cmd_count - 1) {
                dup2(pipes[cmd_idx][1], STDOUT_FILENO);
            }
            
            // 关闭所有管道的文件描述符
            for (int i = 0; i < cmd_count - 1; ++i) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            
            // 应用命令的重定向（仅对最后一个命令有实际意义）
            apply_redirs(node->cmd->redirs);
            
            // 转换链表为数组
            char **argv = malloc((node->cmd->argc + 1) * sizeof(char*));
            ArgNode *arg_node = node->cmd->args;
            int idx = 0;
            while (arg_node != NULL) {
                argv[idx++] = arg_node->val;
                arg_node = arg_node->next;
            }
            argv[idx] = NULL;
            
            // 执行命令
            execvp(argv[0], argv);
            fprintf(stderr,"myshell: %s: command not found\n", argv[0]);
            free(argv);
            exit(127);
        } else {
            // 父进程
            pids[cmd_idx] = pid;
        }
        
        cmd_idx++;
        node = node->next;
    }
    
    // 父进程关闭所有管道
    for (int i = 0; i < cmd_count - 1; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // 等待所有子进程完成
    if (!pipeline->bg) {
        for (int i = 0; i < cmd_count; ++i) {
            waitpid(pids[i], NULL, 0);
        }
    } else {
        // 后台执行：打印后台进程ID
        printf("[%d]\n", pids[cmd_count-1]);
    }
}