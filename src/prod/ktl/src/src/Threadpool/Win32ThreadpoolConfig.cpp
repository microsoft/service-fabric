#include <string>
#include "MinPal.h"
#include "Tracer.h"

using namespace std;

namespace KtlThreadpool {

    const char* TraceType = "threadpool";

    int GetThreadpoolThrottle()
    {
        return 400;
    }

    void TraceThreadpoolMsg(int level, const string &msg)
    {
        //printf("%s[%d]: %s\n", TraceType, level, msg.c_str());
    }

    void ThreadpoolAssert(const char* msg)
    {
        ASSERT(msg);
    }
}

