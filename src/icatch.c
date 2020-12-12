#include "defs.h"
void icatch_add(struct inode* ip){
    struct inode_node* inp = (struct inode_node*)malloc(sizeof(struct inode_node));
    inp->buf = ip;
    inp->next = icatch_list;
    icatch_list = inp;
}

void icatch_refresh(){
    struct inode_node* inp = icatch_list;
    struct inode_node* inp_next = 0;
    struct inode_node* inp_cwd = 0;
    while(inp){
        inp_next = inp->next;
        iupdate(inp->buf);

        if(inp->buf != cwd_node){
            free(inp->buf);
            free(inp);
        }else{
            inp_cwd = inp;
        }
        inp = inp_next;
    }
    icatch_list = inp_cwd;
    if(icatch_list){
        icatch_list->next = 0;
    } 
    iupdate(&root_node);
}

void icatch_free(){
    struct inode_node* inp = icatch_list;
    struct inode_node* inp_next = 0;
    while(inp){
        inp_next = inp->next;
        iupdate(inp->buf);
        free(inp->buf);
        free(inp);
        inp = inp_next;
    }
    iupdate(&root_node);
    icatch_list = 0;
}