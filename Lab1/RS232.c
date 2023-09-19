#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


int main(){
    char path[] = "/dev/sdb1";

    int serial_port = open(path, O_RDWR);
    if(serial_port < 0){
        printf("Unable to open device on %s\n", path);
        return 1;
    }

    return 0;
}
