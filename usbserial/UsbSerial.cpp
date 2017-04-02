#include "UsbSerial.h"

CUsbSerial *g_UsbSerial = NULL;

CUsbSerial::CUsbSerial(void)
{
    g_UsbSerial = this;
    ptty = UsbSerialInit();
}

CUsbSerial::~CUsbSerial(void)
{
    cleanTTY(ptty);
    g_UsbSerial = NULL;
    pthread_exit(0);
}

TTY_INFO *CUsbSerial::readyTTY(void) // 初始化串口设备并进行原有设置的保存
{
    TTY_INFO *ptty = (TTY_INFO *)malloc(sizeof(TTY_INFO));
    if (ptty == NULL)return NULL;

    memset(ptty,0,sizeof(TTY_INFO));

    pthread_mutex_init(&ptty->mt,NULL);

    int id;
    for (id=0; id<256; id++)
    {
        sprintf(ptty->name,"/dev/ttyUSB%d", id);

        // 打开并且设置串口
        ptty->fd = open(ptty->name, O_RDWR | O_NOCTTY |O_NDELAY);
        if (ptty->fd >= 0)
        {
            // 取得并且保存原来的设置
            tcgetattr(ptty->fd,&ptty->otm);
            return ptty;
        }
    }

    printf("Error: ready /dev/ttyUSB0 ~ %s failed!\n", ptty->name);
    free(ptty);
    while (1){}
    return NULL;
}

int CUsbSerial::cleanTTY(TTY_INFO *ptty)// 清理串口设备资源
{// 关闭打开的串口设备
    if (ptty->fd > 0)
    {
        tcsetattr(ptty->fd, TCSANOW, &ptty->otm);
        close(ptty->fd);
        ptty->fd = -1;
        free(ptty);
        ptty = NULL;
    }
    return 0;
}

// 设置串口通信速率
// ptty 参数类型(TTY_INFO *),已经初始化的串口设备信息结构指针
// speed 参数类型(int),用来设置串口的波特率
// return 返回值类型(int),函数执行成功返回零值，否则返回大于零的值
int CUsbSerial::setTTYSpeed(TTY_INFO *ptty, int speed)
{
    // 进行新的串口设置,数据位为8位
    bzero(&ptty->ntm, sizeof(ptty->ntm));
    tcgetattr(ptty->fd,&ptty->ntm);
    ptty->ntm.c_cflag = /*CS8 |*/ CLOCAL | CREAD;

    switch(speed)
    {
    case 300:ptty->ntm.c_cflag |= B300;break;
    case 1200:ptty->ntm.c_cflag |= B1200; break;
    case 2400: ptty->ntm.c_cflag |= B2400;break;
    case 4800:ptty->ntm.c_cflag |= B4800; break;
    case 9600: ptty->ntm.c_cflag |= B9600; break;
    case 19200:ptty->ntm.c_cflag |= B19200;break;
    case 38400:ptty->ntm.c_cflag |= B38400;break;
    case 115200:ptty->ntm.c_cflag |= B115200;break;
    }
    ptty->ntm.c_iflag = IGNPAR;
    ptty->ntm.c_oflag = 0;

    tcflush(ptty->fd, TCIFLUSH);
    tcsetattr(ptty->fd,TCSANOW,&ptty->ntm);

    return 0;
}

// 设置串口数据位，停止位和效验位
// ptty 参数类型(TTY_INFO *),已经初始化的串口设备信息结构指针
// databits 参数类型(int), 数据位,取值为7或者8
// stopbits 参数类型(int),停止位,取值为1或者2
// parity 参数类型(int),效验类型 取值为N,E,O,,S
// return 返回值类型(int),函数执行成功返回零值，否则返回大于零的值
int CUsbSerial::setTTYParity(TTY_INFO *ptty,int databits,int parity,int stopbits)
{
    // 取得串口设置
    if( tcgetattr(ptty->fd,&ptty->ntm) != 0)
    {
        printf("SetupSerial [%s]\n",ptty->name);
        return 1;
    }

    bzero(&ptty->ntm, sizeof(ptty->ntm));
    ptty->ntm.c_cflag = CS8 | CLOCAL | CREAD;
    ptty->ntm.c_iflag = IGNPAR;
    ptty->ntm.c_oflag = 0;

    // 设置串口的各种参数
    ptty->ntm.c_cflag &= ~CSIZE;
    switch (databits)
    { //设置数据位数
    case 7:
        ptty->ntm.c_cflag |= CS7;
        break;
    case 8:
        ptty->ntm.c_cflag |= CS8;
        break;
    default:
        printf("Unsupported data size\n");
        return 5;
    }

    switch (parity)
    { // 设置奇偶校验位数
    case 'n':
    case 'N':
        ptty->ntm.c_cflag &= ~PARENB; /* Clear parity enable */
        ptty->ntm.c_iflag &= ~INPCK; /* Enable parity checking */
        break;
    case 'o':
    case 'O':
        ptty->ntm.c_cflag |= (PARODD|PARENB); /* 设置为奇效验*/
        ptty->ntm.c_iflag |= INPCK; /* Disnable parity checking */
        break;
    case 'e':
    case 'E':
        ptty->ntm.c_cflag |= PARENB; /* Enable parity */
        ptty->ntm.c_cflag &= ~PARODD; /* 转换为偶效验*/
        ptty->ntm.c_iflag |= INPCK; /* Disnable parity checking */
        break;
    case 'S':
    case 's': /*as no parity*/
        ptty->ntm.c_cflag &= ~PARENB;
        ptty->ntm.c_cflag &= ~CSTOPB;
        break;
    default:
        printf("Unsupported parity\n");
        return 2;
    }

    // 设置停止位
    switch (stopbits)
    {
    case 1:
        ptty->ntm.c_cflag &= ~CSTOPB;
        break;
    case 2:
        ptty->ntm.c_cflag |= CSTOPB;
        break;
    default:
        printf("Unsupported stop bits\n");
        return 3;
    }

    ptty->ntm.c_lflag = 0;
    ptty->ntm.c_cc[VTIME] = 0; // inter-character timer unused
    ptty->ntm.c_cc[VMIN] = 1; // blocking read until 1 chars received
    tcflush(ptty->fd, TCIFLUSH);
    if (tcsetattr(ptty->fd,TCSANOW,&ptty->ntm) != 0)
    {
        printf("SetupSerial \n");
        return 4;
    }

    return 0;
}

int CUsbSerial::recvnTTY(TTY_INFO *ptty,char *pbuf,int size)
{
    int ret,left,bytes;

    left = size;

    while(left>0)
    {
        ret = 0;
        bytes = 0;

        pthread_mutex_lock(&ptty->mt);
        ioctl(ptty->fd, FIONREAD, &bytes);
        if(bytes>0)
        {
            ret = read(ptty->fd,pbuf,left);
        }
        pthread_mutex_unlock(&ptty->mt);
        if(ret >0)
        {
            left -= ret;
            pbuf += ret;
        }
        usleep(100);
    }

    return size - left;
}

int CUsbSerial::sendnTTY(TTY_INFO *ptty,char *pbuf,int size)
{
    int ret,nleft;
    char *ptmp;

    ret = 0;
    nleft = size;
    ptmp = pbuf;

    while(nleft>0)
    {
        pthread_mutex_lock(&ptty->mt);
        ret = write(ptty->fd,ptmp,nleft);
        pthread_mutex_unlock(&ptty->mt);

        if(ret >0)
        {
            nleft -= ret;
            ptmp += ret;
        }
        //usleep(100);
    }

    return size - nleft;
}

int CUsbSerial::lockTTY(TTY_INFO *ptty)
{
    if (ptty->fd < 0) return 1;
    return flock(ptty->fd,LOCK_EX);
}

int CUsbSerial::unlockTTY(TTY_INFO *ptty)
{
    if (ptty->fd < 0) return 1;
    return flock(ptty->fd,LOCK_UN);
}

TTY_INFO *CUsbSerial::TTY_Init(void)
{
    TTY_INFO *ptty = readyTTY();
    //lockTTY(ptty);
    setTTYSpeed(ptty, 115200);
    setTTYParity(ptty,8,'N',1);
    return ptty;
}

void *thread(void *ptty)
{
    while (1)
    {
        if (g_UsbSerial != NULL)
        {
            char Buffer[2] = {0};
            if (g_UsbSerial->recvnTTY((TTY_INFO*)ptty, Buffer, 1) > 0)
            {
                printf("%s", Buffer); //fflush(stdout);
            }
        }
    }
    return NULL;
}

TTY_INFO *CUsbSerial::UsbSerialInit(void)
{
    setvbuf(stdout,NULL,_IONBF,0); //直接将缓冲区禁止了. 它就直接输出了

    ptty = TTY_Init();

    pthread_t pth;
    pthread_create(&pth, NULL, thread, ptty);

    printf("Please use ctrl+c to close, or will be wrong when we open it in second time.\n");
    return ptty;
}

void CUsbSerial::ScanInput(void)
{
    char a = 0;
    scanf("%c",&a);
    sendnTTY(ptty,&a,1);
}





