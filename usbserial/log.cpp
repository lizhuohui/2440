#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

//int write_log(FILE* pFile, const char *format, ...)
//{
//    va_list arg;
//    int done;

//    va_start (arg, format);
//    //done = vfprintf (stdout, format, arg);

//    time_t time_log = time(NULL);
//    struct tm* tm_log = localtime(&time_log);
//    fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d ", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);

//    done = vfprintf (pFile, format, arg);
//    va_end (arg);

//    fflush(pFile);
//    return done;
//}

//int log_test(void)
//{
//    FILE* pFile = fopen("log_test.txt", "a");
//    write_log(pFile, "%s %d %f\n", "is running", 10, 55.55);
//    fclose(pFile);

//    return 0;
//}

void PrintTime(FILE* pFile)
{
    time_t time_log = time(NULL);
    struct tm* tm_log = localtime(&time_log);
    fprintf(pFile, "\n\n\n\n\n======= %04d-%02d-%02d %02d:%02d:%02d =========\n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);
}

int WriteCommandHistory(string str)
{
    WriteCommandHistory(str.c_str());
}

int WriteCommandHistory(const char *format, ...)
{
    static FILE* pFile;
    static bool IsFirst = true;
    if (IsFirst == true)
    {
        IsFirst = false;
        pFile = fopen("CommandHistory.txt", "a");
    }

    va_list arg;
    va_start (arg, format);
    vfprintf (pFile, format, arg);
    va_end (arg);

    fflush(pFile);

    return 0;
}

string ReadCommandHistory(int &BackwardIndex, bool IsUpArrow)
{
    ifstream fin("CommandHistory.txt");
    vector<string> vBuf;
    string str;
    for (int i=0; getline(fin, str); i++)
    {
        vBuf.push_back(str);
    }

    int BackwardMaxIndex = vBuf.size() - 1;

    if (IsUpArrow)
    {
        if (BackwardIndex < BackwardMaxIndex)BackwardIndex++;
    }
    else
    {
        if (BackwardIndex > 0)BackwardIndex--;
    }

    string strRet = vBuf[BackwardMaxIndex - BackwardIndex];

    return strRet;
}





















