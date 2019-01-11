#ifndef Pattern_IOCTL_H
#define Pattern_IOCTL_H
#include <linux/ioctl.h>
 
typedef struct
{
    uint8_t led[10][8];
                                  
} Pattern;

#define CONFIG _IO('q',1)
#endif
