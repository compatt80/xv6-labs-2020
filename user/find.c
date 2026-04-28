#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
// 基本都参照ls.c代码
char*
fmtname(char *path) // 找到最后一个斜杠后的第一个字符
{// 如果传入 path 是 dir/subdir/target.txt，
// 循环结束后 p 指向最后一个 /。
// 执行 p++ 后，函数返回指向 target.txt 中 t 的指针。
    char *p;

    // Find first character after last slash. 
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ; // 从字符串最末尾（\0 的位置）开始，倒着往前找
    p++; // 找到最后一个 '/' 后，指针往右移一位，指向真正的文件名开头

    return p;
}

void
find(char *path, char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de; // 目录项 kernel/fs.h
  struct stat st; // kernel/stat.h

  if((fd = open(path, 0)) < 0){// 尝试打开当前路径
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){// 获取文件/目录的元数据信息存入 st
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  // 无论文件还是目录，先比对名称
  if(strcmp(fmtname(path), target) == 0)
  {
    printf("%s\n", path);
  }

  switch(st.type){
  case T_FILE: // 普通文件

    break;

  case T_DIR: // 文件目录
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    // 准备缓冲区 buf，并拼接当前的路径和斜杠
    strcpy(buf, path); // path传给buf .
    p = buf+strlen(buf);
    *p++ = '/'; // buf 变成了 "./"，且 p 指向 '/' 后面的空位
    while(read(fd, &de, sizeof(de)) == sizeof(de)){ // 读目录项
      if(de.inum == 0) // inode 为 0 代表这个文件条目是空的/已被删除
        continue;

    // 将子文件名拼接到缓冲区的斜杠后面
      memmove(p, de.name, DIRSIZ);  // 把读到的文件名a拼到 "./" 后面
      p[DIRSIZ] = 0;// 确保字符串以 null 结尾 现在的 buf 变成了 "./a"

      // . 为当前自己目录 .. 为父目录
      if(strcmp(de.name, ".")==0 || strcmp(de.name, "..") == 0) continue;
      // 递归调用
      find(buf, target);

    }
    break;
  }
  close(fd);
}


int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        fprintf(2, "Usage: find <path> <filename>\n");
        exit(1);
    }
    find(argv[1], argv[2]);

    exit(0);
}