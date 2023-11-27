#include <fcntl.h>
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/ioctl.h>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <stdint.h>
#include <unistd.h> // for sleep
#include <stdio.h> // for sprintf_s
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <pthread.h>
#include <signal.h>

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

static struct termios oldt, newt;
volatile int flag = 0; //volatile: ask compiler not optimize this parameter
volatile bool video_flag = false;

int x_offset,y_offset;

void *wait_input(void *arg);

void signal_handler(int signum) {
    std::cout << "signal_handler: caught signal " << signum << std::endl;
    if (signum == SIGINT) {
        std::cout << "SIGINT" << std::endl;
        video_flag = true;
    }
}


int main(int argc, const char *argv[])
{
    cv::Mat image;
    cv::Size2f image_size;
	
	
	//int count = 0; // for file name postfix
	char key_input;
	//char img_path[100];
	pthread_t pt1; // thread to get input

    	tcgetattr(STDIN_FILENO, &oldt); // save original STDIN attribute
   	newt = oldt;
    	newt.c_lflag &= ~(ICANON);	// get input without blocking by disable canonical mode
    	tcsetattr(STDIN_FILENO, TCSANOW, &newt); // set new STDIN attribute

        // get framebuffer info
    	framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
        // open framebuffer device
    	std::ofstream ofs("/dev/fb0");	
	// open video stream device(camera)
	//cv::VideoCapture camera (2+cv::CAP_DSHOW);

	if(!ofs){
		std::cout<<"can't open /dev/fb/"<<std::endl;
	}

	char img_path[] = "/root/lab4/picture.png";
	FILE *fp;
    	if (fp = fopen(img_path, "r")) {
        	fclose(fp);
        	std::cout<<"success" << std::endl;
    	}
	else
		std::cout<<"fail" << std::endl;	

        image = cv::imread(img_path, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);

        if(image.empty()){
                std::cout<<"image not found"<<std::endl;
		std::cout << img_path << std::endl;
                return 1;
        }

	image_size = image.size();

	cv::Mat shiftedImage;
	cv::Mat ini_image;
	shiftedImage = image.clone();
	ini_image = image.clone();
	//cvtColor(image, image, cv::COLOR_BGR2BGR565);

        int framebuffer_width = fb_info.xres_virtual;
        int framebuffer_depth = fb_info.bits_per_pixel;
	
	// Set the shift amount (positive for right shift, negative for left shift)
	int shiftAmount = 100; // Adjust this value as needed

	if(pthread_create(&pt1, NULL, wait_input, NULL)){ //start thread
		std::cout << "Error while pthread creation" << std::endl;
		return 1;
	}

	pthread_detach(pt1); //background execute pthread
	

	//cv::VideoWriter video("group27.avi",CV_FOURCC('M','J','P','G'),10, cv::Size(frame_width,frame_height),true); // mjpg format, fps=10

	//if (signal(SIGINT, signal_handler) == SIG_ERR)
        //	std::cout << "Failed to caught signal" << std::endl;
    	//std::cout << "before while" << std::endl;
	while (1){

		// Determine the direction of the shift
		//std::cout << "outer while" << std::endl;
		if (shiftAmount >= 0) {
			cv::copyMakeBorder(shiftedImage, shiftedImage, 0, 0, shiftAmount, 0, cv::BORDER_WRAP);
			shiftedImage = shiftedImage(cv::Rect(0, 0, ini_image.cols, ini_image.rows));
		} else {
			cv::copyMakeBorder(shiftedImage, shiftedImage, 0, 0, 0, -shiftAmount, cv::BORDER_WRAP);
			shiftedImage = shiftedImage(cv::Rect(abs(shiftAmount), 0, ini_image.cols, ini_image.rows));
		}
		//std::cout << "after shift" << std::endl;
		//shiftAmount = -100;                       
		cvtColor(shiftedImage, image, cv::COLOR_BGR2BGR565);

		usleep(100);

		// output to framebufer row by row
		for (int y = 0; y < image_size.height; y++) // image size will exceed screen resolution, will this be a problem?
		{
			// move to the next written position of output device framebuffer by "std::ostream::seekp()".
			// posisiotn can be calcluated by "y", "fb_info.xres_virtual", and "fb_info.bits_per_pixel".
			// http://www.cplusplus.com/reference/ostream/ostream/seekp/

			ofs.seekp(y*framebuffer_width*2);

			// write to the framebuffer by "std::ostream::write()".
			// you could use "cv::Mat::ptr()" to get the pointer of the corresponding row.
			// you also have to count how many bytes to write to the buffer
			// http://www.cplusplus.com/reference/ostream/ostream/write/
			// https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a13acd320291229615ef15f96ff1ff738


			ofs.write(reinterpret_cast<char*>(image.ptr(y)), image_size.width*2);

		}	
		if(flag){ // save screenshot after input
			if(flag == 2)
				shiftAmount = 100;
			else if(flag == 1)
				shiftAmount = -100;
			flag = 0; //release flag
		}
		
	}
	
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //set to default attribute

    return 0;
}

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path)
{
    struct framebuffer_info fb_info;        // Used to return the required attrs.
    struct fb_var_screeninfo screen_info;   // Used to get attributes of the device from OS kernel.

    // open device with linux system call "open()"
    // https://man7.org/linux/man-pages/man2/open.2.html

        int fd = -1;
        fd = open(framebuffer_device_path, O_RDWR);

    // get attributes of the framebuffer device thorugh linux system call "ioctl()".
    // the command you would need is "FBIOGET_VSCREENINFO"
    // https://man7.org/linux/man-pages/man2/ioctl.2.html
    // https://www.kernel.org/doc/Documentation/fb/api.txt

    // put the required attributes in variable "fb_info" you found with "ioctl() and return it."
    // fb_info.xres_virtual =       // 8
    // fb_info.bits_per_pixel =     // 16

        if(fd >= 0){

                if(!ioctl(fd, FBIOGET_VSCREENINFO, &screen_info)){
                        fb_info.xres_virtual = screen_info.xres_virtual;
                        fb_info.bits_per_pixel = screen_info.bits_per_pixel;
                }
		printf("%dx%d, %dbpp\n", screen_info.xres, screen_info.yres, screen_info.bits_per_pixel);
		//y_offset = screen_info.xres * screen_info.yres / 2;
		//x_offset = screen_info.xres;
        }

    return fb_info;
};

void *wait_input(void *arg){
    int input;

    while(1){
	if(!flag){
		input = getchar();
		if(input == 'j'){ // shift left
		    //std::cout << "wait" << std::endl;
			flag = 1;
		}
		else if(input == 'l'){ // shift right
		    //std::cout << "wait" << std::endl;
			flag = 2;
		}
	}
    }
}
