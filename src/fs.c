#include "defs.h"
#define MAGICNUM 0x112233

// 规定: 根目录文件夹文件的inode号为0
// 规定: 查找inode传入的path是绝对路径
// 规定: 将根目录的inode常驻内存
struct super_block* sb;

struct inode_catch icatch;

void
bit_set(char* buf, int index){
    int idx = index / 8;
    buf[idx] = buf[idx] | (1 << (index % 8));
}
// 返回0代表没set, 返回1代表set
int
bit_isset(char* buf, int index){
    int idx = index / 8;
    return buf[idx] & (1 << (index % 8));
}

int
fs_init(){
    char* buf = (char*)malloc(2*DEVICE_BLOCK_SIZE);
    memset(buf,0,2*DEVICE_BLOCK_SIZE);
    sb = (struct super_block*)buf;
    sb->magic_num = MAGICNUM;
    sb->free_block_count = 128 * 32 - 1 - sizeof(struct inode) * 32 * 32 / DEVICE_BLOCK_SIZE / 2; // 除去超级块和inode占用的块还剩4096 - 1 - 32 = 4063
    //sb->free_inode_count = 32 * 32 - 1;
    //sb->dir_inode_count = 1;
    sb->first_inode_block = 1;
    sb->first_data_block = 33;
    for(int i=0; i < 33; i++){
        bit_set((char*)sb->block_map, i);
        block_init(i);
    }
    if(create_inode(T_DIR) != 0){
        printf("error: 创建根目录inode失败\n");
        return -1;
    }
    disk_write_block(0, buf);
    disk_write_block(1, buf+DEVICE_BLOCK_SIZE);
    free(buf);

    memset(icatch.valid, 0, sizeof(icatch.valid));
    return 0;
}

int
fs_load(){
    char* buf = (char*)malloc(2*DEVICE_BLOCK_SIZE);
    disk_read_block(0,buf);
    disk_read_block(1,buf+DEVICE_BLOCK_SIZE);
    sb = (struct super_block*)buf;
    if(sb->magic_num != MAGICNUM){
        fs_init();
    }

    disk_read_block(0,buf);
    disk_read_block(1,buf+DEVICE_BLOCK_SIZE);

    char temp[DBLOCK_SIZE];
    disk_read_block(sb->first_inode_block*2,temp);
    struct inode* inodes = (struct inode*)temp;
    icatch.root_node = inodes[0];

    return 0;
}


// 创建inode, 返回文件的inode号
int
create_inode(uint16_t file_type){
    if(sb->free_block_count < 6){
        printf("error: no more free blocks\n");
        return -1;
    }

    int inode_no = -1;
    for(int i=0; i<MAX_INODE; i++){
        if(!bit_isset((char*)sb->inode_map, i)){
            bit_set((char*)sb->inode_map, i);
            inode_no = i;
            break;
        }
    }

    
    // 找到该inode号的inode结构体所在的dblock的块号dblock_no
    int dblock_no = inode_no / (DBLOCK_SIZE / sizeof(struct inode)) + sb->first_inode_block * 2;

    char* buf = (char*)malloc(DEVICE_BLOCK_SIZE);
    disk_read_block(dblock_no, buf);
    struct inode* inodes = (struct inode*)buf;

    int dblock_offset = inode_no % (DBLOCK_SIZE / sizeof(struct inode));    // 磁盘块内的inode号偏移
    inodes[dblock_offset].file_type = file_type;
    inodes[dblock_offset].link = 1;
    inodes[dblock_offset].size = 0;   // 分配block, 但是不计入文件的size
    inodes[dblock_offset].inode_id = inode_no;
    for(int i=0; i<INODE_ADDR_LENGH; i++){
        inodes[dblock_offset].block_point[i] = block_alloc();
    }

    sb->free_block_count -= 6;
    disk_write_block(dblock_no, buf);
    free(buf);
    return inode_no;
}
// 在磁盘中寻找空闲data block, 返回block号
int
block_alloc(){
    for(int i=0; i<MAX_BLOCK; i++){
        if(!bit_isset((char*)sb->block_map, i)){
            bit_set((char*)sb->block_map, i);
            block_init(i);
            return i;
        }
    }
}
// 将给定block_no的size = 1024的块初始化全0
int
block_init(int block_no){
  char* buf = (char*)malloc(DEVICE_BLOCK_SIZE);
  memset(buf,0,DEVICE_BLOCK_SIZE);
  disk_write_block(2*block_no, buf);
  disk_write_block(2*block_no+1, buf);
  free(buf);
}

// 从给定path中获取该文件的inode或者其父文件夹的inode, 并将该文件的文件名复制到name中
// nameiparent=0则读取到该inode, nameiparent=1则读取文件的父文件夹inode
// 暂时限制path为绝对路径
static struct inode*
namex(char *path, int nameiparent, char *name)
{
  struct inode *ip, *next;
  
  ip = &icatch.root_node;
  while((path = skipelem(path, name)) != 0){
    if(ip->file_type != T_DIR){
      return 0;
    }
    if(nameiparent && *path == '\0'){
      // Stop one level early.
      return ip;
    }
    if((next = dirlookup(ip, name, 0)) == 0){
      return 0;
    }
    ip = next;
  }
  if(nameiparent){
    return 0;
  }
  return ip;
}

struct inode*
namei(char *path)
{
  char name[DIRSIZ];
  return namex(path, 0, name);
}

struct inode*
nameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}

static char*
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while(*path != '/' && *path != 0)
    path++;
  len = path - s;
  if(len >= DIRSIZ)
    memmove(name, s, DIRSIZ);
  else {
    memmove(name, s, len);
    name[len] = 0;
  }
  while(*path == '/')
    path++;
  return path;
}

struct inode*
dirlookup(struct inode *dp, char *name, int *poff)
{
  int off, inum;
  struct dir_item de;

  if(dp->file_type != T_DIR){
    printf("error: dirlookup必须是T_DIR类型的inode\n");
    return 0;
  }

  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)){
      printf("error: 从inode中读取数据失败\n");
      return 0;
    }
    if(de.inode_id == 0)
      continue;
    if(strncmp(name, de.name, DIRSIZ) == 0){
      // entry matches path element
      if(poff)
        *poff = off;
      inum = de.inode_id;
      return inode_get(inum);
    }
  }

  return 0;
}

int
strncmp(const char *p, const char *q, int n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (unsigned char)*p - (unsigned char)*q;
}

struct inode*
inode_get(uint32_t inode_num){
  int dblock_no = inode_num/(DBLOCK_SIZE / sizeof(struct inode)) + (sb->first_inode_block)*2;
  char* buf = (char*)malloc(DEVICE_BLOCK_SIZE);
  disk_read_block(dblock_no, buf);
  struct inode* inodes = (struct inode*)buf;

  int dblock_offset = inode_num % (DBLOCK_SIZE / sizeof(struct inode)); // inode在磁盘块内的数组下标

  struct inode* inode_new = &(icatch.inodes[icatch.tail % INODE_CATCH_SIZE]);
  if(icatch.valid[(icatch.tail) % INODE_CATCH_SIZE]){ // icatch中将已存在的inode写回磁盘
    iupdate(&icatch.valid[(icatch.tail) % INODE_CATCH_SIZE]);
  }

  icatch.inodes[(icatch.tail) % INODE_CATCH_SIZE] = inodes[dblock_offset];
  icatch.valid[(icatch.tail) % INODE_CATCH_SIZE] = 1;
  icatch.tail = (icatch.tail+1)% INODE_CATCH_SIZE;
  
  free(buf);
  return inode_new;
}

// 从给定inode文件的第off字节数据开始, 读取len字节到地址addr中
int
readi(struct inode* ip, char* addr, int offset, int len){
  int addr_idx = offset / BLOCK_SIZE; // 首先获取offset所在inode中addrs[]数组的下标
  int dblock_num = ip->block_point[addr_idx]*2 + (offset % BLOCK_SIZE) / DBLOCK_SIZE; // 数据起点所在磁盘块DBLOCK的块号
  int dblock_offset = offset % DEVICE_BLOCK_SIZE; // 数据起点所在磁盘块DBLOCK的offset
  
  int end_offset = offset + len;
  
  char* buf = (char*)malloc(DEVICE_BLOCK_SIZE);
  int read_size_sum = 0;

  while(offset < end_offset){
    int read_size = min(DEVICE_BLOCK_SIZE - dblock_offset, end_offset - offset);
    disk_read_block(dblock_num, buf);
    memmove(addr, buf+dblock_offset, read_size);

    offset += read_size;
    addr += read_size;
    dblock_num++;

    addr_idx = offset / BLOCK_SIZE; // 首先获取offset所在inode中addrs[]数组的下标
    dblock_num = ip->block_point[addr_idx]*2 + (offset % BLOCK_SIZE) / DBLOCK_SIZE; // 数据起点所在磁盘块DBLOCK的块号
    dblock_offset = offset % DEVICE_BLOCK_SIZE; // 数据起点所在磁盘块DBLOCK的offset
    read_size_sum += read_size;
  }
  free(buf);
  return read_size_sum; 
}

int
writei(struct inode* ip, char* src, int offset, int len){
  int tot, m;
  struct buf *bp;

  if(offset > ip->size || offset + len < offset)
    return -1;
  if(offset + len > INODE_ADDR_LENGH * BLOCK_SIZE)
    return -1;
  char* buf = (char*)malloc(DBLOCK_SIZE);
  for(tot=0; tot<len; tot+=m, offset+=m, src+=m){
    int dblock_no = ip->block_point[offset/BLOCK_SIZE]*2 + (offset%BLOCK_SIZE)/DBLOCK_SIZE;
    disk_read_block(dblock_no,buf);
    m = min(len - tot, DBLOCK_SIZE - offset%DBLOCK_SIZE);
    memmove(buf+offset%DBLOCK_SIZE, src, m);
    disk_write_block(dblock_no,buf);
  }

  if(len > 0 && offset > ip->size){
    ip->size = offset;
  }
  return len;
}

int
dirlink(struct inode *dp, char *name, int inum)
{
  int off;
  struct dir_item de;
  struct inode *ip;
  if(dp->file_type != T_DIR){
    printf("error: 传入目录inode错误\n");
    return -1;
  }

  // Check that name is not present.
  if((ip = dirlookup(dp, name, 0)) != 0){
    printf("error: 目录中不含需要的文件\n");
    return -1;
  }

  // Look for an empty dirent.
  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)){
      printf("error: 从目录中读取文件失败\n");
    }
    if(de.inode_id == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inode_id = inum;
  de.valid = 1;
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de)){
    printf("error: 向目录写入文件失败\n");
  }
  return 0;
}

int iupdate(struct inode* ip){
  int dblock_no = (ip->inode_id / (DBLOCK_SIZE/sizeof(struct inode))) + sb->first_inode_block*2;
  int dblock_offset = ip->inode_id % (DBLOCK_SIZE/sizeof(struct inode));
  char buf[DBLOCK_SIZE];
  disk_read_block(dblock_no, buf);
  struct inode* inodes = (struct inode*)buf;
  inodes[dblock_offset] = *ip;
  disk_write_block(dblock_no, buf);
  return 0;
}

int fs_close(){
  for(int i=0; i<INODE_CATCH_SIZE; i++){
    if(icatch.valid[i]){
      iupdate(&icatch.inodes[i]);
    }
  }
  iupdate(&icatch.root_node);
  return 0;
}
// 在指定路径上创建给定类型的文件
struct inode*
create_in_path(char *path, short type)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0){
    printf("error: 给定路径的上层文件夹打开失败\n");
    return 0;
  }


  if((ip = dirlookup(dp, name, 0)) != 0){
    return ip;
  }

  if((ip = create_inode(type)) == 0){
    printf("error: 创建inode失败\n");
    return 0;
  }

  if(type == T_DIR){  // Create . and .. entries.
    dp->link++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inode_id) < 0 || dirlink(ip, "..", dp->inode_id) < 0){
      printf("error: 在目录中创建.与..失败\n");
      return 0;
    }
  }

  if(dirlink(dp, name, ip->inode_id) < 0){
    printf("error: 在父目录中创建子文件失败\n");
    return 0;
  }

  return ip;
}