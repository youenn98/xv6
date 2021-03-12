#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(){
    int pipes[4];
    pipe(pipes);
    pipe(&pipes[2]);
    int p_id;
    char buff[2];
    if(fork() == 0){
        close(pipes[1]);
        close(pipes[2]);

        p_id = getpid();
        //wait to read successfully from pipe
        while(read(pipes[0],buff,1) <= 0);
        close(pipes[0]);
        printf("%d: received ping\n",p_id);
        //send parent a byte
        write(pipes[3],"a",1);
        close(pipes[3]);
        exit(0);
    }else{
        close(pipes[0]);
        close(pipes[3]);
        p_id = getpid();
        //send child a byte
        write(pipes[1],"b",1);
        //wait to read from child
        while(read(pipes[2],&buff[1],1) <= 0);
        printf("%d: received pong\n",p_id);
        exit(0);
    }

}