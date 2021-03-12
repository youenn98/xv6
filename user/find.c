#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAX_LEN 256

int unvalid_dir(char name[]){
    if(strcmp(name,".") == 0 || strcmp(name,"..") == 0 || strlen(name) == 0) return 1;
    return 0;
}

int open_f(char path[],struct stat * st){
    int fd;
    if((fd = open(path,0)) < 0){
        fprintf(2,"cannot open %s\n",path);
        return -1;
    }
    if(fstat(fd,st) < 0){
        fprintf(2,"cannot stat %s\n",path);
        return -1;
    }
    return fd;
}

void read_dir(char dirpath[],char filename[]){
    struct dirent de;
    struct stat   st; 
    int fd;
    char buf[MAX_LEN],*p;

    fd = open_f(dirpath,&st);

    if(st.type != T_DIR){
        fprintf(2,"%s is not a directory\n");
        exit(3);
    }

    strcpy(buf,dirpath);
    p = buf + strlen(buf);
    *p++ = '/';

    while(read(fd,&de,sizeof(de)) == sizeof(de)){
        if(unvalid_dir(de.name)) continue;
        //printf("de_name:%s\n",de.name);
        strcpy(p,de.name);
        int new_fd = open_f(buf,&st);
        if(st.type == T_DIR){
            close(new_fd);
            read_dir(buf,filename);
        }else{
            if(strcmp(de.name,filename) == 0){
                printf("%s\n",buf);
            }
            close(new_fd);
        }
    }
    close(fd);
}


int main(int ac,char *argv[]){
    if(ac != 3){
        fprintf(3,"use find <dirpath> <filename>\n");
        exit(1);
    }
    read_dir(argv[1],argv[2]);
    exit(0);
    return 0;
}