/*
TARGET:Raspberry Pi B 3
Copyright(c)2019, epicgram Co.,LTD
PROJECT: Egg Incubator kiosk2
REVISION:V1.0
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
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

volatile int lastEncoded = 0;
volatile long value =  -1;

void port_init(void)
{
	gpioSetMode(11, PI_INPUT);
	gpioWrite(11,1);	
	
	gpioSetMode(12, PI_INPUT);
	gpioWrite(12,1);
	
	gpioSetMode(13, PI_INPUT);
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

void update_encoder(void)
{
	int MSB = gpioRead(12);
	int LSB = gpioRead(13);
	
	int encoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | encoded;
    
    if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) value++;
    if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) value--;
    
    lastEncoded = encoded;
}

int Key_Value;

int main(int argc, char *argv[])
{
	int count;
	int count_n = 0;
	int i=1;
	int value_1 = 0;
	
	char temp[1024] = {0,};
	
	if (gpioInitialise() < 0) return 1;
	
	socket_init();
	port_init();
	
	while(1)
	{
		if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)))
		{
			update_encoder();
			
			if(value > 10000) 
			{
				system("mpg321 -q beep-1.mp3");
				value = 0;
				value_1 = 1;
				count = sprintf(temp,"%d",value_1);
				count_n = count + 25;
				//printf("%d %d\n",value_1,count_n);
				Key_Value = 0;
				char data_str[count_n];
				sprintf(data_str,"Washer;encoder;%d;button;%d\n",value_1,Key_Value);   // 25
				write(sock,data_str,sizeof(data_str));
			}
			
			if(value <= -10000) 
			{
				system("mpg321 -q beep-1.mp3");
				value = 0;
				value_1=-1;
			    Key_Value = 0;
			    count = sprintf(temp,"%d",value_1);
				count_n = count + 25;
				//printf("%d %d\n",value_1,count_n);
				char data_str_1[count_n];
				sprintf(data_str_1,"Washer;encoder;%d;button;%d\n",value_1,Key_Value);
				write(sock,data_str_1,sizeof(data_str_1));
		    }
		    
		    if(gpioRead(11) == 0)
			{
				time_sleep(0.1);
				Key_Value = 1;
				
				if(gpioRead(11) == 0)
				{
					//system("mpg321 -q lever.mp3");
					count = sprintf(temp,"%d",value_1);
				    count_n = count + 25;
				    //printf("%d %d\n",value_1,count_n);
				    char data_str_2[count_n];
					value_1 = 0;
					Key_Value = 1;
					sprintf(data_str_2,"Washer;encoder;%d;button;%d\n",value_1,Key_Value);
					write(sock,data_str_2,sizeof(data_str_2));
				}
			}
		}
   }
   gpioTerminate();
   return 0;
}

