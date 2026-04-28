#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main()
{
    int p2c[2]; // 父传子
    int c2p[2]; // 子传父
    char buf[1]; // 存放字节

    if(pipe(p2c) < 0 || pipe(c2p) < 0)
    {
        fprintf(2, "pipe error");
        exit(1);
    }

    int pid = fork();

    if(pid < 0)
    {
        fprintf(2, "fork error");
        exit(1);
    }

    else if(pid == 0) // 子进程 读父进程的传输字符(p2c[0]) 然后打印 最后向父进程写(c2p[1])
    {
        close(p2c[1]);
        close(c2p[0]);

        read(p2c[0],buf,1);

        printf("%d: received ping\n" , getpid());

        write(c2p[1], buf , 1);

        close(p2c[0]);
        close(c2p[1]);

        exit(0);
    }

    else // 父进程 向子进程写一个字节(p2c[1]) 然后读子进程发的字节(c2p[0]) 然后打印
    {
        close(p2c[0]); // 关闭父向子的读
        close(c2p[1]); // 关闭子向父的写

        write(p2c[1], buf , 1); // 必须先写

        read(c2p[0],buf,1); // 然后再读

        printf("%d: received pong\n" , getpid()); // 打印



        close(p2c[1]);
        close(c2p[0]); // 全部关掉

        exit(0);
    }
}