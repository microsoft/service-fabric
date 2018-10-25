// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ktl;
using namespace ktl::kservice;
using namespace Data::Log;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("LogManager");

std::wstring const FabricHostApplicationDirectory(L"Fabric_Folder_Application_OnHost");

KGuid const EmptyGuid(TRUE);
RvdLogId const & EmptyLogId = EmptyGuid;

class OpenLogManagerFabricAsyncOperation : public KtlAwaitableProxyAsyncOperation
{
    DENY_COPY(OpenLogManagerFabricAsyncOperation)

public:

    OpenLogManagerFabricAsyncOperation(
        __in LogManager& owner,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent,
        __in CancellationToken const & cancellationToken,
        __in_opt KtlLogger::SharedLogSettingsSPtr sharedLogSettings,
        __in_opt KtlLoggerMode ktlLoggerMode)
        : KtlAwaitableProxyAsyncOperation(callback, parent)
        , owner_(&owner)
        , cancellationToken_(cancellationToken)
        , sharedLogSettings_(sharedLogSettings)
        , ktlLoggerMode_(ktlLoggerMode)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = KtlAwaitableProxyAsyncOperation::End<OpenLogManagerFabricAsyncOperation>(operation);

        thisPtr->owner_->SetFabricOpenOperationEnding();

        thisPtr->owner_ = nullptr;
        thisPtr->cancellationToken_ = CancellationToken();

        return thisPtr->Error;
    }

protected:

    Awaitable<NTSTATUS>
    ExecuteOperationAsync() override
    {
        owner_->SetFabricOpenOperationExecuting();

        NTSTATUS status = co_await owner_->OpenAsync(cancellationToken_, sharedLogSettings_, ktlLoggerMode_);

        owner_->SetFabricOpenOpenAsyncCompleted();

        co_return status;
    }

private:
    
    LogManager::SPtr owner_;
    CancellationToken cancellationToken_;
    KtlLogger::SharedLogSettingsSPtr sharedLogSettings_;
    KtlLoggerMode ktlLoggerMode_;
};

AsyncOperationSPtr
LogManager::BeginOpen(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent,
    __in CancellationToken const & cancellationToken,
    __in_opt KtlLogger::SharedLogSettingsSPtr sharedLogSettings,
    __in_opt KtlLoggerMode ktlLoggerMode)
{
    SetBeginOpenStarted();
    return AsyncOperation::CreateAndStart<OpenLogManagerFabricAsyncOperation>(
        *this,
        callback,
        parent,
        cancellationToken,
        sharedLogSettings,
        ktlLoggerMode);
}

ErrorCode
LogManager::EndOpen(__in AsyncOperationSPtr const & asyncOperation)
{
    return OpenLogManagerFabricAsyncOperation::End(asyncOperation);
}

class CloseLogManagerFabricAsyncOperation : public KtlAwaitableProxyAsyncOperation
{
    DENY_COPY(CloseLogManagerFabricAsyncOperation)

public:

    CloseLogManagerFabricAsyncOperation(
        __in LogManager& owner,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent,
        __in CancellationToken const & cancellationToken)
        : KtlAwaitableProxyAsyncOperation(callback, parent)
        , owner_(&owner)
        , cancellationToken_(cancellationToken)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = KtlAwaitableProxyAsyncOperation::End<CloseLogManagerFabricAsyncOperation>(operation);

        thisPtr->owner_ = nullptr;
        thisPtr->cancellationToken_ = CancellationToken();

        return thisPtr->Error;
    }

protected:

    Awaitable<NTSTATUS>
    ExecuteOperationAsync() override
    {
        NTSTATUS status = co_await owner_->CloseAsync(cancellationToken_);

        co_return status;
    }

private:

    LogManager::SPtr owner_;
    CancellationToken cancellationToken_;
};

AsyncOperationSPtr
LogManager::BeginClose(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent,
    __in CancellationToken const & cancellationToken)
{
    return AsyncOperation::CreateAndStart<CloseLogManagerFabricAsyncOperation>(
        *this,
        callback,
        parent,
        cancellationToken);
}

ErrorCode
LogManager::EndClose(__in AsyncOperationSPtr const & asyncOperation)
{
    return CloseLogManagerFabricAsyncOperation::End(asyncOperation);
}

// todo: move to someplace common
inline ULONG LongHashnFn(__in LONG const & Key)
{
    return K_DefaultHashFunction(static_cast<LONGLONG>(Key));
}

inline ULONG KGuidHashFn(__in KGuid const & Key)
{
    return K_DefaultHashFunction(Key);
}

FAILABLE
LogManager::LogManager()
    : KAsyncServiceBase()
    , Common::TextTraceComponent<Common::TraceTaskCodes::LogicalLog>()
    , lock_(FALSE, TRUE)
    , handles_(13, &LongHashnFn, GetThisAllocator()) // todo: figure out what size should be
    , logs_(13, &KGuidHashFn, GetThisAllocator()) // todo: figure out what size should be
    , nextHandleId_(0)
{
    NTSTATUS status;
    
    status = handles_.Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "constructor - handles constructed with error status. Status: {0}",
            status);

        SetConstructorStatus(status);
        return;
    }

    status = logs_.Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "constructor - logs constructed with error status. Status: {0}",
            status);

        SetConstructorStatus(status);
        return;
    }
}

LogManager::~LogManager()
{
    KInvariant(logs_.Count() == 0);
    KInvariant(handles_.Count() == 0);
    KInvariant(ktlLogManager_ == nullptr);
}

NTSTATUS LogManager::Create(
    __in KAllocator& allocator,
    __out LogManager::SPtr& logManager)
{
    NTSTATUS status;

    logManager = _new(LOGMANAGER_TAG, allocator) LogManager();

    if (!logManager)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);

        WriteError(
            TraceComponent,
            "Create - Failed to allocate LogManager");

        return status;
    }

    status = logManager->Status();
    if (!NT_SUCCESS(status))
    {
        WriteError(
            TraceComponent,
            "Create - LogManager constructed with error status. Status: {0}",
            status);

        return Ktl::Move(logManager)->Status();
    }

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> LogManager::OpenAsync(
    __in CancellationToken const& cancellationToken,
    __in_opt KtlLogger::SharedLogSettingsSPtr sharedLogSettings,
    __in_opt KtlLoggerMode ktlLoggerMode)
{
    NTSTATUS status;

    if (!IsValidKtlLoggerMode(ktlLoggerMode))
    {
        co_return STATUS_INVALID_PARAMETER_3;
    }

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
            "OpenAsync - Failed to create OpenAwaiter. Status {0}",
            status);

        co_return status;
    }

    if (sharedLogSettings != nullptr)
    {
        status = KString::Create(sharedLogName_, GetThisAllocator(), sharedLogSettings->Settings.Path);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "OpenAsync - Failed to create KString. Status {0}, Path: {1}",
                status,
                sharedLogSettings->Settings.Path);

            co_return status;
        }

        sharedLogSettings_ = sharedLogSettings;
    }

    if (ktlLoggerMode == KtlLoggerMode::Default)
    {
#if !defined(PLATFORM_UNIX)
        ktlLoggerMode = KtlLoggerMode::OutOfProc;
#else
        ktlLoggerMode = KtlLoggerMode::InProc;
#endif
    }

    ktlLoggerMode_ = ktlLoggerMode;

    SetOpenAsyncStartAwaiting();
        
    status = co_await *awaiter;
    SetOpenAsyncFinishAwaiting();

    awaiter = nullptr;

    co_return status;
}

VOID LogManager::OnServiceOpen()
{
    SetDeferredCloseBehavior();
    OpenTask();
}

ktl::Task LogManager::OpenTask()
{
    NTSTATUS status = STATUS_SUCCESS;
    
    CompleteOpen(status);

    co_return;
}

Awaitable<NTSTATUS> LogManager::CloseAsync(__in CancellationToken const& cancellationToken)
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
            "CloseAsync - Failed to create CloseAwaiter. Status: {0}",
            status);

        co_return status;
    }
        
    status = co_await *awaiter;
    awaiter = nullptr;
    
    co_return status;
}

VOID LogManager::OnServiceClose()
{
    // A waiter can only start waiting if it acquired an activity on the service.  Since we are using deferred close,
    // all activities must have been released, so there can be no active waiters.
    KAssert(lock_.CountOfWaiters() == 0);
    KAssert(lock_.IsSignaled());

    CloseTask();
}

ktl::Task LogManager::CloseTask()
{
    NTSTATUS status = STATUS_SUCCESS;
    
    CompleteClose(status);

    co_return;
}

VOID LogManager::Abort()
{
    AbortTask();
}

Task LogManager::AbortTask()
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
                    "AbortTask - CloseAsync failed during abort (likely already closed). Status: {0}",
                    status);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "AbortTask - CloseAsync failed during abort. Status: {0}",
                    status);
            }
        }
    }
}

Awaitable<NTSTATUS> LogManager::GetHandle(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in LPCWSTR workDirectory,
    __in CancellationToken const & cancellationToken,
    __out ILogManagerHandle::SPtr& handle)
{
    KCoService$ApiEntry(TRUE);

    // #1 - Acquire an activity representing the LogManagerHandle, to be released when the handle reports closure in OnCloseHandle.
    BOOLEAN handleActivityAcquired = TryAcquireServiceActivity(); // acquire a service activity to defer close until the handle is closed.
    if (!handleActivityAcquired)
    {
        co_return K_STATUS_API_CLOSED;
    }
    
    {
        AsyncLockBlock(lock_)

        NTSTATUS status = STATUS_SUCCESS;
        KtlLogManager::SPtr newManager = nullptr;
        LogManagerHandle::SPtr newHandle = nullptr;
        handle = nullptr;

        nextHandleId_++;

        if (ktlLogManager_ == nullptr)
        {
            switch (ktlLoggerMode_)
            {
                case KtlLoggerMode::OutOfProc:
                {
                    status = KtlLogManager::CreateDriver(LOGMANAGER_TAG, GetThisAllocator(), newManager);
                    break;
                }

                case KtlLoggerMode::InProc:
                {
                    status = KtlLogManager::CreateInproc(LOGMANAGER_TAG, GetThisAllocator(), newManager);
                    break;
                }

                default:
                    KInvariant(FALSE);
                    co_return STATUS_UNSUCCESSFUL;
            }

            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - GetHandle - Failed to create KtlLogManager. Status: {1}",
                    prId.TraceId,
                    status);

                goto HandleError;
            }

            status = co_await newManager->OpenLogManagerAsync(nullptr, nullptr);

            // If driver is not loaded, open an in-proc logger instead
            if ((ktlLoggerMode_ == KtlLoggerMode::OutOfProc) && (status == STATUS_NO_SUCH_DEVICE))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - GetHandle - OutOfProc ktllogger mode requested however driver is not loaded. This may be normal for some environments (e.g. HyperV containers). Status: {1}",
                    prId.TraceId,
                    status);
                
                ktlLoggerMode_ = KtlLoggerMode::InProc;

                status = KtlLogManager::CreateInproc(LOGMANAGER_TAG, GetThisAllocator(), newManager);
                if (!NT_SUCCESS(status))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - GetHandle - Failed to create KtlLogManager. Status: {1}",
                        prId.TraceId,
                        status);

                    goto HandleError;
                }

                status = co_await newManager->OpenLogManagerAsync(nullptr, nullptr);
            }

            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - GetHandle - Failed to open KtlLogManager. Status: {1}",
                    prId.TraceId,
                    status);

                goto HandleError;
            }

            ktlLogManager_ = newManager;
        }

        // No need to modify the work directory for the driver+container case, as a staging log will
        // not be used so the work directory will not be used
        status = LogManagerHandle::Create(
            GetThisAllocator(),
            nextHandleId_,
            *this,
            prId,
            workDirectory,
            newHandle);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - GetHandle - Failed to create LogManagerHandle. Status: {1}",
                prId.TraceId,
                status);

            goto HandleError;
        }

        status = co_await newHandle->OpenAsync(cancellationToken);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - GetHandle - Failed to open LogManagerHandle. Status: {1}",
                prId.TraceId,
                status);

            goto HandleError;
        }
        
        status = handles_.Put(nextHandleId_, newHandle->GetWeakRef());
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - GetHandle - Failed to insert handle into list. Status: {1}",
                prId.TraceId,
                status);

            goto HandleError;
        }

        handle = newHandle.RawPtr();

        KAssert(NT_SUCCESS(status));
        co_return STATUS_SUCCESS;
    
    HandleError:  // goto instead of try/throw/catch

        KAssert(!NT_SUCCESS(status));
        KAssert(handle == nullptr);

        // #1 - Release the activity representing the LogManagerHandle acquired in GetHandle.
        ReleaseServiceActivity();  // cannot be releasing the last activity, as another activity was acquired prior, so this is still valid
        
        if (newHandle != nullptr)
        {
            if (newHandle->IsOpen())
            {
                NTSTATUS closeStatus = co_await newHandle->CloseAsync(cancellationToken);
                if (!NT_SUCCESS(closeStatus))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - GetHandle - Failed to close new handle when compensating for error. Status: {1}",
                        prId.TraceId,
                        status);
                }
            }

            handles_.Remove(nextHandleId_);
            newHandle->Dispose();
            newHandle = nullptr;
        }

        if (newManager != nullptr)
        {
            if (newManager->IsOpen())
            {
                KAssert(newManager.RawPtr() == ktlLogManager_.RawPtr());
                NTSTATUS closeStatus = co_await newManager->CloseLogManagerAsync(nullptr);
                if (!NT_SUCCESS(closeStatus))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - GetHandle - Failed to close new log manager when compensating for error. Status: {1}",
                        prId.TraceId,
                        status);
                }
            }
            
            newManager = nullptr;
            ktlLogManager_ = nullptr;
        }

        co_return status;
    }
}

Awaitable<NTSTATUS> LogManager::OnCloseHandle(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in ILogManagerHandle& toClose,
    __in CancellationToken const & cancellationToken)
{
    // close may be started but waiting for activities to be released

    NTSTATUS status = STATUS_SUCCESS;

    {
        AsyncLockBlock(lock_)

        LogManagerHandle& toCloseHandle = static_cast<LogManagerHandle&>(toClose);
        KWeakRef<LogManagerHandle>::SPtr weakRef;

        status = handles_.Get(toCloseHandle.Id, weakRef);
        KInvariant(NT_SUCCESS(status));

        status = handles_.Remove(toCloseHandle.Id);
        KInvariant(NT_SUCCESS(status));

        if ((handles_.Count() == 0) && (logs_.Count() == 0))
        {
            KtlLogManager::SPtr mgr = Ktl::Move(ktlLogManager_);
            KAssert(ktlLogManager_ == nullptr);
            status = co_await mgr->CloseLogManagerAsync(nullptr);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - OnCloseHandle - Failed to close KtlLogManager when closing last handle. Status: {1}",
                    prId.TraceId,
                    status);
            }
        }
    }

    // The activity is released outside the lock block, so that the assert that the lock is not held during service close is valid.
    // #1 - Release the activity representing the LogManagerHandle acquired in GetHandle.
    ReleaseServiceActivity(); // "this" may no longer be valid after releasing the last activity

    co_return status;
}

Awaitable<NTSTATUS> LogManager::OnCreateAndOpenPhysicalLogAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in KString const & pathToCommonContainer,
    __in KGuid const & physicalLogId,
    __in LONGLONG containerSize,
    __in ULONG maximumNumberStreams,
    __in ULONG maximumLogicalLogBlockSize,
    __in LogCreationFlags creationFlags,
    __in CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    // close may be started but waiting for activities to be released

    // #2 - Acquire an activity representing the PhysicalLog, to be released when the physical log reports closure during OnPhysicalLogHandleClose OR OnLogicalLogClose
    // (when it reports lastHandleClosed).
    BOOLEAN physicalLogActivityAcquired = TryAcquireServiceActivity(); // acquire a service activity to defer close until the physical log is closed.
    if (!physicalLogActivityAcquired)
    {
        co_return K_STATUS_API_CLOSED;
    }
    
    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        IPhysicalLogHandle::SPtr result = nullptr;
        PhysicalLog::SPtr subjectLog;

        status = logs_.Get(physicalLogId, subjectLog);
        if (status == STATUS_NOT_FOUND)
        {
            // P.Log has not been opened or created - attempt to create it
            KtlLogContainer::SPtr underlyingContainer;

            KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;

            // todo: use proper compensation codepath to avoid repeating cleanup code
            status = ktlLogManager_->CreateAsyncCreateLogContainerContext(createAsync);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenPhysicalLogAsync - Failed to create CreateLogContainer context. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }

            status = co_await createAsync->CreateLogContainerAsync(
                pathToCommonContainer,
                physicalLogId,
                containerSize,
                maximumNumberStreams,
                maximumLogicalLogBlockSize,
                static_cast<DWORD>(creationFlags),
                underlyingContainer,
                nullptr);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                if (status == STATUS_OBJECT_NAME_COLLISION)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - Failed to create log container (already exists). Path: {1}, id: {2}",
                        prId.TraceId,
                        ToStringLiteral(pathToCommonContainer),
                        Common::Guid(physicalLogId));
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - Failed to create log container. Path: {1}, id: {2}, size: {3}, maxStreams: {4}, maxBlockSize: {5}, creationFlags: {6}, status: {7}",
                        prId.TraceId,
                        ToStringLiteral(pathToCommonContainer),
                        Common::Guid(physicalLogId),
                        containerSize,
                        maximumNumberStreams,
                        maximumLogicalLogBlockSize,
                        creationFlags,
                        status);
                }

                co_return status;
            }

            status = PhysicalLog::Create(
                GetThisAllocationTag(),
                GetThisAllocator(),
                *this,
                physicalLogId,
                pathToCommonContainer,
                *underlyingContainer,
                subjectLog);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenPhysicalLogAsync - Failed to crate PhysicalLog. id: {1}, status: {2}",
                    prId.TraceId,
                    Common::Guid(physicalLogId),
                    status);

                co_return status;
            }

            status = co_await subjectLog->OpenAsync(cancellationToken);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenPhysicalLogAsync - Failed to open PhysicalLog. status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }

            status = logs_.Put(physicalLogId, subjectLog);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
                ReleaseServiceActivity();
                
                WriteError(
                    TraceComponent,
                    "{0} - OnCreateAndOpenPhysicalLogAsync - failed to insert physical log into dictionary. id: {1}, status: {2}",
                    prId.TraceId,
                    Common::Guid(physicalLogId),
                    status);

                NTSTATUS status2;

                status2 = co_await subjectLog->CloseAsync(cancellationToken);
                if (!NT_SUCCESS(status2))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - failed to close physical log when compensating for failure. status: {1}",
                        prId.TraceId,
                        status2);
                }

                status2 = co_await underlyingContainer->CloseAsync(nullptr);
                if (!NT_SUCCESS(status2))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - failed to close ktl log container when compensating for failure. status: {1}",
                        prId.TraceId,
                        status2);
                }

                KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamContext;
                status2 = underlyingContainer->CreateAsyncDeleteLogStreamContext(deleteStreamContext);
                if (NT_SUCCESS(status2))
                {
                    status2 = co_await deleteStreamContext->DeleteLogStreamAsync(static_cast<RvdLogStreamId>(physicalLogId), nullptr);
                    if (!NT_SUCCESS(status2))
                    {
                        WriteWarning(
                            TraceComponent,
                            "{0} - OnCreateAndOpenPhysicalLogAsync - failed to delete log stream when compensating for failure. status: {1}",
                            prId.TraceId,
                            status2);
                    }
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - failed to create DeleteLogStream context when compensating for failure. status: {1}",
                        prId.TraceId,
                        status2);
                }

                co_return status;
            }

            status = co_await subjectLog->GetHandle(prId, cancellationToken, result);
            if (!NT_SUCCESS(status))
            {
                // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                NTSTATUS status2;

                status2 = co_await subjectLog->CloseAsync(cancellationToken);
                if (!NT_SUCCESS(status2))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - failed to close physical log when compensating for failure. status: {1}",
                        prId.TraceId,
                        status2);
                }

                status2 = logs_.Remove(physicalLogId);
                KInvariant(NT_SUCCESS(status2));  // put succeeded

                status2 = co_await underlyingContainer->CloseAsync(nullptr);
                if (!NT_SUCCESS(status2))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - failed to close ktl log container when compensating for failure. status: {1}",
                        prId.TraceId,
                        status2);
                }

                KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamContext;
                status2 = underlyingContainer->CreateAsyncDeleteLogStreamContext(deleteStreamContext);
                if (NT_SUCCESS(status2))
                {
                    status2 = co_await deleteStreamContext->DeleteLogStreamAsync(static_cast<RvdLogStreamId>(physicalLogId), nullptr);
                    if (!NT_SUCCESS(status2))
                    {
                        WriteWarning(
                            TraceComponent,
                            "{0} - OnCreateAndOpenPhysicalLogAsync - failed to delete log stream when compensating for failure. status: {1}",
                            prId.TraceId,
                            status2);
                    }
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnCreateAndOpenPhysicalLogAsync - failed to create DeleteLogStream context when compensating for failure. status: {1}",
                        prId.TraceId,
                        status2);
                }

                co_return status;
            }

            physicalLog = Ktl::Move(result);
            co_return STATUS_SUCCESS;
        }

        // #2 - Release the activity representing the PhysicalLog acquired in OnCreateAndOpenPhysicalLogAsync.
        ReleaseServiceActivity();

        KAssert(NT_SUCCESS(status));
        co_return STATUS_OBJECT_NAME_COLLISION;
    }
}

Awaitable<NTSTATUS> LogManager::OnOpenPhysicalLogAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in KString const & pathToCommonContainer,
    __in KGuid const & physicalLogId,
    __in CancellationToken const & cancellationToken,
    __out IPhysicalLogHandle::SPtr& physicalLog)
{
    // close may be started but waiting for activities to be released

    // #3 - Acquire an activity representing the PhysicalLog, to be released when the physical log reports closure during OnPhysicalLogHandleClose OR OnLogicalLogClose
    // (when it reports lastHandleClosed).
    BOOLEAN physicalLogActivityAcquired = TryAcquireServiceActivity(); // acquire a service activity to defer close until the physical log is closed.
    if (!physicalLogActivityAcquired)
    {
        co_return K_STATUS_API_CLOSED;
    }

    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        IPhysicalLogHandle::SPtr result = nullptr;
        PhysicalLog::SPtr subjectLog;

        status = logs_.Get(physicalLogId, subjectLog);
        if (status == STATUS_NOT_FOUND)
        {
            // P.Log has not been opened yet- attempt to open its underpinnings

            KtlLogContainer::SPtr underlyingContainer;

            // todo: use proper compensation codepath to avoid repeating cleanup code
            KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
            status = ktlLogManager_->CreateAsyncOpenLogContainerContext(openAsync);
            if (!NT_SUCCESS(status))
            {
                // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnOpenPhysicalLogAsync - Failed to create OpenLogContainer context. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }

            status = co_await openAsync->OpenLogContainerAsync(
                pathToCommonContainer,
                ktlLoggerMode_ == KtlLoggerMode::InProc ? EmptyLogId : physicalLogId, // when using an inproc log, pass the empty guid (lookup will be based on path). When out of proc, use the provided id as mapping may need to occur
                nullptr,
                underlyingContainer);
            if (!NT_SUCCESS(status))
            {
                // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                if (status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to open log container (does not exist). Path: {1}, id: {2}, status: {3}",
                        prId.TraceId,
                        ToStringLiteral(pathToCommonContainer),
                        Common::Guid(physicalLogId),
                        status);
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to open log container. Path: {1}, id: {2}, status: {3}",
                        prId.TraceId,
                        ToStringLiteral(pathToCommonContainer),
                        Common::Guid(physicalLogId),
                        status);
                }

                co_return status;
            }

            status = PhysicalLog::Create(
                GetThisAllocationTag(),
                GetThisAllocator(),
                *this,
                physicalLogId,
                pathToCommonContainer,
                *underlyingContainer,
                subjectLog);
            if (!NT_SUCCESS(status))
            {
                // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnOpenPhysicalLogAsync - Failed to create physical log. id: {1}, status: {2}",
                    prId.TraceId,
                    Common::Guid(physicalLogId),
                    status);

                co_return status;
            }

            status = co_await subjectLog->OpenAsync(cancellationToken);
            if (!NT_SUCCESS(status))
            {
                // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnOpenPhysicalLogAsync - Failed to open physical log. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }

            status = logs_.Put(physicalLogId, subjectLog);
            if (!NT_SUCCESS(status))
            {
                // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                WriteError(
                    TraceComponent,
                    "{0} - OnOpenPhysicalLogAsync - failed to insert physical log into dictionary. id: {1}, status: {2}",
                    prId.TraceId,
                    Common::Guid(physicalLogId),
                    status);

                NTSTATUS status2;

                status2 = co_await subjectLog->CloseAsync(cancellationToken);
                if (!NT_SUCCESS(status2))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to close physical log when compensating for failure.  Status: {1}",
                        prId.TraceId,
                        status2);
                }

                status2 = co_await underlyingContainer->CloseAsync(nullptr);
                if (!NT_SUCCESS(status2))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to close ktl log container when compensating for failure. Status: {1}",
                        prId.TraceId,
                        status2);
                }

                // todo: preallocate these, so that compensation can't fail due to low-memory condition
                KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamContext;
                status2 = underlyingContainer->CreateAsyncDeleteLogStreamContext(deleteStreamContext);
                if (NT_SUCCESS(status2))
                {
                    status2 = co_await deleteStreamContext->DeleteLogStreamAsync(static_cast<RvdLogStreamId>(physicalLogId), nullptr);
                    if (!NT_SUCCESS(status2))
                    {
                        WriteError(
                            TraceComponent,
                            "{0} - OnOpenPhysicalLogAsync - Failed to delete log stream when compensating for failure. Status: {1}",
                            prId.TraceId,
                            status2);
                    }
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to create deletestream context when compensating for failure. Status: {1}",
                        prId.TraceId,
                        status2);
                }

                co_return status;
            }

            status = co_await subjectLog->GetHandle(prId, cancellationToken, result);
            if (!NT_SUCCESS(status))
            {
                // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
                ReleaseServiceActivity();

                NTSTATUS status2;

                status2 = logs_.Remove(physicalLogId);
                KInvariant(NT_SUCCESS(status2));

                status2 = co_await subjectLog->CloseAsync(cancellationToken);
                if (!NT_SUCCESS(status2))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to close physical log when compensating for failure. Status: {1}",
                        prId.TraceId,
                        status2);
                }

                status2 = co_await underlyingContainer->CloseAsync(nullptr);
                if (!NT_SUCCESS(status2))
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to close ktl log container when compensating for failure. Status: {1}",
                        prId.TraceId,
                        status2);
                }

                KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamContext;
                status2 = underlyingContainer->CreateAsyncDeleteLogStreamContext(deleteStreamContext);
                if (NT_SUCCESS(status2))
                {
                    status2 = co_await deleteStreamContext->DeleteLogStreamAsync(static_cast<RvdLogStreamId>(physicalLogId), nullptr);
                    if (!NT_SUCCESS(status2))
                    {
                        WriteError(
                            TraceComponent,
                            "{0} - OnOpenPhysicalLogAsync - Failed to delete log stream when compensating for failure. Status: {1}",
                            prId.TraceId,
                            status2);
                    }
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} - OnOpenPhysicalLogAsync - Failed to create deletestream context when compensating for failure. Status: {1}",
                        prId.TraceId,
                        status2);
                }

                co_return status;
            }
        }
        else
        {
            // #3 - Release the activity representing the PhysicalLog acquired in OnOpenPhysicalLogAsync.
            ReleaseServiceActivity();  // An activity was previously acquired for this physicallog, so release the activity we acquired here.

            // Already open - just alias with another handle
            status = co_await subjectLog->GetHandle(prId, cancellationToken, result);
            if (!NT_SUCCESS(status))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} - Failed to create handle to existing PhysicalLog. id: {1}, status: {2}",
                    prId.TraceId,
                    Common::Guid(physicalLogId),
                    status);

                co_return status;
            }
        }

        physicalLog = Ktl::Move(result);

        KAssert(NT_SUCCESS(status));
        co_return STATUS_SUCCESS;
    }
}

ktl::Awaitable<NTSTATUS> LogManager::DeleteLogicalLogAndMaybeDeletePhysicalLog(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in IPhysicalLogHandle& physicalLogHandle,
    __in KtlLogContainer& container,
    __in KString const & containerPath,
    __in KGuid const & containerId,
    __in KGuid const & logicalLogId,
    __in ktl::CancellationToken const & cancellationToken)
{
    KCoService$ApiEntry(TRUE);

    {
        AsyncLockBlock(lock_)

        NTSTATUS status;

        status = co_await physicalLogHandle.DeleteLogicalLogOnlyAsync(logicalLogId, cancellationToken);
        if (!NT_SUCCESS(status))
        {
            WriteInfo(
                TraceComponent,
                "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Failed to delete logical log, skipping trying to delete physical log. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        // LogicalLog successfully deleted, so check if it was the last logical log

        KtlLogContainer::AsyncEnumerateStreamsContext::SPtr context;
        status = container.CreateAsyncEnumerateStreamsContext(context);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Failed to create AsyncEnumerateStreams context. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        KArray<KtlLogStreamId> ids(GetThisAllocator());
        status = ids.Status();
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Failed to allocate stream ids array. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        status = co_await context->EnumerateStreamsAsync(ids, nullptr);
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Failed to enumerate streams. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }

        //
        // Free the enumerate streams context to ensure the physical
        // log can go away.
        //
        
        context = nullptr;
        
        // The special checkpoint stream id should be filtered out by the overlay log
        if (ids.Count() == 0)
        {
            KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
            status = ktlLogManager_->CreateAsyncDeleteLogContainerContext(deleteAsync);
            if (!NT_SUCCESS(status))
            {
                WriteError(
                    TraceComponent,
                    "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Failed to create DeleteLogContainer context. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }
            
            status = co_await deleteAsync->DeleteLogContainerAsync(containerPath, EmptyLogId, nullptr);

            if (!NT_SUCCESS(status))
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Failed to delete log container. Status: {1}",
                    prId.TraceId,
                    status);

                co_return status;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} - DeleteLogicalLogAndMaybeDeletePhysicalLog - Deleted log container. Status: {1}",
                    prId.TraceId,
                    status);
            }
        }

        co_return STATUS_SUCCESS;
    }
}

Awaitable<NTSTATUS> LogManager::OnDeletePhysicalLogAsync(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in KString const & pathToCommonPhysicalLog,
    __in KGuid const & physicalLogId,
    __in CancellationToken const & cancellationToken)
{
    // close may be started but waiting for activities to be released

    {
        AsyncLockBlock(lock_)

        NTSTATUS status;
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;

        status = ktlLogManager_->CreateAsyncDeleteLogContainerContext(deleteAsync); // todo: preallocate and reuse, since this happens under lock
        if (!NT_SUCCESS(status))
        {
            WriteError(
                TraceComponent,
                "{0} - OnDeletePhysicalLogAsync - Failed to create DeleteLogContainer context. Status: {1}",
                prId.TraceId,
                status);

            co_return status;
        }
        
        // Always pass the empty guid, as the out-of-proc log is never deleted via these APIs.
        status = co_await deleteAsync->DeleteLogContainerAsync(pathToCommonPhysicalLog, EmptyLogId, nullptr);

        if (!NT_SUCCESS(status))
        {
            WriteInfo(
                TraceComponent,
                "{0} - OnDeletePhysicalLogAsync - Failed to delete log container. Status: {1} Path0 {2} Path1 {3} Path2 {4} Id0 {5} Id1 {6} Id2 {7}",
                prId.TraceId,
                status,
                pathToCommonPhysicalLog.operator const wchar_t *(),
                sharedLogName_->operator const wchar_t *(),
                sharedLogSettings_->Settings.Path,
                Common::Guid(physicalLogId),
                Common::Guid(EmptyGuid),
                Common::Guid(const_cast<RvdLogId&>(sharedLogSettings_->Settings.LogContainerId).GetReference()));
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} - OnDeletePhysicalLogAsync - Deleted log container. Path0 {1} Path1 {2} Path3 {3} Id0 {4} Id1 {5} Id2 {6}",
                prId.TraceId,
                pathToCommonPhysicalLog.operator const wchar_t *(),
                sharedLogName_->operator const wchar_t *(),
                sharedLogSettings_->Settings.Path,
                Common::Guid(physicalLogId),
                Common::Guid(EmptyGuid),
                Common::Guid(const_cast<RvdLogId&>(sharedLogSettings_->Settings.LogContainerId).GetReference()));
        }

        co_return status;
    }
}

Awaitable<NTSTATUS> LogManager::OnPhysicalLogHandleClose(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in IPhysicalLogHandle& toClose,
    __in CancellationToken const& cancellationToken)
{
    // close may be started but waiting for activities to be released
    
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN lastHandleClosed = FALSE;

    {
        AsyncLockBlock(lock_)

        PhysicalLogHandle& handleToClose = static_cast<PhysicalLogHandle&>(toClose);
        PhysicalLog::SPtr handleOwner = nullptr;

        status = logs_.Get(handleToClose.OwnerId, handleOwner);
        KInvariant(NT_SUCCESS(status));
        KInvariant(&handleToClose.Owner == handleOwner.RawPtr());

        status = co_await handleOwner->OnCloseHandle(prId, handleToClose, cancellationToken, lastHandleClosed);
        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnPhysicalLogHandleClose - call to PhysicalLog::OnHandleClose failed. id: {1}, status: {2}",
                prId.TraceId,
                handleToClose.Id,
                status);
        }
        
        if (lastHandleClosed)
        {
            // last reference on the PhysicalLog removed - it is closed, remove from this logs_ index
            status = logs_.Remove(handleToClose.OwnerId);
            KInvariant(NT_SUCCESS(status));

            if ((handles_.Count() == 0) && (logs_.Count() == 0))
            {
                KtlLogManager::SPtr mgr = Ktl::Move(ktlLogManager_);
                KAssert(ktlLogManager_ == nullptr);
                status = co_await mgr->CloseLogManagerAsync(nullptr);
                if (!NT_SUCCESS(status))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnPhysicalLogHandleClose - failed to close KtlLogManager when closing last handle. status: {1}",
                        prId.TraceId,
                        status);
                }
            }
        }
    }

    // The activity is released outside the lock block, so that the assert that the lock is not held during service close is valid.
    if (lastHandleClosed)
    {
        // #2 OR #3 - Release the activity representing the PhysicalLog acquired in either OnCreateAndOpenPhysicalLogAsync or OnOpenPhysicalLogAsync.
        ReleaseServiceActivity(); // "this" may no longer be valid after releasing the last activity
    }

    co_return status;
}

Awaitable<NTSTATUS> LogManager::OnLogicalLogClose(
    __in Data::Utilities::PartitionedReplicaId const & prId,
    __in ILogicalLog& toClose,
    __in CancellationToken const & cancellationToken)
{
    // close may be started but waiting for activities to be released

    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN lastHandleClosed = FALSE;

    {
        AsyncLockBlock(lock_)
        
        LogicalLog& logToClose = static_cast<LogicalLog&>(toClose);
        PhysicalLog::SPtr handleOwner = nullptr;

        status = logs_.Get(logToClose.OwnerId, handleOwner);
        KInvariant(NT_SUCCESS(status));

        status = co_await handleOwner->OnCloseLogicalLog(prId, logToClose, cancellationToken, lastHandleClosed);
        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                TraceComponent,
                "{0} - OnLogicalLogClose - call to PhysicalLog::OnCloseLogicalLog failed. id: {1}, status: {2}",
                prId.TraceId,
                Common::Guid(logToClose.Id),
                status);
        }

        if (lastHandleClosed)
        {
            // last handle or logical log closed on the PhysicalLog, removed from this index
            status = logs_.Remove(logToClose.OwnerId);
            KInvariant(NT_SUCCESS(status));

            if ((handles_.Count() == 0) && (logs_.Count() == 0))
            {
                KtlLogManager::SPtr mgr = Ktl::Move(ktlLogManager_);
                KAssert(ktlLogManager_ == nullptr);
                status = co_await mgr->CloseLogManagerAsync(nullptr);
                if (!NT_SUCCESS(status))
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} - OnLogicalLogClose - failed to close KtlLogManager when closing last log. status: {1}",
                        prId.TraceId,
                        status);
                }
            }
        }
    }

    // The activity is released outside the lock block, so that the assert that the lock is not held during service close is valid.
    if (lastHandleClosed)
    {
        // #2 OR #3 - Release the activity representing the PhysicalLog acquired in either OnCreateAndOpenPhysicalLogAsync or OnOpenPhysicalLogAsync.
        ReleaseServiceActivity(); // "this" may no longer be valid after releasing the last activity
    }

    co_return status;
}
