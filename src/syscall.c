#include "defs.h"

int mkdir(char* path){
    create_in_path(path, T_DIR);
    return 0;
}

int touch(char* path){
    create_in_path(path, T_FILE);
    return 0;
}

int ls(char* path){
    struct inode* ip = namei(path);
    if(ip->inode_id ==0 || ip->file_type != T_DIR){
        printf("error: 文件夹不存在或路径名代表的不是文件夹\n");
        return 0;
    }

    int offset;
    struct dir_item de;
    for(offset=0; offset < 6*BLOCK_SIZE; offset += sizeof(struct dir_item)){
        readi(ip, (char*)&de, offset, sizeof(struct dir_item));
        if(de.valid != 0){
            printf("%s\n",de.name);
        }
    }
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
    return 0;
}

int shutdown(){
    fs_close();
    exit(0);
}