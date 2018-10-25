// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace ktl::kservice;
using namespace Data;
using namespace Data::Log;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("PhysicalLogHandle");

PhysicalLogHandle::PhysicalLogHandle(
    __in LogManager& manager,
    __in PhysicalLog& owner,
    __in LONGLONG id,
    __in Data::Utilities::PartitionedReplicaId const & prId)
    : KAsyncServiceBase()
    , KWeakRefType<PhysicalLogHandle>()
    , PartitionedReplicaTraceComponent(prId)
    , manager_(&manager)
    , owner_(&owner)
    , id_(id)
{
}

PhysicalLogHandle::~PhysicalLogHandle()
{
}

NTSTATUS PhysicalLogHandle::Create(
    __in ULONG allocationTag,
    __in KAllocator& allocator,
    __in LogManager& manager,
    __in PhysicalLog& owner,
    __in LONGLONG id,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __out PhysicalLogHandle::SPtr& handle)
{
    NTSTATUS status;

    handle = _new(allocationTag, allocator) PhysicalLogHandle(manager, owner, id, prId);
    if (!handle)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);

        WriteError(
            TraceComponent,
            "{0} - Create - Failed to allocate PhysicalLogHandle",
            prId.TraceId);

        return status;
    }

    status = handle->Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - Create - PhysicalLogHandle constructed with error status. Status: {1}",
            prId.TraceId,
            status);

        return Ktl::Move(handle)->Status();
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> PhysicalLogHandle::OpenAsync(__in CancellationToken const & cancellationToken)
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
            "{0} - OpenAsync - failed to create OpenAwaiter. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

VOID PhysicalLogHandle::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    CompleteOpen(STATUS_SUCCESS);
}

void PhysicalLogHandle::Dispose()
{
    Abort();
}

void PhysicalLogHandle::Abort()
{
    AbortTask();
}

Task PhysicalLogHandle::AbortTask()
{
    KCoShared$ApiEntry();

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
                    "{0} - AbortTask - CloseAsync failed during abort (likely already closed).  Status: {1}",
                    TraceId,
                    status);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - AbortTask - CloseAsync failed during abort.  Status: {1}",
                    TraceId,
                    status);
            }
        }
    }
}

Awaitable<NTSTATUS> PhysicalLogHandle::CloseAsync(__in CancellationToken const & cancellationToken)
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
            "{0} - CloseAsync - failed to create CloseAwaiter. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

VOID PhysicalLogHandle::OnServiceClose()
{
    CloseTask();
}

Task PhysicalLogHandle::CloseTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    NTSTATUS status = co_await manager_->OnPhysicalLogHandleClose(
        *PartitionedReplicaIdentifier,
        *this,
        CancellationToken::None);

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - CloseTask - LogManager::OnPhysicalLogHandleClose failed. Status: {1}",
            TraceId,
            status);
    }

    manager_ = nullptr;
    // owner_ = nullptr;  // todo: managed code keeps the reference, and it's used in the tests.  However we can probably workaround in the tests and null this here for safety

    CompleteClose(status);
}

Awaitable<NTSTATUS> PhysicalLogHandle::CreateAndOpenLogicalLogAsync(
    __in KGuid const & logicalLogId,
    __in_opt KString::CSPtr & optionalLogStreamAlias,
    __in KString const & path,
    __in KBuffer::SPtr securityDescriptor,
    __in LONGLONG maximumSize,
    __in ULONG maximumBlockSize,
    __in LogCreationFlags creationFlags,
    __in CancellationToken const& cancellationToken,
    __out ILogicalLog::SPtr& logicalLog)
{
    KCoService$ApiEntry(TRUE);

    // todo: remove temp vars when oom  bug is fixed
    auto& tmpLogicalLogId = logicalLogId;
    auto& tmpOptionalLogStreamAlias = optionalLogStreamAlias;
    auto& tmpPath = path;
    auto& tmpSecurityDescriptor = securityDescriptor;
    auto& tmpMaximumSize = maximumSize;
    auto& tmpMaximumBlockSize = maximumBlockSize;

    co_return co_await owner_->OnCreateAndOpenLogicalLogAsync(
        *PartitionedReplicaIdentifier,
        *this,
        tmpLogicalLogId,
        tmpOptionalLogStreamAlias,
        tmpPath,
        tmpSecurityDescriptor,
        tmpMaximumSize,
        tmpMaximumBlockSize,
        creationFlags,
        cancellationToken,
        logicalLog);
}

Awaitable<NTSTATUS> PhysicalLogHandle::OpenLogicalLogAsync(
    __in KGuid const & logicalLogId,
    __in CancellationToken const & cancellationToken,
    __out ILogicalLog::SPtr& logicalLog)
{
    KCoService$ApiEntry(TRUE);

    co_return co_await owner_->OnOpenLogicalLogAsync(
        *PartitionedReplicaIdentifier,
        *this,
        logicalLogId,
        cancellationToken,
        logicalLog);
}

Awaitable<NTSTATUS> PhysicalLogHandle::DeleteLogicalLogAsync(
    __in KGuid const & logicalLogId,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    
    switch (manager_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            status = co_await DeleteLogicalLogOnlyAsync(logicalLogId, cancellationToken);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            status = co_await owner_->manager_->DeleteLogicalLogAndMaybeDeletePhysicalLog(
                *PartitionedReplicaIdentifier,
                *this,
                *(owner_->underlyingContainer_),
                *(owner_->path_),
                owner_->Id,
                logicalLogId,
                cancellationToken);
            break;
        }

        default:
            KInvariant(FALSE);
            co_return STATUS_UNSUCCESSFUL;
    }

    co_return status;
}

Awaitable<NTSTATUS> PhysicalLogHandle::DeleteLogicalLogOnlyAsync(
    __in KGuid const & logicalLogId,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    KtlLogContainer::AsyncDeleteLogStreamContext::SPtr context;

    status = owner_->underlyingContainer_->CreateAsyncDeleteLogStreamContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - failed to allocate DeleteLogStream context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    status = co_await context->DeleteLogStreamAsync(logicalLogId, nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteInfo(
            TraceComponent,
            "{0} - failed to delete log stream. id: {1}, status: {2}",
            TraceId,
            Common::Guid(logicalLogId),
            status);
    }

    co_return status;
}

Awaitable<NTSTATUS> PhysicalLogHandle::AssignAliasAsync(
    __in KGuid const & logicalLogId,
    __in KString const & alias,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    KtlLogContainer::AsyncAssignAliasContext::SPtr context;

    status = owner_->underlyingContainer_->CreateAsyncAssignAliasContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - failed to allocate AssignAlias context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    auto aliasCSPtr = KString::CSPtr(&alias);
    status = co_await context->AssignAliasAsync(aliasCSPtr, logicalLogId, nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - failed to assign alias. id: {1}, alias: {2}, status: {3}",
            TraceId,
            Common::Guid(logicalLogId),
            ToStringLiteral(alias),
            status);
    }

    co_return status;
}

Awaitable<NTSTATUS> PhysicalLogHandle::ResolveAliasAsync(
    __in KString const & alias,
    __in CancellationToken const & cancellationToken,
    __out KGuid& logicalLogId)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    KtlLogContainer::AsyncResolveAliasContext::SPtr context;

    status = owner_->underlyingContainer_->CreateAsyncResolveAliasContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - failed to allocate ResolveAlias context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    RvdLogStreamId id;
    auto aliasCSPtr = KString::CSPtr(&alias);
    status = co_await context->ResolveAliasAsync(aliasCSPtr, &id, nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteInfo(
            TraceComponent,
            "{0} - failed to resolve alias. alias: {1}, status: {2}",
            TraceId,
            ToStringLiteral(alias),
            status);

        co_return status;
    }
    
    logicalLogId = id.Get();
    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> PhysicalLogHandle::RemoveAliasAsync(
    __in KString const & alias,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    KtlLogContainer::AsyncRemoveAliasContext::SPtr context;

    status = owner_->underlyingContainer_->CreateAsyncRemoveAliasContext(context);
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - failed to allocate RemoveAlias context. Status: {1}",
            TraceId,
            status);

        co_return status;
    }

    auto aliasCSPtr = KString::CSPtr(&alias);
    status = co_await context->RemoveAliasAsync(aliasCSPtr, nullptr);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceComponent,
            "{0} - failed to remove alias. alias: {1}, status: {2}",
            TraceId,
            ToStringLiteral(alias),
            status);
    }

    co_return status;
}

Awaitable<NTSTATUS> PhysicalLogHandle::ReplaceAliasLogsAsync(
    __in KString const & sourceLogAliasName,
    __in KString const & logAliasName,
    __in KString const & backupLogAliasName,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    co_return co_await owner_->OnReplaceAliasLogsAsync(
        *PartitionedReplicaIdentifier,
        sourceLogAliasName,
        logAliasName,
        backupLogAliasName,
        cancellationToken);
}

Awaitable<NTSTATUS> PhysicalLogHandle::RecoverAliasLogsAsync(
    __in KString const & sourceLogAliasName,
    __in KString const & logAliasName,
    __in KString const & backupLogAliasName,
    __in CancellationToken const & cancellationToken,
    __out KGuid& logicalLogId)
{
    KCoService$ApiEntry(TRUE);

    co_return co_await owner_->OnRecoverAliasLogsAsync(
        *PartitionedReplicaIdentifier,
        sourceLogAliasName,
        logAliasName,
        backupLogAliasName,
        cancellationToken,
        logicalLogId);
}
