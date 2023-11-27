#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <unistd.h> 

using namespace std;

static struct termios oldt, newt;
volatile bool flag = false;
int count = 0;

void *wait_input(void *arg){
    int input;

    while(1){
	if(!flag){
		input = getchar();
		if(input == 'c'){
		    cout << count++ << endl;
			flag = true;
		}
	}
    }
}

int main(){
    pthread_t pt1;

    	tcgetattr(STDIN_FILENO, &oldt); // save original STDIN attribute
	newt = oldt;
	newt.c_lflag &= ~(ICANON);	// get input without blocking by disable canonical mode
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); // set new STDIN attribute

    if(pthread_create(&pt1, NULL, wait_input, NULL)){
        cout << "Error while pthread creation" << endl;
        return 1;
    }
    pthread_detach(pt1);

    while(1){
        cout << "hey" << endl;
	if(flag){
		cout << "reset flag" << endl;
		flag = false;
	}
	sleep(1);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return 0;
} 
