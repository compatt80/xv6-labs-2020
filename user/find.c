#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path) // 找到最后一个斜杠后的第一个字符
{
    char *p;

    // Find first character after last slash. 
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    return p;
}

void
find(char *path, char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
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
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;

    // 将子文件名拼接到缓冲区的斜杠后面
      memmove(p, de.name, DIRSIZ); // 
      p[DIRSIZ] = 0;// 确保字符串以 null 结尾 现在的 buf 变成了 "a/b"

      if(strcmp(de.name, ".")==0 || strcmp(de.name, "..") == 0) continue;

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