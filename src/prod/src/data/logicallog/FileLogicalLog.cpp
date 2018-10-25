// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace ktl::io;
using namespace Data::Log;
using namespace Data::Utilities;

const ULONG FileLogicalLog::readStreamListOffset_ = FIELD_OFFSET(FileLogicalLogReadStream, listEntry_);

#define VERIFY_SUCCESS_ASSERT(status, msg) \
    if (!NT_SUCCESS(status)) \
    { \
        KDbgErrorWStatus(PtrToActivityId(this), msg, status); \
        CODING_ERROR_ASSERT(FALSE); \
    }

#define VERIFY_SUCCESS_RETURN(status, msg) \
    if (!NT_SUCCESS(status)) \
    { \
        KDbgErrorWStatus(PtrToActivityId(this), msg, status); \
        return status; \
    }

#define VERIFY_SUCCESS_CORETURN(status, msg) \
    if (!NT_SUCCESS(status)) \
    { \
        KDbgErrorWStatus(PtrToActivityId(this), msg, status); \
        co_return status; \
    }

FAILABLE
FileLogicalLog::FileLogicalLog() 
    : readStreamList_(readStreamListOffset_)
{
    NTSTATUS status;
    ReaderWriterAsyncLock::SPtr lock;

    status = ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), lock);
    if (!NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "ReaderWriterAsyncLock::Create", status);
        SetConstructorStatus(status);
        return;
    }

    writeStreamLock_ = Ktl::Move(lock);
}

FileLogicalLog::~FileLogicalLog()
{
}

NTSTATUS FileLogicalLog::Create(
    __in KAllocator& allocator,
    __out FileLogicalLog::SPtr& fileLogicalLog)
{
    NTSTATUS status;
    FileLogicalLog::SPtr context;

    context = _new(FILELOG_TAG, allocator) FileLogicalLog();
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

    fileLogicalLog = Ktl::Move(context);

    return STATUS_SUCCESS;
}

// assumption: only called once, does not guard against multiple calls
ktl::Awaitable<NTSTATUS> FileLogicalLog::OpenAsync(
    __in KWString&  logFileName,
    __in CancellationToken const & cancellationToken
    )
{
    NTSTATUS status;
    LONGLONG endOfFile = 0;

    status = co_await KBlockFile::CreateSparseFileAsync(logFileName,
                                          FALSE,              // IsWriteThrough
                                          KBlockFile::eOpenAlways,
                                          logFile_,
                                          nullptr,
                                          GetThisAllocator(),
                                          GetThisAllocationTag());
    VERIFY_SUCCESS_CORETURN(status, "CreateSparseFileAsync");

    status = co_await logFile_->GetEndOfFileAsync(endOfFile);
    VERIFY_SUCCESS_CORETURN(status, "GetEndOfFileAsync");

    status = ktl::io::KFileStream::Create(writeStream_, GetThisAllocator(), GetThisAllocationTag());
    VERIFY_SUCCESS_CORETURN(status, "KFileStream::Create");

    status = co_await writeStream_->OpenAsync(*logFile_, cacheSize_, fileExtendSize_, nullptr, nullptr);
    VERIFY_SUCCESS_CORETURN(status, "writeStream_->OpenAsync");

    writeStream_->SetPosition(endOfFile);
    
    lastFlushStreamOffset_ = endOfFile;
    
    co_return STATUS_SUCCESS;
}

// write API, assumes write lock is held.  todo: will not be a write API after removing writestream invalidate
Awaitable<void> FileLogicalLog::InvalidateStreamsForWriteAsync(
    __in LONGLONG StreamOffset,
    __in LONGLONG Length
    )
{
    NTSTATUS status;
    FileLogicalLogReadStream* readStream;
    
    K_LOCK_BLOCK(readStreamListLock_)
    {
        readStream = readStreamList_.PeekHead();
        while (readStream)
        {
            KDbgPrintfInformational("%llu Invalidating read stream %llu StreamOffset: %llu Length: %llu", PtrToActivityId(this), PtrToActivityId(readStream), StreamOffset, Length);
            status = co_await readStream->InvalidateForWriteAsync(StreamOffset, Length);
            VERIFY_SUCCESS_ASSERT(status, "readStream->InvalidateForWriteAsync");
            readStream = readStreamList_.Successor(readStream);
        }
    }

    // todo: remove after we are confident that there is no bug
    CODING_ERROR_ASSERT(writeStreamLock_->IsActiveWriter == TRUE); // assumption: write lock is held

    KDbgPrintfInformational("%llu Invalidating write stream.  StreamOffset: %llu, Length: %llu", PtrToActivityId(this), StreamOffset, Length);
    status = writeStream_->InvalidateForWrite(StreamOffset, Length);
    VERIFY_SUCCESS_ASSERT(status, "writeStream_->InvalidateForWrite");

    co_return;
}

Awaitable<void> FileLogicalLog::InvalidateStreamsForTruncateAsync(
    __in LONGLONG StreamOffset
    )
{
    NTSTATUS status;
    FileLogicalLogReadStream* readStream;

    K_LOCK_BLOCK(readStreamListLock_)
    {
        readStream = readStreamList_.PeekHead();
        while (readStream)
        {
            KDbgPrintfInformational("%llu Invalidating read stream %llu for truncate.  StreamOffset: %llu", PtrToActivityId(this), PtrToActivityId(readStream), StreamOffset);
            status = co_await readStream->InvalidateForTruncateAsync(StreamOffset);
            VERIFY_SUCCESS_ASSERT(status, "readStream->InvalidateForTruncate");
            readStream = readStreamList_.Successor(readStream);
        }
    }

    CODING_ERROR_ASSERT(writeStreamLock_->IsActiveWriter == TRUE); // assumption: write lock is held

    KDbgPrintfInformational("%llu Invalidating write stream for truncate.  StreamOffset: %llu", PtrToActivityId(this), StreamOffset);
    status = writeStream_->InvalidateForTruncate(StreamOffset);
    VERIFY_SUCCESS_ASSERT(status, "writeStream_->InvalidateForTruncate");

    co_return;
}

VOID FileLogicalLog::AddReadStreamToList(
    __in FileLogicalLogReadStream& ReadStream
    )
{
    FileLogicalLogReadStream::SPtr readStreamSPtr = &ReadStream;

    //
    // Take ref count to account for the read stream being added to the
    // list. The ref count is release when it is removed from the list.
    //
    readStreamSPtr->AddRef();
    K_LOCK_BLOCK(readStreamListLock_)
    {
        readStreamList_.AppendTail(&ReadStream);
    }
}

VOID FileLogicalLog::RemoveReadStreamFromList(
    __in FileLogicalLogReadStream& ReadStream
    )
{
    FileLogicalLogReadStream::SPtr readStreamSPtr = &ReadStream;
    
    K_LOCK_BLOCK(readStreamListLock_)
    {
        readStreamList_.Remove(&ReadStream);
    }

    //
    // Release the ref count taken when the read stream was added to
    // the list.
    //
    readStreamSPtr->Release();
}

//
// ILogicalLog Interface
//
// write API, takes write lock (and closes it)
ktl::Awaitable<NTSTATUS> FileLogicalLog::CloseAsync(
    __in CancellationToken const & cancellationToken
)
{
    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&] 
    {
        writeStreamLock_->Close();
        writeStreamLock_->ReleaseWriteLock(); 
    });

    status = co_await InternalFlushWithMarkerAsync();
    VERIFY_SUCCESS_CORETURN(status, "InternalFlushWithMarkerAsync");

    FileLogicalLogReadStream* readStream = nullptr;
    do
    {
        
        K_LOCK_BLOCK(readStreamListLock_)
        {
            readStream = readStreamList_.RemoveHead();
        }

        if (readStream)
        {         
            status = co_await readStream->CloseAsync();
            VERIFY_SUCCESS_CORETURN(status, "readStream->CloseAsync()");
        }
    } while (readStream);

    status = co_await writeStream_->CloseAsync();
    VERIFY_SUCCESS_CORETURN(status, "writeStream->CloseAsync()");
    
    writeStream_ = nullptr;

    logFile_->Close();
    logFile_ = nullptr;
    
    co_return STATUS_SUCCESS;
}

// write API, takes write lock (and closes it)
Task FileLogicalLog::AbortTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&]
    {
        writeStreamLock_->Close();
        writeStreamLock_->ReleaseWriteLock();
    });

    status = co_await InternalFlushWithMarkerAsync();
    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "AbortTask_InternalFlushWithMarkerAsync", status);
    }

    FileLogicalLogReadStream* readStream = nullptr;
    do
    {
        
        K_LOCK_BLOCK(readStreamListLock_)
        {
            readStream = readStreamList_.RemoveHead();
        }

        if (readStream)
        {
            //
            // Release reference taken when added to list
            //
            KFinally([&] { readStream->Release(); });
            
            status = co_await readStream->CloseAsync();
            if (! NT_SUCCESS(status))
            {
                KDbgErrorWStatus(PtrToActivityId(this), "readStream->CloseAsync", status);
            }
        }
    } while (readStream);

    status = co_await writeStream_->CloseAsync();

    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(PtrToActivityId(this), "writeStream->CloseAsync", status);
    }
    
    writeStream_ = nullptr;

    logFile_->Close();
    logFile_ = nullptr;
}

void FileLogicalLog::Abort()
{
    AbortTask();      // CONSIDER: Make Abort() a Task 
}

VOID FileLogicalLog::Dispose()
{
    Abort();
}

BOOLEAN FileLogicalLog::GetIsFunctional()
{
    return(TRUE);
}

LONGLONG FileLogicalLog::GetLength() const
{
    //
    // Since the logical log is append only, the length of the log is
    // the same as the next write position
    //
    return(writeStream_->GetPosition());    
}

LONGLONG FileLogicalLog::GetWritePosition() const
{
    return(writeStream_->GetPosition());
}

LONGLONG FileLogicalLog::GetReadPosition() const
{
    // Not Implemented
    return 0;
}

LONGLONG FileLogicalLog::GetHeadTruncationPosition() const
{
    // Not Implemented
    return -1;
}

LONGLONG FileLogicalLog::GetMaximumBlockSize() const
{
    // Not Implemented
    return -1;
}

ULONG FileLogicalLog::GetMetadataBlockHeaderSize() const
{
    // Not Implemented
    return static_cast<ULONG>(-1);
}

ULONGLONG FileLogicalLog::GetSize() const
{
    LONGLONG length = GetLength();
    KInvariant(length >= 0);
    return static_cast<ULONGLONG>(length);
}

ULONGLONG FileLogicalLog::GetSpaceRemaining() const
{
    return ULONGLONG_MAX - GetSize();
}

NTSTATUS FileLogicalLog::CreateReadStream(
    __out ILogicalLogReadStream::SPtr& Stream,
    __in LONG SequentialAccessReadSize
    )
{
    UNREFERENCED_PARAMETER(SequentialAccessReadSize);
    
    NTSTATUS status;
    ktl::io::KFileStream::SPtr fileStream;
    FileLogicalLogReadStream::SPtr readStream;

    status = ktl::io::KFileStream::Create(fileStream,
                                          GetThisAllocator(),
                                          GetThisAllocationTag());
    VERIFY_SUCCESS_RETURN(status, "KFileStream::Create");

    status = FileLogicalLogReadStream::Create(readStream,
                                              *logFile_,
                                              *this,
                                              *fileStream,
                                              GetThisAllocator(),
                                              GetThisAllocationTag());
    VERIFY_SUCCESS_RETURN(status, "FileLogicalLogReadStream::Create");

    AddReadStreamToList(*readStream);

    Stream = readStream.RawPtr();

    return STATUS_SUCCESS;
}

void FileLogicalLog::SetSequentialAccessReadSize(
    __in ILogicalLogReadStream& LogStream,
    __in LONG SequentialAccessReadSize
    )
{
    UNREFERENCED_PARAMETER(LogStream);
    UNREFERENCED_PARAMETER(SequentialAccessReadSize);
    
    // Not Implemented
}

ktl::Awaitable<NTSTATUS> FileLogicalLog::ReadAsync(
    __out LONG& BytesRead,
    __in KBuffer& Buffer,
    __in ULONG Offset,
    __in ULONG Count,
    __in ULONG BytesToRead,
    __in CancellationToken const &
    )
{
    UNREFERENCED_PARAMETER(BytesRead);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(BytesToRead);
    
    // Not Implemented
    co_return STATUS_UNSUCCESSFUL;
}

NTSTATUS FileLogicalLog::SeekForRead(
    __in LONGLONG Offset,
    __in Common::SeekOrigin::Enum Origin
    )
{
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Origin);
    
    // Not Implemented
    return STATUS_UNSUCCESSFUL;
}

// write API, takes write lock
ktl::Awaitable<NTSTATUS> FileLogicalLog::AppendAsync(
    __in KBuffer const & Buffer,
    __in LONG OffsetIntoBuffer,
    __in ULONG Count,
    __in CancellationToken const &
    )
{
    if (OffsetIntoBuffer < 0)
    {
        co_return STATUS_INVALID_PARAMETER_2;
    }
    
    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {writeStreamLock_->ReleaseWriteLock(); });

    status = co_await writeStream_->WriteAsync(Buffer, OffsetIntoBuffer, Count);
    VERIFY_SUCCESS_CORETURN(status, "writeStream_->WriteAsync");

    co_return STATUS_SUCCESS;
}

// write API, takes write lock
ktl::Awaitable<NTSTATUS> FileLogicalLog::FlushAsync(
    __in CancellationToken const &
    )
{
    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {writeStreamLock_->ReleaseWriteLock(); });

    status = co_await InternalFlushWithMarkerAsync();
    VERIFY_SUCCESS_CORETURN(status, "FlushAsync_InternalFlushWithMarkerAsync");

    co_return STATUS_SUCCESS;
}

// write API, takes write lock
ktl::Awaitable<NTSTATUS> FileLogicalLog::FlushWithMarkerAsync(
    __in CancellationToken const &
    )
{
    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {writeStreamLock_->ReleaseWriteLock(); });

    status = co_await InternalFlushWithMarkerAsync();
    VERIFY_SUCCESS_CORETURN(status, "FlushWithMarkerAsync_InternalFlushWithMarkerAsync");
    
    co_return STATUS_SUCCESS;
}

// write API, assumes write lock is held
ktl::Awaitable<NTSTATUS> FileLogicalLog::InternalFlushWithMarkerAsync()
{
    NTSTATUS status;

    CODING_ERROR_ASSERT(writeStreamLock_->IsActiveWriter == TRUE); // assumption: write lock is held

    // Flush the write stream
    {
        status = co_await writeStream_->FlushAsync();
        if (status == STATUS_INVALID_STATE_TRANSITION)
        {
            // write buffer is invalid, nothing to flush
            // todo: remove after ingesting new kfilestream version, which returns success for this case since it's a no-op
            co_return STATUS_SUCCESS;
        }
        VERIFY_SUCCESS_CORETURN(status, "writeStream_->FlushAsync");
    }

    //
    // Need to invalidate the read streams
    //
    LONGLONG newFlushStreamOffset = writeStream_->GetPosition();
    LONGLONG flushLength = newFlushStreamOffset - lastFlushStreamOffset_;

    if (flushLength != 0)
    {
        co_await InvalidateStreamsForWriteAsync(lastFlushStreamOffset_, flushLength);
    }
    lastFlushStreamOffset_ = newFlushStreamOffset;

    co_return STATUS_SUCCESS;
}

// write API, takes write lock
ktl::Awaitable<NTSTATUS> FileLogicalLog::TruncateHead(
    __in LONGLONG StreamOffset
    )
{
    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {writeStreamLock_->ReleaseWriteLock(); });

    KDbgPrintfInformational("%llu Flushing as a part of TruncateHead.  StreamOffset: %llu", PtrToActivityId(this), StreamOffset);
    status = co_await InternalFlushWithMarkerAsync();
    VERIFY_SUCCESS_CORETURN(status, "TruncateHead_InternalFlushWithMarkerAsync");
    
    KDbgPrintfInformational("%llu Trimming the file.", PtrToActivityId(this));
    status = co_await logFile_->TrimAsync(0, StreamOffset);
    VERIFY_SUCCESS_CORETURN(status, "TrimAsync");

    co_await InvalidateStreamsForWriteAsync(0, StreamOffset);
    
    co_return status;
}

// write API, takes write lock
ktl::Awaitable<NTSTATUS> FileLogicalLog::TruncateTail(
    __in LONGLONG StreamOffset,
    __in CancellationToken const &
    )
{
    
    NTSTATUS status;

    BOOL acquired = co_await writeStreamLock_->AcquireWriteLockAsync(defaultLockTimeoutMs_);
    CODING_ERROR_ASSERT(acquired);

    KFinally([&] {writeStreamLock_->ReleaseWriteLock(); });

    status = co_await InternalFlushWithMarkerAsync();
    VERIFY_SUCCESS_CORETURN(status, "TruncateTail_InternalFlushWithMarkerAsync");

    LONGLONG eofRoundedToBlock;
    if (StreamOffset % KBlockFile::BlockSize == 0)
    {
        eofRoundedToBlock = StreamOffset;
    }
    else
    {
        eofRoundedToBlock = StreamOffset + (KBlockFile::BlockSize - StreamOffset % KBlockFile::BlockSize);
    }

    status = co_await logFile_->SetFileSizeAsync(eofRoundedToBlock);
    VERIFY_SUCCESS_CORETURN(status, "SetFileSizeAsync");

    status = co_await logFile_->SetEndOfFileAsync(StreamOffset);
    VERIFY_SUCCESS_CORETURN(status, "SetEndOfFileAsync");

    co_await InvalidateStreamsForTruncateAsync(StreamOffset);

    if (StreamOffset < lastFlushStreamOffset_)
    {
        lastFlushStreamOffset_ = StreamOffset;
    }

    writeStream_->SetPosition(StreamOffset);
    
    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> FileLogicalLog::WaitCapacityNotificationAsync(
    __in ULONG PercentOfSpaceUsed,
    __in CancellationToken const &
    )
{
    // Not Implemented
    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> FileLogicalLog::WaitBufferFullNotificationAsync(
    __in CancellationToken const &
    )
{
    // Not Implemented - CONSIDER: removing this 
    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> FileLogicalLog::ConfigureWritesToOnlyDedicatedLogAsync(
    __in CancellationToken const &
    )
{ 
    // Not Implemented
    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> FileLogicalLog::ConfigureWritesToSharedAndDedicatedLogAsync(
    __in CancellationToken const &
    )
{
    // Not Implemented
    co_return STATUS_NOT_IMPLEMENTED;
}

ktl::Awaitable<NTSTATUS> FileLogicalLog::QueryLogUsageAsync(
    __out ULONG& PercentUsage, __in CancellationToken const &
    )
{
    // Not Implemented
    co_return STATUS_NOT_IMPLEMENTED;
}
