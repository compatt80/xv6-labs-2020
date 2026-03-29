#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int left_pipe[2])
{
    int p; // 存放第一个素数

    close(left_pipe[1]); // 只负责从左管道读，关闭写端

    if(read(left_pipe[0], &p ,sizeof(p)) == 0) // 读完
    {
        close(left_pipe[0]);
        exit(0);
    }

    // 第一个数就是分配自己的素数
    printf("prime %d\n", p); 

    int n; // 存放从左邻居中获取一个数
    int right_pipe[2];
    int has_right_neighbor = 0; // 是否有右邻居，如果没有就创建

    while(read(left_pipe[0], &n, sizeof(n)) > 0)
    {
        if(n % p != 0)
        {
            if(has_right_neighbor == 0)
            {
                pipe(right_pipe); // 创建

                int pid = fork();

                if(pid == 0) // 子进程 新右邻居
                {
                    close(left_pipe[0]);
                    sieve(right_pipe);
                }

                else
                {
                    close(right_pipe[0]);
                    has_right_neighbor = 1;
                }
            }
             // 将这个数字传递给右邻居
            write(right_pipe[1], &n, sizeof(n));
        }

    }
    close(left_pipe[0]);// 左边没数字了，关掉读端

    if(has_right_neighbor == 1)
    {
        close(right_pipe[1]);
        wait(0);
    }
    exit(0);
}



int main()
{
    int initial_pipe[2];
    pipe(initial_pipe);
    int pid = fork();
    if(pid == 0)
    {
        sieve(initial_pipe);
    }
    else
    {
        close(initial_pipe[0]);
        for (int i = 2; i <= 35; i++)  // 一直往里写数
        {
            write(initial_pipe[1], &i, sizeof(i));
        }
        close(initial_pipe[1]); // 写完
        wait(0); // 等待自己的子进程结束
        exit(0);
    }
    return 0;
}