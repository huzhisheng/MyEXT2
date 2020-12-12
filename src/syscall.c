#include "defs.h"

int mkdir(char* path){
    create_in_path(path, T_DIR);
    icatch_refresh();
    return 0;
}

int touch(char* path){
    create_in_path(path, T_FILE);
    icatch_refresh();
    return 0;
}

int ls(char* path){
    struct inode* ip = namei(path);
    if(!ip || ip->file_type ==0 || ip->file_type != T_DIR){
        printf("error: 文件夹不存在或路径名代表的不是文件夹\n");
        return 0;
    }
    int offset;
    struct dir_item de;
    for(offset=0; offset < 6*BLOCK_SIZE; offset += sizeof(struct dir_item)){
        readi(ip, (char*)&de, offset, sizeof(struct dir_item));
        if(de.valid != 0){
            if(de.type == T_DIR)
                printf("%s\tDIR \n", de.name);
            else
                printf("%s\tFILE\n", de.name);
        }
    }
    icatch_refresh();
}

// cp命令将target_path的文件复制其内容到dest_path中
int cp(char* target_path, char* dest_path){
    struct inode* ip = namei(target_path);
    struct inode* dp = create_in_path(dest_path, ip->file_type);

    int offset;
    char buf[DBLOCK_SIZE];
    for(offset=0; offset < 6*BLOCK_SIZE; offset += DBLOCK_SIZE){
        readi(ip, buf, offset, DBLOCK_SIZE);
        writei(dp, buf, offset, DBLOCK_SIZE);
    }
    dp->size = ip->size;
    icatch_refresh();
    struct inode* test = namei(dest_path);
    if(!test){
        printf("cp中创建的inode不存在\n");
    }
    return 0;
}

int shutdown(){
    fs_close();
    return 0;
}

int cd(char* path){
    change_cwd(path);
    return 0;
}

int get_text(char *buf, int nbuf)
{
	printf("-> ");
    fflush(stdout);
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
		return -1;
	return 0;
}

int tee(char* path){
    struct inode* ip = namei(path);
    if(!ip || ip->file_type != T_FILE){
        printf("error: 文件夹不存在或路径名代表的不是文件\n");
        return 0;
    }
    printf("info: 输入quit即可退出编辑\n");
    char buf[100];
    int text_len;
    while (get_text(buf, sizeof(buf)) >= 0){
        text_len = strlen(buf);
        if(strcmp(buf, "quit\n") == 0)
            break;
        if(ip->size + text_len > BLOCK_SIZE * 6){
            printf("error: 超出文件容量\n");
            break;
        }
        if(writei(ip, buf, ip->size, text_len)<0)
            break;
        // printf("读取到的text长度%d",text_len);
        // printf("大小%d\n",ip->size);
    }
    icatch_refresh();
    return 0;
}

int cat(char* path){
    struct inode* ip = namei(path);
    if(!ip || ip->file_type != T_FILE){
        printf("error: 文件夹不存在或路径名代表的不是文件\n");
        return 0;
    }
    char buf[DBLOCK_SIZE];
    int m;
    for(int i=0; i<ip->size; i+=m){
        m = min(DBLOCK_SIZE, ip->size - i);
        if(readi(ip, buf, i, m) != m){
            printf("error: 读取文件出错\n");
            break;
        }
        printf("%s",buf);
    }
    fflush(stdout);
    icatch_refresh();
    return 0;
}
