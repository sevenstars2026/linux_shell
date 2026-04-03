#include "shell.h"

int main() {
    char line[1024];
    char my_path[1024];      //缓冲区getcwd
    Token toks[256];         // 栈上分配
    char *argv[128];         // 临时数组，从 token 里提取 WORD

    while (1) {
        printf("myshell:%s> ", getcwd(my_path, 1024));
        fflush(stdout);//清空缓冲区

        if (fgets(line, sizeof(line), stdin) == NULL) break;

        //分析输入类型
        int n = tokenize(line, toks);
        if (n == 0) continue;  // 空行

        // 临时：从 token 中提取 WORD，创建argv 供 execute 使用
        int argc = 0;
        for (int i = 0; toks[i].type != TOK_END; i++) {
            if (toks[i].type == TOK_WORD)
                argv[argc++] = toks[i].val;
        }
        argv[argc] = NULL;

        if (argc == 0) continue;

        if (strcmp(argv[0], "exit") == 0) {
            exit(0);
        } else if (strcmp(argv[0], "cd") == 0) {
            if (argv[1] == NULL) {
                if (chdir(getenv("HOME")) != 0) perror("cd");
            } else if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        } else {
            execute(argv);
        }
    }
    return 0;
}

