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

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char *framebuffer_device_path);

static struct termios oldt, newt;
volatile bool flag = false; //volatile: ask compiler not optimize this parameter

void *wait_input(void *arg);

int main(int argc, const char *argv[])
{
    cv::Mat frame;
    cv::Size2f frame_size;
	
	
	int count = 0; // for file name postfix
	char key_input;
	char img_path[100];
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
	cv::VideoCapture camera ( 2 );

	if(!ofs){
		std::cout<<"can't open /dev/fb/"<<std::endl;
	}
	if(!camera.isOpened() ){
		std::cout << "Can't open camera." << std::endl;
		return 1;
	}

	camera.set(cv::CAP_PROP_FRAME_WIDTH, fb_info.xres_virtual);
	//camera.set(cv::CAP_PROP_FRAME_WIDTH, 800);
	//camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // read image file (sample.bmp) from opencv libs.
    // https://docs.opencv.org/3.4.7/d4/da8/group__imgcodecs.html#ga288b8b3da0892bd651fce07b3bbd3a56
    // image = .......
	/*FILE *fp;
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
        }*/

	if(pthread_create(&pt1, NULL, wait_input, NULL)){ //start thread
		std::cout << "Error while pthread creation" << std::endl;
		return 1;
	}

	pthread_detach(pt1); //background execute pthread

	while (1){
		// get video frame from stream
		// https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a473055e77dd7faa4d26d686226b292c1
		// https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a199844fb74226a28b3ce3a39d1ff6765
		// frame = ...... // ?
		bool check = camera.read(frame); // cap >> frame;
		if (!check){
		    std::cout << "Cannot read frame!" << std::endl;
		}

		// get size of the video frame
		// https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a146f8e8dda07d1365a575ab83d9828d1
		// frame_size = ......
		frame_size = frame.size();		

		// transfer color space from BGR to BGR565 (16-bit image) to fit the requirement of the LCD
		// https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga397ae87e1288a81d2363b61574eb8cab
		// https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga4e0972be5de079fed4e3a10e24ef5ef0                                 
		cvtColor(frame, frame, cv::COLOR_BGR2BGR565);
		int framebuffer_width = fb_info.xres_virtual;
        	int framebuffer_depth = fb_info.bits_per_pixel;

		usleep(5000);

		// output to framebufer row by row
	    for (int y = 0; y < frame_size.height; y++)
	    {
		// move to the next written position of output device framebuffer by "std::ostream::seekp()".
		// posisiotn can be calcluated by "y", "fb_info.xres_virtual", and "fb_info.bits_per_pixel".
		// http://www.cplusplus.com/reference/ostream/ostream/seekp/

			ofs.seekp(y*framebuffer_width*2 + 150);

		// write to the framebuffer by "std::ostream::write()".
		// you could use "cv::Mat::ptr()" to get the pointer of the corresponding row.
		// you also have to count how many bytes to write to the buffer
		// http://www.cplusplus.com/reference/ostream/ostream/write/
		// https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a13acd320291229615ef15f96ff1ff738


			ofs.write(reinterpret_cast<char*>(frame.ptr(y)), frame_size.width*2);

	    }

		//key_input = cv::waitKey(30);
		/*if((cv::waitKey(50) & 0xFF) == 'c'){
			cv::waitKey(10);
			sprintf(img_path, "/root/lab3/screenshot/screenshot%d.png", count);
			cv::imwrite(img_path, frame);
			std::cout << count << std::endl;
			count++;
		}*/
		//else
		//	std::cout << key_input << std::endl;	
		if(flag){ // save screenshot after input
			sprintf(img_path, "/root/lab3/screenshot/screenshot%d.png", count);
			cv::imwrite(img_path, frame);
			std::cout << count++ << std::endl;
			flag = false; //release flag
		}
	}
	// close camera
	camera.release();
	
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
        }

    return fb_info;
};

void *wait_input(void *arg){
    int input;

    while(1){
	if(!flag){
		input = getchar();
		if(input == 'c'){
		    //std::cout << count++ << std::endl;
			flag = true;
		}
	}
    }
}
