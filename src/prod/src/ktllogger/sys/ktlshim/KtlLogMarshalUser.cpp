// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define KDRIVER 1
#endif

#include <ktl.h>
#include <ktrace.h>

#include "KtlLogShimUser.h"

RequestMarshallerUser::RequestMarshallerUser()
{
}

RequestMarshallerUser::~RequestMarshallerUser()
{
}

NTSTATUS
RequestMarshallerUser::Create(
    __out RequestMarshallerUser::SPtr& Result,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in ULONG InKBufferSize,
    __in ULONG OutKBufferSize
    )
{
    NTSTATUS status;
    RequestMarshallerUser::SPtr requestMarshaller;

    Result = nullptr;

    requestMarshaller = _new(AllocationTag, Allocator) RequestMarshallerUser();
    if (requestMarshaller == nullptr)
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

    //
    // Allocate space for input and output buffer
    //
    status = KBuffer::Create(InKBufferSize,
                             requestMarshaller->_InKBuffer,
                             Allocator,
                             AllocationTag);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = KBuffer::Create(OutKBufferSize,
                             requestMarshaller->_OutKBuffer,
                             Allocator,
                             AllocationTag);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    requestMarshaller->_Buffer = NULL;
    requestMarshaller->_ExtendBuffer = TRUE;
    
    requestMarshaller->_AllocationTag = AllocationTag;
    
    Result = Ktl::Move(requestMarshaller);
    return(status); 
}


//
// Routines to use from within user mode
//

NTSTATUS
RequestMarshallerUser::InitializeRequest(
    __in REQUEST_TYPE RequestType,
    __in ULONG RequestVersion,
    __in ULONGLONG RequestId,
    __in OBJECT_TYPE ObjectType,
    __in ULONGLONG ObjectId
)
{
    NTSTATUS status;

    _KBuffer = _InKBuffer;
    _Buffer = static_cast<PUCHAR>(_InKBuffer->GetBuffer());
    _WriteIndex = 0;
    _ReadIndex = 0;
    _MaxIndex = _InKBuffer->QuerySize();
    
    status = SetWriteIndex(FIELD_OFFSET(REQUEST_BUFFER, Data));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    //
    // Initialize header of the input buffer. The input buffer is the
    // buffer sent from user to kernel modes
    //
    REQUEST_BUFFER* requestBuffer;

    requestBuffer = (REQUEST_BUFFER*)(_Buffer);
    requestBuffer->RequestType = RequestType;
    requestBuffer->Version = RequestVersion;

    requestBuffer->RequestId = RequestId;
    requestBuffer->ObjectTypeAndId.ObjectType = ObjectType;
    requestBuffer->ObjectTypeAndId.ObjectId = ObjectId;

    requestBuffer->FinalStatus = STATUS_SUCCESS;
    return(status);
}

NTSTATUS
RequestMarshallerUser::InitializeForCreateObject(
    OBJECT_TYPE ObjectType
    )
{
    NTSTATUS status;
    OBJECT_TYPEANDID objectTypeAndId;

    KInvariant( (ObjectType == LogManager) ||
            (ObjectType == LogContainer) ||
            (ObjectType == LogStream) );
    
    status = InitializeRequest(CreateObjectRequest, REQUEST_VERSION_1, 0, NullObject, OBJECTID_NULL);
    
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    objectTypeAndId.ObjectType = ObjectType;
    objectTypeAndId.ObjectId = OBJECTID_NULL;
    
    status = WriteBytes((PUCHAR)(&objectTypeAndId),
                        sizeof(objectTypeAndId),
                        sizeof(LONGLONG));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerUser::InitializeForObjectMethod(
    __in OBJECT_TYPE ObjectType,
    __in ULONGLONG RequestId,
    __in ULONGLONG ObjectId,
    __in OBJECT_METHOD ObjectMethod
    )
{
    NTSTATUS status;
    ULONG requestVersion = REQUEST_VERSION_1;

    if (ObjectMethod == MultiRecordRead)
    {
        requestVersion = REQUEST_VERSION_2;
    }
    else if (ObjectMethod == WriteAndReturnLogUsage)
    {
        requestVersion = REQUEST_VERSION_5;
    }
    
    status = InitializeRequest(ObjectMethodRequest,
                               requestVersion,
                               RequestId,                              
                               ObjectType,
                               ObjectId);
    
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = WriteBytes((PUCHAR)(&ObjectMethod),
                        sizeof(ObjectMethod),
                        sizeof(OBJECT_METHOD));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerUser::InitializeForDeleteObject(
    __in OBJECT_TYPE ObjectType,
    __in ULONGLONG ObjectId
    )
{
    NTSTATUS status;
    
    status = InitializeRequest(DeleteObjectRequest,
                               REQUEST_VERSION_1,
                               0,
                               ObjectType,
                               ObjectId);
    
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    return(STATUS_SUCCESS);
}

VOID
RequestMarshallerUser::Reset(
    )
{
}

NTSTATUS
RequestMarshallerUser::InitializeForResponse(
    __in ULONG OutBufferSize
    )
{
    NTSTATUS status;
    RequestMarshaller::REQUEST_BUFFER* request;
    
    _MaxIndex = OutBufferSize;

    if (OutBufferSize < FIELD_OFFSET(RequestMarshaller::REQUEST_BUFFER, Data))
    {
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    //
    // Update buffer pointer as it could have changed as a result of a
    // call to kernel
    //
    _Buffer = (PUCHAR)(_OutKBuffer->GetBuffer());
    
    request = (RequestMarshaller::REQUEST_BUFFER*)(_Buffer);
    if ((! IsValidRequestType(request->RequestType)) ||
        (! IsValidRequestVersion(request->Version)) ||
        (! IsValidObjectType(request->ObjectTypeAndId.ObjectType)))
    {
        return(STATUS_UNSUCCESSFUL);        
    }
        
    status = SetReadIndex(FIELD_OFFSET(RequestMarshaller::REQUEST_BUFFER, Data));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    return(STATUS_SUCCESS);
}


NTSTATUS
RequestMarshallerUser::ReadObject(
    __in OBJECT_TYPE ObjectTypeExpected,
    __out ULONGLONG& ObjectId
)
{
    NTSTATUS status;
    OBJECT_TYPEANDID objectTypeAndId;
    
    status = ReadObjectTypeAndId(objectTypeAndId);
    if (! NT_SUCCESS(status))
    {
        return(status);             
    }

    if (objectTypeAndId.ObjectType != ObjectTypeExpected)
    {
        return(STATUS_OBJECT_TYPE_MISMATCH);
    }

    ObjectId = objectTypeAndId.ObjectId;
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerUser::ReadKIoBufferPreAlloc(
    __out KIoBuffer::SPtr& IoBuffer,
    __in KIoBuffer::SPtr& PreAllocIoBuffer,
    __out ULONG& Size
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    KIoBufferElement::SPtr preallocIoBufferElement;
    KIoBufferElement::SPtr ioBufferElement;

    oldReadIndex = _ReadIndex;
    
    //
    // ULONG - updated size
    //
    status = ReadData<ULONG>(Size);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    if (Size > PreAllocIoBuffer->QuerySize())
    {
        //
        // Buffer size needed is larger than the prealloc buffer. This
        // is the indication from kernel mode that a larger IoBuffer is
        // needed.
        //
        return(STATUS_BUFFER_TOO_SMALL);
    }
    
    status = KIoBuffer::CreateEmpty(IoBuffer,
                                    GetThisAllocator(),
                                    _AllocationTag);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }
    
    preallocIoBufferElement = PreAllocIoBuffer->First();
    status = KIoBufferElement::CreateReference(*preallocIoBufferElement,
                                               0,                          // Offset
                                               Size,
                                               ioBufferElement,
                                               GetThisAllocator(),
                                               _AllocationTag);

    if (! NT_SUCCESS(status))
    {
        IoBuffer = nullptr;
        _ReadIndex = oldReadIndex;
        return(status);
    }

    IoBuffer->AddIoBufferElement(*ioBufferElement);
    ioBufferElement = nullptr;      
    
    return(STATUS_SUCCESS);
}


NTSTATUS
RequestMarshallerUser::WriteKIoBufferElement(
    __in KIoBufferElement& IoBufferElement,
    __in BOOLEAN MakeReadOnly
    )
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;
    
    status = WriteData(IoBufferElement.QuerySize());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

#ifdef UDRIVER
    //
    // For the UDRIVER, we pass the IoBufferElement as-is and take a reference to it on the "kernel" side
    // This works fine as the "user" and "kernel" sides are in the same
    // process.
    //
    status = WritePVOID((PVOID)(&IoBufferElement));
#endif
#ifdef KDRIVER
    status = WritePVOID((PVOID)(IoBufferElement.GetBuffer()));
#endif
    
    if (!NT_SUCCESS(status))
    {
        _WriteIndex = oldWriteIndex;
        return(status);
    }   

    if (MakeReadOnly)
    {
        IoBufferElement.SetPageAccess(PAGE_READONLY);
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshallerUser::WriteKIoBuffer(
    __in KIoBuffer& IoBuffer,
    __in BOOLEAN MakeReadOnly
)
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;
    
    //
    // ULONG - Number io buffer elements
    // Each io buffer element
    //     io buffer element size
    //     io buffer element pointer
    //

    status = WriteData(IoBuffer.QueryNumberOfIoBufferElements());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    for (KIoBufferElement* ioBufferElement = IoBuffer.First();
         ioBufferElement != NULL;
         ioBufferElement = IoBuffer.Next(*ioBufferElement))
    {
        status = WriteKIoBufferElement(*ioBufferElement, MakeReadOnly);
        
        if (! NT_SUCCESS(status))
        {
            _WriteIndex = oldWriteIndex;
            return(status);
        }       
    }
    
    return(STATUS_SUCCESS);
}
