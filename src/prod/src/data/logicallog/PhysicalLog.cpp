// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace ktl::kservice;
using namespace Data::Log;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("PhysicalLog");

// todo: move to someplace common
inline ULONG KGuidHashFn(const KGuid &Key)
{
    return K_DefaultHashFunction(Key);
}

FAILABLE
PhysicalLog::PhysicalLog(
    __in LogManager& manager,
    __in KGuid id,
    __in KString const & path,
    __in KtlLogContainer& underlyingContainer)
    : nextHandleId_(0)
    , id_(id)
    , path_(&path)
    , underlyingContainer_(&underlyingContainer)
    , handles_(13, K_DefaultHashFunction, GetThisAllocator())
    , logicalLogs_(653, &KGuidHashFn, GetThisAllocator())
    , manager_(&manager)
    , lock_(FALSE, TRUE)
{
    if (!NT_SUCCESS(handles_.Status()))
    {
        SetConstructorStatus(handles_.Status());
        return;
    }

    if (!NT_SUCCESS(logicalLogs_.Status()))
    {
        SetConstructorStatus(logicalLogs_.Status());
        return;
    }
}

PhysicalLog::~PhysicalLog()
{
    KInvariant(handles_.Count() == 0);
    KInvariant(logicalLogs_.Count() == 0);
}

NTSTATUS PhysicalLog::Create(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in LogManager& manager,
    __in KGuid const & id,
    __in KString const & path,
    __in KtlLogContainer& underlyingContainer,
    __out PhysicalLog::SPtr& physicalLog)
{
    NTSTATUS status;

    physicalLog = _new(allocationTag, allocator) PhysicalLog(manager, id, path, underlyingContainer);
    if (!physicalLog)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }

    status = physicalLog->Status();
    if (!NT_SUCCESS(status))
    {
        return Ktl::Move(physicalLog)->Status();
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> PhysicalLog::OpenAsync(__in CancellationToken const & cancellationToken)
{
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
            "OpenAsync - failed to create OpenAwaiter. Status: {0}",
            status);

        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

VOID PhysicalLog::OnServiceOpen()
{
    SetDeferredCloseBehavior();

    CompleteOpen(STATUS_SUCCESS);
}

Awaitable<NTSTATUS> PhysicalLog::CloseAsync(__in CancellationToken const & cancellationToken)
{
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
            "CloseAsync - failed to create CloseAwaiter. Status: {0}",
            status);

        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
// todo: add locking asserts to onserviceclose

Awaitable<NTSTATUS> PhysicalLog::GetHandle(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in CancellationToken const& cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KCoService$ApiEntry(TRUE);

    // #1 - Acquire an activity representing the PhysicalLogHandle, to be released when the handle reports closure in OnCloseHandle.
    BOOLEAN handleAcivityAcquired = TryAcquireServiceActivity(); // acquire a service activity to defer close until the handle is closed.
    if (!handleAcivityAcquired)
    {
        co_return K_STATUS_API_CLOSED;
    }

    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        PhysicalLogHandle::SPtr result;
        status = PhysicalLogHandle::Create(GetThisAllocationTag(), GetThisAllocator(), *manager_, *this, nextHandleId_++, prId, result);
        if (!NT_SUCCESS(status))
        {
            // #1 - Release the activity representing the PhysicalLogHandle acquired in GetHandle.
            ReleaseServiceActivity();

            WriteError(
                TraceComponent,
                "{0} - GetHandle: Failed to create PhysicalLogHandle. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        status = co_await result->OpenAsync(cancellationToken);
        if (!NT_SUCCESS(status))
        {
            // #1 - Release the activity representing the PhysicalLogHandle acquired in GetHandle.
            ReleaseServiceActivity();

            WriteError(
                TraceComponent,
                "{0} - GetHandle: Failed to open PhysicalLogHandle. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        KWeakRef<PhysicalLogHandle>::SPtr ref;
        LONGLONG id = result->Id;
        status = handles_.Put(id, ref);
        if (!NT_SUCCESS(status))
        {
            // #1 - Release the activity representing the PhysicalLogHandle acquired in GetHandle.
            ReleaseServiceActivity();
            
            WriteError(
                TraceComponent,
                "{0} - GetHandle: Failed to insert handle into handles list. Status: {1}",
                prId.TraceId,
                status);

            NTSTATUS status2 = co_await result->CloseAsync(cancellationToken);
            if (!NT_SUCCESS(status2))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - GetHandle: Failed to close handle when compensating for failure. Status: {1}",
                    prId.TraceId,
                    status2);
            }

            co_return status;
        }

        physicalLog = result.RawPtr();
        co_return STATUS_SUCCESS;
    }
}

Awaitable<NTSTATUS> PhysicalLog::OnCloseHandle(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in PhysicalLogHandle& toClose,
    __in CancellationToken const& cancellationToken,
    __out BOOLEAN& lastHandleClosed)
{
    // close may be started but waiting for activities to be released

    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        LONGLONG id = toClose.Id;
        status = handles_.Remove(id, nullptr);
        KInvariant(NT_SUCCESS(status)); // handle missing from handles_

        // #1 - Release the activity representing the PhysicalLogHandle acquired in GetHandle.
        ReleaseServiceActivity();

        if ((handles_.Count() == 0) && (logicalLogs_.Count() == 0))
        {
            lastHandleClosed = TRUE;

            status = co_await underlyingContainer_->CloseAsync(nullptr);
            if (!NT_SUCCESS(status))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnCloseHandle: Failed to close underlying container when last handle closed. Status: {1}",
                    prId.TraceId,
                    status);
            }

            NTSTATUS closeStatus = co_await CloseAsync(cancellationToken);
            if (!NT_SUCCESS(closeStatus))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnCloseHandle: Close of PhysicalLog failed. Status: {1}",
                    prId.TraceId,
                    closeStatus);
            }

            if (!NT_SUCCESS(status))
            {
                co_return status;
            }

            co_return closeStatus;
        }
        else
        {
            lastHandleClosed = FALSE;

            co_return STATUS_SUCCESS;
        }
    }
}

Awaitable<NTSTATUS> PhysicalLog::OnCloseLogicalLog(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in LogicalLog& toClose,
    __in CancellationToken const & cancellationToken,
    __out BOOLEAN& lastHandleClosed)
{
    // close may be started but waiting for activities to be released

    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        LogicalLogInfo llInfo;
        status = logicalLogs_.Get(toClose.Id, llInfo);
        KInvariant(NT_SUCCESS(status));

        status = co_await llInfo.UnderlyingStream->CloseAsync(nullptr);
        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_OBJECT_NO_LONGER_EXISTS)
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnCloseLogicalLog: failed to close underlying stream (delete may have raced ahead). Status: {1}",
                    prId.TraceId,
                    status);
            }
            else
            {
                WriteError(
                    TraceComponent,
                    "{0} - OnCloseLogicalLog: failed to close underlying stream. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }
        }

        status = logicalLogs_.Remove(toClose.Id);
        KInvariant(NT_SUCCESS(status));

        // #2 OR #3 - Release the activity representing the LogicalLog acquired in OnCreateAndOpenLogicalLogAsync or OnOpenLogicalLogAsync.
        ReleaseServiceActivity();

        if ((handles_.Count() == 0) && (logicalLogs_.Count() == 0))
        {
            lastHandleClosed = TRUE;

            status = co_await underlyingContainer_->CloseAsync(nullptr);
            if (!NT_SUCCESS(status))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnCloseLogicalLog: Failed to close underlying container when last logical log closed. Status: {1}",
                    prId.TraceId,
                    status);
            }

            NTSTATUS closeStatus = co_await CloseAsync(cancellationToken);
            if (!NT_SUCCESS(closeStatus))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnCloseLogicalLog: Close of PhysicalLog failed. Status: {1}",
                    prId.TraceId,
                    closeStatus);
            }

            if (!NT_SUCCESS(status))
            {
                co_return status;
            }

            co_return closeStatus;
        }
        else
        {
            lastHandleClosed = FALSE;

            co_return STATUS_SUCCESS;
        }
    }
}

Awaitable<NTSTATUS> PhysicalLog::OnCreateAndOpenLogicalLogAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in PhysicalLogHandle& owningHandle,
    __in KGuid const & logicalLogId,
    __in_opt KString::CSPtr & optionalLogStreamAlias,
    __in KString const & path,
    __in_opt KBuffer::SPtr securityDescriptor,
    __in LONGLONG maximumSize,
    __in ULONG maximumBlockSize,
    __in LogCreationFlags creationFlags,
    __in CancellationToken const & cancellationToken,
    __out ILogicalLog::SPtr& logicalLog)
{
    // close may be started but waiting for activities to be released

    // #2 - Acquire an activity representing the LogicalLog, to be released when the logical log reports closure in OnCloseLogicalLog.
    BOOLEAN logicalLogAcivityAcquired = TryAcquireServiceActivity(); // acquire a service activity to defer close until the logical log is closed.
    if (!logicalLogAcivityAcquired)
    {
        co_return K_STATUS_API_CLOSED;
    }
    
    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        LogicalLogInfo llInfo;

        KString::CSPtr actualPath;

#if !defined(PLATFORM_UNIX)
        KStringView windowsPrefix(L"\\??\\");

        if (path.Length() < windowsPrefix.Length()
            || path.SubString(0, windowsPrefix.Length()) != windowsPrefix)
        {
            KString::SPtr prefixedPath;
            status = KString::Create(
                prefixedPath,
                GetThisAllocator(),
                windowsPrefix.Length() + path.Length() + 12);
            if (!NT_SUCCESS(status))
            {
                co_return status;
            }

            BOOL res = prefixedPath->Concat(windowsPrefix);
            if (!res)
            {
                co_return STATUS_INSUFFICIENT_RESOURCES;
            }

            res = prefixedPath->Concat(path);
            if (!res)
            {
                co_return STATUS_INSUFFICIENT_RESOURCES;
            }

            actualPath = prefixedPath.RawPtr();
        }
        else
        {
            actualPath = KString::CSPtr(&path);
        }
#else
        actualPath = KString::CSPtr(&path);
#endif

        // if the logical log info is not in the list (meaning there is no weakref),
        // or if it is in the list and the weakref is not alive (meaning the logical log has gone away)
        if (((status = logicalLogs_.Get(logicalLogId, llInfo)) == STATUS_NOT_FOUND)
            || (NT_SUCCESS(status) && (llInfo.LogicalLog->TryGetTarget() == nullptr)))
        {
            KtlLogStream::SPtr underlyingStream;
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr context;

            status = underlyingContainer_->CreateAsyncCreateLogStreamContext(context);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the LogicalLog acquired in OnCreateAndOpenLogicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenLogicalLogAsync: CreateAsyncCreateLogStreamContext failed. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }

            status = co_await context->CreateLogStreamAsync(
                logicalLogId,
                KLogicalLogInformation::GetLogicalLogStreamType(),
                optionalLogStreamAlias,
                actualPath,
                securityDescriptor,
                0,
                maximumSize,
                maximumBlockSize,
                static_cast<DWORD>(creationFlags),
                underlyingStream,
                nullptr);

            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the LogicalLog acquired in OnCreateAndOpenLogicalLogAsync.
                ReleaseServiceActivity();

                RvdLogStreamType streamType = KLogicalLogInformation::GetLogicalLogStreamType();
                GUID& asGuid = static_cast<GUID&>(streamType.GetReference());

                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenLogicalLogAsync: CreateLogStreamAsync failed.  Status: {1}  Id: {2}  StreamType: {3}  Alias: {4}  Path: {5}  MaximumSize: {6}  MaximumBlockSize: {7}  CreationFlags: {8}",
                    prId.TraceId,
                    status,
                    Common::Guid(logicalLogId),
                    Common::Guid(asGuid),
                    optionalLogStreamAlias != nullptr ? ToStringLiteral(*optionalLogStreamAlias) : L"nullptr",
                    ToStringLiteral(*actualPath),
                    maximumSize,
                    maximumBlockSize,
                    creationFlags);

                co_return status;
            }

            LogicalLog::SPtr newLogicalLog;
            status = LogicalLog::Create(
                GetThisAllocationTag(),
                GetThisAllocator(),
                *this,
                *manager_,
                owningHandle.Id,
                logicalLogId,
                *underlyingStream,
                prId,
                newLogicalLog);

            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenLogicalLogAsync: Failed to create Logical Log. Status: {1}",
                    prId.TraceId,
                    status);

                goto HandleError;
            }

            status = co_await newLogicalLog->OpenForCreate(cancellationToken);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenLogicalLogAsync: Failed to open newly created Logical Log. Status: {1}",
                    prId.TraceId,
                    status);

                goto HandleError;
            }

            if (llInfo.LogicalLog == nullptr)
            {
                // if the llInfo was not in the list, llInfo did not get assigned to, so its weakref::SPtr is the default value (nullptr)
                status = logicalLogs_.Put(logicalLogId, LogicalLogInfo(*newLogicalLog, *underlyingStream));
                if (!NT_SUCCESS(status))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnCreateAndOpenLogicalLogAsync: Failed to insert new logical log into logs list. Status: {1}",
                        prId.TraceId,
                        status);

                    goto HandleError;
                }
            }
            else
            {
                // llInfo was in the list, so its kweakref::sptr is valid (but the weakref is not alive)
                llInfo.LogicalLog = newLogicalLog->GetWeakRef();
            }

            WriteInfo(
                TraceComponent,
                "{0} - OnCreateAndOpenLogicalLogAsync: Created logical log with id {1}.",
                prId.TraceId,
                Common::Guid(logicalLogId));

            logicalLog = newLogicalLog.RawPtr();
            co_return STATUS_SUCCESS;

        HandleError:

            // #2 - Release the activity representing the LogicalLog acquired in OnCreateAndOpenLogicalLogAsync.
            ReleaseServiceActivity();

            if (newLogicalLog != nullptr && newLogicalLog->IsOpen())
            {
                NTSTATUS closeStatus = co_await newLogicalLog->CloseAsync(cancellationToken);

                if (!NT_SUCCESS(closeStatus))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenLogicalLogAsync: Failed to close new logical log when cleaning up after failure. Status: {1}",
                        prId.TraceId,
                        status);
                }
            }

            logicalLogs_.Remove(logicalLogId);

            // consider tracing these status
            {
                NTSTATUS closeStatus = co_await underlyingStream->CloseAsync(nullptr);

                if (!NT_SUCCESS(closeStatus))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenLogicalLogAsync: Failed to close underlying stream when cleaning up after failure. Status: {1}",
                        prId.TraceId,
                        status);
                }
            }

            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteContext;
            NTSTATUS status2 = underlyingContainer_->CreateAsyncDeleteLogStreamContext(deleteContext);
            if (NT_SUCCESS(status2))
            {
                NTSTATUS deleteStatus = co_await deleteContext->DeleteLogStreamAsync(logicalLogId, nullptr);

                if (!NT_SUCCESS(deleteStatus))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenLogicalLogAsync: Failed to delete new log stream when cleaning up after failure. Status: {1}",
                        prId.TraceId,
                        status);
                }
            }

            co_return status;
        }
        else
        {
            // #2 - Release the activity representing the LogicalLog acquired in OnCreateAndOpenLogicalLogAsync.
            ReleaseServiceActivity();

            if (NT_SUCCESS(status))  // if the llinfo was in the list and its weakref was active
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - OnCreateAndOpenLogicalLogAsync: Attempt to create logical log with id {1} rejected, as a log with this id already exists.",
                    prId.TraceId,
                    Common::Guid(logicalLogId));

                co_return STATUS_OBJECT_NAME_COLLISION;
            }
            else  // some error status trying to check the weakref status
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnCreateAndOpenLogicalLogAsync: Failed to lookup logical log with id {1}. Status: {2}",
                    prId.TraceId,
                    Common::Guid(logicalLogId),
                    status);

                co_return status;
            }
        }
    }
}

Awaitable<NTSTATUS> PhysicalLog::OnOpenLogicalLogAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in PhysicalLogHandle& owningHandle,
    __in KGuid const & logicalLogId,
    __in CancellationToken const & cancellationToken,
    __out ILogicalLog::SPtr& logicalLog)
{
    // close may be started but waiting for activities to be released

    // #3 - Acquire an activity representing the LogicalLog, to be released when the logical log reports closure in OnCloseLogicalLog.
    BOOLEAN logicalLogAcivityAcquired = TryAcquireServiceActivity(); // acquire a service activity to defer close until the logical log is closed.
    if (!logicalLogAcivityAcquired)
    {
        co_return K_STATUS_API_CLOSED;
    }
    
    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        LogicalLogInfo llInfo;

        // if the logical log info is not in the list (meaning there is no weakref),
        // or if it is in the list and the weakref is not alive (meaning the logical log has gone away)
        if (((status = logicalLogs_.Get(logicalLogId, llInfo)) == STATUS_NOT_FOUND)
            || (NT_SUCCESS(status) && (llInfo.LogicalLog->TryGetTarget() == nullptr)))
        {
            KtlLogStream::SPtr underlyingStream;
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr context;
            int retries = 0;
            const int retryLimit = 3;

            do
            {
                status = underlyingContainer_->CreateAsyncOpenLogStreamContext(context);
                if (!NT_SUCCESS(status))
                {
                    // #3 - Release the activity representing the LogicalLog acquired in OnOpenLogicalLogAsync.
                    ReleaseServiceActivity();

                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenLogicalLogAsync: Failed to create OpenLogStream context. Status: {1}",
                        prId.TraceId,
                        status);

                    co_return status;
                }

                ULONG mms;
                status = co_await context->OpenLogStreamAsync(logicalLogId, &mms, underlyingStream, nullptr);

                if (NT_SUCCESS(status))
                {
                    retries = retryLimit;
                }
                else if (HRESULT_FROM_NT(status) == 0x80070020)
                {
                    retries++;
                    if (retries == retryLimit)
                    {
                        WriteWarning(
                            TraceComponent,
                            "{0} - OnOpenLogicalLogAsync: Exhausted retry when attempting to open log stream with id {1}. Status: {2}",
                            prId.TraceId,
                            Common::Guid(logicalLogId),
                            status);

                        co_return status;
                    }

                    // todo?: backoff before retry
                }
                else
                {
                    // #3 - Release the activity representing the LogicalLog acquired in OnOpenLogicalLogAsync.
                    ReleaseServiceActivity();

                    KAssert(!NT_SUCCESS(status));

                    WriteWarning(
                        TraceComponent,
                        "{0} - OnOpenLogicalLogAsync: Failed to open log stream with id {1}. Status: {2}",
                        prId.TraceId,
                        Common::Guid(logicalLogId),
                        status);

                    co_return status;
                }
            } while (retries < retryLimit);

            KAssert(NT_SUCCESS(status));

            LogicalLog::SPtr newLogicalLog;
            status = LogicalLog::Create(
                GetThisAllocationTag(),
                GetThisAllocator(),
                *this,
                *manager_,
                owningHandle.Id,
                logicalLogId,
                *underlyingStream,
                prId,
                newLogicalLog);

            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - OnOpenLogicalLogAsync: Failed to create logical log. Status: {1}",
                    prId.TraceId,
                    status);

                goto HandleError;
            }

            status = co_await newLogicalLog->OpenForRecover(cancellationToken);
            if (!NT_SUCCESS(status))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnOpenLogicalLogAsync: Failed to to open logical log with id {1}. Status: {2}",
                    prId.TraceId,
                    Common::Guid(logicalLogId),
                    status);

                goto HandleError;
            }

            if (llInfo.LogicalLog == nullptr)
            {
                // if the llInfo was not in the list, llInfo did not get assigned to, so its weakref::SPtr is the default value (nullptr)
                status = logicalLogs_.Put(logicalLogId, LogicalLogInfo(*newLogicalLog, *underlyingStream));
                if (!NT_SUCCESS(status))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenLogicalLogAsync: Failed to insert logical log into list. Status: {1}",
                        prId.TraceId,
                        status);

                    goto HandleError;
                }
            }
            else
            {
                // llInfo was in the list, so its kweakref::sptr is valid (but the weakref is not alive)
                llInfo.LogicalLog = newLogicalLog->GetWeakRef();
            }

            WriteInfo(
                TraceComponent,
                "{0} - OnOpenLogicalLogAsync: Opened logical log with id {1}.",
                prId.TraceId,
                Common::Guid(logicalLogId));

            logicalLog = newLogicalLog.RawPtr();
            co_return STATUS_SUCCESS;

        HandleError:

            // #3 - Release the activity representing the LogicalLog acquired in OnOpenLogicalLogAsync.
            ReleaseServiceActivity();

            NTSTATUS status2;
            if (newLogicalLog != nullptr && newLogicalLog->IsOpen())
            {
                status2 = co_await newLogicalLog->CloseAsync(cancellationToken);

                if (!NT_SUCCESS(status2))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnOpenLogicalLogAsync: Failed to close logical log with id {1} when cleaning up after failure. Status: {2}",
                        prId.TraceId,
                        Common::Guid(logicalLogId),
                        status);
                }
            }

            logicalLogs_.Remove(logicalLogId);

            status2 = co_await underlyingStream->CloseAsync(nullptr);

            if (!NT_SUCCESS(status2))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnOpenLogicalLogAsync: Failed to close underlying stream when cleaning up after failure. Status: {1}",
                    prId.TraceId,
                    status);
            }

            co_return status;
        }
        else
        {
            // #3 - Release the activity representing the LogicalLog acquired in OnOpenLogicalLogAsync.
            ReleaseServiceActivity();

            if (NT_SUCCESS(status))  // if the llinfo was in the list and its weakref was active
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - OnOpenLogicalLogAsync: Attempt to open logical log with id {1} rejected, as a log with this id already exists and is open.",
                    prId.TraceId,
                    Common::Guid(logicalLogId));

                co_return STATUS_OBJECT_NAME_COLLISION;
            }
            else  // some error status trying to check the weakref status
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnOpenLogicalLogAsync: Failed to lookup logical log with id {1}. Status: {2}",
                    prId.TraceId,
                    Common::Guid(logicalLogId),
                    status);

                co_return status;
            }
        }
    }
}

Awaitable<NTSTATUS> PhysicalLog::OnRemoveAliasAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in KString const & alias,
    __in CancellationToken const & cancellationToken)
{
    // todo: implement
    co_return STATUS_NOT_IMPLEMENTED;
}

Awaitable<NTSTATUS> PhysicalLog::OnReplaceAliasLogsAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in KString const & sourceLogAliasName,
    __in KString const & logAliasName,
    __in KString const & backupLogAliasName,
    __in CancellationToken const & cancellationToken)
{
    {
        AsyncLockBlock(lock_)

        KtlLogStreamId logAliasGuid;
        KtlLogStreamId sourceLogAliasGuid;
        KtlLogStreamId idOfOldBackupLog;

        KString::CSPtr constLogAliasName = KString::CSPtr(&logAliasName);
        KString::CSPtr constSourceLogAliasName = KString::CSPtr(&sourceLogAliasName);
        KString::CSPtr constBackupLogAliasName = KString::CSPtr(&backupLogAliasName);

        KtlLogContainer::AsyncResolveAliasContext::SPtr context;
        NTSTATUS status = underlyingContainer_->CreateAsyncResolveAliasContext(context);

        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: Failed to allocate resolve alias context. Status: {1}",
                prId.TraceId,
                status);
            
            co_return status;
        }

        status = co_await context->ResolveAliasAsync(
            constLogAliasName,
            &logAliasGuid,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: ResolveAlias failed for alias {1}. Status: {2}",
                prId.TraceId,
                ToStringLiteral(logAliasName),
                status);

            co_return status;
        }

        context->Reuse();
        status = co_await context->ResolveAliasAsync(
            constSourceLogAliasName,
            &sourceLogAliasGuid,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: ResolveAlias failed for source alias {1}. Status: {2}",
                prId.TraceId,
                ToStringLiteral(sourceLogAliasName),
                status);

            co_return status;
        }

        WriteInfo(
            TraceComponent,
            "{0} - OnReplaceAliasLogsAsync: Enter {1} {2}, {3} {4}, {5}",
            prId.TraceId,
            ToStringLiteral(sourceLogAliasName),
            Common::Guid(sourceLogAliasGuid.GetReference()),
            ToStringLiteral(logAliasName),
            Common::Guid(logAliasGuid.GetReference()),
            ToStringLiteral(backupLogAliasName));

        context->Reuse();
        status = co_await context->ResolveAliasAsync(
            constBackupLogAliasName,
            &idOfOldBackupLog,
            nullptr);

        // The backup log alias may not be found
        if (status == STATUS_NOT_FOUND)
        {
            WriteInfo(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: GetBackupLogId resolve failure (not found) for backup alias {1}.",
                prId.TraceId,
                ToStringLiteral(backupLogAliasName));
        }
        else if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: GetBackupLogId unexpected resolve falure for backup alias {1}. Status: {2}",
                prId.TraceId,
                ToStringLiteral(backupLogAliasName),
                status);
            
            co_return status;
        }

        if (idOfOldBackupLog != logAliasGuid)
        {
            // Delete underlying logical log
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteContext;
            status = underlyingContainer_->CreateAsyncDeleteLogStreamContext(deleteContext);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - OnReplaceAliasLogsAsync: Failed to allocate delete log stream context. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }

            status = co_await deleteContext->DeleteLogStreamAsync(idOfOldBackupLog, nullptr);

            // Delete may have failed if the backup log alias is not found
            if (status == STATUS_NOT_FOUND)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - OnReplaceAliasLogsAsync: DeleteBackupLog failure (not found) for backup alias {1} and id {2}.",
                    prId.TraceId,
                    ToStringLiteral(backupLogAliasName),
                    Common::Guid(idOfOldBackupLog.GetReference()));
            }
            else if (!NT_SUCCESS(status))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - OnReplaceAliasLogsAsync: DeleteBackupLog unexpected failure for backup alias {1} and id {2}. Status: {3}",
                    prId.TraceId,
                    ToStringLiteral(backupLogAliasName),
                    Common::Guid(idOfOldBackupLog.GetReference()),
                    status);
                
                co_return status;
            }
        }

        KtlLogContainer::AsyncAssignAliasContext::SPtr logAliasAssignContext;
        status = underlyingContainer_->CreateAsyncAssignAliasContext(logAliasAssignContext);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: Failed to allocate assign alias context. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        // Reassign the log alias to the id of the old backup log alias
        status = co_await logAliasAssignContext->AssignAliasAsync(
            constBackupLogAliasName,
            logAliasGuid,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: AssignAlias unexpected failure for backup alias {1} and log id {2}. Status: {3}",
                prId.TraceId,
                ToStringLiteral(backupLogAliasName),
                Common::Guid(logAliasGuid.GetReference()),
                status);

            co_return status;
        }

        KtlLogContainer::AsyncAssignAliasContext::SPtr sourceLogAliasAssignContext;
        status = underlyingContainer_->CreateAsyncAssignAliasContext(sourceLogAliasAssignContext);

        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: Failed to allocate assign alias context. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        status = co_await sourceLogAliasAssignContext->AssignAliasAsync(
            constLogAliasName,
            sourceLogAliasGuid,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnReplaceAliasLogsAsync: AssignAlias unexpected failure for alias {1} and source id {2}.  Status: {3}",
                prId.TraceId,
                ToStringLiteral(logAliasName),
                Common::Guid(sourceLogAliasGuid.GetReference()),
                status);

            co_return status;
        }

        co_return STATUS_SUCCESS;
    }
}

Awaitable<NTSTATUS> PhysicalLog::OnRecoverAliasLogsAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in KString const & sourceLogAliasName,
    __in KString const & logAliasName,
    __in KString const & backupLogAliasName,
    __in CancellationToken const & cancellationToken,
    __out KGuid& logicalLogId)
{
    KtlLogContainer::AsyncResolveAliasContext::SPtr context;
    NTSTATUS status = underlyingContainer_->CreateAsyncResolveAliasContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - OnRecoverAliasLogsAsync: Failed to allocate resolve alias context. Status: {1}",
            prId.TraceId,
            status);

        co_return status;
    }

    KtlLogStreamId logId;
    KString::CSPtr constLogAliasName = KString::CSPtr(&logAliasName);
    KString::CSPtr constBackupLogAliasName = KString::CSPtr(&backupLogAliasName);

    status = co_await context->ResolveAliasAsync(
        constLogAliasName,
        &logId,
        nullptr);

    if (status == STATUS_NOT_FOUND)  // FabricElementNotFoundException?
    {
        // Desired alias missing - could be due to result of failure in OnReplaceAliasLogsAsync during 
        // Steps 1 and 2 - rollback
        KtlLogStreamId idOfBackupLogAlias;

        KtlLogContainer::AsyncResolveAliasContext::SPtr backupAliasContext;
        status = underlyingContainer_->CreateAsyncResolveAliasContext(backupAliasContext); // todo: reuse previous context
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - OnRecoverAliasLogsAsync: Failed to allocate resolve alias context. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        // Resolve backup log alias
        status = co_await backupAliasContext->ResolveAliasAsync(
            constBackupLogAliasName,
            &idOfBackupLogAlias,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteInfo(
                TraceComponent,
                "{0} - OnRecoverAliasLogsAsync: ResolveAlias failed for backup alias {1}. Status: {2}",
                prId.TraceId,
                ToStringLiteral(backupLogAliasName),
                status);

            co_return status;
        }

        KtlLogContainer::AsyncAssignAliasContext::SPtr assignAliasContext;
        status = underlyingContainer_->CreateAsyncAssignAliasContext(assignAliasContext);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - OnRecoverAliasLogsAsync: Failed to allocate assign alias context. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        // Reassign the log alias to the id of the current backup log alias
        status = co_await assignAliasContext->AssignAliasAsync(
            constLogAliasName,
            idOfBackupLogAlias,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnRecoverAliasLogsAsync: AssignAlias unexpected failure for alias {1} and backup id {2}. Status: {3}",
                prId.TraceId,
                ToStringLiteral(logAliasName),
                Common::Guid(idOfBackupLogAlias.GetReference()),
                status);

            co_return status;
        }

        logicalLogId = idOfBackupLogAlias.Get();
        co_return status;
    }
    else if(!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - OnRecoverAliasLogsAsync: ResolveAlias failed for alias {1}. Status: {2}",
            prId.TraceId,
            ToStringLiteral(logAliasName),
            status);

        co_return status;
    }

    logicalLogId = logId.Get();
    co_return status;
}
