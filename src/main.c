#include "defs.h"

int main(){
    open_disk();
    fs_load();
    shell();
    close_disk();
}