#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(2, "Usage: xargs <command> [args...]\n");
        exit(1);
    }

    char *xargv[MAXARG]; // 所有命令的拼接数组 是字符串数组
    int xargc = 0;

    // 识别xargs后面的字符串
    for(int i = 1; i < argc; i++)
    {
        xargv[xargc++] = argv[i]; 
    }

    // 逐行标准读取
    char buf[512]; // 存放读取的字符组合 字符数组
    char c;
    int buf_len = 0;

    while(read(0, &c, sizeof(c)) != 0)
    {
        if(c == '\n')
        {
            buf[buf_len] = '\0'; // 把这一行变成一个标准的 C 语言字符串（结尾加 '\0'）

            xargv[xargc] = buf; // 把buf放到xargv 的最后一个位置

            xargv[xargc + 1] = 0; // exec要求参数数组的最后一个元素必须是 0 (NULL)

            int pid = fork();
            if(pid == 0) // 子进程
            {
                exec(xargv[0], xargv);
                exit(0);
            }
            else
            {
                wait(0);
            }

            buf_len = 0;
        }

        else
        {
            buf[buf_len++] = c;
        }

        
    }

    exit(0);

}