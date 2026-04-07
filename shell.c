#include "shell.h"

int main() {
    char line[1024];
    char my_path[1024];      //缓冲区getcwd
    Token toks[256];         // 栈上分配

    while (1) {
        printf("myshell:%s> ", getcwd(my_path, 1024));
        fflush(stdout);//清空缓冲区
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        //分析输入类型
        int n = tokenize(line, toks);
        if (n == 0) continue;  // 空行
        Cmd *cmd =parse_tokens_to_cmd(toks);
        if (cmd->argc==0) {
            free_cmd(cmd);
            continue;
        }
        if (strcmp(cmd->argv[0],"exit")==0) {
            exit(0);
        }else if (strcmp(cmd->argv[0],"cd")==0) {
            if (cmd->argv[1]==NULL) {
                if (chdir(getenv("HOME")) != 0) perror("cd");
            }else if (chdir(cmd->argv[1]) != 0) perror("cd");
        }else{execute_cmd(cmd);}
        free_cmd(cmd);
    }
    return 0;
}

