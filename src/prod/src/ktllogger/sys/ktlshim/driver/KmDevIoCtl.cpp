// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "logger.h"

//
// Kernel side of IoControl
//

DevIoControlKernel::DevIoControlKernel()
{
}

DevIoControlKernel::~DevIoControlKernel()
{
}

DevIoControlKm::DevIoControlKm()
{
}

DevIoControlKm::DevIoControlKm(
    __in ULONG AllocationTag
    ) : _AllocationTag(AllocationTag)
{
}

DevIoControlKm::~DevIoControlKm()
{
}

NTSTATUS
DevIoControlKm::Create(
    __in WDFREQUEST Request,
    __out DevIoControlKernel::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    DevIoControlKm::SPtr context;
    
    context = _new(AllocationTag, Allocator) DevIoControlKm(AllocationTag);
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // For bufffered ioctls WdfRequestRetrieveInputBuffer &
    // WdfRequestRetrieveOutputBuffer return the same buffer
    // pointer (Irp->AssociatedIrp.SystemBuffer)
    //
    PVOID buffer = NULL;
    size_t inBufferSize = 0;
    size_t outBufferSize = 0;
    
    status = WdfRequestRetrieveOutputBuffer(Request,
                                           FIELD_OFFSET(RequestMarshaller::REQUEST_BUFFER, Data),
                                           (PVOID*)&buffer,
                                           &outBufferSize);
    if (! NT_SUCCESS(status))
    {
        //
        // if there is an error reading the outbuffer then we assume
        // there is no outbuffer. This isn't an error in the case of
        // the DeleteObjectRequest.
        //
        status = STATUS_SUCCESS;
        outBufferSize = 0;
    }
    
    status = WdfRequestRetrieveInputBuffer(Request,
                                           FIELD_OFFSET(RequestMarshaller::REQUEST_BUFFER, Data),
                                           (PVOID*)&buffer,
                                           &inBufferSize);
    if (! NT_SUCCESS(status))
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        return(status);
    }

    context->_Buffer = (PUCHAR)buffer;
    context->_InBufferSize = (ULONG)inBufferSize;
    context->_OutBufferSize = (ULONG)outBufferSize;

    context->_Request = Request;

    context->_IsPassedQueue = FALSE;
    context->_IsFinishedProcessing = FALSE;
    context->_HasCancelCallbackInvoked = FALSE;
    context->_CancelCallbackShouldComplete = FALSE;
    context->_CompletionStatus = STATUS_SUCCESS;
    context->_CompletionSize = 0;
        
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

VOID
DevIoControlKm::CompleteRequest(
    __in NTSTATUS Status,
    __in ULONG OutBufferSize
    )
{
    BOOLEAN completeRequest = FALSE;

    //
    // Entrypoint for request completion when it has been completed.
    // This routine must synchronize its completion with the management
    // of the request through the WDF queue. If the request has not
    // been passed through the WDF queue yet then do not complete here but
    // set a flag so it gets completed at the time it passes through
    // the queue
    //
    
    K_LOCK_BLOCK(_CompletionLock)
    {
        if (_IsPassedQueue)
        {
            //
            // Since the request has passed through the queue, we will
            // complete it.
            //
            completeRequest = TRUE;
        }

        //
        // Remember that the request has finished processing and the
        // results of that. Note that unless the routine is completing
        // the request below, the request may be gone once out of the
        // lock and so don't touch it.
        //
        _CompletionStatus = Status;
        _CompletionSize = OutBufferSize;
        _IsFinishedProcessing = TRUE;
    }

    if (completeRequest)
    {
        //
        // This routine may own completion since _IsPassedQueue and
        // _IsFinishedProcessing are both TRUE. However completion
        // needs to be also synchronized with cancel.
        //
#ifdef VERBOSE
        RequestMarshaller::REQUEST_BUFFER* r = (RequestMarshaller::REQUEST_BUFFER*)_Buffer;
        KDbgCheckpointWData(0, "CompleteRequest when done", _CompletionStatus,
                            r->RequestId,
                            (ULONGLONG)this,
                            0,
                            0);
#endif
        
        K_LOCK_BLOCK(_CompletionLock)
        {
            if (! _HasCancelCallbackInvoked)
            {
                //
                // If the request has not yet had its cancel callback
                // invoked then unmark needs to be called to unregister
                // the callback from WDF. If the cancel callback has
                // been invoked then unmark does not need to be called.
                //
                Status = WdfRequestUnmarkCancelable(_Request);

#ifdef VERBOSE
                KDbgCheckpointWData(0, "Unmark", Status,
                                    r->RequestId,
                                    (ULONGLONG)this,
                                    0,
                                    0);
#endif
                
                if (Status == STATUS_CANCELLED)
                {
                    //
                    // We have hit a race where WDF has a cancellation
                    // pending for this request which must invoke the
                    // cancel callback before the request can be completed. So set
                    // the flag to tell the cancellation routine that it must complete
                    // the request. Note that unless the request is
                    // being cancelled below that the routine should
                    // not touch the request.
                    //
                    completeRequest = FALSE;
                    _CancelCallbackShouldComplete = TRUE;
                    return;
                }
            }
        }

        if (completeRequest)
        {
            WdfRequestCompleteWithInformation(_Request, _CompletionStatus, _CompletionSize);
        }
    }
}

VOID
DevIoControlKm::SyncRequestCompletion(
    )
{
    BOOLEAN completeRequest = FALSE;

    //
    // This routine is called when the request is dispatch from the
    // queue. It needs to determine if the request has finished and if
    // so then complete it. Otherwise set a flag indicating that since
    // the request has passed through the queue it can be completed at
    // any time.
    //
    K_LOCK_BLOCK(_CompletionLock)
    {
        if (_IsFinishedProcessing)
        {
            completeRequest = TRUE;
        }

        _IsPassedQueue = TRUE;
        //
        // Once we leave the lock, don't touch the request unless we
        // are completing it
        //
    }

    if (completeRequest)
    {
        //
        // The request is ready to be completed however it first must
        // synchronize the actual completion with cancel
        //
#ifdef VERBOSE
        RequestMarshaller::REQUEST_BUFFER* r = (RequestMarshaller::REQUEST_BUFFER*)_Buffer;
        KDbgCheckpointWData(0, "CompleteRequest From Queue", _CompletionStatus,
                            r->RequestId,
                            (ULONGLONG)this,
                            0,
                            0);
#endif
        
        K_LOCK_BLOCK(_CompletionLock)
        {
            NTSTATUS status;

            if (! _HasCancelCallbackInvoked)
            {
                //
                // If the request has not yet had its cancel callback
                // invoked then unmark needs to be called to unregister
                // the callback from WDF
                //
                status = WdfRequestUnmarkCancelable(_Request);

#ifdef VERBOSE
                KDbgCheckpointWData(0, "Unmark from queue", status,
                                    r->RequestId,
                                    (ULONGLONG)this,
                                    0,
                                    0);
#endif
                
                if (status == STATUS_CANCELLED)
                {
                    //
                    // We have hit a race where WDF has a cancellation
                    // pending for this request which must call the
                    // cancelation callback before
                    // the request can be completed. So set the flag to
                    // tell the cancellation callback routine that it must complete
                    // the request. Note that unless the request is
                    // being cancelled below that it should not be
                    // touched after leaving the lock.
                    //
                    completeRequest = FALSE;
                    _CancelCallbackShouldComplete = TRUE;
                    return;
                }
            }
        }

        if (completeRequest)
        {
            WdfRequestCompleteWithInformation(_Request, _CompletionStatus, _CompletionSize);
        }
    }
}

VOID
DevIoControlKm::SyncRequestCancellation(
    __in RequestMarshallerKernel::SPtr Marshaller
    )
{
    BOOLEAN cancelRequest = FALSE;
    BOOLEAN completeRequest = FALSE;

    //
    // This routine is called from the cancel callback and synchronizes
    // cancel with completion.
    //
    K_LOCK_BLOCK(_CompletionLock)
    {
        if (! _IsFinishedProcessing)
        {
            //
            // Only invoke cancellation if the request is not finished
            // yet. No point in cancelling if the request is done.
            //
            cancelRequest = TRUE;
        }
    }

    if (cancelRequest)
    {
#ifdef VERBOSE
        RequestMarshaller::REQUEST_BUFFER* r = (RequestMarshaller::REQUEST_BUFFER*)_Buffer;
        KDbgCheckpointWData(0, "SyncRequestCancellation", STATUS_SUCCESS,
                            r->RequestId,
                            (ULONGLONG)this,
                            0,
                            0);
#endif
        
        //
        // Be sure to hold a refcount on the marshaller over the cancel callback.
        // There could be a race where completion is racing with cancel
        // and we do not want the marshaller to disappear while in
        // PerformCancel(). A refcount is being held by virtue of the
        // Marshaller being passed as an SPtr.
        //
        Marshaller->PerformCancel();

#ifdef VERBOSE
        KDbgCheckpointWData(0, "SyncRequestCancellation Completed", STATUS_SUCCESS,
                            r->RequestId,
                            (ULONGLONG)this,
                            0,
                            0);
#endif
    }

    K_LOCK_BLOCK(_CompletionLock)
    {
        //
        // Set flag to allow normal completion path to skip calling
        // unmark and complete the request on its own. Note that
        // unless completing the operation below, the request may
        // not be touched once out of this spinlock.
        //
        _HasCancelCallbackInvoked = TRUE;           

        if (_CancelCallbackShouldComplete)
        {
            //
            // If the request is all finished up but couldn't be
            // completed due to a WDF cancel callback race then we need
            // to complete it here
            //
            completeRequest = TRUE;
        } else {
            //
            // Completion should be done through the normal
            // completion path. Setting _HasCancelCallbackInvoked
            // allows this to happen.
            //
            return;
        }
    }        
    
    if (completeRequest)
    {
        //
        // NOTE: since this is the cancellation routine there is no
        // need to call UnMark
        //
#ifdef VERBOSE
        RequestMarshaller::REQUEST_BUFFER* r = (RequestMarshaller::REQUEST_BUFFER*)_Buffer;
        KDbgCheckpointWData(0, "CompleteRequest From CancelRoutine", STATUS_SUCCESS,
                            r->RequestId,
                            (ULONGLONG)this,
                            0,
                            0);
#endif
        
        WdfRequestCompleteWithInformation(_Request, _CompletionStatus, _CompletionSize);        
    }
}
