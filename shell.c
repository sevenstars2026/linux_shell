#include "shell.h"

int main() {
    char line[1024];
    char my_path[1024];      //缓冲区getcwd
    Token toks[256];         // 栈上分配

    while (1) {
        printf("shell:%s> ", getcwd(my_path, 1024));
        fflush(stdout);//清空缓冲区
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        //分析输入类型
        int n = tokenize(line, toks);
        if (n == 0) continue;  // 空行
        
        // 检查是否是内置命令（不支持管道的命令）
        if (toks[0].type == TOK_WORD && strcmp(toks[0].val,"exit")==0) {
            exit(0);
        }else if (toks[0].type == TOK_WORD && strcmp(toks[0].val,"cd")==0) {
            if (toks[1].type == TOK_END) {
                if (chdir(getenv("HOME")) != 0) perror("cd");
            }else if (toks[1].type == TOK_WORD && chdir(toks[1].val) != 0) {
                perror("cd");
            }
            continue;
        }
        
        // 解析为管道
        Pipeline *pipeline = parse_tokens_to_pipeline(toks, n);
        if (pipeline->head != NULL && pipeline->head->cmd->argc > 0) {
            execute_pipeline(pipeline);
        }
        free_pipeline(pipeline);
    }
    return 0;
}

