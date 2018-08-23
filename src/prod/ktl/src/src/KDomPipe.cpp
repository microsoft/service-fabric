
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KDomPipe.cpp

    Description:
      KTL Document Object Model pipe

    History:
      richhas          05-Mar-2012         Initial version.

--*/

#include <ktl.h>

//*** KDomPipeQueuedWriteOp implementation
KDomPipeQueuedWriteOp::KDomPipeQueuedWriteOp()
{
}

KDomPipeQueuedWriteOp::~KDomPipeQueuedWriteOp()
{
    KAssert(_Dom == nullptr);
}

void KDomPipeQueuedWriteOp::OnStart()
{
}


//*** KDomPipe implementation
KDomPipe::KDomPipe()
    :   KAsyncQueue<KDomPipeQueuedWriteOp>(KTL_TAG_DOM, FIELD_OFFSET(KDomPipeQueuedWriteOp, _QueueEntry))
{
}

KDomPipe::~KDomPipe()
{
}

NTSTATUS
KDomPipe::Create(__out KDomPipe::SPtr& Result, __in KAllocator& Allocator, __in ULONG AllocationTag)
{
    NTSTATUS        status;

    Result = _new(AllocationTag, Allocator) KDomPipe();
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
    }

    return status;
}

NTSTATUS
KDomPipe::StartWrite(
    __in KIDomNode& Dom,         
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt CompletionCallback CallbackPtr)
{
    NTSTATUS        status;
    KDomPipeQueuedWriteOp::SPtr op = _new(KTL_TAG_DOM, GetThisAllocator()) KDomPipeQueuedWriteOp();
    if (op == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    op->_Dom = &Dom;
    op->Start(ParentAsyncContext, CallbackPtr);
    status = __super::Enqueue(*op);
    if (!NT_SUCCESS(status))
    {
        op->_Dom = nullptr;
        op->Complete(status);
    }

    return STATUS_PENDING;
}

//*** KDomPipe::DequeueOperation implementation
NTSTATUS
KDomPipe::CreateDequeueOperation(__out KDomPipe::DequeueOperation::SPtr& Result)
{
    NTSTATUS status = STATUS_SUCCESS;

    Result = _new(_AllocationTag, GetThisAllocator()) KDomPipe::DequeueOperation(*this, _ThisVersion);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;
}

KDomPipe::DequeueOperation::DequeueOperation(KDomPipe& Owner, ULONG const OwnerVersion)
    :   KAsyncQueue<KDomPipeQueuedWriteOp>::DequeueOperation(Owner, OwnerVersion)
{
}

KDomPipe::DequeueOperation::~DequeueOperation()
{
}

void    /* override */
KDomPipe::DequeueOperation::CompleteDequeue(KDomPipeQueuedWriteOp& ItemDequeued)
{
    _Dom = Ktl::Move(ItemDequeued._Dom);
    KFatal(ItemDequeued.Complete(STATUS_SUCCESS));
    __super::CompleteDequeue(ItemDequeued);
}

void 
KDomPipe::DequeueOperation::OnReuse()
{
    _Dom = nullptr;
    __super::OnReuse();
}
