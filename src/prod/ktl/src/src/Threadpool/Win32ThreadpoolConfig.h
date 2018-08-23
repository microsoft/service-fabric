#pragma once
#include <string>

namespace KtlThreadpool
{
    int GetThreadpoolThrottle();
    void TraceThreadpoolMsg(int level, const std::string &msg);
    void ThreadpoolAssert(const char* msg);
}
