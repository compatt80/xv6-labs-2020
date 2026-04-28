#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_trace(void)
{
  int mask;
  // argint 专门用来获取系统调用的整型参数
  if(argint(0, &mask) < 0) // 参考 kill
    return -1;

  myproc()->trace_mask = mask; // 存入当前进程

  return 0;
}

// sysinfo 要求内核收集好数据后，把一个完整的结构体（struct sysinfo）安全地拷贝回用户空间。
// 用户态开辟空间 -> 传地址给内核 -> 内核填表 -> 拷贝回用户态。
uint64 sys_sysinfo(void)
{
  uint64 user_addr;   // 用于存放用户传进来的结构体指针地址
  struct sysinfo info;
  struct proc *p = myproc();

  if(argaddr(0, &user_addr) < 0)// 获取用户的第一个参数 系统调用采用一个参数：一个指向struct sysinfo的指针
    return -1;

  info.freemem = count_free_mem();
  info.nproc = count_active_procs();
  // copyout 参数：页表, 目标用户地址, 源内核地址, 拷贝大小
  // 携带当前进程的页表 找到虚拟地址(user_addr) 对应的真实物理地址 然后写回
  if(copyout(p->pagetable, user_addr, (char *)&info, sizeof(info)) < 0)
    return -1;
  
  return 0;
}
