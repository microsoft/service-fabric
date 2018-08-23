// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace ktl::kservice;
using namespace Data::Log;

Common::StringLiteral const TraceComponent("LogManagerHandle");

NTSTATUS GetStagingLogPath(
    __in KStringView const & workDirectory,
    __in KGuid const & partitionId,
    __in LONGLONG replicaId,
    __in KAllocator& allocator,
    __out KString::CSPtr& path)
{
    NTSTATUS status;
    BOOLEAN res;
    KString::SPtr stagingLogPath;

    KStringView const extension(L".stlog");

#if !defined(PLATFORM_UNIX)
    KStringView const windowsPrefix(L"\\??\\");
#endif

    ULONG bufferSize =
        workDirectory.Length()
        + (workDirectory.PeekLast() == KVolumeNamespace::PathSeparatorChar ? 0 : 1)   // path separator
#if !defined(PLATFORM_UNIX)
        + windowsPrefix.Length()
#endif
        + KStringView::GuidLengthInChars                                            // partitionId
        + 1                                                                         // '_'
        + 19                                                                        // max digits for a LONGLONG
        + extension.Length()                                                        // extension including L'\0'
        + 12;                                                                       // extra buffer

    status = KString::Create(
        stagingLogPath,
        allocator,
        bufferSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Format: <workDir>/<partitionId>_<replicaId>.stlog

#if !defined(PLATFORM_UNIX)
    if (workDirectory.Length() < windowsPrefix.Length()
        || workDirectory.SubString(0, windowsPrefix.Length()) != windowsPrefix)
    {
        res = stagingLogPath->Concat(windowsPrefix);
        if (!res)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
#endif

    res = stagingLogPath->Concat(workDirectory);
    if (!res)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (stagingLogPath->PeekLast() != KVolumeNamespace::PathSeparatorChar)
    {
        res = stagingLogPath->Concat(KVolumeNamespace::PathSeparator);
        if (!res)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    res = stagingLogPath->FromGUID(partitionId, TRUE);
    if (!res)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    res = stagingLogPath->Concat(L"_");
    if (!res)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    res = stagingLogPath->FromLONGLONG(replicaId, TRUE);
    if (!res)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    res = stagingLogPath->Concat(extension);
    if (!res)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    path = stagingLogPath.RawPtr();
    return STATUS_SUCCESS;
}

NTSTATUS CheckStagingLogPathLength(
    __in Common::StringLiteral const & traceId,
    __in KStringView const & stagingLogPath)
{
    // despite the documentation that it gives the length in code units (the size of which are encoding-defined), this gives
    // the length in WCHAR.  This is only correct when using the windows standard UCS-2 encoding, which has fixed-width code points,
    // each one 16-bit code unit.
    if (stagingLogPath.Length() > KtlLogManager::MaxPathnameLengthInChar - 1) // must leave room for null character
    {
        LogManagerHandle::WriteError(
            TraceComponent,
            "{0} - CheckStagingLogPathLength - Staging log path length exceeds maximum path length.  path: {1}, path length: {2}, Max length: {3}",
            traceId,
            stagingLogPath.operator const wchar_t *(),
            stagingLogPath.Length(),
            KtlLogManager::MaxPathnameLengthInChar - 1);

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

FAILABLE
LogManagerHandle::LogManagerHandle(
    __in long id,
    __in LogManager& owner,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in LPCWSTR workDirectory)
    : KAsyncServiceBase()
    , KWeakRefType<LogManagerHandle>()
    , PartitionedReplicaTraceComponent(prId)
    , id_(id)
    , owner_(&owner)
{
    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            stagingLogPath_ = nullptr;
            break;
        }

        case KtlLoggerMode::InProc:
        {
            NTSTATUS status = GetStagingLogPath(
                workDirectory,
                prId.PartitionId,
                static_cast<LONGLONG>(prId.ReplicaId),
                GetThisAllocator(),
                stagingLogPath_);

            if (!NT_SUCCESS(status))
            {
                SetConstructorStatus(status);

                owner_ = nullptr;

                return;
            }
            break;
        }

        default:
            KInvariant(FALSE);
    }
}

LogManagerHandle::~LogManagerHandle()
{
    KAssert(owner_ == nullptr);
}

NTSTATUS LogManagerHandle::Create(
    __in KAllocator& allocator,
    __in long id,
    __in LogManager& owner,
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in LPCWSTR workDirectory,
    __out LogManagerHandle::SPtr& handle)
{
    NTSTATUS status;

    handle = _new(LOGMANAGER_HANDLE_TAG, allocator) LogManagerHandle(id, owner, prId, workDirectory);
    if (!handle)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);

        WriteError(
            TraceComponent,
            "{0} - Create - Failed to allocate LogManagerHandle",
            prId.TraceId);

        return status;
    }

    status = handle->Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - Create - LogManagerHandle constructed with error status. Status: {1}",
            prId.TraceId,
            status);

        return Ktl::Move(handle)->Status();
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogManagerHandle::OpenAsync(__in CancellationToken const& cancellationToken)
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

VOID LogManagerHandle::OnServiceOpen()
{
    SetDeferredCloseBehavior();

    CompleteOpen(STATUS_SUCCESS);
}

Awaitable<NTSTATUS> LogManagerHandle::CloseAsync(__in CancellationToken const & cancellationToken)
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

VOID LogManagerHandle::OnServiceClose()
{
    CloseTask();
}

Task LogManagerHandle::CloseTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    NTSTATUS status = co_await owner_->OnCloseHandle(
        *PartitionedReplicaIdentifier,
        *this,
        CancellationToken::None);

    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "{0} - CloseTask - LogManager::OnCloseHandle failed. Status: {1}",
            TraceId,
            status);
    }

    owner_ = nullptr;

    CompleteClose(status);
}

VOID LogManagerHandle::Abort()
{
    AbortTask();
}

Task LogManagerHandle::AbortTask()
{
    KCoShared$ApiEntry(); // explicitly keep this alive

    if (IsOpen())
    {
        NTSTATUS status = co_await CloseAsync(CancellationToken::None);
        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_UNSUCCESSFUL && !IsOpen())
            {
                // This likely means that the LogicalLog was already closed when CloseAsync was initiated.
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

VOID LogManagerHandle::Dispose()
{
    Abort();
}

KGuid const & ToKGuid(__in RvdLogId const & logId)
{
    return static_cast<KGuid&>(const_cast<RvdLogId&>(logId).GetReference());
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::CreateAndOpenPhysicalLogAsync(
    __in KString const & pathToCommonContainer,
    __in KGuid const & physicalLogId,
    __in LONGLONG containerSize,
    __in ULONG maximumNumberStreams,
    __in ULONG maximumLogicalLogBlockSize,
    __in LogCreationFlags creationFlags,
    __in CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    
    WriteInfo(
        TraceComponent,
        "{0} - CreateAndOpenPhysicalLogAsync - Creating physical log. Path0 {1} Path1 {2} Path3 {3} Id0 {4} Id1 {5}",
        PartitionedReplicaIdentifier->TraceId,
        pathToCommonContainer.operator const wchar_t *(),
        owner_->sharedLogName_ == nullptr ? L"" : owner_->sharedLogName_->operator const wchar_t *(),
        owner_->sharedLogSettings_ == nullptr ? L"" : owner_->sharedLogSettings_->Settings.Path,
        Common::Guid(physicalLogId),
        owner_->sharedLogSettings_ == nullptr ? Common::Guid::Empty() : Common::Guid(ToKGuid(owner_->sharedLogSettings_->Settings.LogContainerId)));
    
    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            status = co_await owner_->OnCreateAndOpenPhysicalLogAsync(
                *PartitionedReplicaIdentifier,
                pathToCommonContainer,
                physicalLogId,
                containerSize,
                maximumNumberStreams,
                maximumLogicalLogBlockSize,
                creationFlags,
                cancellationToken,
                physicalLog);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            if (physicalLogId == KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID())
            {
                status = co_await CreateAndOpenStagingLogAsync(cancellationToken, physicalLog);
            }
            else
            {
                status = co_await owner_->OnCreateAndOpenPhysicalLogAsync(
                    *PartitionedReplicaIdentifier,
                    pathToCommonContainer,
                    physicalLogId,
                    containerSize,
                    maximumNumberStreams,
                    maximumLogicalLogBlockSize,
                    creationFlags,
                    cancellationToken,
                    physicalLog);
            }
            break;
        }

        default:
            KInvariant(FALSE);
            co_return STATUS_UNSUCCESSFUL;
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::CreateAndOpenPhysicalLogAsync(
    __in CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KCoService$ApiEntry(TRUE);

    KInvariant(owner_->sharedLogSettings_ != nullptr);
    KInvariant(owner_->sharedLogName_ != nullptr);

    NTSTATUS status;
    KStringView const * name = nullptr;
    KGuid const & id = ToKGuid(owner_->sharedLogSettings_->Settings.LogContainerId);

    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            name = owner_->sharedLogName_.RawPtr();

            WriteInfo(
                TraceComponent,
                "{0} - CreateAndOpenPhysicalLogAsync - Creating physical log. Path0 {1} Path1 {2} Id0 {3} Id1 {4}",
                PartitionedReplicaIdentifier->TraceId,
                name->operator const wchar_t *(),
                owner_->sharedLogSettings_->Settings.Path,
                Common::Guid(id),
                Common::Guid(ToKGuid(owner_->sharedLogSettings_->Settings.LogContainerId)));

            status = co_await owner_->OnCreateAndOpenPhysicalLogAsync(
                *PartitionedReplicaIdentifier,
                *owner_->sharedLogName_,
                id,
                owner_->sharedLogSettings_->Settings.LogSize,
                owner_->sharedLogSettings_->Settings.MaximumNumberStreams,
                owner_->sharedLogSettings_->Settings.MaximumRecordSize,
                static_cast<LogCreationFlags>(owner_->sharedLogSettings_->Settings.Flags),
                cancellationToken,
                physicalLog);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            if (id == KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID())
            {
                name = stagingLogPath_.RawPtr();
                status = co_await CreateAndOpenStagingLogAsync(cancellationToken, physicalLog);
            }
            else
            {
                name = owner_->sharedLogName_.RawPtr();

                status = co_await owner_->OnCreateAndOpenPhysicalLogAsync(
                    *PartitionedReplicaIdentifier,
                    *owner_->sharedLogName_,
                    id,
                    owner_->sharedLogSettings_->Settings.LogSize,
                    owner_->sharedLogSettings_->Settings.MaximumNumberStreams,
                    owner_->sharedLogSettings_->Settings.MaximumRecordSize,
                    static_cast<LogCreationFlags>(owner_->sharedLogSettings_->Settings.Flags),
                    cancellationToken,
                    physicalLog);
            }
            break;
        }

        default:
            KInvariant(FALSE);
            co_return STATUS_UNSUCCESSFUL;
    }

    KInvariant(name != nullptr);
    if (!NT_SUCCESS(status))
    {
        LogManagerHandle::WriteInfo(
            TraceComponent,
            "{0} - CreateAndOpenPhysicalLogAsync - {1} {2} {3}",
            TraceId,
            status,
            Common::Guid(id),
            name->operator wchar_t *());
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::CreateAndOpenStagingLogAsync(
    __in ktl::CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KInvariant(owner_->ktlLoggerMode_ == KtlLoggerMode::InProc);
    KInvariant(stagingLogPath_ != nullptr);

    Common::Guid sharedLogId = Common::Guid::NewGuid();

    NTSTATUS status = CheckStagingLogPathLength(TraceId, *stagingLogPath_);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await owner_->OnCreateAndOpenPhysicalLogAsync(
        *PartitionedReplicaIdentifier,
        *stagingLogPath_,
        sharedLogId.AsGUID(),
        256 * 1024 * 1024, // 256MB staging log
        256, // 256 maximum streams
        owner_->sharedLogSettings_->Settings.MaximumRecordSize,
        LogCreationFlags::UseNonSparseFile,
        cancellationToken,
        physicalLog);

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::OpenPhysicalLogAsync(
    __in KString const & pathToCommonContainer,
    __in KGuid const & physicalLogId,
    __in CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;
    WriteInfo(
        TraceComponent,
        "{0} - OpenPhysicalLogAsync - Open physical log. Path {1} Id0 {2}",
        PartitionedReplicaIdentifier->TraceId,
        pathToCommonContainer.operator const wchar_t *(),
        Common::Guid(physicalLogId));

    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            status = co_await owner_->OnOpenPhysicalLogAsync(
                *PartitionedReplicaIdentifier,
                pathToCommonContainer,
                physicalLogId,
                cancellationToken,
                physicalLog);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            if (physicalLogId == KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID())
            {
                status = co_await OpenStagingLogAsync(cancellationToken, physicalLog);
            }
            else
            {
                status = co_await owner_->OnOpenPhysicalLogAsync(
                    *PartitionedReplicaIdentifier,
                    pathToCommonContainer,
                    physicalLogId,
                    cancellationToken,
                    physicalLog);
            }
            break;
        }

        default:
            KInvariant(FALSE);
            co_return STATUS_UNSUCCESSFUL;
    }
    
    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::OpenPhysicalLogAsync(
    __in CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KCoService$ApiEntry(TRUE);

    KInvariant(owner_->sharedLogSettings_ != nullptr);
    KInvariant(owner_->sharedLogName_ != nullptr);

    NTSTATUS status;
    KGuid const & id = ToKGuid(owner_->sharedLogSettings_->Settings.LogContainerId);

    WriteInfo(
        TraceComponent,
        "{0} - OpenPhysicalLogAsync - Open physical log.",
        PartitionedReplicaIdentifier->TraceId);    
    
    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            status = co_await owner_->OnOpenPhysicalLogAsync(
                *PartitionedReplicaIdentifier,
                *owner_->sharedLogName_,
                id,
                cancellationToken,
                physicalLog);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            if (id == KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID())
            {
                status = co_await OpenStagingLogAsync(cancellationToken, physicalLog);
            }
            else
            {
                status = co_await owner_->OnOpenPhysicalLogAsync(
                    *PartitionedReplicaIdentifier,
                    *owner_->sharedLogName_,
                    id,
                    cancellationToken,
                    physicalLog);
            }
            break;
        }

        default:
            KInvariant(FALSE);
            co_return STATUS_UNSUCCESSFUL;
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::OpenStagingLogAsync(
    __in ktl::CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    KInvariant(owner_->ktlLoggerMode_ == KtlLoggerMode::InProc);
    KInvariant(stagingLogPath_ != nullptr);

    NTSTATUS status = CheckStagingLogPathLength(TraceId, *stagingLogPath_);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await owner_->OnOpenPhysicalLogAsync(
        *PartitionedReplicaIdentifier,
        *stagingLogPath_,
        Common::Guid::NewGuid().AsGUID(), // choose a random guid for uniqueness in the logs table, the underlying open will just use the path
        cancellationToken,
        physicalLog);

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::DeletePhysicalLogAsync(
    __in KString const & pathToCommonPhysicalLog,
    __in KGuid const & physicalLogId,
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    NTSTATUS status;

    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            WriteInfo(
                TraceComponent,
                "{0} - DeletePhysicalLogAsync - Deleting physical log. Path0 {1} Path1 {2} Path2 {3} Id0 {4} Id1 {5}",
                PartitionedReplicaIdentifier->TraceId,
                pathToCommonPhysicalLog.operator const wchar_t *(),
                owner_->sharedLogName_ == nullptr ? L"" : owner_->sharedLogName_->operator const wchar_t *(),
                owner_->sharedLogSettings_ == nullptr ? L"" : owner_->sharedLogSettings_->Settings.Path,
                Common::Guid(physicalLogId),
                owner_->sharedLogSettings_ == nullptr ? Common::Guid::Empty() : Common::Guid(ToKGuid(owner_->sharedLogSettings_->Settings.LogContainerId)));

            status = co_await owner_->OnDeletePhysicalLogAsync(
                *PartitionedReplicaIdentifier,
                pathToCommonPhysicalLog,
                physicalLogId,
                cancellationToken);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            if (physicalLogId == KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID())
            {
                status = co_await DeleteStagingLogAsync(cancellationToken);
            }
            else
            {
                status = co_await owner_->OnDeletePhysicalLogAsync(
                    *PartitionedReplicaIdentifier,
                    pathToCommonPhysicalLog,
                    physicalLogId,
                    cancellationToken);
            }
            break;
        }

        default:
            KInvariant(FALSE);
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::DeletePhysicalLogAsync(
    __in CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    KInvariant(owner_->sharedLogSettings_ != nullptr);
    KInvariant(owner_->sharedLogName_ != nullptr);

    NTSTATUS status;
    KGuid const & id = ToKGuid(owner_->sharedLogSettings_->Settings.LogContainerId);

    switch (owner_->ktlLoggerMode_)
    {
        case KtlLoggerMode::OutOfProc:
        {
            WriteInfo(
                TraceComponent,
                "{0} - DeletePhysicalLogAsync - Deleting physical log. Path0 {1} Path1 {2} Id0 {3}",
                PartitionedReplicaIdentifier->TraceId,
                owner_->sharedLogName_->operator const wchar_t *(),
                owner_->sharedLogSettings_->Settings.Path,
                Common::Guid(id));

            status = co_await owner_->OnDeletePhysicalLogAsync(
                *PartitionedReplicaIdentifier,
                *owner_->sharedLogName_,
                id,
                cancellationToken);
            break;
        }

        case KtlLoggerMode::InProc:
        {
            if (id == KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID())
            {
                status = co_await DeleteStagingLogAsync(cancellationToken);
            }
            else
            {
                status = co_await owner_->OnDeletePhysicalLogAsync(
                    *PartitionedReplicaIdentifier,
                    *owner_->sharedLogName_,
                    id,
                    cancellationToken);
            }
            break;
        }

        default:
            KInvariant(FALSE);
            co_return STATUS_UNSUCCESSFUL;
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> LogManagerHandle::DeleteStagingLogAsync(
    __in ktl::CancellationToken const & cancellationToken)
{
    KInvariant(owner_->ktlLoggerMode_ == KtlLoggerMode::InProc);
    KInvariant(stagingLogPath_ != nullptr);

    NTSTATUS status = CheckStagingLogPathLength(TraceId, *stagingLogPath_);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await owner_->OnDeletePhysicalLogAsync(
        *PartitionedReplicaIdentifier,
        *stagingLogPath_,
        Common::Guid::Empty().AsGUID(),
        cancellationToken);

    co_return status;
}
