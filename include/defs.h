#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include <unistd.h>

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#define T_FILE 0x1
#define T_DIR 0x2
#define MAX_INODE 1024
#define MAX_BLOCK 4096
#define INODE_SIZE 32
#define DBLOCK_SIZE 512
#define BLOCK_SIZE (2 * DBLOCK_SIZE)
#define INODE_ADDR_LENGH 6
#define DIRSIZ 121
#define MAXARGS 10

struct super_block {    // 其中的所有的block都指的是1024大小的
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    uint32_t first_inode_block;         // 第一个inode所在块的块号
    uint32_t first_data_block;          // 第一个data block块的块号
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
};

struct inode {
    uint32_t size;                  // 文件大小
    uint8_t file_type;              // 文件类型（文件/文件夹）
    uint8_t link;                   // 连接数
    uint16_t inode_id;                  // inode号
    uint32_t block_point[INODE_ADDR_LENGH];    // 数据块指针
};

struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
};

// // 磁盘块, 大小为512
// struct dblock {
//     uint32_t dblock_no;
//     char data[DEVICE_BLOCK_SIZE];
// };

# define INODE_CATCH_SIZE 4

struct inode root_node;
struct inode* cwd_node;  //当前路径
struct inode_node{
    struct inode* buf;
    struct inode_node* next;
};
struct inode_node* icatch_list;

// fs.c
void bit_set(char* buf, int index);
int bit_isset(char* buf, int index);
int fs_init();
int fs_load();
int fs_close();
struct inode* create_inode(uint16_t file_type);
int block_alloc();
int block_init(int block_no);
static struct inode* namex(char *path, int nameiparent, char *name);
struct inode* namei(char *path);
struct inode* nameiparent(char *path, char *name);
static char* skipelem(char *path, char *name);
struct inode* dirlookup(struct inode *dp, char *name, int *poff);
struct inode* inode_get(uint32_t inode_num);
int readi(struct inode* ip, char* addr, int offset, int len);
int writei(struct inode* ip, char* src, int offset, int len);
int dirlink(struct inode *dp, char *name, int inum, uint8_t file_type);
int iupdate(struct inode* ip);
struct inode* create_in_path(char* path, short type);
void inode_print(struct inode* ip);
void dir_item_print(struct dir_item* de);
void change_cwd(char* path);

//syscall.c
int mkdir(char* path);
int touch(char* path);
int ls(char* path);
int cp(char* target_path, char* dest_path);
int shutdown();
int cd(char* path);
int tee(char* path);
int cat(char* path);

//sh.c
int getcmd(char *buf, int nbuf);
int shell();
char* gets(char *buf, int max);

//icatch.c
void icatch_add(struct inode* ip);
void icatch_free();
void icatch_refresh();