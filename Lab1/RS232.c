//This program must executed under sudo

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // forr O_RDWR
#include <errno.h>
#include <termios.h>

#define BAUDRATE B115200

int main(int argc, char *argv[]){
    //by using "sudo dmesg | tail" to check the USB device name and fill into the path variable
    char path[] = "/dev/sdb1";
    int serial_port = open(path, O_RDWR); //open file descripter with READ WRITE permission

    if(serial_port < 0){ //fail to read device file
        printf("%s\n", strerror(errno)); //POSIX didn't mandate to define error return value in kernel space, so it would be always -1. Using errno could get correct error info
        printf("Unable to open device on %s\n", path);
        return 1;
    }

    if(!isatty(serial_port)){
        printf("The file descripter opened is not a TTY device, EXIT\n");
        return 1;
    }

    // Set the serial port configuration.
    struct termios serial_port_config; //describe a general terminal interface that is provided to control asynchronous communications ports.
    if(tcgetattr(serial_port, &serial_port_config) < 0)
        printf("Warning: Initial configurarion error\n");
    

    serial_port_config.c_cflag &= (BAUDRATE | CS8 | CLOCAL | CREAD); //set to control modes, baud rate = 115200, CSIZE = 8, local connection, enable readu=ing
    tcsetattr(serial_port, TCSANOW, &serial_port_config);

    if(argc){ //if there's user input
        char buf[1024];
        int bytes_read;
        FILE *file;

        if(argv[1] == "R"){ //READ from PC and write to RS232
            file = fopen(argv[2], "rb");
            if(!file){ //fail to open file
                printf("%s\n", strerror(errno));
                printf("Unable to open file %s\n", argv[1]);
                return 1;
            }

            while(bytes_read = fread(buf, 1, sizeof(buf), file))
                write(serial_port, buf, bytes_read);
        }
        else if(argv[1] == "W"){ //READ from RS232 and write to PC
            file = fopen(argv[2], "wb");
            if(!file){ //fail to open file
                printf("%s\n", strerror(errno));
                printf("Unable to open file %s\n", argv[1]);
                return 1;
            }

            while(bytes_read = read(serial_port, buf, sizeof(buf)))
                fwrite(buf, 1, bytes_read, file);
        }

        fclose(file);
    }

    close(serial_port);

    return 0;
}
