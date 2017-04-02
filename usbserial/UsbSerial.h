#ifndef __USB_SERIAL__H_______
#define __USB_SERIAL__H_______

/*从头文件中的函数定义不难看出，函数的功能，使用过程如下：
（1） 打开串口设备，调用函数setTTYSpeed（）；
（2） 设置串口读写的波特率，调用函数setTTYSpeed（）；
（3） 设置串口的属性，包括停止位、校验位、数据位等，调用函数setTTYParity（）；
（4） 向串口写入数据，调用函数sendnTTY（）；
（5） 从串口读出数据，调用函数recvnTTY（）；
（6） 操作完成后，需要调用函数cleanTTY（）来释放申请的串口信息接口；
其中，lockTTY（）和unlockTTY（）是为了能够在多线程中使用。在读写操作的前后，需要锁定和释放串口资源。
具体的使用方法，在代码实现的原文件中，main（）函数中进行了演示。下面就是源代码文件： */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <string.h>
#include<sys/file.h> // 定义函数  int flock(int fd,int operation);

typedef struct tty_info_t // 串口设备信息结构
{
    int fd; // 串口设备ID
    pthread_mutex_t mt; // 线程同步互斥对象
    char name[24]; // 串口设备名称，例："/dev/ttyS0"
    struct termios ntm; // 新的串口设备选项
    struct termios otm; // 旧的串口设备选项
} TTY_INFO;

class CUsbSerial
{
public:
    TTY_INFO *ptty;

    CUsbSerial();
    ~CUsbSerial();
    TTY_INFO *readyTTY(void); // 初始化串口设备并进行原有设置的保存
    int cleanTTY(TTY_INFO *ptty);// 清理串口设备资源
    int setTTYSpeed(TTY_INFO *ptty, int speed);
    int setTTYParity(TTY_INFO *ptty,int databits,int parity,int stopbits);
    int recvnTTY(TTY_INFO *ptty,char *pbuf,int size);
    int sendnTTY(TTY_INFO *ptty,char *pbuf,int size);
    int lockTTY(TTY_INFO *ptty);
    int unlockTTY(TTY_INFO *ptty);
    TTY_INFO * TTY_Init(void);
    TTY_INFO *UsbSerialInit(void);
    void ScanInput(void);
};

void *thread(void *ptty);

#endif
