// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/WaitHandle.h"

using namespace Common;

static const StringLiteral TraceType("WaitHandle");

template <bool ManualReset>
bool WaitHandle<ManualReset>::WaitOne(uint timeout)
{
    return Wait(timeout).IsSuccess();
}

template <bool ManualReset>
bool WaitHandle<ManualReset>::WaitOne(TimeSpan timeout)
{
    return Wait(timeout).IsSuccess();
}

static uint TimeSpanToMilliseconds(TimeSpan timeout)
{
    return (timeout == TimeSpan::MaxValue) ? INFINITE : (uint)timeout.TotalPositiveMilliseconds();
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Wait(TimeSpan timeout)
{
    return Wait(TimeSpanToMilliseconds(timeout));
}

template <bool ManualReset>
bool WaitHandle<ManualReset>::IsSet()
{
    return WaitOne(0);
}

#ifdef PLATFORM_UNIX

template <bool ManualReset>
WaitHandle<ManualReset>::WaitHandle(bool initialState, std::wstring eventName) : signaled_(initialState)
{
    // @TODO - eventName isn't actually supported in linux. we need to find another way
    UNREFERENCED_PARAMETER(eventName);
    pthread_cond_init(&cond_, nullptr);
    pthread_mutex_init(&mutex_, nullptr);
}

template <bool ManualReset>
WaitHandle<ManualReset>::~WaitHandle()
{
    Close();

    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);
}

template <bool ManualReset>
void WaitHandle<ManualReset>::Close() 
{
    if (closed_) return;

    {
        pthread_mutex_lock(&mutex_);
        KFinally([this] { pthread_mutex_unlock(&mutex_); });

        if (closed_) return;

        closed_ = true;
        signaled_ = true;

        pthread_cond_broadcast(&cond_);
    }

    OnClose();
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Wait(uint timeout)
{
    timeval now = {};
    ZeroRetValAssert(gettimeofday(&now, nullptr));
    timespec to =
    {
        .tv_sec = now.tv_sec + (now.tv_usec + timeout * 1e3)/1e6,
        .tv_nsec = ((uint64)(now.tv_usec + timeout * 1e3) % 1000000) * 1e3
    };

    WriteNoise(
        TraceType, "{0}({1}): wait abstime: tv_sec = {2}, tv_nsec = {3}",
        TextTraceThis, ManualReset, to.tv_sec, to.tv_nsec);

    pthread_mutex_lock(&mutex_);
    KFinally([this] { pthread_mutex_unlock(&mutex_); });

    while (!signaled_ && !closed_)
    {
        // "to" is in absolute time (CLOCK_REALTIME), Linux will convert it to
        // CLOCK_MONOTONIC internally, which will not be affected by clock adjustment.
        // The small vulnerable window is between gettimeofday and the conversion.
        auto retval = pthread_cond_timedwait(&cond_, &mutex_, &to);
        if (retval == EINTR) continue;

        if (retval == ETIMEDOUT)
        {
            if (signaled_ || closed_) break;
    
            return ErrorCodeValue::Timeout;
        }
    }

    if (!ManualReset) signaled_ = false;

    return ErrorCodeValue::Success;
}

template <bool ManualReset>
void WaitHandle<ManualReset>::swap(WaitHandle &rhs) throw()
{
    std::swap(cond_, rhs.cond_);
    std::swap(mutex_, rhs.mutex_);
    std::swap(signaled_, rhs.signaled_);
    std::swap(closed_, rhs.closed_);
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Set()
{
    WriteNoise(TraceType, "{0}({1}): set", TextTraceThis, ManualReset);

    pthread_mutex_lock(&mutex_);
    KFinally([this] { pthread_mutex_unlock(&mutex_); });

    signaled_ = true;
    OnSetEvent();

    if (ManualReset)
    {
        pthread_cond_broadcast(&cond_);
        return ErrorCodeValue::Success;
    }

    pthread_cond_signal(&cond_);
    return ErrorCodeValue::Success;
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Reset()
{
    pthread_mutex_lock(&mutex_);
    KFinally([this] { pthread_mutex_unlock(&mutex_); });

    signaled_ = false;

    return ErrorCodeValue::Success;
}

#else

template <bool ManualReset>
WaitHandle<ManualReset>::WaitHandle(bool initialState, std::wstring eventName)
{
    handle_ = CreateEventW(
        nullptr,
        ManualReset ? TRUE : FALSE,
        initialState,
        eventName.empty() ? nullptr : eventName.c_str());

    ASSERT_IFNOT(handle_, "CreateEvent failed: {0}", ErrorCode::FromWin32Error());
}

template <bool ManualReset>
WaitHandle<ManualReset>::~WaitHandle()
{
    Close();
}

template <bool ManualReset>
void WaitHandle<ManualReset>::Close() 
{
    if ( nullptr != handle_)
    {
        closed_ = true;
        CHK_WBOOL( ::CloseHandle( handle_ ));
        handle_ = nullptr;
    }
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Wait(uint timeout)
{
    if (closed_) return ErrorCodeValue::Success;

    Invariant(handle_);

    auto status = ::WaitForSingleObject(handle_, timeout);

    if (status == WAIT_OBJECT_0)
        return ErrorCodeValue::Success;

    if (status == WAIT_TIMEOUT)
        return ErrorCodeValue::Timeout;

    if (status == WAIT_ABANDONED)
        THROW(WinError(status), "object was abandoned");

    THROW(WinError(::GetLastError()), "WaitForSingleObject failed");
}

template <bool ManualReset>
void WaitHandle<ManualReset>::swap(WaitHandle &rhs) throw()
{
    std::swap(handle_, rhs.handle_);
    std::swap(closed_, rhs.closed_);
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Set()
{
    ErrorCode err;
    if(!::SetEvent(handle_))
    {
        err = ErrorCode::FromWin32Error();
        TRACE_ERROR_AND_ASSERT(TraceType, "SetEvent failed: {0}", err); 
    }

    return err;
}

template <bool ManualReset>
ErrorCode WaitHandle<ManualReset>::Reset()
{
    ErrorCode err;
    if (!::ResetEvent(handle_))
    {
        err = ErrorCode::FromWin32Error();
        TRACE_ERROR_AND_ASSERT(TraceType, "ResetEvent failed: {0}", err); 
    }

    return err;
}

#endif

namespace Common
{
    template class WaitHandle<false>;
    template class WaitHandle<true>;
}
