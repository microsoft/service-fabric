// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class DbgHelper
    {
    public:
        static std::string CxaDemangle(const char* mangledName);
        static std::map<void*, std::string> Addr2Line(std::string const & module, std::vector<void*> const & addresses);
    };

    class BackTrace
    {
    public:
        BackTrace(int frameToCapture, int skipCount = 0);
        BackTrace(void* const * rawBacktrace, size_t frameCount);

        ErrorCode ResolveSymbols();

        std::vector<std::string> const & ModuleList() const { return moduleList_; }
        std::vector<std::string> const & FuncAddrList() const { return funcAddrList_; }
        std::vector<std::string> const & SrcFileLineNoList() const { return srcFileLineNoList_; }

        bool operator == (BackTrace const & rhs) const;
        bool operator != (BackTrace const & rhs) const;
        bool operator < (BackTrace const & rhs) const;

        void WriteTo(TextWriter &w, FormatOptions const&) const;

    private:
        void Initialize(void* const * bt, size_t count);

        std::vector<void*> backtrace_; 
        std::vector<std::string> moduleList_;
        std::vector<std::string> funcAddrList_;
        std::vector<std::string> srcFileLineNoList_;
    };

    class CallStackRanking
    {
    public:
        CallStackRanking(int frameToCapture = 3);

        void Track();
        std::map<BackTrace, size_t> Callstacks() const;

        void ResolveSymbols();
        void WriteTo(TextWriter & w, FormatOptions const &) const;

    private:
        mutable RwLock lock_;
        std::map<BackTrace, size_t/* call count */> callstacks_;
        int frameToCapture_;
        bool symbolResolved_;
    };

    template <TraceTaskCodes::Enum traceTaskId>
    static void CheckCallDuration(
        std::function<void(void)> const & call,
        Common::TimeSpan threshold,
        StringLiteral traceType,
        std::wstring const & traceId,
        wchar_t const* api,
        std::function<void(wchar_t const* api, TimeSpan duration, TimeSpan threshold)> const & onSlowCall = nullptr)
    {
        auto beforeCall = Stopwatch::Now();
        call();
        auto duration = Stopwatch::Now() - beforeCall;
        if (threshold <= duration)
        {
            TextTraceComponent<traceTaskId>::WriteWarning(
                traceType,
                traceId,
                "{0} is slow, duration = {1}, threshold = {2}",
                api,
                duration,
                threshold);

            if (onSlowCall)
            {
                onSlowCall(api, duration, threshold);
            }
        }
    }
}
