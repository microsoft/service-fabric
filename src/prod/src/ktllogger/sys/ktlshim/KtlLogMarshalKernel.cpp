// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include <ktrace.h>

#include "KtlLogShimKernel.h"

#ifdef KDRIVER
#include "../driver/public.h"
#endif

RequestMarshallerKernel::RequestMarshallerKernel() :
   _ResultsArray(GetThisAllocator()),
    _LogStreamIdArray(GetThisAllocator()),
    _LogIdArray(GetThisAllocator())
{
}

RequestMarshallerKernel::~RequestMarshallerKernel()
{
}

NTSTATUS
RequestMarshallerKernel::Create(
    __out RequestMarshallerKernel::SPtr& Result,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    RequestMarshallerKernel::SPtr requestMarshaller;

    Result = nullptr;

    requestMarshaller = _new(AllocationTag, Allocator) RequestMarshallerKernel();
    if (! requestMarshaller)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
    }

    status = requestMarshaller->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    requestMarshaller->_AllocationTag = AllocationTag;
    requestMarshaller->_ExtendBuffer = FALSE;
    
    Result = Ktl::Move(requestMarshaller);
    return(status); 
}

NTSTATUS
RequestMarshallerKernel::VerifyOutBufferSize(
    __in ULONG OutBufferSizeNeeded
    )
{
    //
    // Will the output buffer be large enough ?
    //
    if (OutBufferSizeNeeded != 0)
    {
        if (OutBufferSizeNeeded > _DevIoCtl->GetOutBufferSize())
        {
            return(STATUS_BUFFER_TOO_SMALL);
        }
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::InitializeRequestFromUser(
    __in FileObjectTable::SPtr& FOT,                                           
    __out KtlLogObjectHolder& Object
    )
{
    NTSTATUS status;
    OBJECT_TYPEANDID objectTypeAndId;
    
    //
    // Grab the input bufer and prepare the marshaller for reading
    //
    _Buffer = _DevIoCtl->GetBuffer();
    _KBuffer = nullptr;
    _WriteIndex = 0;
    _ReadIndex = 0;
    _MaxIndex = _DevIoCtl->GetInBufferSize();

    //
    // Do some basic validation
    //
    _Request = (REQUEST_BUFFER*)(_DevIoCtl->GetBuffer());

    _RequestId = _Request->RequestId;
    
    status = SetReadIndex(FIELD_OFFSET(REQUEST_BUFFER, Data));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    if (! IsValidRequestVersion(_Request->Version))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }
    
    if (! IsValidRequestType(_Request->RequestType))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }
          
    objectTypeAndId = _Request->ObjectTypeAndId;
    
    if (! IsValidObjectType(objectTypeAndId.ObjectType))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);      
    }

    if (objectTypeAndId.ObjectType != NullObject)
    {
        status = FOT->LookupObject(objectTypeAndId.ObjectId,
                                   Object);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
    } else {
        KSharedBase::SPtr nullPtr = nullptr;
        Object.SetObject(NullObject,
                         0,                 // ObjectId
                         nullptr,           // ObjectPtr
                         nullPtr);          // Close Context
    }
    
    
    return(STATUS_SUCCESS);
}


NTSTATUS
RequestMarshallerKernel::WriteObject(
    __in OBJECT_TYPE ObjectType,
    __in ULONGLONG ObjectId
)
{
    NTSTATUS status;
    OBJECT_TYPEANDID objectTypeAndId;

    objectTypeAndId.ObjectType = static_cast<OBJECT_TYPE>(ObjectType);
    objectTypeAndId.ObjectId = ObjectId;
    
    status = WriteBytes((PUCHAR)&objectTypeAndId, sizeof(objectTypeAndId), sizeof(LONGLONG));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::WriteKIoBufferPreAlloc(
    __in KIoBuffer& IoBuffer,
    __in KIoBuffer& UserIoBuffer
)
{
    NTSTATUS status;

    if (IoBuffer.QuerySize() > UserIoBuffer.QuerySize())
    {
        //
        // Preallocated buffer is too small so just return the size and
        // expect caller will retry
        //
        status = WriteData(IoBuffer.QuerySize());
        return(status);
    }

    //
    // For now, just copy the data from the IoBuffer back to the
    // UserIoBuffer and build the return KIoBuffer
    //
    KIoBufferElement* ioBufferElementK;
    KIoBufferElement* ioBufferElementU;

    ioBufferElementU = UserIoBuffer.First();
    ULONG offsetU = 0;
    
    for (ioBufferElementK = IoBuffer.First();
         ioBufferElementK != NULL;
         ioBufferElementK = IoBuffer.Next(*ioBufferElementK))
    {
        if (ioBufferElementK->QuerySize() <= (ioBufferElementU->QuerySize() - offsetU))
        {
            //
            // Fit into a single IoBufferElement
            //
            KMemCpySafe(
                (PVOID)( (PUCHAR)ioBufferElementU->GetBuffer() + offsetU),
                ioBufferElementU->QuerySize() - offsetU,
                (PVOID)ioBufferElementK->GetBuffer(),
                ioBufferElementK->QuerySize());

            offsetU += ioBufferElementK->QuerySize();
        } else {
            //
            // Assume only a single element for the UserIoBuffer
            //
            KInvariant(FALSE);
        }
    }

    //
    // Write KIoBuffer information
    //
    status = WriteData(offsetU);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::ReadKIoBufferElement(
    __out KIoBufferElement::SPtr& IoBufferElement,
    __in BOOLEAN IsReadOnly
    )
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG size = 0;
    PVOID pointer;

    oldReadIndex = _ReadIndex;
    
    status = ReadData<ULONG>(size);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = ReadPVOID(pointer);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }   

#ifdef UDRIVER
    KIoBufferElement* ioBufferElementUser;
    ioBufferElementUser = (KIoBufferElement*)pointer;
#endif  
    
    status = KIoBufferElementKernel::Create(*_FOT,
                                            IsReadOnly,
#ifdef UDRIVER
                                            *ioBufferElementUser,
                                            (PVOID)ioBufferElementUser->GetBuffer(),
#endif
#ifdef KDRIVER
                                            pointer,
#endif
                                            size,
                                            IoBufferElement,
                                            GetThisAllocator(),
                                            _AllocationTag);

    if (!NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::ReadKIoBuffer(
    __out KIoBuffer::SPtr& IoBuffer,
    __in BOOLEAN IsReadOnly
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG count = 0;
    KIoBufferElement::SPtr ioBufferElement;

    oldReadIndex = _ReadIndex;
    
    //
    // ULONG - Number io buffer elements
    // Each io buffer element
    //     io buffer element size
    //     io buffer element pointer
    //

    status = KIoBuffer::CreateEmpty(IoBuffer,
                                    GetThisAllocator(),
                                    _AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    status = ReadData<ULONG>(count);
    if (! NT_SUCCESS(status))
    {
        IoBuffer = nullptr;
        return(status);
    }

    for (ULONG i = 0; i < count; i++)
    {
        status = ReadKIoBufferElement(ioBufferElement, IsReadOnly);
        
        if (! NT_SUCCESS(status))
        {
            IoBuffer = nullptr;
            _ReadIndex = oldReadIndex;
            return(status);
        }

        IoBuffer->AddIoBufferElement(*ioBufferElement);
        
        ioBufferElement = nullptr;
    }
    
    return(STATUS_SUCCESS);
}


VOID
RequestMarshallerKernel::FormatResponseToUser(
    )
{
    NTSTATUS status;

    _MaxIndex = _DevIoCtl->GetOutBufferSize();
    if (_MaxIndex != 0)
    {
        status = SetWriteIndex(FIELD_OFFSET(REQUEST_BUFFER, Data));

        //
        // Caller is expected to verify the outbuffer is large enough
        //
        KInvariant(NT_SUCCESS(status));
    }
}

VOID
RequestMarshallerKernel::FormatCreateObjectResponseToUser(
    __in OBJECT_TYPE ObjectType,
    __in ULONGLONG ObjectId
    )
{
    NTSTATUS status;

    status = WriteObject(ObjectType, ObjectId);
    //
    // Caller is expected to verify the outbuffer is large enough
    //
    KInvariant(NT_SUCCESS(status));
}

VOID
RequestMarshallerKernel::CreateLogManagerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase& Async,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    UNREFERENCED_PARAMETER(Async);
    
    NTSTATUS status;
    ULONGLONG objectId;

    status = Status;
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(Status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    objectId = _FOT->GetNextObjectIdIndex();
    _LogManager->SetObjectId(objectId);
    
    KSharedBase::SPtr objectPtr = up_cast<KSharedBase>(_LogManager);
    _FOT->AddObject(*objectPtr,
                    NULL,                           // No auto-close context
                    *_FileObjectEntry,
                    RequestMarshaller::LogManager,
                    objectId);

    // TODO: REMOVE AFTER DEBUGGING
    _LogManager->_FileObjectTable = _FOT.RawPtr();
    _LogManager->_CleanupCalled = FALSE;
    // TODO: REMOVE AFTER DEBUGGING
    
    FormatCreateObjectResponseToUser(_LogManager->GetObjectType(),
                                     _LogManager->GetObjectId());
    
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::CreateLogManagerCancel(
    )
{
    // StartLogManager will find its way to completion
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::CreateLogManagerMethod(
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::SPtr logManager;

    KInvariant(_ObjectTypeAndId.ObjectType == LogManager);

    status = VerifyOutBufferSize(RequestMarshaller::ObjectReturnBufferSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(RequestMarshaller::ObjectReturnBufferSize);
        return(status);
    }
    
    logManager = _new (_AllocationTag, GetThisAllocator()) KtlLogManagerKernel(*(_FOT->GetOverlayManager()));
    if (logManager == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = logManager->Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }           

    status = FileObjectEntry::Create(GetThisAllocator(),
                                     GetThisAllocationTag(),
                                     _FileObjectEntry);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }           
    
    _LogManager = Ktl::Move(logManager);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::CreateLogManagerCancel);
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        _LogManager = nullptr;
        return(status);
    }
    
    KAsyncServiceBase::OpenCompletionCallback completion(this, &RequestMarshallerKernel::CreateLogManagerCompletion);
    status = _LogManager->StartOpenLogManager(nullptr,
                                             completion);
    
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        _LogManager = nullptr;
        return(status);
    }

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::CreateObjectRequest(
    )
{
    NTSTATUS status;
    KtlLogManagerKernel::SPtr logManager;

    //
    // Verify that there is enough output buffer to return a a full
    // request buffer structure. This is the minimum needed in order to
    // return resize information. 
    //
    status = VerifyOutBufferSize(BaseReturnBufferSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(BaseReturnBufferSize);
        return(status);
    }
    
    status = ReadObjectTypeAndId(_ObjectTypeAndId);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    if (_ObjectTypeAndId.ObjectType != LogManager)
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Processing will continue at CreateLogManagerMethod()
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::CreateLogManagerMethod);
        
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::DeleteObjectMethod(
    )
{
    NTSTATUS status;

	KDbgCheckpointWData(0, "DeleteObjectMethod", STATUS_SUCCESS,
						(ULONGLONG)_FOT.RawPtr(),
						(ULONGLONG)_ParentObject.GetObjectId(),
						(ULONGLONG)this,
						(ULONGLONG)0);
				
    status = _FOT->RemoveObject(_ParentObject.GetObjectId());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    
    return(status);
}

NTSTATUS
RequestMarshallerKernel::DeleteObjectRequest(
    )
{
    //
    // Processing will continue at DeleteObjectMethod()
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::DeleteObjectMethod);

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::SingleContextCancel(
    )
{
    _Context->Cancel();
    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::CreateLogContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    ULONGLONG objectId;

    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    //
    // Extract auto close context from object itself. Note that a side
    // effect is to null the SPtr for the auto close context in the
    // object.
    //
    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr closeContext;
    closeContext = Ktl::Move(_LogContainer->GetAutoCloseContext());
    closeContext->BindToParent(*_LogContainer);
        
    objectId = _FOT->GetNextObjectIdIndex();
    _LogContainer->SetObjectId(objectId);

    KSharedBase::SPtr objectPtr = up_cast<KSharedBase>(_LogContainer);
    KSharedBase::SPtr autoCloseContext = up_cast<KSharedBase>(closeContext);
    _FOT->AddObject(*objectPtr,
                    autoCloseContext.RawPtr(),
                    *_FileObjectEntry,
                    RequestMarshaller::LogContainer,
                    objectId);
        
    FormatCreateObjectResponseToUser(_LogContainer->GetObjectType(),
                                     _LogContainer->GetObjectId());

    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::CreateLogContainerMethod(
    )
{
    NTSTATUS status;
    KtlLogManager::AsyncCreateLogContainer::SPtr context;
    
    status = VerifyOutBufferSize(RequestMarshaller::ObjectReturnBufferSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(RequestMarshaller::ObjectReturnBufferSize);
        return(status);
    }
    
    status = _LogManager->CreateAsyncCreateLogContainerContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = FileObjectEntry::Create(GetThisAllocator(),
                                     GetThisAllocationTag(),
                                     _FileObjectEntry);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }           
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::CreateLogContainerCompletion);

    if (_Path)
    {
        context->StartCreateLogContainer(*_Path,
                                         _LogContainerId,
                                         _LogSize,
                                         _MaximumNumberStreams,
                                         _MaximumRecordSize,
                                         _Flags,
                                         up_cast<KtlLogContainer>(_LogContainer),
                                         nullptr,         // Parent
                                         completion);
    } else {
        context->StartCreateLogContainer(_DiskId,
                                         _LogContainerId,
                                         _LogSize,
                                         _MaximumNumberStreams,
                                         _MaximumRecordSize,
                                         _Flags,
                                         up_cast<KtlLogContainer>(_LogContainer),
                                         nullptr,         // Parent
                                         completion);
    }

    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::CreateLogContainerMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;

    status = ReadGuid(_DiskId);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadString(_Path);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogContainerId = guid;

    status = ReadData<LONGLONG>(_LogSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<ULONG>(_MaximumNumberStreams);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<ULONG>(_MaximumRecordSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<DWORD>(_Flags);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Continues at CreateLogContainerMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::CreateLogContainerMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::OpenLogContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    ULONGLONG objectId;

    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    
    //
    // Extract auto close context from object itself. Note that a side
    // effect is to null the SPtr for the auto close context in the
    // object.
    //
    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr closeContext;
    closeContext = Ktl::Move(_LogContainer->GetAutoCloseContext());
    closeContext->BindToParent(*_LogContainer);
        
    objectId = _FOT->GetNextObjectIdIndex();
    _LogContainer->SetObjectId(objectId);

    KSharedBase::SPtr objectPtr = up_cast<KSharedBase>(_LogContainer);
    KSharedBase::SPtr autoCloseContext = up_cast<KSharedBase>(closeContext);
    _FOT->AddObject(*objectPtr,
                    autoCloseContext.RawPtr(),
                    *_FileObjectEntry,
                    RequestMarshaller::LogContainer,
                    objectId);
    
    FormatCreateObjectResponseToUser(_LogContainer->GetObjectType(),
                                     _LogContainer->GetObjectId());
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::OpenLogContainerMethod(
    )
{
    NTSTATUS status;
    
    KtlLogManager::AsyncOpenLogContainer::SPtr context;
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::OpenLogContainerCompletion);

    status = VerifyOutBufferSize(RequestMarshaller::ObjectReturnBufferSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(RequestMarshaller::ObjectReturnBufferSize);
        return(status);
    }
    
    status = _LogManager->CreateAsyncOpenLogContainerContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = FileObjectEntry::Create(GetThisAllocator(),
                                     GetThisAllocationTag(),
                                     _FileObjectEntry);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }           
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    if (_Path)
    {
        context->StartOpenLogContainer(*_Path,
                                       _LogContainerId,
                                       up_cast<KtlLogContainer>(_LogContainer),
                                       nullptr,         // Parent
                                       completion);
    } else {
        context->StartOpenLogContainer(_DiskId,
                                       _LogContainerId,
                                       up_cast<KtlLogContainer>(_LogContainer),
                                       nullptr,         // Parent
                                       completion);
    }
    
    return(STATUS_PENDING); 
}

NTSTATUS
RequestMarshallerKernel::OpenLogContainerMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;
    
    status = ReadGuid(_DiskId);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadString(_Path);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogContainerId = guid;

    //
    // Continues at CreateLogContainerMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::OpenLogContainerMethod);
    
    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::DeleteLogContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::DeleteLogContainerMethod(
    )
{
    NTSTATUS status;
    
    KtlLogManager::AsyncDeleteLogContainer::SPtr context;
    
    status = _LogManager->CreateAsyncDeleteLogContainerContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::DeleteLogContainerCompletion);

    if (_Path)
    {
        context->StartDeleteLogContainer(*_Path,
                                         _LogContainerId,
                                         nullptr,         // Parent
                                         completion);
    } else {
        context->StartDeleteLogContainer(_DiskId,
                                         _LogContainerId,
                                         nullptr,         // Parent
                                         completion);
    }
    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::DeleteLogContainerMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;

    status = ReadGuid(_DiskId);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadString(_Path);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogContainerId = guid;

    //
    // Continues at DeleteLogContainerMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::DeleteLogContainerMethod);
    
    return(STATUS_SUCCESS);            
}

VOID
RequestMarshallerKernel::EnumerateLogContainersCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();

    if (NT_SUCCESS(status))
    {
        status = WriteKtlLogContainerIdArray(_LogIdArray);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        }
    } else {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::EnumerateLogContainersMethod(
    )
{
    NTSTATUS status;
    
    KtlLogManager::AsyncEnumerateLogContainers::SPtr context;
    
    status = _LogManager->CreateAsyncEnumerateLogContainersContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::EnumerateLogContainersCompletion);

    context->StartEnumerateLogContainers(_DiskId,
                                         _LogIdArray,
                                         nullptr,         // Parent
                                         completion);
    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::EnumerateLogContainersMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    _DiskId = guid;

    //
    // Continues at EnumerateLogContainersMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::EnumerateLogContainersMethod);
    
    return(STATUS_SUCCESS);            
}

VOID
RequestMarshallerKernel::ConfigureLogManagerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteData<ULONG>(_Result);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;             
    }
    
    status = WriteKBuffer(_OutBuffer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;             
    }

    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::ConfigureLogManagerMethod(
    )
{
    NTSTATUS status;
    KtlLogManager::AsyncConfigureContext::SPtr context;

    status = _LogManager->CreateAsyncConfigureContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed a this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::ConfigureLogManagerCompletion);
    context->StartConfigure(_Code,
                        _InBuffer.RawPtr(),
                        _Result,
                        _OutBuffer,
                        nullptr,         // Parent
                        completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::ConfigureLogManagerMarshall(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;

    outSizeNeeded = BaseReturnBufferSize + AlignmentNeeded(BaseReturnBufferSize, sizeof(ULONG)) + 2*sizeof(ULONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }

    status = ReadData<ULONG>(_Code);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }            
    
    status = ReadKBuffer(_InBuffer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }            
    
    //
    // Continues at RequestMarshallerKernel::ConfigureMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::ConfigureLogManagerMethod);

    return(STATUS_SUCCESS);
}
   
NTSTATUS
RequestMarshallerKernel::LogManagerObjectMethodRequest(
    __in OBJECT_METHOD ObjectMethod
    )
{
    NTSTATUS status;

    KInvariant(_ParentObject.GetObjectType() == LogManager);
    _LogManager = static_cast<KtlLogManagerKernel*>(_ParentObject.GetLogManager());
    
    switch(ObjectMethod)
    {
        case CreateLogContainer:
        {
            status = CreateLogContainerMarshall();            
            break;
        }

        case OpenLogContainer:
        {
            status = OpenLogContainerMarshall();
            break;
            
        }
        
        case DeleteLogContainer:
        {
            status = DeleteLogContainerMarshall();
            break;
        }
        
        case EnumerateLogContainers:
        {
            status = EnumerateLogContainersMarshall();
            break;
        }

        case ConfigureLogManager:
        {
            status = ConfigureLogManagerMarshall();
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }
    
    return(status);
}

VOID
RequestMarshallerKernel::CreateLogStreamCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    ULONGLONG objectId;

    KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncCreateLogStreamContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    //
    // Extract auto close context from object itself. Note that a side
    // effect is to null the SPtr for the auto close context in the
    // object.
    //
    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr closeContext;
    closeContext = Ktl::Move(_LogStream->GetAutoCloseContext());
    closeContext->BindToParent(*_LogStream);
        
    objectId = _FOT->GetNextObjectIdIndex();
    _LogStream->SetObjectId(objectId);

    KSharedBase::SPtr objectPtr = up_cast<KSharedBase>(_LogStream);
    KSharedBase::SPtr autoCloseContext = up_cast<KSharedBase>(closeContext);
    _FOT->AddObject(*objectPtr,
                    autoCloseContext.RawPtr(),
                    *_FileObjectEntry,
                    RequestMarshaller::LogStream,
                    objectId);
    
    FormatCreateObjectResponseToUser(_LogStream->GetObjectType(),
                                     _LogStream->GetObjectId());

    status = WriteData(_LogStream->_CoreLoggerMetadataOverhead);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif  
    
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::CreateLogStreamMethod()
{
    NTSTATUS status;
    ULONG outSizeNeeded;
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::CreateLogStreamCompletion);
    
    outSizeNeeded = RequestMarshaller::ObjectReturnBufferSize +
                                 AlignmentNeeded(RequestMarshaller::ObjectReturnBufferSize, sizeof(ULONG)) +
                                 sizeof(ULONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }
    
    status = _LogContainer->CreateAsyncCreateLogStreamContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = FileObjectEntry::Create(GetThisAllocator(),
                                     GetThisAllocationTag(),
                                     _FileObjectEntry);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }           
    
    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncCreateLogStreamContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncCreateLogStreamContextKernel, KtlLogContainer::AsyncCreateLogStreamContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    context->StartCreateLogStream(_LogStreamId,
                                  _LogStreamType,
                                  _Alias,
                                  _Path,
                                  _SecurityDescriptor,
                                  _MaximumMetaDataLength,
                                  _MaximumStreamSize,
                                  _MaximumRecordSize,
                                  _Flags,
                                  up_cast<KtlLogStream>(_LogStream),
                                  nullptr,         // Parent
                                  completion);
    return(STATUS_PENDING);            
}

NTSTATUS
RequestMarshallerKernel::CreateLogStreamMarshall()
{
    NTSTATUS status;
    KGuid guid;

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogStreamId = guid;

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogStreamType = guid;

    status = ReadData<ULONG>(_MaximumMetaDataLength);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<LONGLONG>(_MaximumStreamSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<ULONG>(_MaximumRecordSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<DWORD>(_Flags);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    status = ReadString(_Path);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadKBuffer(_SecurityDescriptor);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    //
    // Continues at CreateLogStreamMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::CreateLogStreamMethod);    
    
    return(STATUS_SUCCESS);
}


VOID
RequestMarshallerKernel::DeleteLogStreamCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::DeleteLogStreamMethod(
    )
{
    NTSTATUS status;
    
    KtlLogContainer::AsyncDeleteLogStreamContext::SPtr context;

    status = _LogContainer->CreateAsyncDeleteLogStreamContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel, KtlLogContainer::AsyncDeleteLogStreamContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
        
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::DeleteLogStreamCompletion);
    context->StartDeleteLogStream(_LogStreamId,
                                  nullptr,         // Parent
                                  completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::DeleteLogStreamMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;            

    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    _LogStreamId = guid;

    //
    // Continues at DeleteLogStreamMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::DeleteLogStreamMethod);
    
    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::OpenLogStreamCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    ULONGLONG objectId;
    
    KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncOpenLogStreamContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    //
    // Extract auto close context from object itself. Note that a side
    // effect is to null the SPtr for the auto close context in the
    // object.
    //
    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr closeContext;
    closeContext = Ktl::Move(_LogStream->GetAutoCloseContext());
    closeContext->BindToParent(*_LogStream);
            
    objectId = _FOT->GetNextObjectIdIndex();
    _LogStream->SetObjectId(objectId);

    KSharedBase::SPtr objectPtr = up_cast<KSharedBase>(_LogStream);
    KSharedBase::SPtr autoClosePtr = up_cast<KSharedBase>(closeContext);
    _FOT->AddObject(*objectPtr,
                    autoClosePtr.RawPtr(),
                    *_FileObjectEntry,
                    RequestMarshaller::LogStream,
                    objectId);
    
    FormatCreateObjectResponseToUser(_LogStream->GetObjectType(),
                                     _LogStream->GetObjectId());
        
    status = WriteData(_MaximumMetaDataLength);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    status = WriteData(_LogStream->_CoreLoggerMetadataOverhead);
#if DBG
    KAssert(NT_SUCCESS(status));
#endif  
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::OpenLogStreamMethod(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;
    KtlLogContainer::AsyncOpenLogStreamContext::SPtr context;

    outSizeNeeded = RequestMarshaller::ObjectReturnBufferSize +
                                 AlignmentNeeded(RequestMarshaller::ObjectReturnBufferSize, sizeof(ULONG)) +
                                 2*sizeof(ULONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }
    
    status = _LogContainer->CreateAsyncOpenLogStreamContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = FileObjectEntry::Create(GetThisAllocator(),
                                     GetThisAllocationTag(),
                                     _FileObjectEntry);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }           
    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncOpenLogStreamContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncOpenLogStreamContextKernel, KtlLogContainer::AsyncOpenLogStreamContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
        
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::OpenLogStreamCompletion);
    context->StartOpenLogStream(_LogStreamId,
                                  &_MaximumMetaDataLength,
                                  up_cast<KtlLogStream>(_LogStream),
                                  nullptr,         // Parent
                                  completion);

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::OpenLogStreamMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;
    
    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogStreamId = guid;

    //
    // Continues at OpenLogStreamMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::OpenLogStreamMethod);
    
    return(STATUS_SUCCESS);
}


VOID
RequestMarshallerKernel::AssignAliasCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    KtlLogContainerKernel::AsyncAssignAliasContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncAssignAliasContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::AssignAliasMethod(
    )
{
    NTSTATUS status;
    KtlLogContainer::AsyncAssignAliasContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::AssignAliasCompletion);

    status = _LogContainer->CreateAsyncAssignAliasContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncAssignAliasContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncAssignAliasContextKernel, KtlLogContainer::AsyncAssignAliasContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object may be closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
        
    context->StartAssignAlias(_Alias,
                              _LogStreamId,
                              nullptr,         // Parent
                              completion);

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::AssignAliasMarshall(
    )
{
    NTSTATUS status;
    KGuid guid;
        
    status = ReadGuid(guid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    _LogStreamId = guid;

    status = ReadString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Continues at AssignAliasMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::AssignAliasMethod);
    
    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::RemoveAliasCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    KtlLogContainerKernel::AsyncRemoveAliasContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncRemoveAliasContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::RemoveAliasMethod(
    )
{
    NTSTATUS status;
    KtlLogContainer::AsyncRemoveAliasContext::SPtr context; 
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::RemoveAliasCompletion);
    
    status = _LogContainer->CreateAsyncRemoveAliasContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncRemoveAliasContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncRemoveAliasContextKernel, KtlLogContainer::AsyncRemoveAliasContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File object is closing at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
        
    context->StartRemoveAlias(_Alias,
                              nullptr,         // Parent
                              completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::RemoveAliasMarshall(
    )
{
    NTSTATUS status;
    
    status = ReadString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
            
    //
    // Continues at RemoveAliasMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::RemoveAliasMethod);

    return(STATUS_SUCCESS);
}


VOID
RequestMarshallerKernel::ResolveAliasCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    KtlLogContainerKernel::AsyncResolveAliasContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncResolveAliasContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        CompleteRequest(status);
        return;
    }

    status = WriteKtlLogStreamId(_LogStreamId);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::ResolveAliasMethod(
    )
{
    NTSTATUS status;
    KtlLogContainer::AsyncResolveAliasContext::SPtr context;
    ULONG outSizeNeeded = RequestMarshaller::BaseReturnBufferSize +
                                 AlignmentNeeded(RequestMarshaller::ObjectReturnBufferSize, sizeof(ULONG)) +
                                 sizeof(KtlLogStreamId);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }
            
    status = _LogContainer->CreateAsyncResolveAliasContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncResolveAliasContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncResolveAliasContextKernel, KtlLogContainer::AsyncResolveAliasContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
        
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::ResolveAliasCompletion);
    context->StartResolveAlias(_Alias,
                               &_LogStreamId,
                               nullptr,         // Parent
                               completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::ResolveAliasMarshall(
    )
{
    NTSTATUS status;
    
    status = ReadString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }            
            
    //
    // Continues at ResolveAliasMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::ResolveAliasMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::EnumerateStreamsCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel*>(&Async);

    BOOLEAN b = _LogContainer->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
        
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteKtlLogStreamIdArray(_LogStreamIdArray);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::EnumerateStreamsMethod(
    )
{
    NTSTATUS status;
    KtlLogContainer::AsyncEnumerateStreamsContext::SPtr context;
            
    status = _LogContainer->CreateAsyncEnumerateStreamsContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel, KtlLogContainer::AsyncEnumerateStreamsContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogContainer->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or container closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
        
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::EnumerateStreamsCompletion);
    context->StartEnumerateStreams(
                               _LogStreamIdArray,
                               nullptr,         // Parent
                               completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::EnumerateStreamsMarshall(
    )
{    
    //
    // Continues at EnumerateStreamsMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::EnumerateStreamsMethod);

    return(STATUS_SUCCESS);
}


VOID
RequestMarshallerKernel::CloseContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    } else {
        //
        // If DeleteObjectMethod fails then it isn't really a problem
        // as all it means is that the object wasn't found in the FOT.
        //
        NTSTATUS statusDontCare;
        
        statusDontCare = DeleteObjectMethod();
        UNREFERENCED_PARAMETER(statusDontCare);
    }
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::CloseContainerMethod(
    )
{
    NTSTATUS status;
    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr context;           
    
    status = _LogContainer->CreateAsyncCloseContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    context->BindToParent(*_LogContainer);

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::CloseContainerCompletion);
    context->StartClose(   nullptr,         // Parent
                           completion);
    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::CloseContainerMarshall(
    )
{
    //
    // Continues at RequestMarshallerKernel::CloseContainerMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::CloseContainerMethod);

    return(STATUS_SUCCESS);
}



NTSTATUS
RequestMarshallerKernel::LogContainerObjectMethodRequest(
    __in OBJECT_METHOD ObjectMethod
    )
{
    UNREFERENCED_PARAMETER(ObjectMethod);
    
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    KInvariant(_ParentObject.GetObjectType() == LogContainer);
    _LogContainer = static_cast<KtlLogContainerKernel*>(_ParentObject.GetLogContainer());

    switch(ObjectMethod)
    {
        case CreateLogStream:
        {
            status = CreateLogStreamMarshall();
            break;
        }
        
        case DeleteLogStream:
        {
            status = DeleteLogStreamMarshall();
            break;
        }

        case OpenLogStream:
        {
            status = OpenLogStreamMarshall();
            break;
        }

        case AssignAlias:
        {
            status = AssignAliasMarshall();
            break;          
        }
        
        case RemoveAlias:
        {
            status = RemoveAliasMarshall();
            break;          
        }
        
        case ResolveAlias:
        {
            status = ResolveAliasMarshall();
            break;
        }
               
        case EnumerateStreams:
        {
            status = EnumerateStreamsMarshall();
            break;
        }
               
        case CloseContainer:
        {
            status = CloseContainerMarshall();
            break;
        }
               
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }
    
    return(status);
}

VOID
RequestMarshallerKernel::QueryRecordRangeCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteData(_LowestAsn);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    status = WriteData(_HighestAsn);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    status = WriteData(_LogTruncationAsn);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::QueryRecordRangeMethod(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;

    outSizeNeeded = BaseReturnBufferSize;
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(KtlLogAsn)) + sizeof(KtlLogAsn);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(KtlLogAsn)) + sizeof(KtlLogAsn);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(KtlLogAsn)) + sizeof(KtlLogAsn); 
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }

    KtlLogStream::AsyncQueryRecordRangeContext::SPtr context;
    status = _LogStream->CreateAsyncQueryRecordRangeContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::QueryRecordRangeCompletion);
    context->StartQueryRecordRange(&_LowestAsn,
                                   &_HighestAsn,
                                   &_LogTruncationAsn,
                                   nullptr,         // Parent
                                   completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::QueryRecordRangeMarshall(
    )
{   
    //
    // Continues at RequestMarshallerKernel::QueryRecordRangeMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::QueryRecordRangeMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::QueryRecordCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncQueryRecordContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncQueryRecordContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteData(_Version);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    status = WriteData(_Disposition);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    status = WriteData(_IoBufferSize);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    status = WriteData(_DebugInfo);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
            
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::QueryRecordMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncQueryRecordContext::SPtr context;
    ULONG outSizeNeeded;

    outSizeNeeded = BaseReturnBufferSize;
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONGLONG)) + sizeof(ULONGLONG);
#ifdef USE_RVDLOGGER_OBJECTS    
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(RvdLogStream::RecordDisposition)) + sizeof(RvdLogStream::RecordDisposition);
#else
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(KtlLogStream::RecordDisposition)) + sizeof(KtlLogStream::RecordDisposition);
#endif
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONG)) + sizeof(ULONG);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONGLONG)) + sizeof(ULONGLONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }
    
    status = _LogStream->CreateAsyncQueryRecordContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncQueryRecordContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncQueryRecordContextKernel>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::QueryRecordCompletion);
    context->StartQueryRecord(_Asn,
                              &_Version,
                              &_Disposition,
                              &_IoBufferSize,
                              &_DebugInfo,
                              nullptr,         // Parent
                              completion);

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::QueryRecordMarshall(
    )
{
    NTSTATUS status;

    status = ReadData<KtlLogAsn>(_Asn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    //
    // Continues at RequestMarshallerKernel::QueryRecordMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::QueryRecordMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::QueryRecordsCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncQueryRecordsContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncQueryRecordsContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteRecordMetadataArray(_ResultsArray);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif
    
    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::QueryRecordsMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncQueryRecordsContext::SPtr context;

    //
    // We do not check the out buffer size here since we will not know
    // how large it will eventually be. So the actual check will be
    // when we marshall back into the output buffer itself
    //

    // TODO: Add a unit test case where a huge records array is returned
    
    status = _LogStream->CreateAsyncQueryRecordsContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncQueryRecordsContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncQueryRecordsContextKernel>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::QueryRecordsCompletion);
    context->StartQueryRecords(_LowestAsn,
                               _HighestAsn,
                               _ResultsArray,
                               nullptr,         // Parent
                               completion);

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::QueryRecordsMarshall(
    )
{
    NTSTATUS status;
    
    status = ReadData<KtlLogAsn>(_LowestAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<KtlLogAsn>(_HighestAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
                
    //
    // Continues at RequestMarshallerKernel::QueryRecordsMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::QueryRecordsMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::WriteCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    KtlLogStreamKernel::AsyncWriteContextKernel::SPtr contextKernel;

    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncWriteContextKernel*>(&Async);
    
    NTSTATUS status;

    status = Async.Status();

    KFinally([&status, contextKernel, this] 
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
        KInvariant(b);

        CompleteRequest(status);
    });

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, GetRequestId(), _ParentObject.GetObjectId());
        return;
    }

    //
    // Marshall out the log usage info if it was requested
    //
    if (_ObjectMethod == WriteAndReturnLogUsage)
    {
        status = WriteData<ULONGLONG>(_CurrentLogSize);
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
            return;
        }

        status = WriteData<ULONGLONG>(_CurrentLogSpaceRemaining);
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
            return;
        }
    }
}

NTSTATUS
RequestMarshallerKernel::WriteMethod(
    )
{
    NTSTATUS status;            
    KtlLogStream::AsyncWriteContext::SPtr context;
    KtlLogStreamKernel::AsyncWriteContextKernel::SPtr contextKernel;
    
    status = _LogStream->CreateAsyncWriteContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    contextKernel = down_cast<KtlLogStreamKernel::AsyncWriteContextKernel>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::WriteCompletion);
    context->StartWrite(_RecordAsn,
                        _Version,
                        _MetaDataLength,
                        _MetaDataBuffer,
                        _IoBuffer,
                        _ReservationSpace,
                        _CurrentLogSize,
                        _CurrentLogSpaceRemaining,
                        nullptr,         // Parent
                        completion);

    return(STATUS_PENDING);
}


NTSTATUS
RequestMarshallerKernel::WriteMarshall(
    )
{
    NTSTATUS status;    

    status = ReadData<KtlLogAsn>(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    status = ReadData<ULONGLONG>(_Version);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    status = ReadData<ULONG>(_MetaDataLength);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    status = ReadData<ULONGLONG>(_ReservationSpace);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    status = ReadKIoBuffer(_MetaDataBuffer,
                           TRUE);           // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    status = ReadKIoBuffer(_IoBuffer,
                           TRUE);           // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    //
    // Continues at RequestMarshallerKernel::WriteMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::WriteMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::ReservationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    
    KtlLogStreamKernel::AsyncReservationContextKernel::SPtr contextKernel;

    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncReservationContextKernel*>(&Async);
    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::ReservationMethod(
    )
{
    NTSTATUS status;            
    KtlLogStream::AsyncReservationContext::SPtr context;
    
    status = _LogStream->CreateAsyncReservationContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncReservationContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncReservationContextKernel, KtlLogStream::AsyncReservationContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::ReservationCompletion);
    contextKernel->StartUpdateReservation(_ReservationDelta,
                                          nullptr,         // Parent
                                          completion);

    return(STATUS_PENDING);
}


NTSTATUS
RequestMarshallerKernel::ReservationMarshall(
    )
{
    NTSTATUS status;    

    status = ReadData<LONGLONG>(_ReservationDelta);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Continues at RequestMarshallerKernel::WriteMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::ReservationMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::ReadCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncReadContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncReadContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteData(_ActualAsn);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    status = WriteData(_Version);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    status = WriteData(_MetaDataLength);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    status = WriteKIoBufferPreAlloc(*_MetaDataBuffer,
                                    *_PreAllocMetaDataBuffer);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    status = WriteKIoBufferPreAlloc(*_IoBuffer,
                                    *_PreAllocIoBuffer);
#if DBG
    KAssert(NT_SUCCESS(status));
#else
    UNREFERENCED_PARAMETER(status);
#endif

    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::ReadMethod(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;
    KtlLogStream::AsyncReadContext::SPtr context;

    outSizeNeeded = BaseReturnBufferSize;
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(KtlLogAsn)) + sizeof(KtlLogAsn);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONGLONG)) + sizeof(ULONGLONG);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONG)) + sizeof(ULONG);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONG)) + sizeof(ULONG);
    outSizeNeeded += AlignmentNeeded(outSizeNeeded, sizeof(ULONG)) + sizeof(ULONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }

    status = _LogStream->CreateAsyncReadContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncReadContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncReadContextKernel, KtlLogStream::AsyncReadContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::ReadCompletion);

    switch(_ReadType)
    {
        case RvdLogStream::AsyncReadContext::ReadExactRecord:
        {
            _ActualAsn = KtlLogAsn::Null();
            context->StartRead(_RecordAsn,
                               &_Version,
                               _MetaDataLength,
                               _MetaDataBuffer,
                               _IoBuffer,
                               nullptr,         // Parent
                               completion);
            break;
        }

        case RvdLogStream::AsyncReadContext::ReadPreviousRecord:
        {
            context->StartReadPrevious(_RecordAsn,
                                       &_ActualAsn,
                                       &_Version,
                                       _MetaDataLength,
                                       _MetaDataBuffer,
                                       _IoBuffer,
                                       nullptr,         // Parent
                                       completion);
            break;
        }

        case RvdLogStream::AsyncReadContext::ReadNextRecord:
        {
            context->StartReadNext(_RecordAsn,
                                       &_ActualAsn,
                                       &_Version,
                                       _MetaDataLength,
                                       _MetaDataBuffer,
                                       _IoBuffer,
                                       nullptr,         // Parent
                                       completion);
            break;
        }

        case RvdLogStream::AsyncReadContext::ReadContainingRecord:
        {
            context->StartReadContaining(_RecordAsn,
                                       &_ActualAsn,
                                       &_Version,
                                       _MetaDataLength,
                                       _MetaDataBuffer,
                                       _IoBuffer,
                                       nullptr,         // Parent
                                       completion);
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
            return(status);
        }
    }           

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::ReadMarshall(
    )
{
    NTSTATUS status;
    
    status = ReadReadType(_ReadType);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<KtlLogAsn>(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadKIoBuffer(_PreAllocMetaDataBuffer,
                           FALSE);                     // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadKIoBuffer(_PreAllocIoBuffer,
                           FALSE);                      // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
            
    //
    // Continues at RequestMarshallerKernel::ReadMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::ReadMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::MultiRecordReadCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncMultiRecordReadContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        CompleteRequest(status);
        return;
    }

    //
    // Data was read directly into buffers passed from user
    //

    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::MultiRecordReadMethod(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;
    KtlLogStream::AsyncMultiRecordReadContext::SPtr context;

    outSizeNeeded = BaseReturnBufferSize;
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }

    status = _LogStream->CreateAsyncMultiRecordReadContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncMultiRecordReadContextKernel, KtlLogStream::AsyncMultiRecordReadContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::MultiRecordReadCompletion);

    context->StartRead(_RecordAsn,
                       *_MetaDataBuffer,
                       *_IoBuffer,
                       nullptr,         // Parent
                       completion);

    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::MultiRecordReadMarshall(
    )
{
    NTSTATUS status;
    
    status = ReadData<KtlLogAsn>(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadKIoBuffer(_MetaDataBuffer,
                           FALSE);                     // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadKIoBuffer(_IoBuffer,
                           FALSE);                      // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
            
    //
    // Continues at RequestMarshallerKernel::MultiRecordReadMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::MultiRecordReadMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::TruncateCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }

    KtlLogStreamKernel::AsyncTruncateContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncTruncateContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::TruncateMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncTruncateContext::SPtr context;           
    KtlLogStreamKernel::AsyncTruncateContextKernel::SPtr contextKernel;           
    
    status = _LogStream->CreateAsyncTruncateContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    contextKernel = down_cast<KtlLogStreamKernel::AsyncTruncateContextKernel>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::TruncateCompletion);
    contextKernel->StartTruncate(_TruncateAsn,
                                 _PreferredTruncateAsn,
                                 nullptr,         // Parent
                                 completion);
    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::TruncateMarshall(
    )
{
    NTSTATUS status;
                
    status = ReadData<KtlLogAsn>(_TruncateAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<KtlLogAsn>(_PreferredTruncateAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Continues at RequestMarshallerKernel::TruncateMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::TruncateMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::WriteMetadataCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
            
    KtlLogStreamKernel::AsyncWriteMetadataContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncWriteMetadataContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::WriteMetadataMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncWriteMetadataContext::SPtr context;

    status = _LogStream->CreateAsyncWriteMetadataContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncWriteMetadataContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncWriteMetadataContextKernel, KtlLogStream::AsyncWriteMetadataContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle has been closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::WriteMetadataCompletion);
    context->StartWriteMetadata(_MetaDataBuffer,
                                nullptr,         // Parent
                                completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::WriteMetadataMarshall(
    )
{
    NTSTATUS status;
    
    status = ReadKIoBuffer(_MetaDataBuffer,
                           TRUE);              // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Continues at RequestMarshallerKernel::WriteMetadataMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::WriteMetadataMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::ReadMetadataCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncReadMetadataContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncReadMetadataContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }
            
    status = WriteKIoBufferPreAlloc(*_MetaDataBuffer,
                                    *_PreAllocMetaDataBuffer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;             
    }

    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::ReadMetadataMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReadMetadataContext::SPtr context;

    status = _LogStream->CreateAsyncReadMetadataContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncReadMetadataContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncReadMetadataContextKernel, KtlLogStream::AsyncReadMetadataContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed a this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::ReadMetadataCompletion);
    context->StartReadMetadata(_MetaDataBuffer,
                               nullptr,         // Parent
                               completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::ReadMetadataMarshall(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;

    outSizeNeeded = BaseReturnBufferSize + AlignmentNeeded(BaseReturnBufferSize, sizeof(ULONG)) + sizeof(ULONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }

    status = ReadKIoBuffer(_PreAllocMetaDataBuffer,
                           FALSE);                  // IsReadOnly
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }            
    
    //
    // Continues at RequestMarshallerKernel::ReadMetadataMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::ReadMetadataMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::IoctlCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    KtlLogStreamKernel::AsyncIoctlContextKernel::SPtr contextKernel;
    contextKernel = reinterpret_cast<KtlLogStreamKernel::AsyncIoctlContextKernel*>(&Async);

    BOOLEAN b = _LogStream->RemoveActiveContext(contextKernel->_ActiveContext);
    KInvariant(b);
    
    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;
    }

    status = WriteData<ULONG>(_Result);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;             
    }
    
    status = WriteKBuffer(_OutBuffer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        CompleteRequest(status);
        return;             
    }

    CompleteRequest(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::IoctlMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr context;

    status = _LogStream->CreateAsyncIoctlContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    KtlLogStreamKernel::AsyncIoctlContextKernel::SPtr contextKernel;
    contextKernel = down_cast<KtlLogStreamKernel::AsyncIoctlContextKernel, KtlLogStream::AsyncIoctlContext>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed a this point
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   

    //
    // Include this on the list of contexts for a stream
    //
    status = _LogStream->AddActiveContext(contextKernel->_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled or stream closed before we got here
        //
        KTraceFailedAsyncRequest(status, nullptr, GetRequestId(), _ParentObject.GetObjectId());
        return(status);
    }   
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::IoctlCompletion);
    context->StartIoctl(_ControlCode,
                        _InBuffer.RawPtr(),
                        _Result,
                        _OutBuffer,
                        nullptr,         // Parent
                        completion);
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::IoctlMarshall(
    )
{
    NTSTATUS status;
    ULONG outSizeNeeded;

    outSizeNeeded = BaseReturnBufferSize + AlignmentNeeded(BaseReturnBufferSize, sizeof(ULONG)) + 2*sizeof(ULONG);
    
    status = VerifyOutBufferSize(outSizeNeeded);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(outSizeNeeded);
        return(status);
    }

    status = ReadData<ULONG>(_ControlCode);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }            
    
    status = ReadKBuffer(_InBuffer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }            
    
    //
    // Continues at RequestMarshallerKernel::IoctlMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::IoctlMethod);

    return(STATUS_SUCCESS);
}
   

VOID
RequestMarshallerKernel::CloseStreamCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    } else {
        //
        // If DeleteObjectMethod fails then it isn't really a problem
        // as all it means is that the object wasn't found in the FOT.
        //
        NTSTATUS statusDontCare;
        
        statusDontCare = DeleteObjectMethod();
        UNREFERENCED_PARAMETER(statusDontCare);
    }
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::CloseStreamMethod(
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr context;           
    
    status = _LogStream->CreateAsyncCloseContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    context->BindToParent(*_LogStream);

    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::CloseStreamCompletion);
    context->StartClose(   nullptr,         // Parent
                           completion);
    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::CloseStreamMarshall(
    )
{
    //
    // Continues at RequestMarshallerKernel::CloseStreamMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::CloseStreamMethod);

    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerKernel::StreamNotificationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();

#ifdef VERBOSE
    KDbgCheckpointWData(0, "StreamNotificationCompletion", status,
                        GetRequestId(),
                        (ULONGLONG)this,
                        0,
                        0);
#endif  
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &Async, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }
    CompleteRequest(status);
}

NTSTATUS
RequestMarshallerKernel::StreamNotificationMethod(
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncStreamNotificationContext::SPtr context;           
    KtlLogStreamKernel::AsyncStreamNotificationContextKernel::SPtr contextKernel;
    
    status = _LogStream->CreateAsyncStreamNotificationContext(context);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Provide a backlink from the context back to the marshaller
    //
    contextKernel = down_cast<KtlLogStreamKernel::AsyncStreamNotificationContextKernel>(context);
    contextKernel->_ActiveContext.SetMarshaller(*this);
    
    //
    // Place on the file object table list so that the request can be
    // cancelled if need be
    //
    _ObjectCancelCallback = ObjectCancelCallback(this, &RequestMarshallerKernel::SingleContextCancel);
    _Context = up_cast<KAsyncContextBase>(context);
    
    status = _FOT->AddRequest(this);
    if (! NT_SUCCESS(status))
    {
        //
        // File handle is being closed at this point
        //
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &RequestMarshallerKernel::StreamNotificationCompletion);
    context->StartWaitForNotification(_ThresholdType,
                                      _ThresholdValue,
                                      nullptr,         // Parent
                                      completion);
    
    return(STATUS_PENDING);
}

NTSTATUS
RequestMarshallerKernel::StreamNotificationMarshall(
    )
{
    NTSTATUS status;
                
    status = ReadData<KtlLogStream::AsyncStreamNotificationContext::ThresholdTypeEnum>(_ThresholdType);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    status = ReadData<ULONGLONG>(_ThresholdValue);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    //
    // Continues at RequestMarshallerKernel::TruncateMethod
    //
    _ObjectMethodCallback = ObjectMethodCallback(this, &RequestMarshallerKernel::StreamNotificationMethod);

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerKernel::LogStreamObjectMethodRequest(
    __in OBJECT_METHOD ObjectMethod
    )
{
    UNREFERENCED_PARAMETER(ObjectMethod);
    
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    
    KInvariant(_ParentObject.GetObjectType() == LogStream);
    _LogStream = static_cast<KtlLogStreamKernel*>(_ParentObject.GetLogStream());

    
    switch(ObjectMethod)
    {
        case QueryRecordRange:
        {
            status = QueryRecordRangeMarshall();
            break;
        }
        
        case QueryRecord:
        {
            status = QueryRecordMarshall();
            break;
        }
        
        case QueryRecords:
        {
            status = QueryRecordsMarshall();
            break;            
        }
        
        case WriteAndReturnLogUsage:
        case Write:
        {
            status = WriteMarshall();
            break;
        }

        case Reservation:
        {
            status = ReservationMarshall();
            break;
        }
        
        case Read:
        {
            status = ReadMarshall();
            break;
        }

        case MultiRecordRead:
        {
            status = MultiRecordReadMarshall();
            break;
        }

        case Truncate:
        {
            status = TruncateMarshall();
            break;
        }
        
        case WriteMetadata:
        {
            status = WriteMetadataMarshall();
            break;            
        }
        
        case ReadMetadata:
        {
            status = ReadMetadataMarshall();
            break;
        }
        
        case Ioctl:
        {
            status = IoctlMarshall();
            break;
        }
        
        case CloseStream:
        {
            status = CloseStreamMarshall();
            break;
        }
        
        case StreamNotification:
        {
            status = StreamNotificationMarshall();
            break;
        }
        
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }
    
    return(status);
}

NTSTATUS
RequestMarshallerKernel::ObjectMethodRequest(
    )
{
    NTSTATUS status;
    OBJECT_METHOD objectMethod;

    //
    // Verify that there is enough output buffer to return a a full
    // request buffer structure. This is the minimum needed in order to
    // return resize information. 
    //
    status = VerifyOutBufferSize(BaseReturnBufferSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        SetOutBufferSizeNeeded(BaseReturnBufferSize);
        return(status);
    }

    objectMethod = ObjectMethodNone;
    status = ReadBytes((PUCHAR)(&objectMethod),
                       sizeof(objectMethod),
                       sizeof(OBJECT_METHOD));

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
        return(status);
    }

    _ObjectMethod = objectMethod;
    
    switch(_ParentObject.GetObjectType())
    {
        case LogManager:
        {
            status = LogManagerObjectMethodRequest(objectMethod);
            break;
        }

        case LogContainer:
        {
            status = LogContainerObjectMethodRequest(objectMethod);
            break;
        }

        case LogStream:
        {
            status = LogStreamObjectMethodRequest(objectMethod);
            break;
        }

        case NullObject:
        default:
        {
            KInvariant(FALSE);
        }
    }
    
    return(status);
}

VOID
RequestMarshallerKernel::Reset(
    )
{
    //
    // Cleanup those members that are used while processing the request
    //
    _ObjectMethodCallback = nullptr;
    
    _LogManager = nullptr;
    _LogContainer = nullptr;
    _LogStream = nullptr;
    _ParentObject.ClearObject();
    _FOT = nullptr;
    _FileObjectEntry = nullptr;

    _Context = nullptr;
    
    _PreAllocIoBuffer = nullptr;
    _PreAllocMetaDataBuffer = nullptr;

    _Alias = nullptr;
    _Path = nullptr;

    _MetaDataBuffer = nullptr;
    _IoBuffer = nullptr;
    _PreAllocMetaDataBuffer = nullptr;
    _PreAllocIoBuffer = nullptr;
}

VOID RequestMarshallerKernel::FormatOutputBuffer(
    __out ULONG& OutBufferSize,
    __inout NTSTATUS& Status    
)
{
    REQUEST_BUFFER* requestBuffer;
    
    if (_MaxIndex == 0)
    {
        //
        // This is the exceptional case where the DeviceIoControl did
        // not pass an output buffer. It is used only in the
        // DeleteObject request since that request comes from a work
        // item that was fired blindly by the destructor and there is
        // no buffer left to which to copy back
        //
        OutBufferSize = 0;
    } else {    
        requestBuffer = (REQUEST_BUFFER*)(_Buffer);

        if (NT_SUCCESS(Status))
        {
            if (_WriteIndex > _MaxIndex)
            {
                //
                // If the output buffer was too small then we need to
                // return this status along with the size needed
                //
                requestBuffer->FinalStatus = STATUS_BUFFER_TOO_SMALL;
                OutBufferSize = FIELD_OFFSET(REQUEST_BUFFER, Data);
            } else {
                //
                // Otherwise the buffer is large enough so return all of
                // the data
                //
                requestBuffer->FinalStatus = Status;
                OutBufferSize = _WriteIndex;
            }
            requestBuffer->SizeNeeded = _WriteIndex;

        } else {
#ifdef VERBOSE
            KDbgCheckpointWData(0, "Request Fail", Status,
                                GetRequestId(),
                                (ULONGLONG)this,
                                0,
                                0);          
#endif          
            //
            // If the method actually failed then return the error code
            // along with an out buffer size of 0 in the results so user can
            // recognize this case from the case where a too small
            // marshalling buffer is passed. The IOCTL itself will succeed but the user mode
            // always checks FinalStatus first
            //
            requestBuffer->FinalStatus = Status;
            requestBuffer->SizeNeeded = 0;
            OutBufferSize = FIELD_OFFSET(REQUEST_BUFFER, Data);
        }
        Status = STATUS_SUCCESS;
    }
}

VOID
RequestMarshallerKernel::CompleteRequest(
    __in NTSTATUS Status
    )
{
    ULONG outBufferSize;

#ifdef VERBOSE
    KDbgCheckpointWData(0, "CompleteRequest", Status,
                        GetRequestId(),
                        (ULONGLONG)this,
                        0,
                        0);
#endif
    
    //
    // Remove from the file object table list as the request no longer
    // needs to be cancellable
    //
    _FOT->RemoveRequest(this);  

    FormatOutputBuffer(outBufferSize,
                       Status);
    
    //
    // Take an extra refcount here to ensure that this object doesn't
    // get cleaned up as part of the CompleteRequest. To understand,
    // consider the pointers between the UserDevIoCtl, KernelDevIoCtl
    // and this RequestMarshallerKernel.
    //
    // UserDevIoCtl -> RequestMarshallerKernel with a ref
    // UserDevIoCtl -> KernelDevIoCtl with a ref
    // RequestMarshallerKernel -> KernelDevIoCtl with a ref
    // KernelDevIoCtl -> UserDevIoCtl without a ref
    //
    // There is a situation where a DeleteObjectRequest is being
    // handled within the context of PerformRequest and thus
    // CompleteRequest is called in that same context. The UserDevIoCtl
    // that invoked was started from the destructor and so within the
    // CompleteRequest below, the UserDevIoCtl is also destructed and
    // along with it the last reference to this RequestMarshallerKernel
    // is also release which in turn releases the last ref on
    // _DevIoCtl. Then when CompleteRequest returns _DevIoCtl is
    // already freed and so it shouldn't be set to nullptr.
    //
    AddRef();

    _DevIoCtl->CompleteRequest(Status, outBufferSize);
    _DevIoCtl = nullptr;

    Release();
}

NTSTATUS
RequestMarshallerKernel::PerformRequest(
    __in FileObjectTable::SPtr& FOT,
    __in DevIoControlKernel::SPtr& DevIoCtl
    )
{
    UNREFERENCED_PARAMETER(FOT);
    UNREFERENCED_PARAMETER(DevIoCtl);
    
    NTSTATUS status;
    
    KInvariant(_FOT);
    KInvariant(_FOT.RawPtr() == FOT.RawPtr());
    KInvariant(_DevIoCtl);
    KInvariant(_DevIoCtl.RawPtr() == DevIoCtl.RawPtr());

    //
    // Reformat the buffer to return the output data structures
    //
    FormatResponseToUser();

    //
    // Initialize pointers to indicate that the request is not on any
    // list. If it is placed on the list then it will be updated and a
    // cancel routine setup.
    //
    _ListEntry.Blink = nullptr;
    _ListEntry.Flink = nullptr;
    
    KInvariant(_ObjectMethodCallback);
    status = (_ObjectMethodCallback)();
    
    if (status != STATUS_PENDING)
    {
        CompleteRequest(status);
        status = STATUS_PENDING;
    }
    
    return(status); 
}

NTSTATUS
RequestMarshallerKernel::MarshallRequest(
    __in PVOID WdfRequest,
    __in FileObjectTable::SPtr& FOT,
    __in DevIoControlKernel::SPtr& DevIoCtl,
    __out BOOLEAN* CompleteRequest,
    __out ULONG* OutBufferSize                                       
    )
{
    NTSTATUS status;
    
    // NOTE: Any locking of pages from user address space must
    // happen in this context as it is invoked in the callers process
    // context.

    _FOT = FOT;
    _DevIoCtl = DevIoCtl;

    _WdfRequest = WdfRequest;

    *CompleteRequest = FALSE;
    *OutBufferSize = 0;
    
    status = InitializeRequestFromUser(_FOT,
                                       _ParentObject);

    if (NT_SUCCESS(status))
    {
        switch(_Request->RequestType)
        {
            case RequestMarshaller::CreateObjectRequest:
            {
                status = CreateObjectRequest();
                break;
            }

            case RequestMarshaller::DeleteObjectRequest:
            {
                status = DeleteObjectRequest();
                break;
            }

            case RequestMarshaller::ObjectMethodRequest:
            {
                status = ObjectMethodRequest();
                break;
            }

            default:
            {
                KInvariant(FALSE);
            }
        }
    } else {
        KTraceFailedAsyncRequest(status, nullptr, _ParentObject.GetObjectId(), reinterpret_cast<ULONGLONG>(this));
    }

    //
    // Since this is called from the IoInCallerCallback, it is not
    // permissible to complete the request in this context
    //
    KInvariant(status != STATUS_PENDING);

    if (! NT_SUCCESS(status))
    {
        //
        // On failure here just cause the request to be completed on
        // return.
        //
        *CompleteRequest = TRUE;
        
        FormatResponseToUser();
        FormatOutputBuffer(*OutBufferSize,
                           status);
        
        _DevIoCtl = nullptr;
    }
    
    return(status); 
}

VOID
RequestMarshallerKernel::Cleanup(
    __in FileObjectTable& FOT
#ifdef UDRIVER
    , __in KAllocator& Allocator
#endif
    )
{
    //
    // This implements the IRP_MJ_CLEANUP operation. It is called in
    // the context of the user process that was the last one to close a
    // handle to the device object. The semantic of the callback is
    // that it is synchronous. Since this is not a KTL system thread,
    // it is ok to block in this thread.
    //
    // The cleanup operation must do the following:
    //
    // * Ensure all outstanding requests for this log manager have completed.
    // * Clean up any memory associated with log manager (ie, File Object table)
    // * Shutdown the log manager
    //

    KDbgPrintfInformational("Shutting down %p\n", &FOT);
        

	//
	// Ensure this is not invoked by a KTL thread
	//
#ifdef UDRIVER
	KInvariant(! Allocator.GetKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
#endif
#ifdef KDRIVER
	KInvariant(! KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
#endif
	
	//
    // Force cancel all requests in the FOT
    //
    FOT.CancelAllRequests();

    //
    // Wait for all KIoBufferElements to be unmapped
    //
    FOT.WaitForAllIoBufferElements();
    
    //
    // Force close all objects in the FOT
    //
    FOT.ClearObjects();

    //
    // Shutdown the log mananger
    //
    KtlLogManagerKernel::SPtr logManager;

    logManager = FOT.GetLogManager();
    
    //
    // Release the reference to the FOT taken in the OpenDevice
    // operation. The FOT should be gone and the LogManager only have
    // one refcount - the one above
    //
    FOT.Release();

    if (logManager)
    {
        // TODO: REMOVE WHEN DEBUGGED
        logManager->_CleanupCalled = TRUE;
        // TODO: REMOVE WHEN DEBUGGED

#ifdef UDRIVER
		NTSTATUS status;
		
		ktl::kservice::CloseAwaiter::SPtr awaiter;
		status = ktl::kservice::CloseAwaiter::Create(
			Allocator,
			KTL_TAG_TEST,
			*logManager, 
			awaiter,
			nullptr,
			nullptr);

		if (!NT_SUCCESS(status))
		{
            KTraceFailedAsyncRequest(status, NULL, 0, 0);
			return;
		}

		status =  ktl::SyncAwait(*awaiter);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, NULL, 0, 0);
        }
#endif		
#ifdef KDRIVER
        KServiceSynchronizer        sync;
        logManager->StartCloseLogManager(nullptr,          // ParentAsync
                                         sync.CloseCompletionCallback());
        //
        // Wait for log manager to close
        //
        sync.WaitForCompletion();
#endif
    }        
}

//
// Cancellation and list management support
//
VOID 
RequestMarshallerKernel::PerformCancel(
    )
{
    _ObjectCancelCallback();
}

KIoBufferElementKernel::KIoBufferElementKernel()
{
#ifdef KDRIVER
    _Mdl = NULL;
#endif
    _FOT = NULL;
}

KIoBufferElementKernel::~KIoBufferElementKernel()
{
#ifdef KDRIVER
    if (_Mdl != NULL)
    {
        _FOT->DecrementPinnedIoBufferUsage(_Size);
        
        MmUnlockPages(_Mdl);
        IoFreeMdl(_Mdl);
        _Mdl = NULL;
    }
#endif
    if (_FOT)
    {
        _FOT->RemoveIoBufferElement(*this);
    }
}

NTSTATUS KIoBufferElementKernel::Create(
    __in FileObjectTable& FOT,
    __in BOOLEAN IsReadOnly,
#ifdef UDRIVER
    __in KIoBufferElement& IoBufferElementUser,
#endif
    __in PVOID Src,
    __in ULONG Size,
    __out KIoBufferElement::SPtr& IoBufferElement,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    KIoBufferElementKernel::SPtr ioBufferElement;

    //
    // Ensure 4K alignment for pointer and size
    //
    if (((Size & ~0xfff) != Size) || ((ULONGLONG)Src & ~0xfff) != (ULONGLONG)Src)
    {
        return(STATUS_INVALID_PARAMETER);
    }
    
    ioBufferElement = _new(AllocationTag, Allocator) KIoBufferElementKernel();
    if (ioBufferElement == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
    }

    status = ioBufferElement->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    ioBufferElement->_Size = Size;
    ioBufferElement->_FreeBuffer = FALSE;

#ifdef UDRIVER
    //
    // Take a reference to the IoBufferElement that is on the user side
    // so that the memory buffer stays in scope for the duration of the
    // kernel side processing.
    //

    UNREFERENCED_PARAMETER(IsReadOnly);
    
    ioBufferElement->_IoBufferElementUser = &IoBufferElementUser;
    ioBufferElement->_Buffer = Src;
	IsReadOnly;		/* Not used */
#endif
#ifdef KDRIVER
    //
    // Track the number of outstanding bytes and if over the limit then
    // return the specific error that will throttle requests
    //
    if (FOT.IsPinnedIoBufferUsageOverLimit())
    {
        status = STATUS_WORKING_SET_QUOTA;
        KTraceFailedAsyncRequest(status,
                                 NULL,
                                 (FOT.GetPinnedIoBufferLimit() * 0x100000000) + Size,
                                 FOT.GetPinnedIoBufferUsage());
#if DBG
		FOT.IncrementPinMemoryFailureCount();
#endif
        return(status);     
    }
    
    ioBufferElement->_Mdl = IoAllocateMdl(Src, Size,  FALSE, TRUE, NULL);
    if (! ioBufferElement->_Mdl)
    {
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, ioBufferElement->_Size);
#if DBG
		FOT.IncrementPinMemoryFailureCount();
#endif
        return(STATUS_WORKING_SET_QUOTA);
    }
    
    //
    // Probe and lock the pages of this buffer in physical memory.
    //
    status = KtlLogMmProbeAndLockPagesNoException(ioBufferElement->_Mdl, UserMode, IsReadOnly ? IoReadAccess : IoWriteAccess);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status,
                                 NULL,
                                 (FOT.GetPinnedIoBufferLimit() * 0x100000000) + Size,
                                 FOT.GetPinnedIoBufferUsage());
        IoFreeMdl(ioBufferElement->_Mdl);
        ioBufferElement->_Mdl = NULL;
#if DBG
		FOT.IncrementPinMemoryFailureCount();
#endif
        return(status);
    }    
    
    //         
    // Map the physical pages described by the MDL into system space. 
    // Note: double mapping the buffer this way causes lot of 
    // system overhead for large size buffers. 
    // 
    ioBufferElement->_Buffer = MmGetSystemAddressForMdlSafe(ioBufferElement->_Mdl, NormalPagePriority );
    
    if (! ioBufferElement->_Buffer) {
        status = STATUS_WORKING_SET_QUOTA;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, ioBufferElement->_Size);
        MmUnlockPages(ioBufferElement->_Mdl);
        IoFreeMdl(ioBufferElement->_Mdl);
        ioBufferElement->_Mdl = NULL;
#if DBG
		FOT.IncrementPinMemoryFailureCount();
#endif
        return(status);
    }
#endif
    ioBufferElement->_FOT = &FOT;
    
    status = ioBufferElement->_FOT->AddIoBufferElement(*ioBufferElement);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, ioBufferElement->_Size);
#ifdef KDRIVER
        MmUnlockPages(ioBufferElement->_Mdl);
        IoFreeMdl(ioBufferElement->_Mdl);
        ioBufferElement->_Mdl = NULL;
#endif
        ioBufferElement->_FOT = NULL;
#if DBG
		FOT.IncrementPinMemoryFailureCount();
#endif
        return(STATUS_WORKING_SET_QUOTA);     
    }

#ifdef KDRIVER
    //
    // PinnedIoBufferUsage is subtracted in the KIoBufferElement
    // destructor only when ioBufferElement->_Mdl != NULL. All of the
    // error cases above set _Mdl == NULL and so we add it here.
    //
    FOT.IncrementPinnedIoBufferUsage(Size);
#endif
            
    ioBufferElement->_IsReference = FALSE;
    
    IoBufferElement = up_cast<KIoBufferElement>(ioBufferElement);
    return(status); 
}

