#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "Win32ThreadpoolConfig.h"

namespace KtlThreadpool {

    const int THREADPOOL_TRACE_FILTER_LEVEL = 3;

    enum TraceLevel
    {
        Error = 0,
        Warning,
        Info,
        Verbose,
        Debug,
        Noisy,
        Invalid
    };

    class Tracer
    {
    public:

        Tracer() { }

        ~Tracer() { }

        static Tracer& GetTracerSingleton()
        {
            static Tracer tracer;
            return tracer;
        }

        void Write(int level, LPCSTR format, ...)
        {
            int maxsize = 1024;
            if (level > THREADPOOL_TRACE_FILTER_LEVEL) {
                return;
            }

            va_list args;
            va_start(args, format);
            std::string buf;
            buf.resize(maxsize+1);
            buf[maxsize] = 0;
            vsnprintf((char *) buf.c_str(), maxsize, format, args);

            TraceThreadpoolMsg(level, buf.c_str());
        }

        void Assert(const char* msg)
        {
            std::string str(msg);
            str += "\r\n\r\n";

            str += "errno: " + std::to_string(errno);
            str += "\r\n\r\n";

            std::ifstream flimits("/proc/self/limits");
            std::string line;
            while (std::getline(flimits, line)) {
                str += line + "\r\n";
            }
            str += "\r\n\r\n";

            std::ifstream fstatus("/proc/self/status");
            while (std::getline(fstatus, line)) {
                str += line + "\r\n";
            }
            str += "\r\n\r\n";

            FILE *ftop = popen("top -b -n 1", "r");
            std::string obuf;
            obuf.resize(1024);
            while(ftop && fgets((char*)obuf.data(), obuf.length(), ftop))
            {
                str += obuf.c_str();
                if (str[str.length() - 1] == '\n')
                {
                    str.pop_back();
                }
                str += "\r\n";
            }
            if (ftop) pclose(ftop);
            str += "\r\n\r\n";

            ThreadpoolAssert(str.c_str());
        }
    };

    #define TP_TRACE(level, format, args...)  { Tracer::GetTracerSingleton().Write(level, format, args); }
    #define TP_ASSERT(expr, msg)              { if (!(expr)) { Tracer::GetTracerSingleton().Assert(msg); } }
}
