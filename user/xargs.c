#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int ac,char *av[]){

    char buf[1024];
    char *argv[MAXARG+1];
    if(ac < 2){
        fprintf(2,"No command found.\n");
        exit(1);
    }

    for(int i = 1; i < ac;i++){
        argv[i-1] = av[i];
    }

    while(gets(buf,MAXARG)){
        if(buf[0] == '\0') exit(0);
        if(fork() == 0){
            int i,bef,aft;
            i = ac -1;
            for(aft = 0,bef = 0;;aft++){
                if(buf[aft] == ' ' || buf[aft] == '\0' || buf[aft] == '\n'){
                    
                    if(aft - bef > 0){
                        argv[i] = (char *)malloc(aft - bef + 1);
                        int k = 0;
                        for(int j = bef;j < aft;){
                            argv[i][k++] = buf[j++];
                        }
                        argv[i][k] = '\0';
                        i++;
                    }
                    bef = aft + 1;
                }
                if(buf[aft] == '\0') {
                    argv[i] = 0;
                    break;
                }
            }
            exec(argv[0],argv);

        }else{
            wait(0);
        }
    }
    exit(0);
    return 0;
}