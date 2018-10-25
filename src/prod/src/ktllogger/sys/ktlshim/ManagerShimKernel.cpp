// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

//************************************************************************************
//
// KtlLogManager Implementation
//
//************************************************************************************

KtlLogManagerKernel::~KtlLogManagerKernel()
{
}


//
// KDRIVER, UDRIVER and DAEMON expect that the overlay manager is
// created outside of the KtlLogManagerKernel
//
#if defined(KDRIVER) || defined(UDRIVER) || defined(DAEMON)
KtlLogManagerKernel::KtlLogManagerKernel(
    __in OverlayManager& GlobalOverlayManager
    ) :
   _LogContainerType(GetThisAllocator())
{
    NTSTATUS status;
    
    //
    // Use hardcoded log container type
    //
    _LogContainerType = L"Winfab Logical Log";
    status = _LogContainerType.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   

    _OverlayManager = &GlobalOverlayManager;

#if defined(UDRIVER) || defined(KDRIVER)
    _ObjectId = 0;
    _FileObjectTable = NULL;
#endif
}

#ifdef KDRIVER
NTSTATUS
KtlLogManager::CreateDriver(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KtlLogManager::SPtr& Result
    )
{
    UNREFERENCED_PARAMETER(AllocationTag);
    UNREFERENCED_PARAMETER(Allocator);
    
    NTSTATUS status;
    
    KInvariant(FALSE);
    Result = nullptr;
    status = STATUS_NOT_SUPPORTED;

    return(status);
}

NTSTATUS
KtlLogManager::CreateInproc(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KtlLogManager::SPtr& Result
    )
{
    UNREFERENCED_PARAMETER(AllocationTag);
    UNREFERENCED_PARAMETER(Allocator);
    
    NTSTATUS status;
    
    KInvariant(FALSE);
    Result = nullptr;
    status = STATUS_NOT_SUPPORTED;

    return(status);
}
#endif

#ifdef UDRIVER
NTSTATUS
KtlLogManager::CreateInproc(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KtlLogManager::SPtr& Result
)
{
    UNREFERENCED_PARAMETER(AllocationTag);
    UNREFERENCED_PARAMETER(Allocator);

    NTSTATUS status;

    KInvariant(FALSE);
    Result = nullptr;
    status = STATUS_NOT_SUPPORTED;

    return(status);
}
#endif

VOID
KtlLogManagerKernel::OnServiceOpen(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    CompleteOpen(status);
}

VOID
KtlLogManagerKernel::OnServiceClose(
)
{
    CompleteClose(STATUS_SUCCESS);
}
#endif

//
// In the UPASSTHROUGH (inproc) case, there is no separate hosting as
// there is for the UDRIVER, KDRIVER and DAEMON cases so the overlay
// manager needs to be created on demand
//
#ifdef UPASSTHROUGH
KtlLogManagerKernel::KtlLogManagerKernel() :
   _LogContainerType(GetThisAllocator())
{
    NTSTATUS status;
    
    //
    // Use hardcoded log container type
    //
    _LogContainerType = L"Winfab Logical Log";
    status = _LogContainerType.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   
}

#if defined(PLATFORM_UNIX)
NTSTATUS
KtlLogManager::CreateDriver(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KtlLogManager::SPtr& Result
    )
{
    UNREFERENCED_PARAMETER(AllocationTag);
    UNREFERENCED_PARAMETER(Allocator);
    
    NTSTATUS status;
    
    KInvariant(FALSE);
    Result = nullptr;
    status = STATUS_NOT_SUPPORTED;

    return(status);
}
#endif

NTSTATUS
KtlLogManager::CreateInproc(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KtlLogManager::SPtr& Result
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::SPtr logManKernel;
    OverlayManager::SPtr overlayManager;

    Result = nullptr;
    
    logManKernel = _new(AllocationTag, Allocator) KtlLogManagerKernel();
    if (! logManKernel)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
    }

    status = logManKernel->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    Result = logManKernel.RawPtr();
    return(status);
}

VOID
KtlLogManagerKernel::OnServiceOpen(
    )
{
    OpenTask();
}

ktl::Task KtlLogManagerKernel::OpenTask()
{
    NTSTATUS status;
    
    status = co_await FileObjectTable::CreateAndRegisterOverlayManagerAsync(GetThisAllocator(), GetThisAllocationTag());

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)&GetThisKtlSystem(), 0);
    }

    FileObjectTable::LookupOverlayManager(GetThisAllocator(), _OverlayManager);
    KInvariant(_OverlayManager != nullptr);
    
    CompleteOpen(status);
    co_return;
}

VOID
KtlLogManagerKernel::OnServiceClose(
    )
{
    CloseTask();
}

ktl::Task KtlLogManagerKernel::CloseTask()
{
    NTSTATUS status;
    
    status = co_await FileObjectTable::StopAndUnregisterOverlayManagerAsync(GetThisAllocator(), _OverlayManager.RawPtr());

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)&GetThisKtlSystem(), (ULONGLONG)_OverlayManager.RawPtr());
    }
    
    _OverlayManager = nullptr;
    CompleteClose(status);
    co_return;
}

#endif


VOID
KtlLogManagerKernel::OnServiceReuse(
    )
{
    KInvariant(FALSE);
}


NTSTATUS
KtlLogManagerKernel::StartOpenLogManager(  
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt OpenCompletionCallback OpenCallbackPtr,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;

    //
    // Start the KtlLogManager service, continue at KtlLogManagerKernel::OnServiceOpen
    //
    status = StartOpen(ParentAsync,
                       OpenCallbackPtr,
                       GlobalContextOverride);
    
    return(status);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::OpenLogManagerAsync(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    ktl::kservice::OpenAwaiter::SPtr awaiter;
    NTSTATUS status = ktl::kservice::OpenAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        ParentAsync,
        GlobalContextOverride);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif


NTSTATUS
KtlLogManagerKernel::StartCloseLogManager(
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt CloseCompletionCallback CloseCallbackPtr
    )
{
    NTSTATUS status;

    //
    // Close the KtlLogManager service, continue at OnCloseService
    //
    status = StartClose(ParentAsync,
                        CloseCallbackPtr);

    return(status);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::CloseLogManagerAsync(
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    ktl::kservice::CloseAwaiter::SPtr awaiter;
    NTSTATUS status = ktl::kservice::CloseAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif
                
//
// CreateLogContainer
//
VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::OnCompleted(
    )
{
    _Path = nullptr;
    _LogContainer = nullptr;
}


VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::LogContainerCreated(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    
    _LogManager->ReleaseActivities();
    
    if (NT_SUCCESS(status))
    {
        (*_ResultLogContainerPtr) = Ktl::Move(reinterpret_cast<KtlLogContainer::SPtr&>(_LogContainer));
    }
    
    Complete(status);
}

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    //
    // Ensure path name is not too long
    //
    if (_Path != nullptr)
    {
        if (_Path->Length() > KtlLogManager::MaxPathnameLengthInChar-1)
        {
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _Path->Length(), KtlLogManager::MaxPathnameLengthInChar-1);
            Complete(status);
            return;
        }
    }
#endif
    
    _LogContainer = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogContainerKernel(_LogContainerId, _DiskId);

    if (! _LogContainer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _LogContainer->Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Perform any mapping of log containers paths
    //
    status = _LogManager->_OverlayManager->MapSharedLogDefaultSettings(_DiskId,
                                                                       _Path,
                                                                       _LogContainerId,
                                                                       _LogSize,
                                                                       _MaximumNumberStreams,
                                                                       _MaximumRecordSize,
                                                                       _Flags);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //
    // If DiskId not specified then map path to it
    //
    GUID nullGuid = { 0 };
    if ((_Path) && (_DiskId == nullGuid))
    {
        status = _LogManager->_OverlayManager->MapPathToDiskId(*_Path,
                                                               _DiskId);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;
        }
    }           
    
    if (!_LogManager->TryAcquireActivities())
    {
        Complete(K_STATUS_SHUTDOWN_PENDING);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerKernel::AsyncCreateLogContainerKernel::LogContainerCreated);
    _BaseAsyncCreateLogContainer->StartCreateLog(_DiskId,
                                                 _Path.RawPtr(),
                                                 _LogContainerId,
                                                 _LogManager->_LogContainerType,
                                                 _LogSize,
                                                 _MaximumNumberStreams,
                                                 _MaximumRecordSize,
                                                 _Flags,
                                                 _LogContainer->_BaseLogContainer,
                                                 this,
                                                 completion);

}

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::StartCreateLogContainer(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = 0;
    _MaximumNumberStreams = MaximumNumberStreams;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::StartCreateLogContainer(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::StartCreateLogContainer(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::StartCreateLogContainer(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncCreateLogContainerKernel::CreateLogContainerAsync(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncCreateLogContainerKernel::CreateLogContainerAsync(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    GUID nullGUID = { 0 };

    _Path = &Path;
    _DiskId = nullGUID;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncCreateLogContainerKernel::CreateLogContainerAsync(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncCreateLogContainerKernel::CreateLogContainerAsync(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);
    
    GUID nullGUID = { 0 };

    _Path = &Path;
    _DiskId = nullGUID;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::OnReuse(
    )
{
    _BaseAsyncCreateLogContainer->Reuse();
}

VOID
KtlLogManagerKernel::AsyncCreateLogContainerKernel::OnCancel(
    )
{
    _BaseAsyncCreateLogContainer->Cancel();
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

KtlLogManagerKernel::AsyncCreateLogContainerKernel::AsyncCreateLogContainerKernel()
{
}

KtlLogManagerKernel::AsyncCreateLogContainerKernel::~AsyncCreateLogContainerKernel()
{
}

NTSTATUS
KtlLogManagerKernel::CreateAsyncCreateLogContainerContext(
    __out AsyncCreateLogContainer::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::AsyncCreateLogContainerKernel::SPtr context;

    KInvariant(_OverlayManager != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCreateLogContainerKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _OverlayManager.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;

    OverlayManagerBase::AsyncCreateLog::SPtr createLogContext;
    
    status = _OverlayManager->CreateAsyncCreateLogContext(createLogContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_BaseAsyncCreateLogContainer = down_cast<OverlayManager::AsyncCreateLogOverlay>(createLogContext);

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// Open log container
//
VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OpenLogContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    OpenLogContainerFSM(Async.Status());
}

VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::DoComplete(
    __in NTSTATUS status
    )
{
    _Path = nullptr;
    _LogContainer = nullptr;
    _LogManager->ReleaseActivities();
    Complete(status);
}

VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OpenLogContainerFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerKernel::AsyncOpenLogContainerKernel::OpenLogContainerCompletion);

    if (! NT_SUCCESS(Status))
    {
        KInvariant(_State != Initial);
        
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            GUID nullGuid = { 0 };
            if ((_LogContainerId.Get() == nullGuid) &&
                (_Path))
            {
                _State = LookupLogId;               
                _BaseAsyncQueryLogIdContainer->StartQueryLogId(*_Path, _LogContainerId, this, completion);
                
                break;
            }

            // Fall through
        }

        case LookupLogId:
        {
            _State = OpenBaseLogContainer;

            _LogContainer = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogContainerKernel(_LogContainerId, _DiskId);
            if (! _LogContainer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(Status, this, 0, 0);
                DoComplete(Status);
                return;
            }

            Status = _LogContainer->Status();
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }   

            //
            // Perform any mapping of log containers paths
            //
            LONGLONG logSize;
            ULONG maximumNumberStreams;
            ULONG maximumRecordSize;
            ULONG flags;
            Status = _LogManager->_OverlayManager->MapSharedLogDefaultSettings(_DiskId,
                                                                               _Path,
                                                                               _LogContainerId,
                                                                               logSize,
                                                                               maximumNumberStreams,
                                                                               maximumRecordSize,
                                                                               flags);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            //
            // If DiskId not specified then map path to it
            //
            GUID nullGuid = { 0 };
            if ((_Path) && (_DiskId == nullGuid))
            {
                Status = _LogManager->_OverlayManager->MapPathToDiskId(*_Path,
                                                                       _DiskId);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }
            }           
            
            _BaseAsyncOpenLogContainer->StartOpenLog(_DiskId,
                                                     _Path.RawPtr(),
                                                     _LogContainerId,
                                                     _LogContainer->_BaseLogContainer,
                                                     this,
                                                     completion);           
            break;
        }

        case OpenBaseLogContainer:
        {
            (*_ResultLogContainerPtr) = Ktl::Move(reinterpret_cast<KtlLogContainer::SPtr&>(_LogContainer));
            
            DoComplete(STATUS_SUCCESS);   
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OnStart(
    )
{
    _State = Initial;
    
    if (!_LogManager->TryAcquireActivities())
    {
        Complete(K_STATUS_SHUTDOWN_PENDING);
        return;
    }

#ifdef UPASSTHROUGH
    //
    // Ensure path name is not too long
    //
    if (_Path != nullptr)
    {
        if (_Path->Length() > KtlLogManager::MaxPathnameLengthInChar-1)
        {
            NTSTATUS status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _Path->Length(), KtlLogManager::MaxPathnameLengthInChar-1);
            DoComplete(status);
            return;
        }
    }
#endif
    
    OpenLogContainerFSM(STATUS_SUCCESS);
}


VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::StartOpenLogContainer(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& LogContainerId,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OpenLogContainerAsync(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __out KtlLogContainer::SPtr& LogContainer
)
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::StartOpenLogContainer(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    
    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OpenLogContainerAsync(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __out KtlLogContainer::SPtr& LogContainer
)
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    GUID nullGUID = { 0 };

    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OnReuse(
    )
{
    _BaseAsyncOpenLogContainer->Reuse();
    _BaseAsyncQueryLogIdContainer->Reuse();
}

VOID
KtlLogManagerKernel::AsyncOpenLogContainerKernel::OnCancel(
    )
{
    _BaseAsyncOpenLogContainer->Cancel();
    _BaseAsyncQueryLogIdContainer->Cancel();
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

KtlLogManagerKernel::AsyncOpenLogContainerKernel::AsyncOpenLogContainerKernel()
{
}

KtlLogManagerKernel::AsyncOpenLogContainerKernel::~AsyncOpenLogContainerKernel()
{
}

NTSTATUS
KtlLogManagerKernel::CreateAsyncOpenLogContainerContext(
    __out AsyncOpenLogContainer::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::AsyncOpenLogContainerKernel::SPtr context;

    KInvariant(_OverlayManager != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncOpenLogContainerKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _OverlayManager.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;

    OverlayManagerBase::AsyncOpenLog::SPtr openLogContext;    
    status = _OverlayManager->CreateAsyncOpenLogContext(openLogContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    context->_BaseAsyncOpenLogContainer = down_cast<OverlayManager::AsyncOpenLogOverlay>(openLogContext);

    OverlayManagerBase::AsyncQueryLogId::SPtr queryLogIdContext;    
    status = _OverlayManager->CreateAsyncQueryLogIdContext(queryLogIdContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    context->_BaseAsyncQueryLogIdContainer = down_cast<OverlayManager::AsyncQueryLogIdOverlay>(queryLogIdContext);
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// Delete log container
//
VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::DoComplete(
    __in NTSTATUS Status
    )
{
    _Path = nullptr;
    _LogManager->ReleaseActivities();
    Complete(Status);
}

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::DeleteLogContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    DeleteLogContainerFSM(Async.Status());
}

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::DeleteLogContainerFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerKernel::AsyncDeleteLogContainerKernel::DeleteLogContainerCompletion);

    KDbgCheckpointWDataInformational(KLoggerUtilities::ActivityIdFromStreamId(_LogContainerId),
                        "KtlLogManagerKernel::AsyncDeleteLogContainerKernel  StartDeleteLog", Status,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0,
                        (ULONGLONG)0);
                           
    if (! NT_SUCCESS(Status))
    {
        KInvariant(_State != Initial);
        
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            //
            // If empty guid passed then look it up from the file
            // itself
            //
            GUID nullGuid = { 0 };
            if ((_LogContainerId.Get() == nullGuid) &&
                (_Path))
            {
                _State = LookupLogId;               
                _BaseAsyncQueryLogIdContainer->StartQueryLogId(*_Path, _LogContainerId, this, completion);
                
                break;
            }

            // Fall through
        }

        case LookupLogId:
        {
            _State = DeleteBaseLogContainer;
            //
            // Perform any mapping of log containers paths
            //
            LONGLONG logSize;
            ULONG maximumNumberStreams;
            ULONG maximumRecordSize;
            ULONG flags;
            
            Status = _LogManager->_OverlayManager->MapSharedLogDefaultSettings(_DiskId,
                                                                               _Path,
                                                                               _LogContainerId,
                                                                               logSize,
                                                                               maximumNumberStreams,
                                                                               maximumRecordSize,
                                                                               flags);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, 0, 0);
                DoComplete(Status);
                return;
            }

            OverlayManager::AsyncDeleteLogOverlay::SPtr baseAsyncDeleteLogContainer =
                                                down_cast<OverlayManager::AsyncDeleteLogOverlay>(_BaseAsyncDeleteLogContainer);

            KDbgCheckpointWDataInformational(
                KLoggerUtilities::ActivityIdFromStreamId(_LogContainerId),
                "KtlLogManagerKernel::AsyncDeleteLogContainerKernel  StartDeleteLog", Status,
                (ULONGLONG)this,
                (ULONGLONG)baseAsyncDeleteLogContainer.RawPtr(),
                (ULONGLONG)0,
                (ULONGLONG)0);

            baseAsyncDeleteLogContainer->StartDeleteLog(_DiskId,
                                                         _Path.RawPtr(),
                                                         _LogContainerId,
                                                         this,
                                                         completion);

            break;
        }

        case DeleteBaseLogContainer:
        {
            DoComplete(Status);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::OnStart(
    )
{
    _State = Initial;
    
    if (!_LogManager->TryAcquireActivities())
    {
        Complete(K_STATUS_SHUTDOWN_PENDING);
        return;
    }

#ifdef UPASSTHROUGH
    //
    // Ensure path name is not too long
    //
    NTSTATUS status;
    
    if (_Path != nullptr)
    {
        if (_Path->Length() > KtlLogManager::MaxPathnameLengthInChar-1)
        {
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _Path->Length(), KtlLogManager::MaxPathnameLengthInChar-1);
            DoComplete(status);
            return;
        }
    }
#endif
    
    DeleteLogContainerFSM(STATUS_SUCCESS);
}

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::StartDeleteLogContainer(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::StartDeleteLogContainer(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    
    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;

    Start(ParentAsyncContext, CallbackPtr);
}


#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::DeleteLogContainerAsync(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext)
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    GUID nullGUID = { 0 };

    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;

    status = co_await awaiter;
    co_return status;
}
#endif  

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::OnReuse(
    )
{
    _BaseAsyncDeleteLogContainer->Reuse();
    _BaseAsyncQueryLogIdContainer->Reuse();
}

VOID
KtlLogManagerKernel::AsyncDeleteLogContainerKernel::OnCancel(
    )
{
    _BaseAsyncDeleteLogContainer->Cancel();
    _BaseAsyncQueryLogIdContainer->Cancel();
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

KtlLogManagerKernel::AsyncDeleteLogContainerKernel::AsyncDeleteLogContainerKernel()
{
}

KtlLogManagerKernel::AsyncDeleteLogContainerKernel::~AsyncDeleteLogContainerKernel()
{
}

NTSTATUS
KtlLogManagerKernel::CreateAsyncDeleteLogContainerContext(
    __out AsyncDeleteLogContainer::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::AsyncDeleteLogContainerKernel::SPtr context;

    KInvariant(_OverlayManager != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteLogContainerKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _OverlayManager.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;
    
    status = _OverlayManager->CreateAsyncDeleteLogContext(context->_BaseAsyncDeleteLogContainer);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    OverlayManagerBase::AsyncQueryLogId::SPtr queryLogIdContext;    
    status = _OverlayManager->CreateAsyncQueryLogIdContext(queryLogIdContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    context->_BaseAsyncQueryLogIdContainer = down_cast<OverlayManager::AsyncQueryLogIdOverlay>(queryLogIdContext);
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// enumerate log containers
//
VOID
KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::LogContainersEnumerated(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    _LogManager->ReleaseActivities();
    
    Complete(status);
}

VOID
KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::OnStart(
    )
{
    if (!_LogManager->TryAcquireActivities())
    {
        Complete(K_STATUS_SHUTDOWN_PENDING);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::LogContainersEnumerated);
    _BaseAsyncEnumerateLogs->StartEnumerateLogs(_DiskId,
                                                        *_LogIdArray,
                                                        this,
                                                        completion);
}

VOID
KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::StartEnumerateLogContainers(
    __in const KGuid& DiskId,
    __out KArray<KtlLogContainerId>& LogIdArray,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _LogIdArray = &LogIdArray;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::OnReuse(
    )
{
    _BaseAsyncEnumerateLogs->Reuse();
}

VOID
KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _BaseAsyncEnumerateLogs->Cancel();
}

KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::AsyncEnumerateLogContainersKernel()
{
}

KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::~AsyncEnumerateLogContainersKernel()
{
}

NTSTATUS
KtlLogManagerKernel::CreateAsyncEnumerateLogContainersContext(
    __out AsyncEnumerateLogContainers::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::AsyncEnumerateLogContainersKernel::SPtr context;

    KInvariant(_OverlayManager != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncEnumerateLogContainersKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _OverlayManager.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;
    
    OverlayManagerBase::AsyncEnumerateLogs::SPtr enumerateLogsContext;
    
    status = _OverlayManager->CreateAsyncEnumerateLogsContext(enumerateLogsContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_BaseAsyncEnumerateLogs = down_cast<OverlayManager::AsyncEnumerateLogsOverlay>(enumerateLogsContext);  

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// Configure
//
VOID
KtlLogManagerKernel::AsyncConfigureContextKernel::ConfigureCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();
        
    _InBuffer = nullptr;
    
    _LogManager->ReleaseActivities();
    
    Complete(status);
}

VOID
KtlLogManagerKernel::AsyncConfigureContextKernel::OnStart(
    )
{
    if (!_LogManager->TryAcquireActivities())
    {
        Complete(K_STATUS_SHUTDOWN_PENDING);
        return;
    }

#ifdef UPASSTHROUGH
    if (! _InBuffer)
    {
        //
        // If user didn't pass an input buffer then build an empty one
        //
        NTSTATUS status = KBuffer::Create(0, _InBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;
        }
    }
#endif
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerKernel::AsyncConfigureContextKernel::ConfigureCompleted);
        
    _BaseAsyncConfigureContext->StartConfigure(_Code,
                                      _InBuffer.RawPtr(),
                                      *_Result,
                                      *_OutBuffer,
                                      this,
                                      completion);
}

VOID
KtlLogManagerKernel::AsyncConfigureContextKernel::StartConfigure(
    __in ULONG Code,
    __in_opt KBuffer* const InBuffer,
    __out ULONG& Result,
    __out KBuffer::SPtr& OutBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Code = Code;
    _InBuffer = InBuffer;
    _Result = &Result;
    _OutBuffer = &OutBuffer;
    *_OutBuffer = nullptr;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogManagerKernel::AsyncConfigureContextKernel::OnReuse(
    )
{
    _InBuffer = nullptr;
    _OutBuffer = NULL;
    _BaseAsyncConfigureContext->Reuse();
}

VOID
KtlLogManagerKernel::AsyncConfigureContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _BaseAsyncConfigureContext->Cancel();
}

KtlLogManagerKernel::AsyncConfigureContextKernel::AsyncConfigureContextKernel()
{
}

KtlLogManagerKernel::AsyncConfigureContextKernel::~AsyncConfigureContextKernel()
{
}

NTSTATUS
KtlLogManagerKernel::CreateAsyncConfigureContext(
    __out AsyncConfigureContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::AsyncConfigureContextKernel::SPtr context;

    KInvariant(_OverlayManager != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncConfigureContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _OverlayManager.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    status = _OverlayManager->CreateAsyncConfigureContext(context->_BaseAsyncConfigureContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_LogManager = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}
