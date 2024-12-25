#include "SysLogger.h"
#include "Common.h"

#include <stdarg.h>
#include <fstream>

void SysDeleteLogs()
{
    _STD remove("output.txt");
}

void SysLog(const char *msg, ...)
{
    char buf[300];
    va_list args;
    va_start(args, msg);
    vsprintf(buf, msg, args);
    va_end(args);

    _STD ofstream log("output.txt", _STD ios::app);
    log << buf << '\n' << _STD endl;
    log.close();
}