// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<bool ManualReset>
    class WaitHandle : public Common::TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(WaitHandle);

    public:
        WaitHandle(bool initialState = false, std::wstring = L"");
        virtual ~WaitHandle();

        ErrorCode Set();
        bool IsSet();
        ErrorCode Reset();

        bool WaitOne(uint millisecondsTimeout = INFINITE);
        bool WaitOne(TimeSpan timeout);
        ErrorCode Wait(uint millisecondsTimeout = INFINITE);
        ErrorCode Wait(TimeSpan timeout);

#ifndef PLATFORM_UNIX
        ::HANDLE GetHandle() const { return handle_; }
#endif

        void swap(WaitHandle &rhs) throw();

        void Close();

     protected:

#ifdef PLATFORM_UNIX

        virtual void OnClose() {}
        virtual void OnSetEvent() {}

        pthread_cond_t cond_;
        pthread_mutex_t mutex_;
        bool signaled_ = false;
        bool closed_ = false;
#else

        ::HANDLE handle_;
        bool closed_ = false;
        std::wstring eventName_;
#endif
    };

    using AutoResetEvent = WaitHandle<false>;
    using ManualResetEvent = WaitHandle<true>;
};
