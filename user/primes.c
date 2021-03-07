#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_NUM 35

int main(){
    int pipes[30];
    int cnt = 0,pid;
    pipe(pipes);
    int num;

    while( 1 ){        
        if((pid = fork()) == 0){
            //close the old pipe's write fd.
            close(pipes[cnt*2+1]);
            //read one prime from old pipe, if there is none exit.
            int rd;
            if(!read(pipes[cnt * 2],&num,sizeof(int))){
                close(pipes[cnt * 2]);
                exit(0);
            }
            printf("prime %d\n",num);
            //create a new pipe
            pipe(&pipes[2*(++cnt)]);

            int k;
            while((rd = read(pipes[cnt*2-2],&k,sizeof(int)))){
                if(k%num){
                    write(pipes[cnt*2+1],&k,sizeof(int));
                }
            }
            //close old pipe's read fd and new pipes write fd.
            close(pipes[cnt*2-2]);
            close(pipes[cnt*2+1]);
            //fork
        }else{
            //feeds data at root
            if(cnt == 0){
                for(int i = 2; i <= MAX_NUM;i++){
                    write(pipes[cnt*2+1],&i,sizeof(int));
                }
                //close new pipe's write fd.
                close(pipes[cnt * 2 + 1]);
            }
            wait(0);
            exit(0);
        }
    }
    return 0;
}