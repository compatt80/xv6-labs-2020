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

    else if(pid == 0)
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

    else
    {
        close(p2c[0]);
        close(c2p[1]);

        write(p2c[1], buf , 1);

        read(c2p[0],buf,1);

        printf("%d: received pong\n" , getpid());



        close(p2c[1]);
        close(c2p[0]);

        exit(0);
    }
}