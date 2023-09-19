//This program must executed under sudo

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> // forr O_RDWR
#include <errno.h>
#include <termios.h>

int main(int argc, char *argv[]){
    //by using "sudo dmesg | tail" to check the USB device name and fill into the path variable
    char path[] = "/dev/sdb1";
    int serial_port = open(path, O_RDWR);

    if(serial_port < 0){ //fail to read device file
        printf("%s\n", strerror(errno));
        printf("Unable to open device on %s\n", path);
        return 1;
    }

    // Set the serial port configuration.
    struct termios serial_port_config;
    serial_port_config.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    tcsetattr(serial_port, TCSANOW, &serial_port_config);

    if(argc){ //if there's user input
        FILE *file = fopen(argv[1], "rb");
        if(!file){ //fail to open file
            printf("%s\n", strerror(errno));
            printf("Unable to open file %s\n", argv[1]);
        }

        fclose(file);
    }

    close(serial_port);

    return 0;
}
