// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include <ktl.h>
#include <ktrace.h>

#include "servicewrapper.h"

const ULONG ServiceWrapper::Operation::ListEntryOffset = FIELD_OFFSET(Operation, _ListEntry);

WrappedServiceBase::~WrappedServiceBase()
{
}

WrappedServiceBase::WrappedServiceBase()
{
}

ServiceWrapper::ServiceWrapper()
{
    KInvariant(FALSE);
}

ServiceWrapper::ServiceWrapper(
    __in CreateServiceFactory CreateUnderlyingService
    ) : _CreateUnderlyingService(CreateUnderlyingService),
        _ObjectState(Closed),
        _UnderlyingService(nullptr),
        _UsageCount(0),
        _WaitForDeactivate(nullptr),
        _DequeueItem(nullptr)
{
    NTSTATUS status;

    status = KAsyncQueue<Operation>::Create(GetThisAllocationTag(),
                                            GetThisAllocator(),
                                            Operation::ListEntryOffset,
                                            _OperationQueue);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

ServiceWrapper::~ServiceWrapper()
{
    //
    // Proper process is for the underlying service to be closed before
    // the wrapper is destructed
    //
    KInvariant(_ObjectState == Closed);
    KInvariant(! _UnderlyingService);
    KInvariant(_UsageCount == 0);
}

ServiceWrapper::WaitForQueueToDeactiveContext::WaitForQueueToDeactiveContext(
    __in ServiceWrapper& Service
    )
{
    _Service = &Service;
}

ServiceWrapper::WaitForQueueToDeactiveContext::~WaitForQueueToDeactiveContext()
{
}

VOID
ServiceWrapper::DroppedItemCallback(
    __in KAsyncQueue<Operation>& DeactivatingQueue,
    __in Operation& DroppedItem
    )
{
    UNREFERENCED_PARAMETER(DeactivatingQueue);

    NTSTATUS status = K_STATUS_SHUTDOWN_PENDING;
    KTraceFailedAsyncRequest(status, &DroppedItem, DroppedItem.GetOperationType(), 0);
    DroppedItem.Complete(status);
}

VOID
ServiceWrapper::WaitForQueueToDeactiveContext::DeactivateCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(Async);
    UNREFERENCED_PARAMETER(ParentAsync);
    
    if (_DeactivateQueueCallback)
    {
        _DeactivateQueueCallback(*_Service);
    }
    
    _Service->_DequeueItem = nullptr;
    
    _Service = nullptr;
    
    Complete(STATUS_SUCCESS);
}

VOID
ServiceWrapper::DeactivateQueue(
    __in_opt DeactivateQueueCallback Callback                        
    )
{
    _WaitForDeactivate->_DeactivateQueueCallback = Callback;

    KAsyncQueue<Operation>::DroppedItemCallback droppedItemCallback(this, &ServiceWrapper::DroppedItemCallback);

    _OperationQueue->Deactivate(droppedItemCallback);
}

NTSTATUS
ServiceWrapper::ActivateQueue(
    )
{
    NTSTATUS status;
    
    _WaitForDeactivate = nullptr;
    _WaitForDeactivate = _new(GetThisAllocationTag(), GetThisAllocator()) WaitForQueueToDeactiveContext(*this);
    if (! _WaitForDeactivate)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(status);
    }

    //
    // A new DequeueItem is required each time the queue is
    // activated since the KAsyncQueue keeps track of the version
    // of the DequeueItem and the Queue so that an older dequeue
    // won't work when the queue is deactivated and then
    // reactivated quickly.
    //
    KInvariant(_DequeueItem == nullptr);
    status = _OperationQueue->CreateDequeueOperation(_DequeueItem);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    KAsyncContextBase::CompletionCallback deactivateCompletion(_WaitForDeactivate.RawPtr(),
                                                     &ServiceWrapper::WaitForQueueToDeactiveContext::DeactivateCompletion);

    _WaitForDeactivate->Start(NULL, nullptr);

    _OperationQueue->Reuse();
    status = _OperationQueue->Activate(_WaitForDeactivate.RawPtr(), deactivateCompletion);
    if (! NT_SUCCESS(status))
    {
        _WaitForDeactivate = nullptr;
        return(status);
    }

    StartDequeueNextOperation();
    
    return(STATUS_SUCCESS);
}


VOID
ServiceWrapper::StartDequeueNextOperation(
    )
{
    KAsyncContextBase::CompletionCallback dequeueCompletion(this,
                                                             &ServiceWrapper::DequeueCompletion);
    _DequeueItem->Reuse();
    _DequeueItem->StartDequeue(_WaitForDeactivate.RawPtr(), dequeueCompletion);
}

VOID
ServiceWrapper::DequeueCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    KAsyncQueue<Operation>::DequeueOperation::SPtr dequeueItem;
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KInvariant(status == K_STATUS_SHUTDOWN_PENDING);

        return;
    }

    dequeueItem = static_cast<KAsyncQueue<Operation>::DequeueOperation*>(&Async);
                 
    Operation& operation = dequeueItem->GetDequeuedItem();

    KDbgCheckpointWDataInformational(
        operation.GetActivityId(),
        "ServiceWrapper::DequeueCompletion",
        status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&operation);

    switch (operation.GetOperationType())
    {
        case Operation::Open:
        {
            OnOperationOpen(operation);
            break;
        }

        case Operation::Close:
        {
            OnOperationClose(operation);
            break;
        }

        case Operation::Shutdown:
        {
            OnOperationDeleteOrShutdown(operation);
            break;
        }

        case Operation::Delete:
        {               
            OnOperationDeleteOrShutdown(operation);
            break;
        }

        case Operation::Creation:
        {
            OnOperationCreate(operation);
            break;
        }

        case Operation::Recover:
        {
            OuterOperationRecover(operation);
            break;
        }

        default:
        {
            KInvariant(FALSE);
            break;
        }
    }
}

VOID
ServiceWrapper::OpenCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase& Async,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Async);

    KInvariant(ParentAsync != NULL);
    Operation* op = (Operation*)ParentAsync;    

    //
    // Since we are transitioning from closed to open, the usage count
    // must be 0
    //
    KInvariant(_ObjectState == Opening);
    KInvariant(_UsageCount == 0);

    if (NT_SUCCESS(Status))
    {
        //
        // Since the open succeeded we need to assign a usage count for
        // it.
        //
        _UsageCount++;
        _ObjectState = Opened;
    } else {
        KTraceFailedAsyncRequest(Status, nullptr, (ULONGLONG)this, (ULONGLONG)op);

        //
        // Opening the service failed so propagate this back to the
        // originator. This does not qualify as a usage count as the
        // object isn't opened
        //
        _UnderlyingService = nullptr;
        _ObjectState = Closed;
    }
    
    KDbgCheckpointWDataInformational(
        op->GetActivityId(),
        "ServiceWrapper::OperationOpenWorker Completion",
        Status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)op);
    
    op->CompleteOpAndStartNext(Status);
}

NTSTATUS ServiceWrapper::OperationOpenWorker(
    __in Operation& Op,
    __in KAsyncServiceBase::OpenCompletionCallback OpenCompletion
    )
{
    NTSTATUS status;
    
    KDbgCheckpointWDataInformational(
        Op.GetActivityId(),
        "ServiceWrapper::OperationOpenWorker",
        STATUS_SUCCESS,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&Op);

    if (_UsageCount == 0)
    {        
        //
        // Service is transitioning from closed to opened so there is
        // work to do to open the service before moving forward
        // 
        KInvariant(_ObjectState == Closed);
        KInvariant(! _UnderlyingService);

        _ObjectState = Opening;
        
        status = _CreateUnderlyingService(_UnderlyingService, *this, Op, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)&Op);

            //
            // Open failed, report it back
            //
            _ObjectState = Closed;
            return(status);
        }

        status = _UnderlyingService->StartServiceOpen(&Op,
                                                      OpenCompletion);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)&Op);

            //
            // Open failed, report it back
            //
            _ObjectState = Closed;
            _UnderlyingService = nullptr;
        }
        
    } else {
        //
        // Service is running and in use, just bump ref count
        //
        KInvariant(_ObjectState == Opened);
        
        _UsageCount++;
        
        status = STATUS_SUCCESS;
    }
    
    return(status);
}

VOID ServiceWrapper::OnOperationOpen(
    __in Operation& Op
    )
{
    NTSTATUS status;
    KAsyncServiceBase::OpenCompletionCallback openCompletion(this,
                                &ServiceWrapper::OpenCompletion);
        
    status = OperationOpenWorker(Op, openCompletion);

    if (status != STATUS_PENDING)
    {
        Op.CompleteOpAndStartNext(status);
    } 
}

VOID
ServiceWrapper::CloseCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase& Async,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Async);

    ServiceWrapper::Operation* op = (ServiceWrapper::Operation*)ParentAsync;
    
    KInvariant(ParentAsync != NULL);

    //
    // Since we are transitioning from open to close, the usage count
    // must be 1
    //
    KInvariant(_ObjectState == Closing);
    KInvariant(_UsageCount == 1);

    if (NT_SUCCESS(Status))
    {
        //
        // Since the close succeeded we need to clean up
        //
        _UsageCount--;
        _ObjectState = Closed;
        _UnderlyingService = nullptr;
    } else {
        KTraceFailedAsyncRequest(Status, nullptr, (ULONGLONG)this, (ULONGLONG)op);

        //
        // Closing the service failed so propagate this back to the
        // originator. This does not qualify as a usage count as the
        // object isn't closed
        //
        _ObjectState = Opened;
    }
    
    KDbgCheckpointWDataInformational(
        op->GetActivityId(),
        "ServiceWrapper::CloseCompletion",
        Status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)op);
    
    op->CompleteOpAndStartNext(Status);     
}

NTSTATUS ServiceWrapper::OpenToCloseTransitionWorker(
    __in NTSTATUS Status,
    __in Operation& Op,
    __in KAsyncServiceBase::CloseCompletionCallback CloseCompletion                                           
    )
{
    if (NT_SUCCESS(Status))
    {
        Status = _UnderlyingService->StartServiceClose(&Op,
                                                       CloseCompletion);
    }
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, nullptr, (ULONGLONG)this, (ULONGLONG)&Op);

        _ObjectState = Opened;
    }

    KDbgCheckpointWDataInformational(
        Op.GetActivityId(),
        "ServiceWrapper::OpenToCloseTransitionWorker",
        Status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&Op);
    
    return(Status);
}

NTSTATUS ServiceWrapper::OperationCloseWorker(
    __in Operation& Op,
    __in KAsyncServiceBase::CloseCompletionCallback CloseCompletion                                           
    )
{
    NTSTATUS status;
    
    KInvariant(_ObjectState == Opened);
    
    KDbgCheckpointWDataInformational(
        Op.GetActivityId(),
        "ServiceWrapper::OperationCloseWorker",
        STATUS_SUCCESS,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&Op);

    if (_UsageCount > 1)
    {
        //
        // Just remove a ref and continue
        //
        _UsageCount--;
        status = STATUS_SUCCESS;
    } else {
        //
        // We are transitioning from open to closed
        //
        _ObjectState = Closing;
        status = OpenToCloseTransitionWorker(STATUS_SUCCESS, Op, CloseCompletion);
    }
    
    return(status);
}

VOID ServiceWrapper::OnOperationClose(
    __in Operation& Op
    )
{
    NTSTATUS status;

    KDbgCheckpointWDataInformational(
        Op.GetActivityId(),
       "ServiceWrapper::OnOperationClose",
        STATUS_SUCCESS,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&Op);
    
    if (Op._ClosingService != _UnderlyingService.RawPtr())
    {
        //
        // This is the case where a user is closing an older instance
        // of our service. Return back an error as it should affect the
        // current instance.
        //
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, (ULONGLONG)&Op);
        Op.CompleteOpAndStartNext(status);
        return;
    }

    KAsyncServiceBase::CloseCompletionCallback closeCompletion(this,
                                                               &ServiceWrapper::CloseCompletion);
    
    status = OperationCloseWorker(Op, closeCompletion);
    
    KDbgCheckpointWDataInformational(
        Op.GetActivityId(),
        "ServiceWrapper::OperationCloseWorker Done",
        status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&Op);
    
    if (status != STATUS_PENDING)
    {
        Op.CompleteOpAndStartNext(status);
    } 
}

VOID
ServiceWrapper::ForceCloseCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase& Async,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Async);
    
    KInvariant(ParentAsync != NULL);

    ServiceWrapper::Operation& op = *(static_cast<ServiceWrapper::Operation*>(ParentAsync));
    
    KDbgCheckpointWDataInformational(
        op.GetActivityId(),
        "ServiceWrapper::ForceCloseCompletion",
        Status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&op);
        
    if (NT_SUCCESS(Status))
    {
        //
        // Since the close succeeded we need to clean up
        //
        _UsageCount = 0;
        _ObjectState = Closed;
        _UnderlyingService = nullptr;
    } else {
        KTraceFailedAsyncRequest(Status, NULL, (ULONGLONG)this, (ULONGLONG)&op);

        //
        // Closing the service failed so propagate this back to the
        // originator. This does not qualify as a usage count as the
        // object isn't closed
        //
        _ObjectState = Opened;
        op.CompleteOpAndStartNext(Status);
        return;
    }

    if (op.GetOperationType() == Operation::Delete)
    {
        //
        // Now pass on the operation to do the actual delete
        //
        OnOperationDelete(op);
    } else {
        //
        // Otherwise we are just shutting down
        //
        KInvariant(op.GetOperationType() == Operation::Shutdown);
        op.CompleteOpAndStartNext(STATUS_SUCCESS);
    }
}

VOID ServiceWrapper::OnOperationDeleteOrShutdown(
    __in Operation& Op
    )
{
    NTSTATUS status;

    KDbgCheckpointWDataInformational(
        Op.GetActivityId(),
        "ServiceWrapper::OnOperationDeleteOrShutdown",
        STATUS_SUCCESS,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)&Op);

    if (_UnderlyingService)
    {
        //
        // If the service is running then force it to shutdown
        //

        _ObjectState = Closing;
        KAsyncServiceBase::CloseCompletionCallback forceCloseCompletion(this,
                                                                   &ServiceWrapper::ForceCloseCompletion);
        
        status = OpenToCloseTransitionWorker(STATUS_SUCCESS, Op, forceCloseCompletion);

        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, (ULONGLONG)&Op);

            //
            // Close failed, report it back
            //
            Op.CompleteOpAndStartNext(status);
        }
    } else {
        if (Op.GetOperationType() == Operation::Delete)
        {
            //
            // Move on to second phase of delete
            //
            OnOperationDelete(Op);
        } else {
            //
            // Otherwise we are just shutting down
            //
            KInvariant(Op.GetOperationType() == Operation::Shutdown);
        }
    }
}

VOID ServiceWrapper::OnOperationCreate(
    __in Operation& Op
    )
{
    //
    // Expect this to be overridden
    //
    Op.CompleteOpAndStartNext(STATUS_SUCCESS);
}

VOID ServiceWrapper::OnOperationDelete(
    __in Operation& Op
    )
{
    //
    // Expect this to be overridden
    //
    Op.CompleteOpAndStartNext(STATUS_SUCCESS);
}

VOID
ServiceWrapper::RecoveryOpenCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase& Async,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Async);

    KInvariant(ParentAsync != NULL);
    Operation* op = (Operation*)ParentAsync;

    KDbgCheckpointWDataInformational(
        op->GetActivityId(),
        "ServiceWrapper::RecoveryOpenCompletion",
        Status,
        (ULONGLONG)_ObjectState,
        (ULONGLONG)this,
        (ULONGLONG)_UsageCount,
        (ULONGLONG)op);

    if (NT_SUCCESS(Status))
    {
        _UsageCount++;
        _ObjectState = Opened;
        
        KAsyncServiceBase::CloseCompletionCallback closeCompletion(this,
                                                                   &ServiceWrapper::CloseCompletion);

        Status = OperationCloseWorker(*op, closeCompletion);

        if (Status == STATUS_PENDING)
        {
            return;
        } 
    } else {
        KTraceFailedAsyncRequest(Status, nullptr, (ULONGLONG)this, (ULONGLONG)op);

        _ObjectState = Closed;
        _UnderlyingService = nullptr;
    }
    
    op->CompleteOpAndStartNext(Status);
}

VOID ServiceWrapper::OuterOperationRecover(
    __in Operation& Op
    )
{
    NTSTATUS status;
    KAsyncServiceBase::OpenCompletionCallback openCompletion(this,
                                &ServiceWrapper::RecoveryOpenCompletion);
    
    status = OperationOpenWorker(Op, openCompletion);
    if (status == STATUS_PENDING)
    {
        return;
    } else if (NT_SUCCESS(status)) {        
        KAsyncServiceBase::CloseCompletionCallback closeCompletion(this,
                                                                   &ServiceWrapper::CloseCompletion);

        status = OperationCloseWorker(Op, closeCompletion);

        if (status == STATUS_PENDING)
        {
            return;
        } 
    }
        
    Op.CompleteOpAndStartNext(status);
}

//
// Operation sub-op
//
ServiceWrapper::GlobalContext::GlobalContext(
    __in KActivityId ActivityId
    ) :
    KAsyncGlobalContext(ActivityId)
{
}

ServiceWrapper::GlobalContext::GlobalContext()
{
}

ServiceWrapper::GlobalContext::~GlobalContext()
{
}

NTSTATUS ServiceWrapper::GlobalContext::Create(
    __in KGuid Id,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out ServiceWrapper::GlobalContext::SPtr& GlobalContext
    )
{
    KActivityId activityId = (KActivityId)Id.Data1;
    ServiceWrapper::GlobalContext::SPtr globalContext;

    globalContext = _new(AllocationTag, Allocator) ServiceWrapper::GlobalContext(activityId);
    if (!globalContext)
    {
        KTraceOutOfMemory(activityId, STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (!NT_SUCCESS(globalContext->Status()))
    {
        return(globalContext->Status());
    }

    GlobalContext = Ktl::Move(globalContext);
    return(STATUS_SUCCESS);
}


ServiceWrapper::Operation::~Operation()
{
}

ServiceWrapper::Operation::Operation(
    __in ServiceWrapper& Service
    ) : _Service(&Service)
{
}

NTSTATUS ServiceWrapper::Operation::Create(
    __out Operation::SPtr& Context,
    __in ServiceWrapper& Service,    
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    Operation::SPtr context;

    context = _new(AllocationTag, Allocator) Operation(Service);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);     
}

VOID
ServiceWrapper::Operation::OnStart()
{
}

VOID
ServiceWrapper::Operation::StartAndQueueOperation(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    NTSTATUS status;

    KDbgCheckpointWDataInformational(
        GetActivityId(),
        "ServiceWrapper::Operation::StartAndQueueOperation",
        STATUS_SUCCESS,
        (ULONGLONG)this,
        (ULONGLONG)_Service.RawPtr(),
        (ULONGLONG)_ClosingService,
        _Type);
    
    Start(ParentAsyncContext, CallbackPtr);
    status = _Service->_OperationQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)ParentAsyncContext, 0);
        Complete(status);
    }
}
   
VOID
ServiceWrapper::Operation::StartOpenOperation(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Type = Open;
    StartAndQueueOperation(ParentAsyncContext, CallbackPtr);
}

VOID
ServiceWrapper::Operation::StartCloseOperation(
    __in VOID* ClosingService,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Type = Close;
    _ClosingService = ClosingService;
    StartAndQueueOperation(ParentAsyncContext, CallbackPtr);
}

VOID
ServiceWrapper::Operation::StartCreateOperation(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Type = Creation;
    StartAndQueueOperation(ParentAsyncContext, CallbackPtr);
}

VOID
ServiceWrapper::Operation::StartShutdownOperation(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Type = Shutdown;
    StartAndQueueOperation(ParentAsyncContext, CallbackPtr);
}

VOID
ServiceWrapper::Operation::StartRecoverOperation(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Type = Recover;
    StartAndQueueOperation(ParentAsyncContext, CallbackPtr);
}

VOID
ServiceWrapper::Operation::StartDeleteOperation(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Type = Delete;
    StartAndQueueOperation(ParentAsyncContext, CallbackPtr);
}

VOID
ServiceWrapper::Operation::CompleteOpAndStartNext(
    __in NTSTATUS Status
    )
{
    KDbgCheckpointWDataInformational(
        GetActivityId(),
        "ServiceWrapper::Operation::CompleteOpAndStartNext",
        Status,
        (ULONGLONG)this,
        (ULONGLONG)_Service.RawPtr(),
        (ULONGLONG)_ClosingService,
        _Type);

    Complete(Status);
    _Service->StartDequeueNextOperation();
}
