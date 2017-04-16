#ifndef LOG_H
#define LOG_H

int WriteCommandHistory(string str);
int WriteCommandHistory(const char *format, ...);
string ReadCommandHistory(int &BackwardIndex, bool IsUpArrow);

#endif // LOG_H

