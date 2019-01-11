#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdint.h>
#include <inttypes.h>
#include "rdtsc.h"
#include "pattern_ioctl.h"


float Dist=0;
pthread_mutex_t lock;
int timeout_flag = 0;

void IO_init()
{
	printf("Initialising GPIOs\n");
 // Exporting the GPIOs
     int FdExport;
     FdExport = open("/sys/class/gpio/export", O_WRONLY);
     if(FdExport < 0)
            {
                printf("gpio export open failed \n");
             }
             
             
 	 // Exporting IO2 pins
 	 
 	 if(write(FdExport,"77",2)<0)
       {
            printf("error in exporting gpio77\n");
       }
   
   if(write(FdExport,"13",2)<0)
       {
            printf("error in exporting gpio13\n");
        }
    if(write(FdExport,"34",2)<0)
       {
            printf("error in exporting gpio34\n");
        }
    
    // Exporting IO3 pins
    if(write(FdExport,"16",2)<0)
       {
            printf("error in exporting gpio16\n");
        }
    if(write(FdExport,"14",2)<0)
       {
            printf("error in exporting gpio14\n");
        }
    if(write(FdExport,"76",2)<0)
       {
            printf("error in exporting gpio76\n");
        }
    if(write(FdExport,"64",2)<0)
       {
            printf("error in exporting gpio64\n");
        }
      
    // Giving direction to GPIOs for IO2
    int FdDir;
    FdDir = open("/sys/class/gpio/gpio13/direction", O_WRONLY);
    if(FdDir<0)
        {
            printf("ERROR: gpio13 direction open failed \n");
        }
    if(write(FdDir,"out",3)<0)       //Setting direction to out
        {
            printf("ERROR in writing to gpio13 direction\n");
        }
    
     FdDir = open("/sys/class/gpio/gpio34/direction", O_WRONLY);
    if(FdDir<0)
        {
            printf("ERROR: gpio34 direction open failed \n");
        }
    if(write(FdDir,"out",3)<0)       //Setting direction to out
        {
            printf("ERROR in writing to gpio34 direction\n");
        }
        
    
    // Giving direction to GPIOs for IO3
    
        
    FdDir = open("/sys/class/gpio/gpio16/direction", O_WRONLY);
    if(FdDir<0)
        {
            printf("ERROR: gpio16 direction open failed \n");
        }
    if(write(FdDir,"out",3)<0)       //Setting direction to out
        {
            printf("ERROR in writing to gpio16 direction\n");
        }

       
    FdDir = open("/sys/class/gpio/gpio14/direction", O_WRONLY | O_RDONLY);
    if(FdDir<0)
        {
            printf("ERROR: gpio14 direction open failed \n");
        }
    if(write(FdDir,"in",2)<0)       //Setting direction to in
        {
            printf("ERROR in writing to gpio14 direction\n");
        }
          
          
    // Setting values of GPIOs
    
    //setting values to GPIO pins of IO2
    int FdValue;
        
    FdValue = open("/sys/class/gpio/gpio77/value", O_WRONLY);
    if(FdValue<0)
    {
    printf("ERROR: gpio77 value open failed \n");
    }
            
    if(write(FdValue,"0",1)<0)  // Sets value to 0
    {
     printf("ERROR in writing gpio77 value\n");
    }
    
    
    FdValue = open("/sys/class/gpio/gpio34/value", O_WRONLY);
    if(FdValue<0)
    {
    printf("ERROR: gpio34 value open failed \n");
    }
            
    if(write(FdValue,"0",1)<0)  // Sets value to 0
    {
     printf("ERROR in writing gpio34 value\n");
    }
     
     
        FdValue = open("/sys/class/gpio/gpio13/value", O_WRONLY);
    if(FdValue<0)
    {
    printf("ERROR: gpio13 value open failed \n");
    }
            
    if(write(FdValue,"0",1)<0)  // Sets value to 0
    {
     printf("ERROR in writing gpio13 value\n");
    }
    
    
    
    
    //setting values for IO3
    
        FdValue = open("/sys/class/gpio/gpio64/value", O_WRONLY);
    if(FdValue<0)
    {
    printf("ERROR: gpio64 value open failed \n");
    }
            
    if(write(FdValue,"0",1)<0)  // Sets value to 0
    {
     printf("ERROR in writing gpio64 value\n");
    }
    
    
        FdValue = open("/sys/class/gpio/gpio76/value", O_WRONLY);
    if(FdValue<0)
    {
    printf("ERROR: gpio76 value open failed \n");
    }
            
    if(write(FdValue,"0",1)<0)  // Sets value to 0
    {
     printf("ERROR in writing gpio76 value\n");
    } 
    
        FdValue = open("/sys/class/gpio/gpio16/value", O_WRONLY);
    if(FdValue<0)
    {
    printf("ERROR: gpio16 value open failed \n");
    }
            
    if(write(FdValue,"1",1)<0)  // Sets value to 0
    {
     printf("ERROR in writing gpio16 value\n");
    }
    printf("Initialising GPIOs\n");

    close(FdExport);
  
 
}	



 // Echo input detect function
 
 void *echo_input_det()
 {
     long double start_time=0, end_time=0, time_diff=0;
     int dist1=0;
     //printf("Echo Detection Thread running\n");
     int fd_echo,fdEdge, Fdtrig;
     struct pollfd PollEch;
     unsigned char ReadValue;
   
     fd_echo = open("/sys/class/gpio/gpio14/value", O_RDONLY);
     fdEdge = open("/sys/class/gpio/gpio14/edge", O_WRONLY | O_RDONLY);
     Fdtrig = open("/sys/class/gpio/gpio13/value", O_WRONLY);
  
      
     // Prepare poll fd structure
        PollEch.fd = fd_echo;
        PollEch.events = POLLPRI|POLLERR;
        PollEch.revents = 0;
        
        
      write(fdEdge,"falling",7);  // Sets edge to rising
      
      while(!timeout_flag)
      {
       // Start polling for rising edge at echo
       
       
       lseek(PollEch.fd, 0, SEEK_SET);
       read(PollEch.fd,&ReadValue,1);
      
       
       //sending trigger pulse

       write(Fdtrig,"1",1);  // trigger
       usleep(12);  
       write(Fdtrig,"0",1);
       start_time = rdtsc();
       
       
           	
         	// Detecting Falling edge and capturing end time

            int poll_ret = poll(&PollEch,1,1000);
            
            if(poll_ret<0)
            {
            printf("error in polling for falling edge");
            }
            else if(poll_ret==0)
            {
            printf("faling egde polling timeout ");
            }
            else 
            {
            	if (PollEch.revents & POLLPRI)
            	{
            	
                	end_time = rdtsc();

                	time_diff = (end_time-start_time);

                	dist1=(time_diff)*(7.5/20000);
                	printf("Distance: %d\n", dist1);
                	if(dist1<150)
                		dist1=150;
                	else if(dist1>750)
                		dist1=750;
                	pthread_mutex_lock(&lock);
                	Dist = dist1;				//calculating speed factor for animation
                	pthread_mutex_unlock(&lock);
                }
				}
					

	usleep(100000);
   }
     
   pthread_exit(0);
 }
 


void* Display_LEDMatrix()
{
	int fd_spi;
	fd_spi = open("/dev/spidev1.0", O_RDWR);
   char buf[20];
	int rest=0;
	while(!timeout_flag)
	{
		
		if(Dist<=750 && Dist>550)             // Range 3
		{
			rest = 0.1*Dist;
			sprintf(buf, "0 1 2 %d", rest);
			write(fd_spi, buf, sizeof(buf));
		}
		else if(Dist<=550 && Dist>350)        // Range 2
		{
			rest = 0.075*Dist;
			sprintf(buf, "3 4 5 %d", rest);
			write(fd_spi, buf, sizeof(buf));
		}
		else if(Dist<=350 && Dist>175)        // Range 1
		{
			rest = 0.05*Dist;
			sprintf(buf, "6 7 8 %d", rest);
			write(fd_spi, buf, sizeof(buf));
		}
		else if(Dist<=175 && Dist>=150)
		{
			rest = 0.1*Dist;
			sprintf(buf, "9 9 9 %d", rest);
			write(fd_spi, buf, sizeof(buf));
		}
		usleep(200*Dist);
	}
	pthread_exit(0);
}


int main()
{
    int fd_spi;
    Pattern p;


    // Pattern 1
    p.led[0][0] = 0x00;
    p.led[0][1] = 0x00;
    p.led[0][2] = 0x9D;
    p.led[0][3] = 0x89;
    p.led[0][4] = 0x00;
    p.led[0][5] = 0x00;
    p.led[0][6] = 0x81;
    p.led[0][7] = 0x81;
    
    // Pattern 2
    p.led[1][0] = 0x00;
    p.led[1][1] = 0x81;
    p.led[1][2] = 0x9D;
    p.led[1][3] = 0x08;
    p.led[1][4] = 0x00;
    p.led[1][5] = 0x81;
    p.led[1][6] = 0x81;
    p.led[1][7] = 0x00;
    
    // Pattern 3
    p.led[2][0] = 0x81;
    p.led[2][1] = 0x81;
    p.led[2][2] = 0x1C;
    p.led[2][3] = 0x08;
    p.led[2][4] = 0x81;
    p.led[2][5] = 0x81;
    p.led[2][6] = 0x00;
    p.led[2][7] = 0x00;
    
    // Pattern 4
    p.led[3][0] = 0x00;
    p.led[3][1] = 0x00;
    p.led[3][2] = 0x81;
    p.led[3][3] = 0xB9;
    p.led[3][4] = 0x10;
    p.led[3][5] = 0x00;
    p.led[3][6] = 0x81;
    p.led[3][7] = 0x81;
    
    // Pattern 5
    p.led[4][0] = 0x00;
    p.led[4][1] = 0x81;
    p.led[4][2] = 0x81;
    p.led[4][3] = 0x38;
    p.led[4][4] = 0x10;
    p.led[4][5] = 0x81;
    p.led[4][6] = 0x81;
    p.led[4][7] = 0x00;
    
    // Pattern 6
    p.led[5][0] = 0x81;
    p.led[5][1] = 0x81;
    p.led[5][2] = 0x00;
    p.led[5][3] = 0x38;
    p.led[5][4] = 0x91;
    p.led[5][5] = 0x81;
    p.led[5][6] = 0x00;
    p.led[5][7] = 0x00;
    
    // Pattern 7
    p.led[6][0] = 0x00;
    p.led[6][1] = 0x00;
    p.led[6][2] = 0x81;
    p.led[6][3] = 0x81;
    p.led[6][4] = 0x70;
    p.led[6][5] = 0x20;
    p.led[6][6] = 0x81;
    p.led[6][7] = 0x81;
    
    // Pattern 8
    p.led[7][0] = 0x00;
    p.led[7][1] = 0x81;
    p.led[7][2] = 0x81;
    p.led[7][3] = 0x00;
    p.led[7][4] = 0x70;
    p.led[7][5] = 0xA1;
    p.led[7][6] = 0x81;
    p.led[7][7] = 0x00;
    
    // Pattern 9
    p.led[8][0] = 0x81;
    p.led[8][1] = 0x81;
    p.led[8][2] = 0x00;
    p.led[8][3] = 0x00;
    p.led[8][4] = 0xF1;
    p.led[8][5] = 0xA1;
    p.led[8][6] = 0x00;
    p.led[8][7] = 0x00;
    
    // Pattern 10
    p.led[9][0] = 0x81;
    p.led[9][1] = 0x20;
    p.led[9][2] = 0x00;
    p.led[9][3] = 0x81;
    p.led[9][4] = 0x91;
    p.led[9][5] = 0x00;
    p.led[9][6] = 0x50;
    p.led[9][7] = 0x81;
    
    
    
    IO_init();										//GPIOs initialization
    fd_spi = open("/dev/spidev1.0", O_RDWR);
    printf("opening spi");
    pthread_t tid_1, tid_2;
    pthread_create(&tid_1,NULL,echo_input_det,NULL);
    pthread_create(&tid_2,NULL,Display_LEDMatrix,NULL);
    sleep(3);
    int i = ioctl(fd_spi, CONFIG, &p);
    if(i==-1)
    {
  	 	exit(0);
    }
    sleep(80);
    timeout_flag = 1;
    pthread_join(tid_1,NULL);
    pthread_join(tid_2,NULL);
    close(fd_spi);
    printf("\nExiting the Program\n");
 
    return 0;
}
