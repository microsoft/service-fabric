// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    OverlappedIo::OverlappedIo()
    {
        ZeroMemory(&overlapped_, sizeof(OVERLAPPED));
    }

    OverlappedIo::~OverlappedIo()
    {
    }

    LPOVERLAPPED OverlappedIo::OverlappedPtr()
    {
        return &overlapped_;
    }

    void CALLBACK OverlappedIo::Callback(
        PTP_CALLBACK_INSTANCE instance/*instance*/,
        PVOID /*context*/,
        PVOID overlapped,
        ULONG result,
        ULONG_PTR bytesTransferred,
        PTP_IO /*io*/)
    {
        Invariant(overlapped);
        OverlappedIo* thisPtr = (OverlappedIo*)((PBYTE)overlapped - FIELD_OFFSET(OverlappedIo, overlapped_));
        thisPtr->OnComplete(instance, ErrorCode::FromWin32Error(result), bytesTransferred);
    }

    void OverlappedIo::Complete(ErrorCode const & error, ULONG_PTR bytesTransferred)
    {
        OnComplete(NULL, error, bytesTransferred);
    }

    ThreadpoolIo::ThreadpoolIo() : io_(nullptr), handle_(nullptr)
    {
    }

    ThreadpoolIo::~ThreadpoolIo()
    {
        Close();
    }

    ErrorCode ThreadpoolIo::Open(HANDLE handle)
    {
        io_ = ::CreateThreadpoolIo(
            handle,
            &Threadpool::IoCallback<OverlappedIo>,
            nullptr,
            nullptr);

        if (io_ == nullptr)
        {
            return ErrorCode::FromWin32Error();
        }

        handle_ = handle;
        return ErrorCodeValue::Success;
    }

    void ThreadpoolIo::Close()
    {
        if (io_)
        {
            ::CancelIoEx(handle_, nullptr);
            ::WaitForThreadpoolIoCallbacks(io_, false);
            ::CloseThreadpoolIo(io_);
            io_ = nullptr;
        }
    }

    _Use_decl_annotations_
    void ThreadpoolIo::Swap(ThreadpoolIo & other)
    {
        HANDLE handle = handle_;
        handle_ = other.handle_;
        other.handle_ = handle;

        PTP_IO io = io_;
        io_ = other.io_;
        other.io_ = io;
    }

    bool ThreadpoolIo::operator!() const
    {
        return !io_;
    }

    void ThreadpoolIo::StartIo()
    {
        StartThreadpoolIo(io_);
    }

    void ThreadpoolIo::CancelIo()
    {
        CancelThreadpoolIo(io_);
    }
}
