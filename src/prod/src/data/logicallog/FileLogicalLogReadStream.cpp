// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::Utilities;

//
// FileLogicalLogReadStream
//
FileLogicalLogReadStream::FileLogicalLogReadStream()
{
}

FileLogicalLogReadStream::~FileLogicalLogReadStream()
{
    CODING_ERROR_ASSERT(initialized_ == FALSE);
}

VOID FileLogicalLogReadStream::Dispose()
{
    DisposeTask();
}

Task FileLogicalLogReadStream::DisposeTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive
    co_await CloseAsync();
}

NTSTATUS FileLogicalLogReadStream::Create(
    __out FileLogicalLogReadStream::SPtr& readStream,
    __in KBlockFile& logFile,
    __in FileLogicalLog& fileLogicalLog,
    __in ktl::io::KFileStream& underlyingStream,
    __in KAllocator& allocator,
    __in ULONG allocationTag)
{
    NTSTATUS status;
    FileLogicalLogReadStream::SPtr context;

    context = _new(allocationTag, allocator) FileLogicalLogReadStream(logFile, fileLogicalLog, underlyingStream);
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;      
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    readStream = Ktl::Move(context);

    return STATUS_SUCCESS;
}

FAILABLE
FileLogicalLogReadStream::FileLogicalLogReadStream(
    __in KBlockFile& logFile,
    __in FileLogicalLog& fileLogicalLog,
    __in ktl::io::KFileStream& underlyingStream)
{
    NTSTATUS status;
    ReaderWriterAsyncLock::SPtr lock;
    
    status = ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), lock);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    apiLock_ = Ktl::Move(lock);

    logFile_ = &logFile;
    fileLogicalLog_ = &fileLogicalLog;
    underlyingStream_ = &underlyingStream;
    initialized_ = FALSE;
}

NTSTATUS FileLogicalLogReadStream::InvalidateForWrite(
    __in LONGLONG StreamOffset,
    __in LONGLONG Length)
{
    CODING_ERROR_ASSERT(apiLock_->IsActiveWriter);

    NTSTATUS status;
    
    status = underlyingStream_->InvalidateForWrite(StreamOffset, Length);
    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->InvalidateForWrite", status);
        return status;
    }
    
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLogicalLogReadStream::InvalidateForWriteAsync(
    __in LONGLONG StreamOffset,
    __in LONGLONG Length)
{
    BOOL acquired = co_await apiLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);

    CODING_ERROR_ASSERT(acquired);

    KFinally([&]
    {
        apiLock_->ReleaseWriteLock();
    });

    co_return InvalidateForWrite(StreamOffset, Length);
}

NTSTATUS FileLogicalLogReadStream::InvalidateForTruncate(__in LONGLONG StreamOffset)
{
    CODING_ERROR_ASSERT(apiLock_->IsActiveWriter);

    NTSTATUS status;

    status = underlyingStream_->InvalidateForTruncate(StreamOffset);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->InvalidateForWrite", status);
        return status;
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLogicalLogReadStream::InvalidateForTruncateAsync(__in LONGLONG StreamOffset)
{
    BOOL acquired = co_await apiLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);

    CODING_ERROR_ASSERT(acquired);

    KFinally([&]
    {
        apiLock_->ReleaseWriteLock();
    });

    co_return InvalidateForTruncate(StreamOffset);
}

ktl::Awaitable<NTSTATUS> FileLogicalLogReadStream::Initialize()
{
    NTSTATUS status;
    
    if (!initialized_)
    {
        status = co_await underlyingStream_->OpenAsync(*logFile_);
        if (! NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->OpenAsync", status);
            co_return status;
        }     

        initialized_ = TRUE;
    }
    
    co_return STATUS_SUCCESS;
}

//
// KStream interface
//
ktl::Awaitable<NTSTATUS> FileLogicalLogReadStream::CloseAsync()
{
    NTSTATUS status;

    BOOL acquired = co_await apiLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);

    CODING_ERROR_ASSERT(acquired);

    KFinally([&]
    {
        apiLock_->ReleaseWriteLock();
    });

    if (initialized_ == FALSE)
    {
        co_return K_STATUS_API_CLOSED;
    }

    if (InterlockedCompareExchange(&closed_, 1, 0) == 0)
    {
        fileLogicalLog_->RemoveReadStreamFromList(*this);
        fileLogicalLog_ = nullptr;

        status = co_await underlyingStream_->CloseAsync();
        if (!NT_SUCCESS(status))
        {
            KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->CloseAsync", status);
            co_return status;
        }

        initialized_ = FALSE;
    }

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> FileLogicalLogReadStream::ReadAsync(
    __in KBuffer& Buffer,
    __out ULONG& BytesRead,
    __in ULONG Offset,
    __in ULONG Count
)
{
    BOOL acquired = co_await apiLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);

    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {apiLock_->ReleaseWriteLock(); });

    NTSTATUS status;

    status = co_await Initialize();
    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "Initialize", status);
        co_return status;
    }
    
    status = co_await underlyingStream_->ReadAsync(Buffer, BytesRead, Offset, Count);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->ReadAsync", status);
        co_return status;
    }

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> FileLogicalLogReadStream::WriteAsync(
    __in KBuffer const & Buffer,
    __in ULONG Offset,
    __in ULONG Count
)
{
    BOOL acquired = co_await apiLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);

    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {apiLock_->ReleaseWriteLock(); });

    NTSTATUS status;

    status = co_await Initialize();
    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "Initialize", status);
        co_return status;
    }

    status = co_await underlyingStream_->WriteAsync(Buffer, Offset, Count);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->WriteAsync", status);
        co_return status;
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLogicalLogReadStream::FlushAsync()
{
    BOOL acquired = co_await apiLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);

    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {apiLock_->ReleaseWriteLock(); });

    NTSTATUS status;

    status = co_await Initialize();
    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "Initialize", status);
        co_return status;
    }

    status = co_await underlyingStream_->FlushAsync();
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "underlyingStream_->FlushAsync", status);
        co_return status;
    }

    co_return status;
}

LONGLONG FileLogicalLogReadStream::GetLength() const
{
    return fileLogicalLog_->GetLength();
}

LONGLONG FileLogicalLogReadStream::GetPosition() const
{
    return underlyingStream_->GetPosition();
}

void FileLogicalLogReadStream::SetPosition(__in LONGLONG Position)
{
    // Must not be racing with anything.  Can't acquire the lock sync without deadlock.
    CODING_ERROR_ASSERT(!apiLock_->IsActiveWriter);

    underlyingStream_->SetPosition(Position);
}
