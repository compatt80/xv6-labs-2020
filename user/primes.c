#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int left_pipe[2])
{
    int p; // p 代表当前进程负责筛选的素数

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
                    close(left_pipe[0]); // 孙子进程：它不需要再保留左边祖父传来的读端了
                    sieve(right_pipe);
                }

                else
                {
                    close(right_pipe[0]); // 当前进程：只往右边写，关掉右边的读端
                    has_right_neighbor = 1;
                }
            }
             // 将这个数字传递给右邻居
            write(right_pipe[1], &n, sizeof(n));
        }

    }
    close(left_pipe[0]);// 左边没数字了，关掉读端

    if(has_right_neighbor == 1) // 如果创建了右邻居，必须告诉它 我这边结束了
    {
        close(right_pipe[1]); // 关闭右侧写端，右邻居的 read 才会收到 0 从而结束循环
        wait(0); // 等待右邻居
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
        close(initial_pipe[0]);  // 父进程只写不读 关闭读端 0读1写
        for (int i = 2; i <= 35; i++)  // 一直往里写数
        {
            write(initial_pipe[1], &i, sizeof(i)); // 将 2-35 写入管道
        }
        close(initial_pipe[1]); // 写完
        wait(0); // 等待自己的子进程结束
        exit(0);
    }
    return 0;
}