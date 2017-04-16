#include "UsbSerial.h"
#include "log.h"
#include <queue>

CUsbSerial *g_UsbSerial = NULL;

string m_strScanBuf;
unsigned long m_ReceiveTick;
queue<char> g_qCmandBuf;

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

// bollow function is direct to read one key form keyboard without enter key.
#include <unistd.h>
#include <termios.h>
char getch()
{
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
    perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror ("tcsetattr ~ICANON");
    return (buf);
}

#include <time.h>
unsigned long GetTickCount(void)// 返回自系统开机以来的毫秒数（tick）
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void *thread_displsy(void *ptty)
{
    while (1)
    {
        if (g_UsbSerial != NULL)
        {
            char a = getch();
           // m_strScanBuf += a;
            g_qCmandBuf.push(a);
            //m_ReceiveTick = GetTickCount();
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

    pthread_t pth_display;
    pthread_create(&pth, NULL, thread_displsy, ptty);

    printf("Please use ctrl+c to close, or will be wrong when we open it in second time.\n");
    return ptty;
}

void CUsbSerial::SendTty(string str)
{
    sendnTTY(ptty, (char*)str.c_str(), str.size());
}

int InUpArrow(char ch)
{
    static int cnt = 0;
    const char *UpArrow = "\x1B\x5B\x41";
    if (cnt == 0 && ch == UpArrow[0])
    {
        cnt++;
        return 0;
    }
    else if (cnt == 1 && ch == UpArrow[1])
    {
        cnt++;
        return 0;
    }
    else if (cnt == 2 && ch == UpArrow[2])
    {
        cnt = 0;
        return 1;
    }
    else
    {
        cnt = 0;
        return -1;
    }
    return -1;
}

void CUsbSerial::ScanInput(void)
{
    vector<string> vBuf;
    string strBuf;
    int BackIndex = 0;
    while (1)
    {
        for (int i=0; g_qCmandBuf.empty()==false; i++)
        {
            char ch = g_qCmandBuf.front();
            g_qCmandBuf.pop();
            int res = InUpArrow(ch);
            if (res >= 0)
            {
                if (res > 0)
                {
                    BackIndex++;
                    if (BackIndex > 0 && BackIndex < vBuf.size())
                    {
                        SendTty("\x15");
                        string strSend = vBuf[vBuf.size() - BackIndex];
                        SendTty(strSend);
                        vBuf.push_back(strSend);
                        BackIndex++;
                    }
                    else
                    {
                        printf("Out of range");
                    }
                }
            }
            else
            {
                BackIndex = 0;
                sendnTTY(ptty, &ch, 1);
                if (ch == '\n')
                {
                    if (strBuf.size() > 0)
                        vBuf.push_back(strBuf);
                    strBuf.clear();
                }
                else
                {
                    strBuf += ch;
                }
            }
        }
    }
}
























