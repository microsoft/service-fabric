// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LoggingReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("FileLog");

class FileLog::ReadStream
    : public KObject<ReadStream>
    , public KShared<ReadStream>
    , public ILogicalLogReadStream
{
    K_FORCE_SHARED(ReadStream)
    K_SHARED_INTERFACE_IMP(KStream)
    K_SHARED_INTERFACE_IMP(ILogicalLogReadStream)
    K_SHARED_INTERFACE_IMP(IDisposable)

public:

    static ReadStream::SPtr Create(
        std::wstring const & fileName,
        KAllocator & allocator)
    {
        ReadStream::SPtr result = _new(FILELOGMANAGER_TAG, allocator) ReadStream(fileName);

        ASSERT_IFNOT(
            result,
            "Failed to create file log read stream");

        return result;
    }

    LONGLONG GetPosition() const override
    {
        return logFile_.Seek(0, Common::SeekOrigin::Current);
    }

    void SetPosition(__in LONGLONG position) override
    {
        logFile_.Seek(position, Common::SeekOrigin::Begin);
    }

    LONGLONG GetLength() const override
    {
        auto currentPos = logFile_.Seek(0, Common::SeekOrigin::Current);
        auto length = logFile_.Seek(0, Common::SeekOrigin::End);
        logFile_.Seek(currentPos, Common::SeekOrigin::Begin);

        return length;
    }

    void Dispose() override
    {
        logFile_.Close2();
    }

    Awaitable<NTSTATUS> CloseAsync() override
    {
        Dispose();
        co_return STATUS_SUCCESS;
    }

    Awaitable<NTSTATUS> ReadAsync(
        __in KBuffer& Buffer,
        __out ULONG& BytesRead,
        __in ULONG Offset,
        __in ULONG Count) override
    {
        ASSERT_IFNOT(
            Offset == 0,
            "ReadAsync Offset non 0 {0} is not supported",
            Offset);

        Common::ErrorCode ec = logFile_.TryRead2(
            Buffer.GetBuffer(),
            Count,
            BytesRead);

        if (!ec.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: ReadAsync at {1} for bytecount {2} failed with {3}",
                logFile_.FileName,
                Position,
                Count,
                ec);

            co_return ec.ToHResult();
        }

        ASSERT_IFNOT(
            BytesRead == Count,
            "{0}: BytesRead {1} at {2} Should be equal to Count {3}",
            logFile_.FileName,
            BytesRead,
            Position,
            Count);

        co_return STATUS_SUCCESS;
    }

    Awaitable<NTSTATUS> WriteAsync(
        __in KBuffer const &,
        __in ULONG,
        __in ULONG) override
    {
        CODING_ASSERT("WriteAsync On read Stream not supported");
    }

    Awaitable<NTSTATUS> FlushAsync() override
    {
        CODING_ASSERT("FlushAsync On read Stream not supported");
    }
    
private:

    ReadStream(
        std::wstring const & fileName)
        : logFile_()
    {
        Common::ErrorCode ec = logFile_.TryOpen(
            fileName,
            Common::FileMode::Enum::OpenOrCreate,
            Common::FileAccess::ReadWrite,
            Common::FileShare::ReadWrite,
            Common::FileAttributes::SparseFile);

        WriteInfo(
            TraceComponent,
            "ReadStream file log {0} completd with {1}",
            fileName,
            ec);

        ASSERT_IFNOT(
            ec.IsSuccess(),
            "{0}: Failed to create ReadStream with {1}",
            fileName,
            ec);
    }

    // This is mutable because Getposition API is not there in windows. Only SetfilePointer that gives current pos
    mutable Common::File logFile_;
};

FileLog::ReadStream::~ReadStream()
{
}

FileLog::FileLog(
    __in PartitionedReplicaId const & traceId)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , logFile_()
{
}

FileLog::~FileLog()
{
}

Awaitable<NTSTATUS> FileLog::OpenAsync(
    __in KWString&  logFileName,
    __in CancellationToken const & cancellationToken)
{
    std::wstring parent = Common::Path::GetParentDirectory(logFileName.operator PWCHAR(), 1);

    if (!Common::Directory::Exists(parent))
    {
        Common::Directory::Create(parent);
    }

    Common::ErrorCode ec = logFile_.TryOpen(
        logFileName.operator PWCHAR(),
        Common::FileMode::Enum::OpenOrCreate,
        Common::FileAccess::ReadWrite,
        Common::FileShare::ReadWrite,
        Common::FileAttributes::SparseFile);

    WriteInfo(
        TraceComponent,
        "{0}: Create file log {1} completd with {2}",
        TraceId,
        logFileName.operator PWCHAR(),
        ec);

    if (ec.IsSuccess())
    {
        // Set the position to the tail of the log
        logFile_.resize(GetLength());
    }

    co_return ec.ToHResult();
}

Awaitable<NTSTATUS> FileLog::CloseAsync(
    __in CancellationToken const &)
{
    Abort();
    co_return STATUS_SUCCESS;
}

void FileLog::Abort()
{
    Common::ErrorCode ec = logFile_.Close2();

    WriteInfo(
        TraceComponent,
        "{0}: Close file log {1} completd with {2}",
        TraceId,
        logFile_.FileName,
        ec);
}

void FileLog::Dispose()
{
    Abort();
}

BOOLEAN FileLog::GetIsFunctional()
{
    return logFile_.IsValid();
}

LONGLONG FileLog::GetLength() const
{
    auto currentPos = logFile_.Seek(0, Common::SeekOrigin::Current);
    auto length = logFile_.Seek(0, Common::SeekOrigin::End);
    logFile_.Seek(currentPos, Common::SeekOrigin::Begin);

    return length;
}

LONGLONG FileLog::GetWritePosition() const
{
    return logFile_.Seek(0, Common::SeekOrigin::Current);
}

LONGLONG FileLog::GetReadPosition() const
{
    return -1;
}

LONGLONG FileLog::GetHeadTruncationPosition() const
{
    return -1;
}

LONGLONG FileLog::GetMaximumBlockSize() const
{
    return -1;
}

ULONG FileLog::GetMetadataBlockHeaderSize() const
{
    return static_cast<ULONG>(-1);
}

ULONGLONG FileLog::GetSize() const
{
    LONGLONG length = GetLength();
    KInvariant(length >= 0);
    return static_cast<ULONGLONG>(length);
}

ULONGLONG FileLog::GetSpaceRemaining() const
{
    return ULONGLONG_MAX - GetSize();
}

NTSTATUS FileLog::CreateReadStream(
    __out ILogicalLogReadStream::SPtr& Stream,
    __in LONG SequentialAccessReadSize)
{
    UNREFERENCED_PARAMETER(SequentialAccessReadSize);

    ReadStream::SPtr stream = ReadStream::Create(
        logFile_.FileName,
        GetThisAllocator());

    Stream = stream.RawPtr();

    return STATUS_SUCCESS;
}

void FileLog::SetSequentialAccessReadSize(
    __in ILogicalLogReadStream&,
    __in LONG)
{
}

Awaitable<NTSTATUS> FileLog::ReadAsync(
    __out LONG& BytesRead,
    __in KBuffer&,
    __in ULONG,
    __in ULONG,
    __in ULONG,
    __in CancellationToken const &)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS FileLog::SeekForRead(
    __in LONGLONG,
    __in Common::SeekOrigin::Enum)
{
    return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> FileLog::AppendAsync(
    __in KBuffer const & Buffer,
    __in LONG OffsetIntoBuffer,
    __in ULONG Count,
    __in CancellationToken const &)
{
    ASSERT_IFNOT(
        OffsetIntoBuffer == 0,
        "{0}: OffsetIntoBuffer {1} not supported. Should be 0",
        TraceId,
        OffsetIntoBuffer);

    DWORD bytesWritten = 0;
    Common::ErrorCode ec = logFile_.TryWrite2(
        Buffer.GetBuffer(),
        Count,
        bytesWritten);

    if (!ec.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0}: Failed To Write To log {1} with Error {2}",
            TraceId,
            logFile_.FileName,
            ec);

        co_return ec.ToHResult();
    }

    ASSERT_IFNOT(
        bytesWritten == Count,
        "{0}: BytesWritten {1} Should be equal to Count {2}",
        TraceId,
        bytesWritten,
        Count);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLog::FlushAsync(
    __in CancellationToken const &)
{
    logFile_.Flush();
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLog::FlushWithMarkerAsync(
    __in CancellationToken const &)
{
    logFile_.Flush();
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLog::TruncateHead(
    __in LONGLONG StreamOffset)
{
    DWORD dw;
    FILE_ZERO_DATA_INFORMATION info = { 0 };
    info.BeyondFinalZero.QuadPart = StreamOffset;

    BOOL result = DeviceIoControl(
        logFile_.GetHandle(),
        FSCTL_SET_ZERO_DATA,
        (void*)&info,
        sizeof(info),
        nullptr,
        0,
        &dw,
        nullptr);

    ASSERT_IFNOT(
        result == TRUE,
        "{0}: SetzeroData Failed with {1} at {2} for file {3}",
        TraceId,
        GetLastError(),
        StreamOffset,
        logFile_.FileName);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLog::TruncateTail(
    __in LONGLONG StreamOffset,
    __in CancellationToken const &)
{
    logFile_.resize(StreamOffset);
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> FileLog::WaitCapacityNotificationAsync(
    __in ULONG,
    __in CancellationToken const &)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> FileLog::WaitBufferFullNotificationAsync(
    __in CancellationToken const &)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> FileLog::ConfigureWritesToOnlyDedicatedLogAsync(
    __in CancellationToken const &)
{ 
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> FileLog::ConfigureWritesToSharedAndDedicatedLogAsync(
    __in CancellationToken const &)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> FileLog::QueryLogUsageAsync(
    __out ULONG&,
    __in CancellationToken const &)
{
    co_return STATUS_NOT_IMPLEMENTED;
}
