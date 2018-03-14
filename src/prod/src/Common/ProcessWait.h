// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ProcessWait;
    typedef std::shared_ptr<ProcessWait> ProcessWaitSPtr;

    class ProcessWaitImpl;
    typedef std::shared_ptr<ProcessWaitImpl> ProcessWaitImplSPtr;

    // On Windows, any process can be waited.
    // On Linux, only child processes can be waited.
    class ProcessWait
        : public std::enable_shared_from_this<ProcessWait>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
        DENY_COPY(ProcessWait);

    public:
        typedef std::function<void(pid_t, Common::ErrorCode const & waitResult, DWORD processExitCode)> WaitCallback;

        //LINUXTODO, consider getting pid from handle instead of passing a seperate parameter
        static ProcessWaitSPtr Create();
        static ProcessWaitSPtr CreateAndStart(pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        static ProcessWaitSPtr CreateAndStart(Common::Handle && handle, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        static ProcessWaitSPtr CreateAndStart(Common::Handle && handle, pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);

        ProcessWait();
        ProcessWait(pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        ProcessWait(Common::Handle && handle, pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        ~ProcessWait();

        void StartWait();
        void StartWait(pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        void StartWait(Handle && handle, pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        void Cancel();

#ifdef PLATFORM_UNIX
        static void Setup();
        static size_t Test_CacheSize(); 
        static void Test_Reset();

    private:
        void CreateImpl(Handle && handle, pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout);

        ProcessWaitImplSPtr impl_;
#else
    private:
        ProcessWaitImplSPtr impl_;
#endif
    };
}
