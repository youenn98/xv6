#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int ac, char *argv[])
{
    if(ac != 2){
        fprintf(2,"Lack number.\n");
        exit(1);
    }
    int t = atoi(argv[1]);
    sleep(t);
    exit(0);
}