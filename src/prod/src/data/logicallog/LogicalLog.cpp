// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace ktl::io;
using namespace ktl::kservice;
using namespace Data;
using namespace Data::Log;

Common::StringLiteral const TraceComponent("LogicalLog");

NTSTATUS
LogicalLog::Create(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in PhysicalLog& owner,
    __in LogManager& manager,
    __in LONGLONG owningHandleId,
    __in KGuid const & id,
    __in KtlLogStream& underlyingStream,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __out LogicalLog::SPtr& logicalLog)
{
    NTSTATUS status;

    logicalLog = _new(allocationTag, allocator) LogicalLog(
        owner,
        manager,
        owningHandleId,
        id,
        underlyingStream,
        prId);

    if (!logicalLog)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);

        WriteError(
            TraceComponent,
            "{0} - Create - Failed to allocate LogicalLog",
            prId.TraceId);

        return status;
    }

    status = logicalLog->Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - Create - LogicalLog constructed with error status. Status: {1}",
            prId.TraceId,
            status);

        return Ktl::Move(logicalLog)->Status();
    }

    return STATUS_SUCCESS;
}

FAILABLE
LogicalLog::LogicalLog(
    __in PhysicalLog& owner,
    __in LogManager& manager,
    __in LONGLONG owningHandleId,
    __in KGuid const & id,
    __in KtlLogStream& underlyingStream,
    __in Data::Utilities::PartitionedReplicaId const & prId)
    : KAsyncServiceBase()
    , KWeakRefType<LogicalLog>()
    , PartitionedReplicaTraceComponent(prId)
    , ownerId_(owner.Id)
    , manager_(&manager)
    , id_(id)
    , owningHandleId_(owningHandleId)
    , underlyingStream_(&underlyingStream)
    , nextWritePosition_(0)
    , nextOperationNumber_(0)
    , maxBlockSize_(0)
    , headTruncationPoint_(-1)
    , writeBuffer_(nullptr)
    , blockMetadataSize_(underlyingStream.QueryReservedMetadataSize())
    , readContext_()
    , readTasks_(GetThisAllocator())
    , streams_(GetThisAllocator())
    , randomGenerator_()
    , internalFlushInProgress_(0)
    , openReason_(OpenReason::Invalid)
    , logSize_(0)
    , logSpaceRemaining_(0)
{
    NTSTATUS status;
    
    status = readTasks_.Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - constructor - readTasks constructed with error status. Status: {1}",
            prId.TraceId,
            status);

        SetConstructorStatus(status);
        return;
    }
    
    status = streams_.Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - constructor - streams constructed with error status. Status: {1}",
            prId.TraceId,
            status);

        SetConstructorStatus(status);
        return;
    }

    recordOverhead_ = sizeof(KLogicalLogInformation::MetadataBlockHeader) 
        + sizeof(KLogicalLogInformation::StreamBlockHeader)
        + blockMetadataSize_;
}

LogicalLog::~LogicalLog() {}

Awaitable<NTSTATUS>
LogicalLog::OpenForCreate(__in CancellationToken const &)
{
    co_return co_await OpenAsync(OpenReason::Create);
}

Awaitable<NTSTATUS>
LogicalLog::OpenForRecover(__in CancellationToken const &)
{
    co_return co_await OpenAsync(OpenReason::Recover);
}

ktl::Awaitable<NTSTATUS>
LogicalLog::OpenAsync(__in OpenReason openReason)
{
    openReason_ = openReason;

    NTSTATUS status;

    OpenAwaiter::SPtr awaiter;
    status = OpenAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - OpenAsync - Failed to create OpenAwaiter. Status {1}",
            TraceId,
            status);

        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

VOID LogicalLog::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

Task LogicalLog::OpenTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive
    NTSTATUS status;

    if (openReason_ == OpenReason::Create)
    {
        status = co_await OnCreateAsync(CancellationToken::None);
    }
    else if (openReason_ == OpenReason::Recover)
    {
        status = co_await OnRecoverAsync(CancellationToken::None);
    }
    else
    {
        status = STATUS_INTERNAL_ERROR;
    }

    if (!NT_SUCCESS(status))
    {
        CompleteOpen(status);
        co_return;
    }

    status = co_await QueryLogSizeAndSpaceRemainingInternalAsync(
        CancellationToken::None,
        logSize_,
        logSpaceRemaining_);

    CompleteOpen(status);
    co_return;
}

Awaitable<NTSTATUS>
LogicalLog::QueryLogStreamRecoveryInfo(
    __in CancellationToken const & cancellationToken,
    __out KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation& recoveryInfo)
{
    NTSTATUS status;

    KtlLogStream::AsyncIoctlContext::SPtr context;
    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryLogStreamRecoveryInfo - Failed to allocate Ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - QueryLogStreamRecoveryInfo - Unexpected Ioctl failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KAssert(outBuf != nullptr);
    KInvariant(outBuf->QuerySize() == sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation));

    recoveryInfo = *static_cast<KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*>(outBuf->GetBuffer());
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::QueryLogStreamReadInformation(
    __in CancellationToken const & cancellationToken,
    __out KLogicalLogInformation::LogicalLogReadInformation& readInformation)
{
    NTSTATUS status;

    KtlLogStream::AsyncIoctlContext::SPtr context;
    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryLogStreamReadInformation - Failed to allocate Ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::QueryLogicalLogReadInformation,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteInfo(
            TraceComponent,
            "{0} - QueryLogStreamReadInformation - Ioctl failure: assuming driver is older and doesn't support this ioctl, using default. Status: {1}",
            TraceId,
            status);

        //
        // It is likely that the driver is older and doesn't support
        // this IOCTL. In this case default to 1MB
        //
        readInformation.MaximumReadRecordSize = 1024 * 1024;
        co_return STATUS_SUCCESS;
    }

    KAssert(outBuf != nullptr);
    KInvariant(outBuf->QuerySize() == sizeof(KLogicalLogInformation::LogicalLogReadInformation));

    readInformation = *static_cast<KLogicalLogInformation::LogicalLogReadInformation*>(outBuf->GetBuffer());
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::QueryDriverBuildInformation(
    __in CancellationToken const & cancellationToken,
    __out KLogicalLogInformation::CurrentBuildInformation& buildInformation)
{
    NTSTATUS status;

    KtlLogStream::AsyncIoctlContext::SPtr context;
    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryDriverBuildInformation - Failed to allocate Ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::QueryCurrentBuildInformation,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - QueryDriverBuildInformation - Unexpected Ioctl failure.  Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KAssert(outBuf != nullptr);
    KInvariant(outBuf->QuerySize() == sizeof(KLogicalLogInformation::CurrentBuildInformation));

    buildInformation = *static_cast<KLogicalLogInformation::CurrentBuildInformation*>(outBuf->GetBuffer());
    co_return STATUS_SUCCESS;
}


// lifted from KPhysicallog com interop
//
// Interface version 1 adds MultiRecordRead apis
//
const USHORT InterfaceVersion = 1;

// lifted from KPhysicallog com interop
VOID
LogicalLog::QueryUserBuildInformation(
    __out ULONG& buildNumber,
    __out BOOLEAN& isFreeBuild)
{
    buildNumber = (InterfaceVersion << 16) + VER_PRODUCTBUILD;
#if DBG
    isFreeBuild = FALSE;
#else
    isFreeBuild = TRUE;
#endif
}

Awaitable<NTSTATUS>
LogicalLog::VerifyUserAndDriverBuildMatch()
{
    NTSTATUS status;
    KLogicalLogInformation::CurrentBuildInformation driverBuildInfo;
    ULONG userBuildNumber;
    ULONG driverBuildNumber;
    BOOLEAN userIsFreeBuild;
    BOOLEAN driverIsFreeBuild;

    status = co_await QueryDriverBuildInformation(CancellationToken::None, driverBuildInfo);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - VerifyUserAndDriverBuildMatch - Unexpected QueryDriverBuildInformation failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    driverBuildNumber = driverBuildInfo.BuildNumber;
    if (driverBuildInfo.IsFreeBuild == FALSE)
    {
        driverIsFreeBuild = FALSE;
    }
    else
    {
        driverIsFreeBuild = TRUE;
    }

    QueryUserBuildInformation(userBuildNumber, userIsFreeBuild);

    //
    // Separate user build number into high 16 bits and low 16 bits.  High 16 bits is the interface version
    // between managed and native code. Low 16 bits is the build number from the build
    //

    interfaceVersion_ = (USHORT)(userBuildNumber >> 16);
    userBuildNumber = userBuildNumber & 0x0000ffff;

    //
    // Build number cannot be checked in production as the WHQL signed ktllogger driver is 
    // always from an earlier build
    //

    WriteInfo(
        TraceComponent,
        "{0} - VerifyUserAndDriverBuildMatch - User/Driver build number mismatch. User: {1} {2}, Driver {3} {4}",
        TraceId,
        userBuildNumber,
        (userIsFreeBuild == 0) ? "CHK" : "FRE",
        driverBuildNumber,
        (driverIsFreeBuild == 0) ? "CHK" : "FRE");

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::OnCreateAsync(__in CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = co_await VerifyUserAndDriverBuildMatch();
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnCreateAsync - Unexpected VerifyUserAndDriverBuildMatch failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation currentState;
    status = co_await QueryLogStreamRecoveryInfo(cancellationToken, currentState);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnCreateAsync - Unexpected QueryLogStreamRecoveryInfo failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KLogicalLogInformation::LogicalLogReadInformation readInfo;
    status = co_await QueryLogStreamReadInformation(cancellationToken, readInfo);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnCreateAsync - Unexpected QueryLogStreamReadInformation failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    maximumReadRecordSize_ = readInfo.MaximumReadRecordSize;

    maxBlockSize_ = currentState.MaximumBlockSize;
    nextOperationNumber_ = 1;
    nextWritePosition_ = 0;
    headTruncationPoint_ = -1;

    if (writeBuffer_ != nullptr)
    {
        writeBuffer_->Dispose();
        writeBuffer_ = nullptr;
    }

    status = LogicalLogBuffer::CreateWriteBuffer(
        GetThisAllocationTag(),
        GetThisAllocator(),
        blockMetadataSize_,
        maxBlockSize_,
        nextWritePosition_,
        nextOperationNumber_,
        id_,
        *PartitionedReplicaIdentifier,
        writeBuffer_);

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - OnCreateAsync - Failed to create write buffer. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::OnRecoverAsync(__in CancellationToken const & cancellationToken)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = co_await VerifyUserAndDriverBuildMatch();
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnRecoverAsync - Unexpected VerifyUserAndDriverBuildMatch failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    nextWritePosition_ = -1;
    nextOperationNumber_ = -1;
    headTruncationPoint_ = -1;

    KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation currentState;
    status = co_await QueryLogStreamRecoveryInfo(cancellationToken, currentState);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnRecoverAsync - Unexpected QueryLogStreamRecoveryInfo failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KLogicalLogInformation::LogicalLogReadInformation readInfo;
    status = co_await QueryLogStreamReadInformation(cancellationToken, readInfo);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnRecoverAsync - Unexpected QueryLogStreamReadInformation failure. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    maximumReadRecordSize_ = readInfo.MaximumReadRecordSize;

    if (currentState.HighestOperationCount == 0)
    {
        // Special case of a recovered empty log
        KInvariant(currentState.TailAsn == 1);

        currentState.RecoveredLogicalLogHeader.HeadTruncationPoint = static_cast<ULONGLONG>(-1);
    }

    // todo?: safe casts?
    nextOperationNumber_ = static_cast<LONGLONG>(currentState.HighestOperationCount) + 1;
    nextWritePosition_ = static_cast<LONGLONG>(currentState.TailAsn - 1);
    maxBlockSize_ = currentState.MaximumBlockSize;
    headTruncationPoint_ = currentState.RecoveredLogicalLogHeader.HeadTruncationPoint;

    if (writeBuffer_ != nullptr)
    {
        writeBuffer_->Dispose();
        writeBuffer_ = nullptr;
    }

    status = LogicalLogBuffer::CreateWriteBuffer(
        GetThisAllocationTag(),
        GetThisAllocator(),
        blockMetadataSize_,
        maxBlockSize_,
        nextWritePosition_,
        nextOperationNumber_,
        id_,
        *PartitionedReplicaIdentifier,
        writeBuffer_);

    co_return STATUS_SUCCESS;
}

NTSTATUS
LogicalLog::CreateAndAddLogicalLogStream(
    __in LONG sequentialAccessReadSize,
    __out LogicalLogStream::SPtr& stream)
{
    NTSTATUS status = STATUS_SUCCESS;

    stream = nullptr;

    K_LOCK_BLOCK(streamsLock_)
    {
        ULONG length = streams_.Count();
        for (ULONG i = 0; i < length; i++)
        {
            if (streams_[i] == nullptr)
            {
                status = LogicalLogStream::Create(
                    GetThisAllocationTag(),
                    GetThisAllocator(),
                    *this,
                    interfaceVersion_,
                    sequentialAccessReadSize,
                    i,
                    *PartitionedReplicaIdentifier,
                    stream);
                if (!NT_SUCCESS(status))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - CreateAndAddLogicalLogStream - Failed to create LogicalLogStream (insert). Status: {1}, InterfaceVersion: {2}, SequentialAccessReadSize: {3}, index: {4}",
                        TraceId,
                        status,
                        interfaceVersion_,
                        sequentialAccessReadSize,
                        i);

                    KAssert(stream == nullptr);
                    break;
                }

                streams_[i] = stream->GetWeakRef();
                stream->addedToStreamArray_ = TRUE;

                break; // break out of for loop
            }
        }

        // Tried to create/add in an open slot, but failed
        if (!NT_SUCCESS(status))
        {
            KAssert(stream == nullptr);
            break; // break out of lock block
        }

        // Successfully created/added in an open slot
        if (stream != nullptr)
        {
            KAssert(stream->addedToStreamArray_ == TRUE);
            break; // break out of lock block
        }

        // no open slots in the array, so append
        status = LogicalLogStream::Create(
            GetThisAllocationTag(),
            GetThisAllocator(),
            *this,
            interfaceVersion_,
            sequentialAccessReadSize,
            length,
            *PartitionedReplicaIdentifier,
            stream);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - CreateAndAddLogicalLogStream - Failed to create LogicalLogStream (append). Status: {1}, InterfaceVersion: {2}, SequentialAccessReadSize: {3}, index: {4}",
                TraceId,
                status,
                interfaceVersion_,
                sequentialAccessReadSize,
                length);

            KAssert(stream == nullptr);
            break; // break out of lock block
        }

        status = streams_.Append(stream->GetWeakRef());
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - CreateAndAddLogicalLogStream - Failed to append LogicalLogStream. Status: {1}",
                TraceId,
                status);

            stream = nullptr;
            break; // break out of lock block
        }
        stream->addedToStreamArray_ = TRUE;
    }

    if (!NT_SUCCESS(status))
    {
        KAssert(stream == nullptr);
        return status;
    }

    KAssert(stream != nullptr);
    KAssert(stream->addedToStreamArray_ == TRUE);
    return STATUS_SUCCESS;
}

VOID
LogicalLog::RemoveLogicalLogStream(__in LogicalLogStream const & stream)
{
    K_LOCK_BLOCK(streamsLock_)
    {
        KInvariant(streams_[stream.streamArrayIndex_] != nullptr);
        streams_[stream.streamArrayIndex_] = nullptr;
    }
}

VOID
LogicalLog::InvalidateAllReads()
{
    K_LOCK_BLOCK(readTasksLock_)
    {
        ULONG length = readTasks_.Count();
        for (ULONG i = 0; i < length; i++)
        {
            readTasks_[i]->InvalidateRead();
        }
    }

    K_LOCK_BLOCK(streamsLock_)
    {
        ULONG length = streams_.Count();
        for (ULONG i = 0; i < length; i++)
        {
            if (streams_[i] != nullptr)
            {
                LogicalLogStream::SPtr stream = streams_[i]->TryGetTarget();
                if (stream != nullptr)
                {
                    stream->InvalidateReadAhead();
                }
            }
        }
    }
}

NTSTATUS
LogicalLog::StartBackgroundRead(
    __in LONGLONG offset,
    __in LONG length,
    __out LogicalLogReadTask::SPtr& readTask)
{
    NTSTATUS status;
    LogicalLogReadTask::SPtr task;

    status = LogicalLogReadTask::Create(
        GetThisAllocationTag(),
        GetThisAllocator(),
        *underlyingStream_,
        task);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - StartBackgroundRead - Failed to create LogicalLogReadTask. Status: {1}",
            TraceId,
            status);

        return status;
    }
    
    // todo?: track background reads with activities
    task->StartRead(offset, length);
    
    K_LOCK_BLOCK(readTasksLock_)
    {
        status = readTasks_.Append(task);
    }

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - StartBackgroundRead - Failed to append LogicalLogReadTask. Status: {1}",
            TraceId,
            status);

        task->InvalidateRead(); // todo?: do I need to do this?
        return status;
    }

    task->AddRef(); // add a ref while the task is in the array

    readTask = Ktl::Move(task);
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogicalLog::GetReadResultsAsync(
    __in LogicalLogReadTask& readTask,
    __out PhysicalLogStreamReadResults& results)
{
    LogicalLogReadTask::SPtr readTaskSPtr = nullptr;

    K_LOCK_BLOCK(readTasksLock_)
    {
        ULONG length = readTasks_.Count();
        ULONG i = 0;

        while (i < length && readTaskSPtr == nullptr)
        {
            if (readTasks_[i].RawPtr() == &readTask)
            {
                BOOLEAN res = readTasks_.Remove(i);
                KInvariant(res);

                readTaskSPtr = &readTask;

                readTask.Release(); // release the ref added when put into the array
            }

            i++;
        }
    }
    // todo: take an argument which says whether or not the task has already been removed (e.g. during close)

    co_return co_await readTask.GetReadResults(results);
}

Awaitable<NTSTATUS> LogicalLog::InternalReadAsync(
    __in LogicalLogReadContext& readContext,
    __inout KBuffer& buffer,
    __in LONG offset,
    __in LONG count,
    __in LONG bytesToRead,
    __in CancellationToken const & cancellationToken,
    __out ReadAsyncResults& readAsyncResults)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;

    //
    // We allow up to 3 attempts to read our record from the logical log.
    // - First could be that the current buffer is right at the end and so the first read
    //   will return zero since there is no data in the buffer
    // - Second could be the read from the logger fails due to a transient truncation of the shared log
    //   See TFS 5044108.
    // - Third attempt should succeed, if not then there is a bug and so we assert
    //
    const LONG zeroBytesReadLimit = 3;
    LONG zeroBytesReadCount = 0;

    LogicalLogBuffer::SPtr readBuffer = readContext_.ReadBuffer;
    const LONGLONG readLocation = readContext_.ReadLocation;

    BOOLEAN startNextRead = FALSE;
    BOOLEAN isNextRead;
    LONGLONG readOffset;
    ReadAsyncResults results;
    results.TotalDone = 0;
    results.Context = readContext;

    if ((results.Context.ReadLocation < headTruncationPoint_)
        || (results.Context.ReadLocation >= nextWritePosition_))
    {
        // attempt to start a read in non-existing space

        // Trace this as 'info' since it is considered success.
        WriteInfo(
            TraceComponent,
            "{0} - InternalReadAsync - Read in a nonexistant space. location: {1}, HeadTruncation: {2}, NextWritePosition: {3}",
            TraceId,
            readLocation,
            headTruncationPoint_,
            nextWritePosition_);

        readAsyncResults = Ktl::Move(results);
        co_return STATUS_SUCCESS;
    }

    LONGLONG todo = min(nextWritePosition_ - results.Context.ReadLocation, static_cast<LONGLONG>(count)); // NOTE: Can go negative when read cursor is beyond EOS

    while (todo > 0)
    {
        isNextRead = FALSE;
        if (results.Context.ReadBuffer == nullptr)
        {
            PhysicalLogStreamReadResults r;

            Common::Stopwatch sw;

            if ((interfaceVersion_ == 0) || (bytesToRead == 0))
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - InternalReadAsync - legacy read. count: {1}, readLocation: {2}",
                    TraceId,
                    count,
                    results.Context.ReadLocation);

                sw.Start();

                KtlLogStream::AsyncReadContext::SPtr context;
                status = underlyingStream_->CreateAsyncReadContext(context);  // todo?: reuse
                if (!NT_SUCCESS(status))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - InternalReadAsync - failed to allocate async read context.  Status: {1}",
                        TraceId,
                        status);

                    co_return status;
                }

                status = co_await context->ReadContainingAsync(
                    results.Context.ReadLocation + 1,
                    &r.ResultingAsn,
                    &r.Version,
                    r.ResultingMetadataSize,
                    r.ResultingMetadata,
                    r.ResultingIoPageAlignedBuffer,
                    nullptr);
                if (!NT_SUCCESS(status))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - InternalReadAsync - legacy read failed. count: {1}, readLocation: {2}, status: {3}",
                        TraceId,
                        count,
                        results.Context.ReadLocation,
                        status);
                    
                    co_return status;
                }
                readOffset = results.Context.ReadLocation;
            }
            else
            {
                // Try0
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} - InternalReadAsync - MultiRecordRead. count: {1}, bytesToRead: {2}, readLocation: {3}",
                        TraceId,
                        count,
                        bytesToRead,
                        results.Context.ReadLocation);

                    sw.Start();

                    // Try1
                    {
                        //
                        // See if the read can be satisfied by the _NextReadBuffer
                        //
                        LogicalLogReadTask::SPtr readTask;
                        LONGLONG offsetToRead = results.Context.ReadLocation;

                        if (results.Context.NextReadTask != nullptr)
                        {
                            if (results.Context.NextReadTask->IsValid() && results.Context.NextReadTask->IsInRange(offsetToRead))
                            {
                                readTask = results.Context.NextReadTask;
                            }
                            else
                            {
                                //
                                // The readahead is not valid since it was either invalidated by a truncate or the read is out 
                                // of range. Abandon the read and clean it up before moving forward.
                                //
                                WriteInfo(
                                    TraceComponent,
                                    "{0} - InternalReadAsync - abandon read ahead. offset: {1}, length: {2}",
                                    TraceId,
                                    results.Context.NextReadTask->Offset,
                                    results.Context.NextReadTask->Length);

                                // CONSIDER: Why do we need to await here ? Can't we do this on another thread somehow ?

                                status = co_await GetReadResultsAsync(*results.Context.NextReadTask, r);

                                if (!NT_SUCCESS(status))
                                {
                                    //
                                    // We never care if a next read fails
                                    //
                                    WriteInfo(
                                        TraceComponent,
                                        "{0} - InternalReadAsync - background read task failure. offset: {1}, length: {2}, status: {3}",
                                        TraceId,
                                        results.Context.NextReadTask->Offset,
                                        results.Context.NextReadTask->Length,
                                        status);
                                }
                                results.Context.NextReadTask = nullptr;
                            }
                            results.Context.NextReadTask = nullptr;
                        }

                        if (readTask == nullptr)
                        {
                            //
                            // Since the next read doesn't have data we need then we need to do the read right now
                            //
                            status = StartBackgroundRead(offsetToRead, bytesToRead, readTask);
                            if (!NT_SUCCESS(status))
                            {
                                WriteWarning(
                                    TraceComponent,
                                    "{0} - InternalReadAsync - failed to start background read. offsetToRead: {1}, bytesToRead: {2}, status: {3}",
                                    TraceId,
                                    offsetToRead,
                                    bytesToRead,
                                    status);

                                goto Catch1;
                            }
                        }
                        else
                        {
                            isNextRead = TRUE;
                        }

                        status = co_await GetReadResultsAsync(*readTask, r);
                        if (!NT_SUCCESS(status))
                        {
                            WriteWarning(
                                TraceComponent,
                                "{0} - InternalReadAsync - failed to get read results. Status: {1}",
                                TraceId,
                                status);

                            goto Catch1;
                        }

                        readOffset = readTask->GetOffset();

                        startNextRead = TRUE;
                        goto EndCatch1;
                    }
                    Catch1:
                    {
                        KAssert(!NT_SUCCESS(status));
                        if (!isNextRead)
                        {
                            //
                            // If this was a foreground read then we want to pass the exception back to the caller as
                            // the read was invalid.
                            //
                            WriteWarning(
                                TraceComponent,
                                "{0} - InternalReadAsync - Multirecord read failed. count: {1}, bytesToRead: {2}, readLocation: {3}, status: {4}",
                                TraceId,
                                count,
                                bytesToRead,
                                results.Context.ReadLocation,
                                status);

                            goto Catch0;
                        }
                        else
                        {
                            //
                            // Otherwise if this was next read then we don't care if it fails, we will just try again
                            //
                            WriteInfo(
                                TraceComponent,
                                "{0} - InternalReadAsync - Multirecord read failed. count: {1}, bytesToRead: {2}, readLocation: {3}, status: {4}",
                                TraceId,
                                count,
                                bytesToRead,
                                results.Context.ReadLocation,
                                status);

                            continue;     // while (todo > 0);
                        }
                    }
                EndCatch1:
                    goto EndCatch0;
                }
            Catch0:
                {
                    // todo: figure out what status I need to check for which corresponds to FabricException with a ComException inside?
                    //catch (System.Fabric.FabricException ex)
                    //{
                    //    if (ex.InnerException is System.Runtime.InteropServices.COMException)
                    //    {
                    //        AppTrace.TraceSource.WriteWarning(
                    //            TraceType,
                    //            "InternalReadAsync: ComInteropException, assuming downlevel driver {0}",
                    //            ex.ToString()
                    //            );
                    //        this._InterfaceVersion = 0;
                    //        continue;  // while(todo > 0)
                    //    }

                    //    throw;
                    //}

                    co_return status;
                }
            }
            EndCatch0:
            
            sw.Stop();

            WriteInfo(
                TraceComponent,
                "{0} - InternalReadAsync - Read completed. elapsedMilliseconds: {1}, count: {2}, bytesToRead: {3}, readLocation: {4}",
                TraceId,
                sw.ElapsedMilliseconds,
                count,
                bytesToRead,
                results.Context.ReadLocation);

            KAssert(r.ResultingMetadata != nullptr);
            KAssert(r.ResultingIoPageAlignedBuffer != nullptr);
            status = LogicalLogBuffer::CreateReadBuffer(
                GetThisAllocationTag(),
                GetThisAllocator(),
                blockMetadataSize_,
                readOffset,
                *r.ResultingMetadata,
                *r.ResultingIoPageAlignedBuffer,
                *PartitionedReplicaIdentifier,
                results.Context.ReadBuffer);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - InternalReadAsync - Failed to create read buffer. BlockMetadataSize: {1}, ReadOffset: {2}, Status: {3}",
                    TraceId,
                    blockMetadataSize_,
                    readOffset,
                    status);

                co_return status;
            }

            ULONG leftToRead = results.Context.ReadBuffer->SizeLeftToRead;

            if ((isNextRead) &&
                ((results.Context.ReadLocation < readOffset) ||
                    (results.Context.ReadLocation >= (readOffset + leftToRead))))
            {
                //
                // Ensure that our read ahead actually contains the data we want. It is possible that
                // the read ahead will return a shorter record than we asked for and although we think 
                // it might be in the read ahead, it actually is not
                //
                results.Context.ReadBuffer->Dispose();
                results.Context.ReadBuffer = nullptr;

                WriteWarning(
                    TraceComponent,
                    "{0} - InternalReadAsync - read ahead is a short record. readOffset: {1}, leftToRead: {2}, readLocation: {3}",
                    TraceId,
                    readOffset,
                    leftToRead,
                    results.Context.ReadLocation);

                continue;  // while(todo > 0)
            }

            if (startNextRead)
            {
                //
                // Prime to read following record
                //
                LONGLONG offsetToNextRead = readOffset + results.Context.ReadBuffer->SizeLeftToRead;

                status = StartBackgroundRead(offsetToNextRead, bytesToRead, results.Context.NextReadTask);
                if (!NT_SUCCESS(status))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - InternalReadAsync - failed to start background read. offsetToNextRead: {1}, bytesToRead: {2}, status: {3}",
                        TraceId,
                        offsetToNextRead,
                        bytesToRead,
                        status);

                    co_return status;
                }
            }
        }

        ULONG done;
        KInvariant(todo >= 0 && todo <= ULONG_MAX);
        status = results.Context.ReadBuffer->Get(
            static_cast<PBYTE>(buffer.GetBuffer()),
            done,
            offset,
            static_cast<ULONG>(todo));
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - InternalReadAsync - failed to read data from readbuffer. done: {1}, offset: {2}, todo: {3}, status: {4}",
                TraceId,
                done,
                offset,
                todo,
                status);

            co_return status;
        }

        if (done == 0)
        {
            zeroBytesReadCount++;
            if (zeroBytesReadCount == zeroBytesReadLimit)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - InternalReadAsync - MultiRecordRead too many retries. count: {1}, bytesToRead: {2}, todo: {3}, readLocation: {4}",
                    TraceId,
                    count,
                    bytesToRead,
                    todo,
                    results.Context.ReadLocation);

                co_return STATUS_IO_DEVICE_ERROR; //todo?: better status
            }
        }
        else
        {
            zeroBytesReadCount = 0;
        }

        todo -= done;
        offset += done;
        results.Context.ReadLocation += done;
        results.TotalDone += done;

        if (todo > 0)
        {
            results.Context.ReadBuffer->Dispose();
            results.Context.ReadBuffer = nullptr;
        }
    }

    readAsyncResults = Ktl::Move(results);
    co_return STATUS_SUCCESS;
}

VOID LogicalLog::Dispose()
{
    Abort();
}

VOID LogicalLog::Abort()
{
    AbortTask();
}

Task LogicalLog::AbortTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    if (IsOpen())
    {
        NTSTATUS status = co_await CloseAsync(CancellationToken::None);
        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_UNSUCCESSFUL && !IsOpen())
            {
                // This likely means that the logicallog was already closed when CloseAsync was initiated.
                WriteInfo(
                    TraceComponent,
                    "{0} - AbortTask - CloseAsync failed during abort (likely already closed). Status: {1}",
                    TraceId,
                    status);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - AbortTask - CloseAsync failed during abort. Status: {1}",
                    TraceId,
                    status);
            }
        }
    }
}

Awaitable<NTSTATUS>
LogicalLog::CloseAsync(__in CancellationToken const & cancellationToken)
{
    if (!IsOpen())
    {
        co_return STATUS_SUCCESS;  // matches semantic from managed code
    }

    NTSTATUS status;

    CloseAwaiter::SPtr awaiter;
    status = CloseAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        nullptr,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - CloseAsync - failed to create CloseAwaiter. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

VOID LogicalLog::OnServiceClose()
{
    CloseTask();
}

Task LogicalLog::CloseTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN drainingReadTasks = TRUE;
    while (drainingReadTasks)
    {
        LogicalLogReadTask::SPtr readTask;

        K_LOCK_BLOCK(readTasksLock_)
        {
            ULONG count = readTasks_.Count();
            if (count != 0)
            {
                readTask = readTasks_[count - 1];
                readTasks_.Remove(count - 1);
                readTask->Release(); // release the ref added when put into the array
            }
        }

        if (readTask == nullptr)
        {
            drainingReadTasks = FALSE;
            break;
        }

        PhysicalLogStreamReadResults readResults;

        status = co_await GetReadResultsAsync(*readTask, readResults);
        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_NOT_FOUND)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - CloseTask - background read task failure (not found). Offset: {1}, Length: {2}, Status: {3}",
                    TraceId,
                    readTask->Offset,
                    readTask->Length,
                    status);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - CloseTask - background read task failure. Offset: {1}, Length: {2}, Status: {3}",
                    TraceId,
                    readTask->Offset,
                    readTask->Length,
                    status);
            }
        }
    }

    // todo?: fully move to activity model (e.g. background reads use an activity, and are drained in OnDeferredClose)
    // All operations were represented by an activity, so they have all been drained already

    status = co_await manager_->OnLogicalLogClose(
        *PartitionedReplicaIdentifier,
        *this,
        CancellationToken::None);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - CloseTask - OnLogicalLogClose failure. Status: {1}",
            TraceId,
            status);
    }

    manager_ = nullptr;
    underlyingStream_ = nullptr;

    CompleteClose(status);
}

NTSTATUS
LogicalLog::CreateReadStream(
    __out ILogicalLogReadStream::SPtr& stream,
    __in LONG sequentialAccessReadSize)
{
    KService$ApiEntry(TRUE);

    LogicalLogStream::SPtr newStream;
    NTSTATUS status;

    status = CreateAndAddLogicalLogStream(sequentialAccessReadSize, newStream);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - CreateReadStream - failed to create and add logical log stream. sequentialAccessReadSize: {1}, Status: {2}",
            TraceId,
            sequentialAccessReadSize,
            status);

        return status;
    }

    stream = ILogicalLogReadStream::SPtr(newStream.RawPtr());
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::ReadAsync(
    __out LONG& bytesRead,
    __in KBuffer& buffer,
    __in ULONG offset,
    __in ULONG count,
    __in ULONG bytesToRead,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    ReadAsyncResults results;

    // workaround the compiler oom bug
    auto& tmpBuffer = buffer;
    auto& tmpOffset = offset;
    auto& tmpCount = count;

    status = co_await InternalReadAsync(
        readContext_,
        tmpBuffer,
        tmpOffset,
        tmpCount,
        bytesToRead,
        cancellationToken,
        results);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - ReadAsync - read failed. Offset: {1}, Count: {2}, bytesToRead: {3}, status: {4}",
            TraceId,
            tmpOffset,
            tmpCount,
            bytesToRead,
            status);

        co_return status;
    }

    readContext_ = results.Context;

    bytesRead = results.TotalDone;
    co_return STATUS_SUCCESS;
}

NTSTATUS
LogicalLog::SeekForRead(
    __in LONGLONG offset, 
    __in Common::SeekOrigin::Enum origin)
{
    KService$ApiEntry(TRUE);

    NTSTATUS status;
    LONGLONG newReadLocation = -1;

    switch (origin)
    {
        case Common::SeekOrigin::Enum::Begin:
            newReadLocation = offset;
            break;

        case Common::SeekOrigin::Enum::End:
            newReadLocation = nextWritePosition_ + offset;
            break;

        case Common::SeekOrigin::Enum::Current:
            newReadLocation = readContext_.ReadLocation + offset;
            break;
    }

    // Check read buffer (if there is one) to see if we can reposition within that buffer
    // 
    if (readContext_.ReadBuffer != nullptr)
    {
        BOOLEAN intersects;
        status = readContext_.ReadBuffer->Intersects(intersects, newReadLocation);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - SeekForRead - failed to check buffer intersection. offset: {1}, origin: {2}, newReadLocation: {3}, status: {4}",
                TraceId,
                offset,
                (int)origin,
                newReadLocation,
                status);

            return status;
        }

        if (intersects)
        {
            status = readContext_.ReadBuffer->SetPosition(newReadLocation - readContext_.ReadBuffer->BasePosition);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - SeekForRead - failed to set buffer position. offset: {1}, origin: {2}, newReadLocation: {3}, basePosition: {4}, status: {5}",
                    TraceId,
                    offset,
                    (int)origin,
                    newReadLocation,
                    readContext_.ReadBuffer->BasePosition,
                    status);

                return status;
            }
        }
        else
        {
            // TODO: Work out Reuse() for read approach
            if (readContext_.ReadBuffer != nullptr) // todo: why is this being checked again???
            {
                readContext_.ReadBuffer->Dispose();
                readContext_.ReadBuffer = nullptr;
            }
        }
    }

    readContext_.ReadLocation = newReadLocation;
    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::AppendAsync(
    __in KBuffer const & buffer,
    __in LONG offset,
    __in ULONG count,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    LONG originalCount = count;

    while (count > 0)
    {
        ULONG bytesWritten;
        status = writeBuffer_->Put(bytesWritten, static_cast<BYTE const *>(buffer.GetBuffer()), offset, count);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - AppendAsync - Failed to put into writebuffer. originalCount: {1}, offset: {2}, count: {3}, status: {4}",
                TraceId,
                originalCount,
                offset,
                count,
                status);

            co_return status;
        }

        count -= bytesWritten;
        offset += bytesWritten;
        nextWritePosition_ += bytesWritten;

        if (count > 0)
        {
            //
            // No need to make a barrier here as if the buffer is just completely full then count == 0 and
            // this code path isn't executed. Flush will happen by replicator just after this.
            //
            status = co_await InternalFlushAsync(false, cancellationToken);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - AppendAsync - InternalFlushAsync failed. Status: {1}",
                    TraceId,
                    status);

                co_return status;
            }
        }
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::DelayBeforeFlush(__in CancellationToken const &)
{
    // todo: implement
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS>
LogicalLog::InternalFlushAsync(
    __in BOOLEAN isBarrier,
    __in CancellationToken const & cancellationToken)
{
    LONG flushInProgress = InterlockedCompareExchange(&internalFlushInProgress_, 1, 0);

    if (flushInProgress == 0)
    {
        KInvariant(!writeBuffer_->IsSealed);

        NTSTATUS status;

        KIoBuffer::SPtr mdBuffer;
        ULONG mdBufferSize;
        KIoBuffer::SPtr pageAlignedBuffer;
        LONGLONG sizeOfUserDataSealed;
        LONGLONG asnOfRecord; // todo: why is this longlong instead of ulonglong?
        LONGLONG opOfRecord; // todo: why is this longlong instead of ulonglong?

        // todo: co_await DelayBeforeFlush

        status = writeBuffer_->SealForWrite(
            headTruncationPoint_,
            isBarrier,
            mdBuffer,
            mdBufferSize,
            pageAlignedBuffer,
            sizeOfUserDataSealed,
            asnOfRecord,
            opOfRecord);

        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - InternalFlushAsync - failed to seal write buffer. headTruncationPoint_: {1}, isBarrier: {2}, mdBufferSize: {3},  sizeOfUserDataSealed: {4}, asnOfRecord: {5}, opOfrecord: {6}, status: {7}",
                TraceId,
                headTruncationPoint_,
                isBarrier,
                mdBufferSize,
                sizeOfUserDataSealed,
                asnOfRecord,
                opOfRecord,
                status);

            co_return status;
        }

        if (sizeOfUserDataSealed > 0)
        {
            nextOperationNumber_++;

            {
                KtlLogStream::AsyncWriteContext::SPtr context;
                status = underlyingStream_->CreateAsyncWriteContext(context);
                if (!NT_SUCCESS(status))
                {
                    WriteError(
                        "{0} - InternalFlushAsync - failed to create write context. status: {1}",
                        TraceId,
                        status);

                    co_return status;
                }

                status = co_await context->WriteAsync(
                    asnOfRecord,
                    opOfRecord,
                    mdBufferSize,
                    mdBuffer,
                    pageAlignedBuffer,
                    0, // todo: figure out what reservation to pass
                    logSize_,
                    logSpaceRemaining_,
                    nullptr);

                if (!NT_SUCCESS(status))
                {
                    WriteWarning(
                        "{0} - InternalFlushAsync - write failed. asnOfRecord: {1}, opOfRecord: {2}, mdBufferSize: {3}, status: {4}",
                        TraceId,
                        asnOfRecord,
                        opOfRecord,
                        mdBufferSize,
                        status);

                    co_return status;
                }
            }
        }

        if (writeBuffer_ != nullptr)
        {
            writeBuffer_->Dispose();
            writeBuffer_ = nullptr;
        }

        // TODO: Come up with a better reuse approach; unseal/reuse if sizeOfUserDataSealed == 0
        // Switch to new output buffer home at next location in stream (asn) and op (version) space 

        status = LogicalLogBuffer::CreateWriteBuffer(
            GetThisAllocationTag(),
            GetThisAllocator(),
            blockMetadataSize_,
            maxBlockSize_,
            nextWritePosition_,
            nextOperationNumber_,
            id_,
            *PartitionedReplicaIdentifier,
            writeBuffer_);

        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - InternalFlushAsync - failed to create write buffer. blockMetadataSize: {1}, maxBlockSize: {2}, nextWritePosition: {3}, nextOperationNumber: {4}, id: {5}, status: {6}",
                TraceId,
                blockMetadataSize_,
                maxBlockSize_,
                nextWritePosition_,
                nextOperationNumber_,
                Common::Guid(id_),
                status);

            co_return status;
        }

        KInvariant(!writeBuffer_->IsSealed);

        internalFlushInProgress_ = 0;
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} - InternalFlushAsync - flush ignored since flush is in progress.",
            TraceId);
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::FlushAsync(__in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    co_return co_await InternalFlushAsync(FALSE, cancellationToken);
}

Awaitable<NTSTATUS>
LogicalLog::FlushWithMarkerAsync(__in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    co_return co_await InternalFlushAsync(TRUE, cancellationToken);
}

ktl::Awaitable<NTSTATUS>
LogicalLog::TruncateHead(__in LONGLONG headTruncationPoint)
{
    KCoService$ApiEntry(TRUE);

    WriteInfo(
        TraceComponent,
        "{0} - TruncateHead - headTruncationPoint_: {1}, nextWritePosition: {2}, headTruncationPoint: {3}",
        TraceId,
        headTruncationPoint_,
        nextWritePosition_,
        headTruncationPoint);

    NTSTATUS status;

    KAssert(headTruncationPoint <= nextWritePosition_);
    if (headTruncationPoint_ < headTruncationPoint)
    {
        KtlLogStream::AsyncTruncateContext::SPtr context;
        status = underlyingStream_->CreateAsyncTruncateContext(context);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - TruncateHead - Failed to create truncate context. status: {1}",
                TraceId,
                status);

            co_return status;
        }

        context->Truncate(headTruncationPoint + 1, headTruncationPoint + 1);
        headTruncationPoint_ = headTruncationPoint;
    }

    co_return STATUS_SUCCESS;
}

// todo: prevent from racing or assert if races occur
// todo: add async to function name
Awaitable<NTSTATUS>
LogicalLog::TruncateTail(
    __in LONGLONG streamOffset,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;

    WriteInfo(
        TraceComponent,
        "{0} - TruncateTail - headTruncationPoint_: {1}, nextWritePosition_: {2}, streamOffset: {3}",
        TraceId,
        headTruncationPoint_,
        nextWritePosition_,
        streamOffset);

    if ((streamOffset >= nextWritePosition_) || (streamOffset < 0))
    {
        WriteWarning(
            TraceComponent,
            "{0} - TruncateTail - Rejecting TruncateTail. offset {1} above nextWritePosition {2}",
            TraceId,
            streamOffset,
            nextWritePosition_);

        co_return STATUS_INVALID_PARAMETER_1;
    }
    else if (streamOffset <= headTruncationPoint_)
    {
        WriteWarning(
            TraceComponent,
            "{0} - TruncateTail - Rejecting TruncateTail. offset {1} below nextWritePosition {2}",
            TraceId,
            streamOffset,
            nextWritePosition_);

        co_return STATUS_INVALID_PARAMETER_1;
    }

    status = co_await InternalFlushAsync(false, cancellationToken);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - TruncateTail - InternalFlushAsync failed. status: {1}",
            TraceId,
            status);

        co_return status;
    }

    LogicalLogBuffer::SPtr nullWriteBuffer;
    status = LogicalLogBuffer::CreateWriteBuffer(
        GetThisAllocationTag(),
        GetThisAllocator(),
        blockMetadataSize_,
        maxBlockSize_,
        streamOffset,
        nextOperationNumber_,
        id_,
        *PartitionedReplicaIdentifier,
        nullWriteBuffer);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - TruncateTail - failed to create write buffer. blockMetadataSize: {1}, maxBlockSize: {2}, streamOffset: {3}, nextOperationNumber: {4}, id: {5}, status: {6}",
            TraceId,
            blockMetadataSize_,
            maxBlockSize_,
            streamOffset,
            nextOperationNumber_,
            Common::Guid(id_),
            status);

        co_return status;
    }

    nextOperationNumber_++;

    KIoBuffer::SPtr mdBuffer;
    ULONG mdBufferSize;
    KIoBuffer::SPtr pageAlignedBuffer;
    LONGLONG sizeOfUserDataSealed;
    LONGLONG asnOfRecord;
    LONGLONG opOfRecord;

    status = nullWriteBuffer->SealForWrite(
        headTruncationPoint_,
        TRUE, // TruncateTail records must have barrier
        mdBuffer,
        mdBufferSize,
        pageAlignedBuffer,
        sizeOfUserDataSealed,
        asnOfRecord,
        opOfRecord);

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - TruncateTail - failed to seal write buffer. headTruncationPoint: {1}, mdBufferSize: {2}, sizeOfUserDataSealed: {3}, asnOfRecord: {4}, opOfRecord: {5}, status: {6}",
            TraceId,
            headTruncationPoint_,
            mdBufferSize,
            sizeOfUserDataSealed,
            asnOfRecord,
            opOfRecord,
            status);

        co_return status;
    }

    KtlLogStream::AsyncWriteContext::SPtr context;
    status = underlyingStream_->CreateAsyncWriteContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            "{0} - TruncateTail - failed to create write context. status: {1}",
            TraceId,
            status);

        co_return status;
    }

    status = co_await context->WriteAsync(
        asnOfRecord,
        opOfRecord,
        mdBufferSize,
        mdBuffer,
        pageAlignedBuffer,
        0, // todo: figure out what reservation to pass
        nullptr);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            "{0} - TruncateTail - write failed. asnOfRecord: {1}, opOfRecord: {2}, mdBufferSize: {3}, status: {4}",
            TraceId,
            asnOfRecord,
            opOfRecord,
            mdBufferSize,
            status);

        co_return status;
    }

    nextWritePosition_ = streamOffset;

    //
    // Rebuild a new write buffer at the correct opNumber and asn
    //
    if (writeBuffer_ != nullptr)
    {
        writeBuffer_->Dispose();
        writeBuffer_ = nullptr;
    }

    status = LogicalLogBuffer::CreateWriteBuffer(
        GetThisAllocationTag(),
        GetThisAllocator(),
        blockMetadataSize_,
        maxBlockSize_,
        nextWritePosition_,
        nextOperationNumber_,
        id_,
        *PartitionedReplicaIdentifier,
        writeBuffer_);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - TruncateTail - failed to create write buffer. blockMetadataSize: {1}, maxBlockSize: {2}, nextWritePosition: {3}, nextOperationNumber: {4}, id: {5}, status: {6}",
            TraceId,
            blockMetadataSize_,
            maxBlockSize_,
            nextWritePosition_,
            nextOperationNumber_,
            Common::Guid(id_),
            status);

        co_return status;
    }

    if (readContext_.ReadBuffer != nullptr)
    {
        readContext_.ReadBuffer->Dispose();
        readContext_.ReadBuffer = nullptr;
    }

    InvalidateAllReads();

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::WaitCapacityNotificationAsync(
    __in ULONG,
    __in CancellationToken const &)
{
    // todo: implement
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS>
LogicalLog::ConfigureWritesToOnlyDedicatedLogAsync(__in CancellationToken const &)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr context;

    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - ConfigureWritesToOnlyDedicatedLogAsync - Failed to allocate ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::WriteOnlyToDedicatedLog,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - ConfigureWritesToOnlyDedicatedLogAsync - Ioctl failed. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KAssert(outBuf == nullptr);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::ConfigureWritesToSharedAndDedicatedLogAsync(__in CancellationToken const &)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr context;

    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - ConfigureWritesToSharedAndDedicatedLogAsync - Failed to allocate ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::WriteToSharedAndDedicatedLog,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - ConfigureWritesToSharedAndDedicatedLogAsync - Ioctl failed. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KAssert(outBuf == nullptr);

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::QueryLogStreamUsageInternalAsync(
    __in CancellationToken const &,
    __out KLogicalLogInformation::CurrentLogUsageInformation& streamUsageInformation)
{
    // todo: I think there is a bug in the managed code, where EndOperation was called without a corresponding StartOperation

    NTSTATUS status;

    KtlLogStream::AsyncIoctlContext::SPtr context;
    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryLogStreamUsageInternalAsync - Failed to allocate ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::QueryCurrentLogUsageInformation,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryLogStreamUsageInternalAsync - Ioctl failed. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KAssert(outBuf != nullptr);
    KInvariant(outBuf->QuerySize() == sizeof(KLogicalLogInformation::CurrentLogUsageInformation));

    streamUsageInformation = *static_cast<KLogicalLogInformation::CurrentLogUsageInformation*>(outBuf->GetBuffer());
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::QueryLogUsageAsync(
    __out ULONG& percentUsage,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    KLogicalLogInformation::CurrentLogUsageInformation info;

    NTSTATUS status = co_await QueryLogStreamUsageInternalAsync(cancellationToken, info);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - QueryLogUsageAsync - QueryLogStreamUsageInternalAsync failed. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    percentUsage = info.PercentageLogUsage;
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::QueryLogSizeAndSpaceRemainingInternalAsync(
    __in ktl::CancellationToken const & cancellationToken,
    __out ULONGLONG& logSize,
    __out ULONGLONG& spaceRemaining)
{
    NTSTATUS status;
    KLogicalLogInformation::LogSizeAndSpaceRemaining* info;

    KtlLogStream::AsyncIoctlContext::SPtr context;
    status = underlyingStream_->CreateAsyncIoctlContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryLogSizeAndSpaceRemainingInternalAsync - Failed to allocate ioctl context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    ULONG result;
    KBuffer::SPtr outBuf;
    status = co_await context->IoctlAsync(
        KLogicalLogInformation::QueryLogSizeAndSpaceRemaining,
        nullptr,
        result,
        outBuf,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - QueryLogSizeAndSpaceRemainingInternalAsync - Ioctl failed. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    KAssert(outBuf != nullptr);
    KInvariant(outBuf->QuerySize() == sizeof(KLogicalLogInformation::LogSizeAndSpaceRemaining));

    info = static_cast<KLogicalLogInformation::LogSizeAndSpaceRemaining*>(outBuf->GetBuffer());
    logSize = info->LogSize;
    spaceRemaining = info->SpaceRemaining;
    
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS>
LogicalLog::WaitBufferFullNotificationAsync(__in CancellationToken const & cancellationToken)
{
    co_return STATUS_NOT_IMPLEMENTED;
}

VOID
LogicalLog::SetSequentialAccessReadSize(__in ILogicalLogReadStream& logStream, __in LONG sequentialAccessReadSize)
{
    // dynamic_cast yields null for failed ptr casts
    LogicalLogStream* streamPtr = dynamic_cast<LogicalLogStream*>(&logStream);
    KInvariant(streamPtr != nullptr);

    streamPtr->SetSequentialAccessReadSize(sequentialAccessReadSize);
}
