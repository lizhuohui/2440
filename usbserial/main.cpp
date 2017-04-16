#include "UsbSerial.h"
#include "log.h"

void SetArg(int *argc, char **argv[])
{
    int i = 0;
    static char *argment[20] = {0};
    argment[i++] = strdup("flashimg");
    argment[i++] = strdup("-s");
    *argv = argment;
    *argc = i;
}

void PrintArg(int argc, char *argv[])
{
    int i = 0;
    for (i=0; i<argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n\n");
}

int main(int argc, char *argv[])
{
#if 0
    if (argc == 1)
    {
        execl("/usr/bin/gnome-terminal", "gnome-terminal", "-e", "sudo ./usbserial null", NULL);
    }
#endif

//    if (argc <= 2 )
//        SetArg(&argc, &argv);

//    PrintArg(argc, argv);

    CUsbSerial usbserial;
    usbserial.ScanInput();

}

