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
// KtlLogContainer Implementation
//
//************************************************************************************

const ULONG KtlLogContainerKernel::OverlayLinkageOffset = FIELD_OFFSET(KtlLogContainerKernel, _OverlayContainerEntry);

KtlLogContainerKernel::KtlLogContainerKernel(
    __in KtlLogContainerId LogContainerId,
    __in KGuid DiskId
    )  : _ContextList(ActiveContext::_LinkOffset)
{
    NTSTATUS status;

    _LogContainerId = LogContainerId;
    _DiskId = DiskId;
    _BaseLogContainer = nullptr;
#if defined(UDRIVER) || defined(KDRIVER)
    _ObjectId = 0;
#endif
    _IsClosed = FALSE;

    status = CreateAsyncCloseContext(_AutoCloseContext);
    if ( ! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }       
}

KtlLogContainerKernel::~KtlLogContainerKernel()
{
}

//
// IsFunctional()
//
BOOLEAN
KtlLogContainerKernel::IsFunctional(
    )
{
    return(TRUE);
}

NTSTATUS
KtlLogContainerKernel::AddActiveContext(
    __in ActiveContext& Context
)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    K_LOCK_BLOCK(_ThisLock)
    {
        KInvariant(! _ContextList.IsOnList(&Context));
        
        //
        // Container is already closed, we don't want to move forward here
        //
        if (_IsClosed)
        {
            status = STATUS_OBJECT_NO_LONGER_EXISTS;
            KDbgCheckpointWData(0, "Container Already Closed", status,
                                (ULONGLONG)Context.GetMarshaller()->GetRequestId(),
                                (ULONGLONG)this,
                                (ULONGLONG)&Context,
                                (ULONGLONG)Context.GetMarshaller());                                
            return(status);
        }

        if (Context.IsCancelled())
        {
            status = STATUS_CANCELLED;
            KDbgCheckpointWData(0, "Context Already Cancelled", status,
                                (ULONGLONG)Context.GetMarshaller()->GetRequestId(),
                                (ULONGLONG)this,
                                (ULONGLONG)&Context,
                                (ULONGLONG)Context.GetMarshaller());                                
            return(status);
        }
        
        //
        // Put the request on the list
        //
        _ContextList.Add(&Context);
        status = STATUS_SUCCESS;
    }
    
    return(status);
}

BOOLEAN
KtlLogContainerKernel::RemoveActiveContextNoSpinLock(
    __in ActiveContext& Context
)
{
    BOOLEAN b;

    //
    // NOTE: This routine assumes that the caller holds the spinlock
    //
    
    b = _ContextList.Remove(&Context);
    
    return(b);
}

BOOLEAN
KtlLogContainerKernel::RemoveActiveContext(
    __in ActiveContext& Context
)
{
    BOOLEAN b = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        b = _ContextList.Remove(&Context);
    }
    
    return(b);
}

VOID
KtlLogContainerKernel::SetCancelFlag(
    __in ActiveContext& Context
)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        Context.SetCancelFlag();
    }   
}

//
// Create Log Stream
//
VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::OnCompleted(
    )
{   
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    
    _BaseAsyncCreateLogStream = nullptr;
    _LogStream = nullptr;
    
    _LogStreamAlias = nullptr;
    _Path = nullptr;
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::LogStreamCreateFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::LogStreamCreateCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = CreateStream;

            if ((_LogStreamAlias != nullptr) && (_LogStreamAlias->Length() > KtlLogContainer::MaxAliasLength))
            {
                Status = STATUS_NAME_TOO_LONG;
                
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);               
                return;             
            }

            
            _LogStream = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogStreamKernel(
                                                               *_LogContainer,
                                                               _LogContainer->_DiskId,
                                                               _LogContainer->_BaseLogContainer->QueryUserRecordSystemMetadataOverhead());
            if (! _LogStream)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, Status, 0, 0, 0);
                Complete(Status);               
                return;             
            }
            
            Status = _LogStream->Status();
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);               
                return;
            }   

            KStringView prefix(L"KernelWrite");
            Status = _LogStream->_WriteIC->SetComponentName(prefix, _LogStreamId.Get());
            if (! NT_SUCCESS(Status))
            {               
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);               
                return;
            }   
            
            OverlayLog::AsyncCreateLogStreamContext::SPtr asyncCreateLogStreamContext;

            Status = _LogContainer->_BaseLogContainer->CreateAsyncCreateLogStreamContext(asyncCreateLogStreamContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);               
                return;
            }

            //
            // If path specified then extract its disk id
            //
            KGuid diskId;
            
            if (_Path)
            {
                Status = _LogContainer->_BaseLogContainer->GetOverlayManager()->MapPathToDiskId(*_Path,
                                                                                                diskId);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);               
                    return;
                }
            } else {
                diskId = _LogContainer->_DiskId;
            }
            
            _BaseAsyncCreateLogStream = down_cast<OverlayLog::AsyncCreateLogStreamContextOverlay>(asyncCreateLogStreamContext);            
            _BaseAsyncCreateLogStream->StartCreateLogStream(_LogStreamId,
                                                            _LogStreamType,
                                                            diskId,
                                                            _Path.RawPtr(),
                                                            _MaximumRecordSize,
                                                            _MaximumStreamSize,
                                                            _MaximumMetaDataLength,
                                                            _LogStreamAlias.RawPtr(),
                                                            _Flags,
                                                            _LogStream->_BaseLogStream,
                                                            this,
                                                            completion);

            break;
        }

        case CreateStream:
        {
            _LogStream->_BaseLogStream->AddKtlLogStream(*_LogStream);
            
            _LogStream->_LogStreamId = _LogStreamId;
            
            _LogStream->_WriteIC->SetReportFrequency(KInstrumentedComponent::DefaultReportFrequency);
            
            (*_ResultLogStreamPtr) = Ktl::Move(reinterpret_cast<KtlLogStream::SPtr&>(_LogStream));

            Complete(STATUS_SUCCESS);   
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::LogStreamCreateCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    LogStreamCreateFSM(status); 
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::OnStart(
    )
{
    KInvariant(_State == Initial);

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    NTSTATUS status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;

    GUID guidNull = { 0 };

    //
    // Validate log stream id is not null
    //
    if (IsEqualGUID(_LogStreamId.Get(),guidNull))
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Validate the log stream type
    //
    if ((_LogStreamType != KtlLogInformation::KtlLogDefaultStreamType()) &&
        (_LogStreamType != KLogicalLogInformation::GetLogicalLogStreamType()))
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
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
    
    LogStreamCreateFSM(STATUS_SUCCESS);   
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::StartCreateLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Version with LogStreamType parameter should be used instead
    //
    KInvariant(FALSE);
#endif
    _State = Initial;

    _LogStreamId = LogStreamId;
    _LogStreamType = KtlLogInformation::KtlLogDefaultStreamType();
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;   
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = 0;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::StartCreateLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in const KtlLogStreamType& LogStreamType,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Version with LogStreamType and flags parameter should be used instead
    //
    KInvariant(FALSE);
#endif
    _State = Initial;
    
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;   
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = 0;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::StartCreateLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in const KtlLogStreamType& LogStreamType,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = Flags;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::CreateLogStreamAsync(
    __in const KtlLogStreamId& LogStreamId,
    __in const KtlLogStreamType& LogStreamType,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext
)
{
    _State = Initial;
        
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = Flags;
    _ResultLogStreamPtr = &LogStream;
    _State = Initial;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);
    
    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::OnReuse(
    )
{
}

VOID
KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncCreateLogStream->Cancel();
}

KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::AsyncCreateLogStreamContextKernel()
{
}

KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::~AsyncCreateLogStreamContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncCreateLogStreamContext(
    __out AsyncCreateLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::SPtr context;

    KInvariant(_BaseLogContainer != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCreateLogStreamContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
        
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogContainer = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Delete log stream
//
VOID
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    _BaseAsyncDeleteLogStream = nullptr;
}

VOID
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::LogStreamDeleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    KDbgCheckpointWDataInformational(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId), "AsyncDeleteStreamContextKernel Deleted", status,
                        (ULONGLONG)0,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);
    
    Complete(status);   
}

VOID
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::OnStart(
    )
{
    NTSTATUS status;
    
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    status = _LogContainer->_BaseLogContainer->CreateAsyncDeleteLogStreamContext(_BaseAsyncDeleteLogStream);
    if (! NT_SUCCESS(status))
    {       
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    KDbgCheckpointWDataInformational(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId), "AsyncDeleteStreamContextKernel", status,
                        (ULONGLONG)0,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::LogStreamDeleted);
    _BaseAsyncDeleteLogStream->StartDeleteLogStream(_LogStreamId,
                                                    this,
                                                    completion);

}

VOID
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::StartDeleteLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::DeleteLogStreamAsync(
    __in const KtlLogStreamId& LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _LogStreamId = LogStreamId;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);
    
    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::OnReuse(
    )
{
}

VOID
KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncDeleteLogStream->Cancel();
}

KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::AsyncDeleteLogStreamContextKernel()
{
}

KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::~AsyncDeleteLogStreamContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncDeleteLogStreamContext(
    __out AsyncDeleteLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::SPtr context;

    KInvariant(_BaseLogContainer != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteLogStreamContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// Open log stream
//
VOID
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    _BaseAsyncOpenLogStream = nullptr;
}

VOID
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::LogStreamOpened(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (NT_SUCCESS(status))
    {
        _LogStream->_BaseLogStream->AddKtlLogStream(*_LogStream);
        
        _LogStream->_LogStreamId = _LogStreamId;

        _LogStream->_WriteIC->SetReportFrequency(KInstrumentedComponent::DefaultReportFrequency);
        
        *_MaximumMetaDataLength = _LogStream->_BaseLogStream->QueryMaxMetadataLength();
        (*_ResultLogStreamPtr) = Ktl::Move(reinterpret_cast<KtlLogStream::SPtr&>(_LogStream));
    }

    _LogStream = nullptr;
    
    Complete(status);   
}

VOID
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::OnStart(
    )
{
    NTSTATUS status;
    
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    _LogStream = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogStreamKernel(
                                                               *_LogContainer,
                                                               _LogContainer->_DiskId,
                                                               _LogContainer->_BaseLogContainer->QueryUserRecordSystemMetadataOverhead());
        
    if (!_LogStream)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _LogStream->Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }   

    KStringView prefix(L"KernelWrite");
    status = _LogStream->_WriteIC->SetComponentName(prefix, _LogStreamId.Get());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }   
    
    OverlayLog::AsyncOpenLogStreamContext::SPtr asyncOpenLogStream;
    
    status = _LogContainer->_BaseLogContainer->CreateAsyncOpenLogStreamContext(asyncOpenLogStream);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    _BaseAsyncOpenLogStream = down_cast<OverlayLog::AsyncOpenLogStreamContextOverlay>(asyncOpenLogStream);
        
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::LogStreamOpened);
    _BaseAsyncOpenLogStream->StartOpenLogStream(_LogStreamId,
                                                _LogStream->_BaseLogStream,
                                                this,
                                                completion);
}

VOID
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::StartOpenLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __out ULONG* MaximumMetaDataLength,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _LogStreamId = LogStreamId;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::OpenLogStreamAsync(
    __in const KtlLogStreamId& LogStreamId,
    __out ULONG* MaximumMetaDataLength,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _LogStreamId = LogStreamId;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _ResultLogStreamPtr = &LogStream;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::OnReuse(
    )
{
}

VOID
KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncOpenLogStream->Cancel();
}

KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::AsyncOpenLogStreamContextKernel()
{
}

KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::~AsyncOpenLogStreamContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncOpenLogStreamContext(
    __out AsyncOpenLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::SPtr context;

    KInvariant(_BaseLogContainer != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncOpenLogStreamContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogContainer = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Close
//
VOID
KtlLogContainerKernel::AsyncCloseContextKernel::OnCompleted(
    )
{
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    
    _BaseAsyncCloseLog = nullptr;
    
#ifdef UPASSTHROUGH
    if (_CloseCallbackPtr)
    {
        _CloseCallbackPtr(_ParentAsyncContext,
            *up_cast<KtlLogContainer, KtlLogContainerKernel>(_LogContainer),
            Status());
    }
#endif
}

VOID
KtlLogContainerKernel::AsyncCloseContextKernel::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncCloseContextKernel::OperationCompletion);

    KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogContainer->_LogContainerId),
                        "KtlLogContainerKernel::AsyncCloseContextKernel", Status,
                        _State, (ULONGLONG)this, 0, 0);
    
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _State = CompletedWithError;
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = StartOperationDrain;

            //
            // Take lock here to ensure that no new requests get added to the
            // container after marking the container as closed
            //
            K_LOCK_BLOCK(_LogContainer->_ThisLock)
            {
                if (_LogContainer->_IsClosed)
                {
                    //
                    // Since this shim object was already closed there
                    // is nothing to do
                    //
                    _State = Completed;
#if UPASSTHROUGH
                    KTraceFailedAsyncRequest(STATUS_OBJECT_NO_LONGER_EXISTS, this, 0, 0);
                    Complete(STATUS_OBJECT_NO_LONGER_EXISTS);
#else
                    Complete(STATUS_SUCCESS);
#endif
                    return;
                }
                
                //
                // Indicate that the stream is closed and no new requests should be
                // added
                //
                _LogContainer->_IsClosed = TRUE;

                //
                // Set flag so that when list is empty the event is signalled.
                // Functionality for Close is to wait for all operations to be
                // drained.
                //
                _LogContainer->_ContextList.SignalWhenEmpty();
            }

#if 0
            if (_Abort)
            {
                // TODO: abort all other requests
            }
#endif
            _WaitContext->StartWaitUntilSet(this,
                                            completion);
            
            break;
        }

        case StartOperationDrain:
        {
            //
            // Now close overlay log downstream
            //
            _State = CloseOverlayLog;

            Status = _LogContainer->_BaseLogContainer->GetOverlayManager()->CreateAsyncCloseLogContext(_BaseAsyncCloseLog);
            if (!NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);                
                _State = CompletedWithError;
                Complete(Status);
                return;
            }

            _BaseAsyncCloseLog->StartCloseLog(_LogContainer->_BaseLogContainer,
                                              this,
                                              completion);
            
            break;
        }

        case CloseOverlayLog:
        {
            Complete(STATUS_SUCCESS);
            break;
        }

        
        default:
        {
            KInvariant(FALSE);
            
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            
            _State = CompletedWithError;
            Complete(STATUS_UNSUCCESSFUL);
            return;
        }
    }    
}

#ifdef UPASSTHROUGH
VOID
KtlLogContainerKernel::AsyncCloseContextKernel::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
    )
{
    _State = Initial;
    _CloseCallbackPtr = CloseCallbackPtr;
    _ParentAsyncContext = ParentAsyncContext;
    Start(ParentAsyncContext, nullptr); 
}
#endif

VOID
KtlLogContainerKernel::AsyncCloseContextKernel::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncCloseContextKernel::CloseAsync(
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _State = Initial;
    
    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncCloseContextKernel::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
KtlLogContainerKernel::AsyncCloseContextKernel::OnStart(
    )
{
    KInvariant(_State == Initial);

    FSMContinue(STATUS_SUCCESS);   
}

VOID
KtlLogContainerKernel::AsyncCloseContextKernel::OnReuse(
    )
{
    _WaitContext->Reuse();
}

VOID
KtlLogContainerKernel::AsyncCloseContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

KtlLogContainerKernel::AsyncCloseContextKernel::AsyncCloseContextKernel()
{
#ifdef UPASSTHROUGH
    _CloseCallbackPtr = NULL;
    _ParentAsyncContext = NULL;
#endif
}

KtlLogContainerKernel::AsyncCloseContextKernel::~AsyncCloseContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncCloseContext(
    __out AsyncCloseContextKernel::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCloseContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _ContextList.CreateWaitContext(GetThisAllocator(),
                                            GetThisAllocationTag(),
                                            context->_WaitContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

NTSTATUS
KtlLogContainerKernel::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
)
{
#ifdef UPASSTHROUGH
    NTSTATUS status;
    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (NT_SUCCESS(status))
    {
        closeContext->_LogContainer = this;
        closeContext->StartClose(ParentAsyncContext,
                                 CloseCallbackPtr);
    }
    
    return(status);
#endif
#if defined(UDRIVER) || defined(KDRIVER)    
    UNREFERENCED_PARAMETER(ParentAsyncContext);
    UNREFERENCED_PARAMETER(CloseCallbackPtr);
    
    KInvariant(FALSE);

    return(STATUS_SUCCESS);
#endif
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::CloseAsync(
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }
    
    closeContext->_LogContainer = this;

    co_return co_await closeContext->CloseAsync(ParentAsyncContext);
}
#endif

VOID
KtlLogContainerKernel::AsyncAssignAliasContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    _Alias = nullptr;
}

VOID
KtlLogContainerKernel::AsyncAssignAliasContextKernel::AliasAssigned(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }   

    Complete(status);   
}

VOID
KtlLogContainerKernel::AsyncAssignAliasContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    if (_Alias->Length() > KtlLogContainer::MaxAliasLength)
    {
        status = STATUS_NAME_TOO_LONG;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //
    // Now persist the aliases on disk
    //
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncAssignAliasContextKernel::AliasAssigned); 
    _AsyncAliasOperation->StartUpdateAlias(*_Alias,
                                           _LogStreamId,
                                           this,
                                           completion);
}

VOID
KtlLogContainerKernel::AsyncAssignAliasContextKernel::StartAssignAlias(
    __in KString::CSPtr& Alias,
    __in KtlLogStreamId LogStreamId,                             
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    
    _LogStreamId = LogStreamId;
    _Alias = Alias;
    
    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncAssignAliasContextKernel::AssignAliasAsync(
    __in KString::CSPtr& Alias,
    __in KtlLogStreamId LogStreamId,                             
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{    
    _LogStreamId = LogStreamId;
    _Alias = Alias;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncAssignAliasContextKernel::OnReuse(
    )
{
    _AsyncAliasOperation->Reuse();
}

VOID
KtlLogContainerKernel::AsyncAssignAliasContextKernel::OnCancel(
    )
{
    //
    // AssignAlias is invoked as a subop from CreateLogStream. In that
    // case the marshaller is not set
    //
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller() ? _ActiveContext.GetMarshaller()->GetRequestId() : 0);
    _AsyncAliasOperation->Cancel();
}

KtlLogContainerKernel::AsyncAssignAliasContextKernel::AsyncAssignAliasContextKernel()
{
}

KtlLogContainerKernel::AsyncAssignAliasContextKernel::~AsyncAssignAliasContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncAssignAliasContext(
    __out AsyncAssignAliasContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncAssignAliasContextKernel::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncAssignAliasContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _BaseLogContainer->CreateAsyncAliasOperationContext(context->_AsyncAliasOperation);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
            
    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// Remove alias
//
VOID
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    _Alias = nullptr;
}

VOID
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::AliasRemoved(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }   

    Complete(status);   
}

VOID
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::OnStart(
    )
{
    //
    // Remove the alias
    //
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncRemoveAliasContextKernel::AliasRemoved);
    
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    NTSTATUS status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif

    _AsyncAliasOperation->StartRemoveAlias(*_Alias,
                                           this,
                                           completion);
}

VOID
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::StartRemoveAlias(
    __in KString::CSPtr& Alias,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Alias = Alias;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::RemoveAliasAsync(
    __in KString::CSPtr& Alias,                         
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{    
    _Alias = Alias;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::OnReuse(
    )
{
    _AsyncAliasOperation->Reuse();
}

VOID
KtlLogContainerKernel::AsyncRemoveAliasContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _AsyncAliasOperation->Cancel();
}

KtlLogContainerKernel::AsyncRemoveAliasContextKernel::AsyncRemoveAliasContextKernel()
{
}

KtlLogContainerKernel::AsyncRemoveAliasContextKernel::~AsyncRemoveAliasContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncRemoveAliasContext(
    __out AsyncRemoveAliasContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncRemoveAliasContextKernel::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncRemoveAliasContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _BaseLogContainer->CreateAsyncAliasOperationContext(context->_AsyncAliasOperation);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
                            
    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Resolve alias
//
VOID
KtlLogContainerKernel::AsyncResolveAliasContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
    _Alias = nullptr;
}

VOID
KtlLogContainerKernel::AsyncResolveAliasContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif  
    
    status = _LogContainer->_BaseLogContainer->LookupAlias(*_Alias, *_LogStreamId);
    if (! NT_SUCCESS(status))
    {
        if (status == STATUS_NOT_FOUND)
        {
            KDbgCheckpointWDataInformational(0,
                                "AsyncResolveAlias", status, (ULONGLONG)this, 0, 0, 0);
        } else {
            KTraceFailedAsyncRequest(status, this, 0, 0);
        }
    }
    
    Complete(status);
}

VOID
KtlLogContainerKernel::AsyncResolveAliasContextKernel::StartResolveAlias(
    __in KString::CSPtr& Alias,
    __out KtlLogStreamId* LogStreamId,                           
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Alias = Alias;
    _LogStreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncResolveAliasContextKernel::ResolveAliasAsync(
    __in KString::CSPtr & Alias,
    __out KtlLogStreamId* LogStreamId,            
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{    
    _Alias = Alias;
    _LogStreamId = LogStreamId;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncResolveAliasContextKernel::OnReuse(
    )
{
}

VOID
KtlLogContainerKernel::AsyncResolveAliasContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
}

KtlLogContainerKernel::AsyncResolveAliasContextKernel::AsyncResolveAliasContextKernel()
{
}

KtlLogContainerKernel::AsyncResolveAliasContextKernel::~AsyncResolveAliasContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncResolveAliasContext(
    __out AsyncResolveAliasContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncResolveAliasContextKernel::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncResolveAliasContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// EnumerateStreams
//
VOID
KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogContainer->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log container here so that destructor for
    // log container will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log container is cleaned up before
    // the request completes
    //
    _LogContainer = nullptr;
#endif
}

VOID KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::Completion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }   

    Complete(status);   
}

VOID
KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    OverlayLog::AsyncEnumerateStreamsContextOverlay::SPtr asyncEnumerateStreamsContext;

    status = _LogContainer->_BaseLogContainer->CreateAsyncEnumerateStreamsContext(asyncEnumerateStreamsContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);               
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::Completion); 
    asyncEnumerateStreamsContext->StartEnumerateStreams(*_LogStreamIds,
                                                        this,
                                                        completion);    
}

VOID
KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::StartEnumerateStreams(
    __out KArray<KtlLogStreamId>& LogStreamIds,                           
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _LogStreamIds = &LogStreamIds;
    
    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::EnumerateStreamsAsync(
    __out KArray<KtlLogStreamId>& LogStreamIds,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _LogStreamIds = &LogStreamIds;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::OnReuse(
    )
{
}

VOID
KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
}

KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::AsyncEnumerateStreamsContextKernel()
{
}

KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::~AsyncEnumerateStreamsContextKernel()
{
}

NTSTATUS
KtlLogContainerKernel::CreateAsyncEnumerateStreamsContext(
    __out AsyncEnumerateStreamsContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncEnumerateStreamsContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}
