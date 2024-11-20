/*
TARGET:Raspberry Pi B 3
Copyright(c)2019, epicgram Co.,LTD
PROJECT: Egg Incubator kiosk5
REVISION:V1.0
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
//#include <wiringPi.h>
//#include <wiringSerial.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <stdarg.h>
#include <stdint.h>
#include <dirent.h>
#include <assert.h>
#include <sys/mman.h>
//#include <sys/types.h>
#include <sys/stat.h>

/*External library*/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

/*Audio*/
#include <pthread.h>
//#include <bcm2835.h>

#include <pigpio.h>

int fd,sock,sockfd;
struct sockaddr_in serv_addr;

#define HIGH 1
#define LOW  0  

int X=0;
int Y=0;

int Key_Value;


void joystick_init()
{
    gpioSetMode(11, PI_INPUT);
    
    gpioSetMode(12, PI_INPUT);
    gpioSetMode(13, PI_INPUT);
    
    gpioWrite(12,1);
    gpioWrite(13,1);
}

void socket_init(void)
{
	memset(&serv_addr,0,sizeof(serv_addr));

	sock = socket(PF_INET,SOCK_STREAM,0);
	if(sock == -1)
	{
		printf("could not socket error");
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("192.168.1.88");
	serv_addr.sin_port=htons(8000);	
}

void setup(void)
{
	 joystick_init();
	 socket_init();
}

int main(int argc,char *argv[])
{
    int count;
    int count_n=0;
    int i=1;
    int value_1 = 0;
    char temp[1024]={0,};
    
    if (gpioInitialise() < 0)
    {
      fprintf(stderr, "pigpio initialisation failed\n");
      return 1;
    }
    setup();
    
    while(1)
	{
		if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)))
		{
			if(gpioRead(11) == 1)
			{
				time_sleep(0.01);
				Key_Value = 0;
				if(gpioRead(11) == 0)
				{
					time_sleep(0.01);
					if(gpioRead(11) == 0)
					{
						//system("mpg321 -q beep-1.mp3");
						X=0;
						Key_Value = 1;
						
						count = sprintf(temp,"%d",Key_Value);
				        	count_n = count + 31;
						
						char data_str_2[count_n];
						
						sprintf(data_str_2,"ClawMachine;joystick;%d;button;%d\n",X,Key_Value);
						write(sock,data_str_2,sizeof(data_str_2));
					}
				}
			}
			if (gpioRead(12) == 0)
			{
				//system("mpg321 -q lever-1.mp3");
				X = 1;
				count = sprintf(temp,"%d",X);
				count_n = count + 31;
				char data_str[count_n];
				sprintf(data_str,"ClawMachine;joystick;%d;button;%d\n",X,Key_Value);     // 31
				write(sock,data_str,sizeof(data_str));
				time_sleep(0.05);
				
				if(X > 100)X = 0;
			}
			if (gpioRead(13) == 0)
			{
				//system("mpg321 -q beep-1.mp3");
				X = -1;
				count = sprintf(temp,"%d",X);
				count_n = count + 31;
				char data_str_1[count_n];
				sprintf(data_str_1,"ClawMachine;joystick;%d;button;%d\n",X,Key_Value);
				write(sock,data_str_1,sizeof(data_str_1));
				time_sleep(0.05);
				
				if(X <= -100)X = 0;
			}
		}	 
	}
	gpioTerminate();
	return 0;
}
























