// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ThreadpoolWait;
    typedef std::shared_ptr<ThreadpoolWait> ThreadpoolWaitSPtr;

    //
    // Asynchronously waits for the specified handle to be signalled using the ThreadpoolWait APIs. These 
    // APIs provide an effiecient way to wait asynchronously for multiple handles. The Windows API performs 
    // the thread management functions and uses a single thread to wait for up to 64 handles.
    //
    class ThreadpoolWait
        : public std::enable_shared_from_this<ThreadpoolWait>
        , Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
        DENY_COPY(ThreadpoolWait);

    public:
        typedef std::function<void(Common::Handle const &, Common::ErrorCode const & waitResult)> WaitCallback;

        static ThreadpoolWaitSPtr Create(Common::Handle && handle, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);

        ThreadpoolWait(Common::Handle && handle, WaitCallback const & callback, Common::TimeSpan timeout = Common::TimeSpan::MaxValue);
        ~ThreadpoolWait();

        void Cancel();

    private:
        class ThreadpoolWaitImpl;
        typedef std::shared_ptr<ThreadpoolWaitImpl> ThreadpoolWaitImplSPtr;

        ThreadpoolWaitImplSPtr impl_;
    };
}
