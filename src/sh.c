#include "defs.h"

char* gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int getcmd(char *buf, int nbuf)		//从sh.c中借鉴的函数，用来获取输入
{
	printf("@ ");
    fflush(stdout);
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
		return -1;
	return 0;
}

int shell(){
    char buf[100];
    while (getcmd(buf, sizeof(buf)) >= 0){
        if(buf[0] == '\n')
            continue;
		int str_len = strlen(buf);
		buf[str_len - 1] = 0;
        char *args[MAXARGS];
        int arg_num = 0;
        char *p = buf;
        // 获取输入, 根据空格将其中的参数字符串切分出来
        while (*p)
        {
            while (*p && *p == ' ')
            {
                *p = 0;
                p++;
            }
            if (*p)
            {
                args[arg_num++] = p;
            }
            while (*p && *p != ' ')
            {
                if(*p == '\n'){
                    *p = 0;
                }
                p++;
            }
        }
        args[arg_num] = 0;

        if(strcmp(args[0],"mkdir") == 0){
            mkdir(args[1]);
        }else if(strcmp(args[0],"touch") == 0){
            touch(args[1]);
        }else if(strcmp(args[0],"cp") == 0){
            cp(args[1], args[2]);
        }else if(strcmp(args[0],"ls") == 0){
            ls(args[1]);
        }else if(strcmp(args[0],"shutdown") == 0){
            shutdown();
            return 0;
        }else if(strcmp(args[0],"cd") == 0){
            cd(args[1]);
        }else if(strcmp(args[0],"cat") == 0){
            cat(args[1]);
        }else if(strcmp(args[0],"tee") == 0){
            tee(args[1]);
        }else{
            printf("error: 不支持的命令\n");
        }
    }
}