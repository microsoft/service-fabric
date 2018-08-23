/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    kserial.cpp

    Description:
      Kernel Tempate Library (KTL): Serialization Helpers

      Serialization implementations for cases where cases
      are too long to inline.

    History:
      raymcc          24-Mar-2011         First version

--*/

#include <ktl.h>


#define RETURN_ON_ERROR(Res) \
    if (!NT_SUCCESS(Res))\
    {\
        return Res;\
    }



VOID KOutChannel::Log(
    __in NTSTATUS Status
    ) const
{
    // TBD
    UNREFERENCED_PARAMETER(Status);            
}

VOID KOutChannel::Enqueue(
    __in KMemRef& Ref
    )
{
    Ref._Param = Ref._Size;
    _DeferredQ.Enq(Ref);
}


VOID KInChannel::Log(
    __in NTSTATUS Status
    )const
{
    // TBD
    UNREFERENCED_PARAMETER(Status);            
}


VOID KInChannel::Enqueue(
    __in KMemRef const & Ref
    )
{
    _DeferredQ.Enq(Ref);
}





//
//  KIoBuffer serializer
//
KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KIoBuffer::SPtr& Buf
    )
{
    BOOLEAN IsNull = TRUE;

    // Output a flag to indicate whether the pointer is null.
    //
    if (!Buf)
    {
        Out << IsNull;
        return Out;
    }
    else
    {
        IsNull = FALSE;
        Out << IsNull;
    }

    KIoBufferElement* Elem = Buf->First();

    // Write out the number of elements
    //
    ULONG BufCount = Buf->QueryNumberOfIoBufferElements();
    Out << BufCount;

    // Now write out each address.
    //
    while (BufCount--)
    {
        ULONG Size = Elem->QuerySize();
        Out << Size;
        if (Size)
        {
            const void* Address = Elem->GetBuffer();
            KMemRef Ref(Size, PVOID(Address));

            // Enqueue
            //
            Out.Enqueue(Ref);
        }
        if (BufCount)
        {
            Elem = Buf->Next(*Elem);
        }
    }

    return Out;
}

//
//  KIoBuffer deserializer
//
KInChannel& operator>>(
    __in KInChannel& In,
    __in KIoBuffer::SPtr& Buf
    )
{
    NTSTATUS Res;
    BOOLEAN IsNull;

    // Check the null/non-null flag.
    //
    In >> IsNull;
    if (IsNull)
    {
        Buf = 0;
        return In;
    }

    // If here, there is a non-null KIoBuffer to get.
    //
    if (!Buf)
    {
        KIoBuffer::CreateEmpty(Buf, In.Allocator());
    }
    else
    {
        Buf->Clear();
    }

    ULONG BufCount = 0;
    In >> BufCount;

    // Now write out each address.
    //
    while (BufCount--)
    {
        ULONG Size = 0;
        In >> Size;
        if (Size)
        {
            KIoBufferElement::SPtr NewEl;
            VOID* RawBuf = 0;
            Res = KIoBufferElement::CreateNew(
                Size,
                NewEl,
                RawBuf,
                In.Allocator());

            if (!NT_SUCCESS(Res))
            {
                Buf->Clear();
                In.SetStatus(Res);
                return In;
            }

            Buf->AddIoBufferElement(*NewEl);
            KMemRef Ref(Size, RawBuf);
            In.Enqueue(Ref);
        }
    }

    return In;
}


//
//  KBuffer serializer
//
KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KBuffer::SPtr& Buf  
    )
{
    BOOLEAN IsNull = TRUE;

    // Output a flag to indicate whether the pointer is null.
    //
    if (!Buf)
    {
        Out << IsNull;
        return Out;
    }
    else
    {
        IsNull = FALSE;
        Out << IsNull;
    }

    // Query the size
    //
    ULONG Size = Buf->QuerySize();
    Out << Size;

    // If there is a nonzero-sized buffer, enqueue it for later writing.
    //
    if (Size)
    {
        const void* Address = Buf->GetBuffer();
        KMemRef Ref(Size, PVOID(Address));

        // Enqueue
        //            
        Out.Enqueue(Ref);
    }

    return Out;
}

//
// This serializer for KBuffer copies the data into the out channel so that the KBuffer object can go away immediately after this call returns.
//

KOutChannel& 
_KSerializeBuffer(
    __in KOutChannel& Out,    
    __in KBuffer::SPtr& Buf
    )
{
    BOOLEAN IsNull = TRUE;

    // Output a flag to indicate whether the pointer is null.
    //
    if (!Buf)
    {
        Out << IsNull;
        return Out;
    }
    else
    {
        IsNull = FALSE;
        Out << IsNull;
    }

    // Query the size
    //
    ULONG Size = Buf->QuerySize();
    Out << Size;    

    // If there is a nonzero-sized buffer, copy into the out channel.
    //
    if (Size)
    {
        const void* Address = Buf->GetBuffer();
        KMemRef Ref(Size, PVOID(Address));
        
        // Copy the data
        //
        Out << Ref;
    }

    return Out;
}


//
//  KBuffer deserializer
//
KInChannel& operator>>(
    __in KInChannel& In,
    __in KBuffer::SPtr& Buf
    )
{
    BOOLEAN IsNull;

    // Check the null/non-null flag.
    //
    In >> IsNull;
    if (IsNull)
    {
        Buf = 0;
        return In;
    }

    // Query the size
    //
    ULONG Size = 0;
    In >> Size;

    // If there is a nonzero-sized buffer, enqueue it for later reading in pass 2
    //
    if (Size)
    {
        if (!Buf)
        {
            KBuffer::Create(Size, Buf, In.Allocator());
        }
        else
        {
            Buf->SetSize(Size);
        }
        const void* Address = Buf->GetBuffer();
        KMemRef Ref(Size, PVOID(Address));

        // Enqueue
        //
        In.Enqueue(Ref);            
    }

    return In;
}

//
// This deserializer for KBuffer should be paired with a KBuffer serialized using _KSerializeBuffer().
//

KInChannel& 
_KDeserializeBuffer(
    __in KInChannel& In,
    __in KBuffer::SPtr& Buf
    )
{
    BOOLEAN IsNull;

    // Check the null/non-null flag.
    //
    In >> IsNull;
    if (IsNull)
    {
        Buf = 0;
        return In;
    }

    // Query the size
    //
    ULONG Size = 0;
    In >> Size;

    // If there is a nonzero-sized buffer, copy the data out.
    //
    if (Size)
    {
        if (!Buf)
        {
            KBuffer::Create(Size, Buf, In.Allocator());
        }
        else
        {
            Buf->SetSize(Size);
        }
        const void* Address = Buf->GetBuffer();
        KMemRef Ref(Size, PVOID(Address));

        // Copy data
        //
        In >> Ref;
    }

    return In;
}

//
//  Serializer for KVariant
//
KMemChannel& operator <<(
    __in KMemChannel& Ch,
    __in KVariant const & Var
    )
{
    // Write out the type field.
    //
    Ch << Var.Type();

    // Write out the data.
    //
    switch (Var.Type())
    {
        default:
        case KVariant::Type_EMPTY:
            break;

        case KVariant::Type_LONG:
            Ch << LONG(Var);
            break;

        case KVariant::Type_ULONG:
            Ch << ULONG(Var);
            break;

        case KVariant::Type_ULONGLONG:
            Ch << ULONGLONG(Var);
            break;

        case KVariant::Type_GUID:
            Ch << GUID(Var);
            break;

        case KVariant::Type_KString_SPtr:
            {
               UNICODE_STRING Tmp = Var;
               Ch << &Tmp;
               break;
            }
        case KVariant::Type_KMemRef:
             //TBD
            break;
    };

    return Ch;
}

//
//   Deserializer for KVariant
//
KMemChannel& operator >>(
    __in KMemChannel& In,
    __in KVariant& Var
    )
{
    // Read the type field.
    //
    KVariant::KVariantType TheType;

    In >> TheType;

    // Write out the data.
    //
    switch (TheType)
    {
        default:
        case KVariant::Type_EMPTY:
            Var.Clear();
            break;

        case KVariant::Type_LONG:
        {
            LONG Val = 0;
            In >> Val;
            Var = Val;
            break;
        }

        case KVariant::Type_ULONG:
        {
            ULONG Val = 0;
            In >> Val;
            Var = Val;
            break;
        }

        case KVariant::Type_ULONGLONG:
        {
            ULONGLONG Val = 0;
            In >> Val;
            Var = Val;
            break;
        }

        case KVariant::Type_GUID:
        {
            GUID Guid;
            RtlZeroMemory(&Guid, sizeof(GUID));
            In >> Guid;
            Var = Guid;
            break;
        }

        case KVariant::Type_KString_SPtr:
        {
            UNICODE_STRING Str;
            RtlInitUnicodeString(&Str, NULL);
            PUNICODE_STRING PStr = &Str;
            In >> PStr;     // BUGBUG: Don't know if this succeeded
            Var = KVariant::Create(*PStr, In.Allocator()    );
            _delete(PStr->Buffer);
            break;
        }

        case KVariant::Type_KMemRef:
             //TBD
            break;
    };

    return In;
}

//
// Serializer for KIDomNode and KIMutableDomNode
//

KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KSharedPtr<KIDomNode>& Dom
    )
{
    BOOLEAN ObjectPresent = FALSE;
    if (Dom)
    {
       ObjectPresent = TRUE;
    }
    Out << ObjectPresent;
    if (!ObjectPresent)
    {
        return Out;
    }
    
    NTSTATUS status = STATUS_SUCCESS;
    KSharedPtr<KDom::UnicodeOutputStream> outputStream;

    status = KDom::CreateOutputStream(outputStream, Out.Allocator(), KTL_TAG_DOM);
    if (!NT_SUCCESS(status))
    {
        Out.Log(status);
        return Out;
    }
    
    status = KDom::ToStream(Dom, *outputStream);
    if (NT_SUCCESS(status))
    {
        KBuffer::SPtr buffer = outputStream->GetBuffer();
        buffer->SetSize(outputStream->GetStreamSize(), TRUE);
        
        _KSerializeBuffer(Out, buffer);
    }
    else
    {
        Out.Log(status);
    }
    
    return Out;
}

KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KSharedPtr<KIMutableDomNode>& Dom
    )
{
    BOOLEAN ObjectPresent = FALSE;
    if (Dom)
    {
       ObjectPresent = TRUE;
    }
    Out << ObjectPresent;
    if (!ObjectPresent)
    {
        return Out;
    }
    
    NTSTATUS status = STATUS_SUCCESS;
    KSharedPtr<KDom::UnicodeOutputStream> outputStream;

    status = KDom::CreateOutputStream(outputStream, Out.Allocator(), KTL_TAG_DOM);
    if (!NT_SUCCESS(status))
    {
        Out.Log(status);
        return Out;
    }
    
    status = KDom::ToStream(Dom, *outputStream);
    if (NT_SUCCESS(status))
    {
        KBuffer::SPtr buffer = outputStream->GetBuffer();        
        buffer->SetSize(outputStream->GetStreamSize(), TRUE);
        
        _KSerializeBuffer(Out, buffer);
    }
    else
    {
        Out.Log(status);
    }
    
    return Out;
}

//
// Deserializer for KIDomNode and KIMutableDomNode
//

KInChannel& operator>>(
    __in    KInChannel& In,
    __inout KSharedPtr<KIDomNode>& Dom
    )
{    
    Dom.Reset();
    
    // Read the stream to see if there is an object there or not.
    BOOLEAN ObjectPresent = FALSE;
    In >> ObjectPresent;
    if (!ObjectPresent)
    {        
        return In;
    }
    
    KBuffer::SPtr buffer;
    _KDeserializeBuffer(In, buffer);

    if (buffer)
    {
        NTSTATUS status = KDom::CreateReadOnlyFromBuffer(buffer, In.Allocator(), KTL_TAG_DOM, Dom);
        if (!NT_SUCCESS(status))
        {
            In.Log(status);
        }
    }
    
    return In;
}

KInChannel& operator>>(
    __in    KInChannel& In,
    __inout KSharedPtr<KIMutableDomNode>& Dom
    )
{    
    Dom.Reset();
    
    // Read the stream to see if there is an object there or not.
    BOOLEAN ObjectPresent = FALSE;
    In >> ObjectPresent;
    if (!ObjectPresent)
    {        
        return In;
    }
    
    KBuffer::SPtr buffer;
    _KDeserializeBuffer(In, buffer);

    if (buffer)
    {
        NTSTATUS status = KDom::CreateFromBuffer(buffer, In.Allocator(), KTL_TAG_DOM, Dom);
        if (!NT_SUCCESS(status))
        {
            In.Log(status);
        }
    }
    
    return In;
}

NTSTATUS
_KMemChannelTransfer(
    __in KMemChannel& Dest,
    __in KMemChannel& Src,
    __in KAllocator& Allocator
    )
{
    KArray<KMemRef> Ary(Allocator);

    ULONG NumBlocks = Src.GetBlockCount();

    for (ULONG i = 0; i < NumBlocks; i++)
    {
        KMemRef x;
        Src.GetBlock(i, x);
        Ary.Append(x);
    }

    Src.DetachMemory();

    Dest.StartCapture();

    for (ULONG i = 0; i < Ary.Count(); i++)
    {
        Dest.Capture(Ary[i]);
    }

    Dest.EndCapture();
    Dest.ResetCursor();
    return STATUS_SUCCESS;
}

// _KIoChannelTransfer
//
//
NTSTATUS
_KIoChannelTransfer(
    __in KInChannel&  Dest,
    __in KOutChannel& Src,
    __in KAllocator& Allocator
    )
{
    NTSTATUS Result;
    
    // Write the deferred blocks at the end of the stream
    Src._DeferredQ.ResetCursor();
    while (! Src._DeferredQ.IsEmpty())
    {
        KMemRef ref = Src._DeferredQ.Peek();
        Result = Src.Data.Write(ref._Size, ref._Address);
        RETURN_ON_ERROR(Result);
        KInvariant(Src._DeferredQ.Deq());
    }


    // Transfer all the normal memory channels.
    Result = _KMemChannelTransfer(Dest.Data, Src.Data, Allocator);
    RETURN_ON_ERROR(Result);

    Result = _KMemChannelTransfer(Dest.Meta, Src.Meta, Allocator);
    RETURN_ON_ERROR(Result);

    return STATUS_SUCCESS;
}

// KSerializer::GetSizes
//
NTSTATUS
KSerializer::GetSizes(
    __out ULONG& MetaBytes,
    __out ULONG& DataBytes,
    __out ULONG& DeferredBytes
   )
{
    if (_OutChannel.Meta.GetBlockCount() == 0 &&
        _OutChannel.Data.GetBlockCount() == 0 &&
        _OutChannel._DeferredQ.Count() ==0)
    {
        return STATUS_NO_DATA_DETECTED;
    }

    MetaBytes = _OutChannel.Meta.Size();
    DataBytes = _OutChannel.Data.Size();

    DeferredBytes = 0;
    _OutChannel._DeferredQ.ResetCursor();

    KMemRef BlockRef;
    while (_OutChannel._DeferredQ.Next(BlockRef))
    {
        DeferredBytes += BlockRef._Size;
    }

    return STATUS_SUCCESS;
}

// KSerializer::Reset
//
NTSTATUS
KSerializer::Reset(
    __in SerialBlockType Filter
   )
{
    _Iterator = 0;
    _OutChannel._DeferredQ.ResetCursor();
    _MetaBlockCount = _OutChannel.Meta.GetBlockCount();
    _DataBlockCount = _OutChannel.Data.GetBlockCount();

    if (Filter == eBlockTypeMeta || Filter == eBlockTypeDeferred || Filter == eBlockTypeData)
    {
        _FilterType = Filter;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER_1;
}

//  KSerializer::HasDeferredBlocks
//
//
BOOLEAN
KSerializer::HasDeferredBlocks()
{
    if (_OutChannel._DeferredQ.Count())
    {
        return TRUE;
    }
    return FALSE;
}


// KSerializer::NextBlock
//
// Retrieves the next block of memory in the serialzation sequence.
// The caller must COPY this memory to a network transport, file
// or other medium.
//
NTSTATUS
KSerializer::NextBlock(
    __out KMemRef& BlockRef
    )
{
    NTSTATUS Result;

    if (_FilterType == eBlockTypeMeta)
    {
        if (_Iterator == _MetaBlockCount)
        {
            return STATUS_NO_MORE_ENTRIES;
        }
        Result = _OutChannel.Meta.GetBlock(_Iterator, BlockRef);
        if (!NT_SUCCESS(Result))
        {
            return STATUS_NO_MORE_ENTRIES;
        }
        _Iterator++;
        return STATUS_SUCCESS;
   }
    else if (_FilterType == eBlockTypeData)
    {
        if (_Iterator == _DataBlockCount)
        {
            return STATUS_NO_MORE_ENTRIES;
        }
        Result = _OutChannel.Data.GetBlock(_Iterator, BlockRef);
        if (!NT_SUCCESS(Result))
        {
            return STATUS_NO_MORE_ENTRIES;
        }
        _Iterator++;
        return STATUS_SUCCESS;
    }
    else if (_FilterType == eBlockTypeDeferred)
    {
       BOOLEAN Result1 = _OutChannel._DeferredQ.Next(BlockRef);
       if (!Result1)
       {
           return STATUS_NO_MORE_ENTRIES;
       }
       return STATUS_SUCCESS;
    }
    return STATUS_INTERNAL_ERROR;
}

KSerializer::KSerializer()
    :   _OutChannel(GetThisAllocator())
{
    _Iterator = 0;
    _MetaBlockCount = 0;
    _DataBlockCount = 0;
}

KSerializer::~KSerializer()
{
}



///////////////////////////////////////////////////////////////////////////////
//
// KDeserializer
//

KDeserializer::KDeserializer()
    :   _InChannel(GetThisAllocator())
{

}

KDeserializer::~KDeserializer()
{

}


NTSTATUS
KDeserializer::UntypedDeserialize(
    __in PVOID UserContext,
    __in PfnKConstruct   ConstructorPtr,
    __in PfnKDeserialize DeserializerPtr,
    __out VOID*& TheObject
    )
{

    _InChannel.ResetCursors();
    _InChannel.UserContext = UserContext;
    TheObject = nullptr;
    NTSTATUS Result = ConstructorPtr(_InChannel, TheObject);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }
    DeserializerPtr(_InChannel, TheObject);
    return _InChannel.Status();
}


NTSTATUS
KDeserializer::LoadBlock(
    __in SerialBlockType Type,
    __in KMemRef const& BlockRef
    )
{
    NTSTATUS Result;

    if (Type == eBlockTypeMeta)
    {
        Result = _InChannel.Meta.Write(BlockRef._Param, BlockRef._Address);
        return Result;
    }
    else if (Type == eBlockTypeData)
    {
        Result = _InChannel.Data.Write(BlockRef._Param, BlockRef._Address);
        return Result;
    }

    return STATUS_INVALID_PARAMETER_1;
}

//  KDeserializer::HasDeferredBlocks
//
//
BOOLEAN
KDeserializer::HasDeferredBlocks()
{
   _InChannel._DeferredQ.ResetCursor();
   return _InChannel._DeferredQ.Count() ? TRUE : FALSE;
}

// NextDeferredBlock
//

NTSTATUS
KDeserializer::NextDeferredBlock(
    KMemRef& Destination
    )
{
    BOOLEAN Result = _InChannel._DeferredQ.Next(Destination);
    if (Result == FALSE)
    {
        return STATUS_NO_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//
// _KTransferAgent
//


// _KTransferAgent::Read
//
// Reads all the data blocks from the serializer and makes
// internal copies.   The serializer can be deallocated once
// this completes.
//
NTSTATUS
_KTransferAgent::Read(
    __in KSerializer& Ser
    )
{
    NTSTATUS Result;

    for (Ser.Reset(eBlockTypeMeta);;)
    {
        KMemRef Src;
        Result = Ser.NextBlock(Src);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        Result = _Meta.Write(Src._Param, Src._Address);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
    }

    for (Ser.Reset(eBlockTypeData);;)
    {
        KMemRef Src;
        Result = Ser.NextBlock(Src);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        Result = _Data.Write(Src._Param, Src._Address);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

    }

    for (Ser.Reset(eBlockTypeDeferred);;)
    {
        KMemRef Src;
        Result = Ser.NextBlock(Src);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        Result = _BlockData.Write(Src._Param, Src._Address);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
_KTransferAgent::GetMessageId(
    __out KGuid& MsgId
    )
{
    NTSTATUS Result;

    // Read the GUID as the first 16 bytes of the InChannel
    //
    _Data.ResetCursor();
    ULONG Size = _Data.Size();
    if (Size < sizeof(GUID))
    {
        // This should not have happened.
        // Something went wrong well before this.
        //
        return STATUS_INTERNAL_ERROR;
    }

    // Read the GUID
    //
    ULONG TotalRead = 0;
    Result = _Data.Read(sizeof(GUID), &MsgId, &TotalRead);
    if (TotalRead != sizeof(GUID))
    {
        return STATUS_INTERNAL_ERROR;
    }
    _Data.ResetCursor();
    return STATUS_SUCCESS;
}

// Destructor
//
//
_KTransferAgent::~_KTransferAgent()
{
    Cleanup();
}

// Cleanup
//
//
VOID
_KTransferAgent::Cleanup()
{
    _Meta.Reset();
    _Data.Reset();
    _BlockData.Reset();
}

//  _KTransferAgent::WritePass1
//
//
NTSTATUS
_KTransferAgent::WritePass1(
    __in KDeserializer& Deser
    )
{
    NTSTATUS Res;

    // Process the metadata blocks.
    //
    ULONG MetaBlocks = _Meta.GetBlockCount();
    KMemRef Mem;

    for (ULONG i = 0; i < MetaBlocks; i++)
    {
        Res = _Meta.GetBlock(i, Mem);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
        Res = Deser.LoadBlock(
            eBlockTypeMeta,
            Mem
            );
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
    }

    ULONG DataBlocks = _Data.GetBlockCount();

    for (ULONG i = 0; i < DataBlocks; i++)
    {
        Res = _Data.GetBlock(i, Mem);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
        Res = Deser.LoadBlock(
            eBlockTypeData,
            Mem
            );
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }
    }

    return STATUS_SUCCESS;
}

// _KTransferAgent::WritePass2
//
//
NTSTATUS
_KTransferAgent::WritePass2(
    __in KDeserializer& Deser
    )
{
    // If there is no second pass required, we're done already.
    //
    if (Deser.HasDeferredBlocks() == FALSE)
    {
        return STATUS_SUCCESS;
    }

    // Tranfer stuff from the _BlockData channel into the
    // object buffers
    //
    NTSTATUS Res;
    KMemRef Mem;
    _BlockData.ResetCursor();


    for (;;)
    {
       Res = Deser.NextDeferredBlock(Mem);
       if (Res == STATUS_NO_MORE_ENTRIES)
       {
           break;
       }
       if (!NT_SUCCESS(Res))
       {
            return Res;
       }
       ULONG TotalRead = 0;
       Res = _BlockData.Read(Mem._Size, Mem._Address, &TotalRead);
       if (!NT_SUCCESS(Res) || TotalRead != Mem._Size)
       {
            return Res;
       }
    }
    return STATUS_SUCCESS;
}

//
// KMessageTypeInfo methods.
//

NTSTATUS
KMessageTypeInfo::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KMessageTypeInfo::SPtr& Result
    )
{
    Result = _new(AllocationTag, Allocator) KMessageTypeInfo();
    return (Result == nullptr )? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS;
}

KMessageTypeInfo::KMessageTypeInfo()
{
    RtlZeroMemory(&_MessageTypeId, sizeof(_MessageTypeId));

    _KConstruct = nullptr;
    _KDeserializer = nullptr;
    _KSerializer = nullptr;
    _KDestruct = nullptr;
    _KCastToSharedBase = nullptr;

    _UserContext = nullptr;
}

KMessageTypeInfo::~KMessageTypeInfo()
{
}

//
// KMessageTypeInfoTable methods.
//

NTSTATUS
KMessageTypeInfoTable::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KMessageTypeInfoTable::SPtr& Result,
    __in ULONG HashTableSize
    )
{
    Result = _new(AllocationTag, Allocator) KMessageTypeInfoTable(AllocationTag, HashTableSize);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result = nullptr;
    }

    return status;
}

KMessageTypeInfoTable::KMessageTypeInfoTable(
    __in ULONG AllocationTag,
    __in ULONG HashTableSize
    ) :
    _AllocationTag(AllocationTag),
    _TypeTable(GetThisAllocator())
{
    NTSTATUS status = _TypeTable.Initialize(HashTableSize, K_DefaultHashFunction);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

KMessageTypeInfoTable::~KMessageTypeInfoTable()
{
}

