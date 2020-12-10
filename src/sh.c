#include "defs.h"

int getcmd(char *buf, int nbuf)		//从sh.c中借鉴的函数，用来获取输入
{
	printf("@ ");
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
		return -1;
	return 0;
}

int shell(){
    char buf[100];
    while (getcmd(buf, sizeof(buf)) >= 0){
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
        }else{
            printf("error: 不支持的命令\n");
        }
    }
}