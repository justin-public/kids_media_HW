/*
TARGET:Raspberry Pi B 3
Copyright(c)2019, epicgram Co.,LTD
PROJECT: Egg Incubator kiosk1
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

//#include <winsock2.h>
#include <pthread.h>
#include <pigpio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define TWINKLE_LOOP 15

#define   RED   0x00ff0000  
#define   GREEN   0x0000ff00  
#define   BLUE    0x000000ff  
#define   YELLOW    0x00ffff00  
#define   SKYBLUE   0x0073d1f7  
#define   PINK    0x00f69fa8  
#define   BLACK   0x00000000  
#define   BROWN   0x0088563f  
#define   ORANGE  0x00f3753a  
#define   DEEPBLUE  0x002a365c  

static void pabort(const char *s)
{
  perror(s);
  abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 2500000;
static uint16_t delay_1;

int byte_index = 0;
uint8_t* spread_msg;
uint8_t* rx;
uint16_t NUMPIXELS;


void spread_spi_bits(uint8_t start, uint8_t *msg, uint16_t led_num);
void run_func();
void sel_func();

struct {
  //volatile uint8_t* tx;
  uint8_t* tx;
  int fd;
  void(*show)();
  void(*setPixelColor)(uint16_t n, uint32_t rgb);
  long (*Color)(uint8_t r, uint8_t g, uint8_t b);
}pixels;

struct {
  uint8_t num;
  uint8_t* r;
  uint8_t* g;
  uint8_t* b;
  uint16_t* from;
  uint16_t* to;
  uint16_t* wait;
  uint16_t* run_time;
  uint16_t* func_num;
}f;

int fd,sock,sockfd;
struct sockaddr_in serv_addr;

char data_str[11];

int count1;
int button1;
int audio_flag;

static void transfer()
{
	int ret;
  
  	struct spi_ioc_transfer tr = {
    		.tx_buf = (unsigned long)spread_msg,
    		.rx_buf = (unsigned long)rx,
    		.len = NUMPIXELS * 3 * 3,
    		//.delay_usecs = delay,
    	.speed_hz = speed,
    	.bits_per_word = bits,
  	};
  	spread_spi_bits(0, pixels.tx, NUMPIXELS);
	ret = ioctl(pixels.fd, SPI_IOC_MESSAGE(1), &tr);
  	if (ret < 1)
    		pabort("can't send spi message");
	for (ret = 0; ret < ARRAY_SIZE(pixels.tx); ret++) {
    		//if (!(ret % 6))
      		//puts("");
    		//printf("%.2X ", rx[ret]);
  	}
  	//puts("");
}

static void print_usage(const char *prog)
{
  	//printf("Usage: %s [-DsbdlHOLC3]\n", prog);
  	puts("  -D --device   device to use (default /dev/spidev1.1)\n"
       		"  -s --speed    max speed (Hz)\n"
      	 	"  -d --delay    delay (usec)\n"
       		"  -b --bpw      bits per word \n"
       		"  -l --loop     loopback\n"
       		"  -H --cpha     clock phase\n"
       		"  -O --cpol     clock polarity\n"
       		"  -L --lsb      least significant bit first\n"
       		"  -C --cs-high  chip select active high\n"
       		"  -3 --3wire    SI/SO signals shared\n");
  	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
  while (1) {
    static const struct option lopts[] = {
      { "device",  1, 0, 'D' },
      { "speed",   1, 0, 's' },
      { "delay",   1, 0, 'd' },
      { "bpw",     1, 0, 'b' },
      { "loop",    0, 0, 'l' },
      { "cpha",    0, 0, 'H' },
      { "cpol",    0, 0, 'O' },
      { "lsb",     0, 0, 'L' },
      { "cs-high", 0, 0, 'C' },
      { "3wire",   0, 0, '3' },
      { "no-cs",   0, 0, 'N' },
      { "ready",   0, 0, 'R' },
      { NULL, 0, 0, 0 },
    };
    int c;

    c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL);

    if (c == -1)
      break;

    switch (c) {
    case 'D':
      device = optarg;
      break;
    case 's':
      speed = atoi(optarg);
      break;
    case 'd':
      //delay = atoi(optarg);
      break;
    case 'b':
      bits = atoi(optarg);
      break;
    case 'l':
      mode |= SPI_LOOP;
      break;
    case 'H':
      mode |= SPI_CPHA;
      break;
    case 'O':
      mode |= SPI_CPOL;
      break;
    case 'L':
      mode |= SPI_LSB_FIRST;
      break;
    case 'C':
      mode |= SPI_CS_HIGH;
      break;
    case '3':
      mode |= SPI_3WIRE;
      break;
    case 'N':
      mode |= SPI_NO_CS;
      break;
    case 'R':
      mode |= SPI_READY;
      break;
    default:
      print_usage(argv[0]);
      break;
    }
  }
}

void delay_ms(uint16_t wait)
{
  usleep(wait*1000);
}

void PixelColor(uint16_t n, uint32_t rgb)
{
  	uint8_t color[3] = {0};
  	color[0] = (rgb>>8);
  	color[1] = (rgb>>16);
  	color[2] = rgb;
  	memset(spread_msg+(9*n),0,9);
  	byte_index = 0;
  	spread_spi_bits(n, color,1);
}

long setColor(uint8_t r, uint8_t g, uint8_t b)
{
  	static uint32_t rgb = {0};
  	rgb = ((r<<20)|(r<<16))|((g<<12)|(g<<8))|((b<<4)|b);
  	return rgb;
}

void Led(uint16_t from,uint16_t to,uint16_t wait,uint8_t r,uint8_t g,uint8_t b)
{
  	uint16_t i;
 	for(i=from; i<=to; i++)
  	{
     	pixels.setPixelColor(i,pixels.Color(r,g,b));
  	}
  	pixels.show();
  	//delay(wait);
        //time_sleep(wait);
}
/*
void colorWipe(uint16_t from,uint16_t to,uint16_t wait,uint32_t c)
{
	uint16_t i;
	for(i=from; i<=to; i++)
	{
		pixels.setPixelColor(i,c);
		pixels.show();
		//delay(wait);
                //time_sleep(wait);	
	}	
}
*/

void *ThreadAudio(void * pArg)
{
	for(;;)
	{
			//system("omxplayer -o local /tmp/soft.mp3");
			system("omxplayer plane.mp3");
			//delay(1000);
                        time_sleep(1000);
			pthread_exit(pArg);
	}
}

void error(char *msg)
{
	perror(msg);
	exit(0);		
}

int getData(int sock)
{
	char buffer[21];
	int n;
	
	if ((n = read(sock,buffer,20)) < 0){
			printf("could not socket error");
	buffer[n] = '\0';
	}
	return atoi(buffer);
}

void port_init()
{
	gpioSetMode(11,PI_INPUT);
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
	serv_addr.sin_addr.s_addr = inet_addr("192.168.1.88");     // 192.168.0.139      
	serv_addr.sin_port=htons(8000);                         	// 8005
}

int main(int argc,char *argv[])
{
    	
        if (gpioInitialise() < 0)
        {
      	 fprintf(stderr, "pigpio initialisation failed\n");
     	 return 1;
    	}	
	
        port_init();
        socket_init();

    pixels.show = transfer;
	pixels.setPixelColor = PixelColor;
	pixels.Color = setColor;
	parse_opts(argc, argv);
	int ret = 0;
	pixels.fd = open(device, O_RDWR);
	if (pixels.fd < 0)
		pabort("can't open device");
	
 	ret = ioctl(pixels.fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");
	
	ret = ioctl(pixels.fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");
	//*/
	/*
	* max speed hz
	*/
	///*
	ret = ioctl(pixels.fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");
	
	ret = ioctl(pixels.fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");
	//*/	
	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	printf("\n");
	printf("Enter the number of LED you have ==> ");
	//scanf("%d",&NUMPIXELS);
	
	NUMPIXELS = 24;
	
	pixels.tx = (uint8_t*)malloc(sizeof(uint8_t)*(NUMPIXELS*3));
	rx = (uint8_t*)malloc(sizeof(uint8_t)*(NUMPIXELS * 3 * 3));
	spread_msg = (uint8_t*)malloc(sizeof(uint8_t)*(NUMPIXELS * 3 * 3));
    
        Led(0,24,1000,0,0,0);
        
        //system("omxplayer /home/pi/kiosk1/lever.mp3");
        while(1)
	{
	    if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)))
	    {
		if(gpioRead(11) == 1){
			Led(0,24,1000,0,0,0);
			count1 = 0;
		}	
		if(gpioRead(11) == 0)
		{
			//time_sleep(0.01);
                        //Led(0,24,1000,0,150,0);
                        //system("omxplayer /home/pi/kiosk1/lever-1.mp3");
			Led(0,24,1000,0,150,0);
			//button1 = 0;
                        if(gpioRead(11)==0){
				//Led(0,24,1000,0,150,0);	
                                system("mpg321 -q /home/pi/kiosk1/lever-1.mp3");
				count1++;
                                if(count1 == 1){
				        
					button1 = 1;
					sprintf(data_str,"button_1;%d\n",button1);
					write(sock,data_str,sizeof(data_str));
					//audio_flag = 1;
                                        /*
                                        if(audio_flag == 1)
					{
						//system("omxplayer /home/pi/kiosk1/lever.mp3");
						Led(0,24,1000,0,150,0);
                                        }
					*/
					
								
				}
			}            
		}
                	
	    }
            			
	}
	gpioTerminate();
	return 0;
}

#define ADV() do { \
      if(bit_mask == 1) \
      {\
         index ++; bit_mask = 0x80; \
      }\
      else \
      {\
         bit_mask >>=1;\
      }\
} while(0)

void spread_spi_bits(uint8_t start, uint8_t *msg, uint16_t led_num)
{
        uint16_t i,j;
        uint16_t len;
        uint16_t index;
        uint8_t mask,bit_mask;
        
        len = led_num*3; // led * 3. 
        bit_mask = 0x80;
        index = start*9;
        for( i = 0 ; i < len  ; i ++)
        {
                mask = 0x80;
                for( j = 0 ; j < 8 ; j ++, mask >>= 1)
                {
                        if(msg[i] & mask)
                        {
                                spread_msg[index] |= bit_mask ; ADV();
                                spread_msg[index] |= bit_mask ; ADV();
                                ADV();
                        }
                        else
                        {
                                spread_msg[index] |= bit_mask ; ADV();
                                ADV(); ADV();
                        }
                }
        }
        byte_index = index;
	for( i = 0 ; i < byte_index ; i ++)
  	{
    		//printf("i=%d v = %02x\n",i,spread_msg[i]);
  	}
}

