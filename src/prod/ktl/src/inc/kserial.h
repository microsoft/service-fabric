/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    kserial.h

    Description:
      Kernel Tempate Library (KTL): Serialization Helpers

      Defines classes for serialization use by FDL compiler.

      Redefining the implementation of these allows us to
      move between WinFab and memcpy serialization.

    History:
      raymcc          15-Mar-2011         First working version

--*/

#pragma once

#define K_TRANSFER_HELPER \
    NTSTATUS _KIoChannelTransfer(__in KInChannel&  Dest,__in KOutChannel& Src, __in KAllocator& Alloc);


struct KOutChannel;
struct KInChannel;

// KOutChannel
//
// An abstraction of an outbound network channel.  This is the primary
// serialization wrapper for KTL.   This is intended to wrap either
// memcpy or WinFab serialization, depending on how it is compiled.
//
// This is intentionally a struct so that the various overloads of <<
// can get at the KMemChannel without having to make every class in
// the universe a 'friend'.
//
struct KOutChannel : public KObject<KOutChannel>
{
    KOutChannel(KAllocator& Alloc)
		:	_Allocator(Alloc),
			_DeferredQ(Alloc),
			Meta(Alloc),
			Data(Alloc)
	{
	}

    KMemChannel     Meta;        // Serialization metadata
    KMemChannel     Data;        // Primary by-copy memory stream

    VOID Enqueue(__in KMemRef& Mem);  // Enqueu a ref to a large block
    VOID Log(__in NTSTATUS) const;

    VOID
    ResetCursors()
    {
        Meta.ResetCursor();
        Data.ResetCursor();
        _DeferredQ.ResetCursor();
    }

	KAllocator&
	Allocator()
	{
		return _Allocator;
	}

    KQueue<KMemRef> _DeferredQ;    // Large memory blocks are sent lazily, by ref

private:
    K_DENY_COPY(KOutChannel);
    friend K_TRANSFER_HELPER;
    friend class KSerializer;

	KAllocator&		_Allocator;
};

// POD Serializer
//
// Supports simple POD types for a KMemChannel
//
template <typename T>
inline KMemChannel& operator <<(
    __in KMemChannel& Ch,
    __in T const & Val
    )
{
    Ch.Write(sizeof(T), &Val);
    return Ch;
};

//  POD Serializer
//
//  Default serializer for KOutChannel
//
template <typename T>
inline KOutChannel& operator <<(
    __in KOutChannel& Out,
    __in T const & Val
    )
{
    Out.Data.Write(sizeof(T), &Val);
    return Out;
}

//
//  Serializer for blobs
//
inline KOutChannel& operator << (
    __in KOutChannel& Out,
    __in KMemRef const & MRef
    )
{
    Out.Data.Write(MRef._Size, MRef._Address);
    return Out;
}

//
//  Serializer for blobs for KMemChannel
//
inline KMemChannel& operator << (
    __in KMemChannel& Ch,
    __in KMemRef const & MRef
    )
{
    Ch.Write(MRef._Size, MRef._Address);
    return Ch;
}

//
//  Default serializer for KArray<T>
//
template <typename T>
inline KOutChannel& operator << (KOutChannel& Out, KArray<T> const & Array)
{
    ULONG Count = Array.Count();
    Out << Count;
    if (Count)
    {
        for (ULONG i = 0; i < Count; i++)
        {
            Out << Array[i];
        }
    }
    return Out;
}


//
//  Serializer for PUNICODE_STRING
//
//  We only serialize the 'real' length of the string,
//  not including null or extra allocation space.
//
inline KMemChannel&
operator << (
    __in KMemChannel& Ch,
    __in const PUNICODE_STRING PStr
    )
{
    USHORT Length = PStr->Length;

    Ch.Write(sizeof(USHORT), &Length);

    if (Length)
    {
        Ch.Write(Length, PStr->Buffer);
    }
    return Ch;
}


//
//  Serializer for PUNICODE_STRING
//  maps down to the generic KMemChannel one.
//
inline KOutChannel&
operator << (
    __in KOutChannel& Out,
    __in const PUNICODE_STRING PStr
    )
{
    Out.Data << PStr;
    return Out;
}

// Serializer for KWString
//
inline KOutChannel&
operator <<(
    __in KOutChannel& Out,
    __in KWString& Str)
{
    Out.Data << PUNICODE_STRING(Str);
    return Out;
}


//
//  Serializer for KSharedPtr<T>
//
template <typename T>
inline KOutChannel&
operator << (
    __in KOutChannel& Out,
    __in KSharedPtr<T>& Ptr
    )
{
    BOOLEAN ObjectPresent = FALSE;
    if (Ptr)
    {
       ObjectPresent = TRUE;
    }
    Out << ObjectPresent;
    if (ObjectPresent)
    {
        Out << *Ptr;
    }
    return Out;
}


//
//  GUID holder for the message class GUID
//
struct KMessageGUID
{
    KGuid _GuidVal;
    KMessageGUID(const GUID& Src) { _GuidVal = Src; }
};


//
// Default serializer for POD pointer types
//
template <typename T>
inline KOutChannel& operator <<(KOutChannel& Out, const T* Val)
{
    BOOLEAN ObjectPresent = FALSE;
    if (Val)
    {
        ObjectPresent = TRUE;
        Out << ObjectPresent;
        Out.Data.Write(sizeof(T), Val);
        Out.SetStatus(Result);
    }
    else
    {
        ObjectPresent = FALSE;
    }
    return Out;
}

KMemChannel& operator<<(
    __in KMemChannel& Out,
    __in KVariant const & Var
    );

inline KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KVariant const & Var
    )
{
    Out.Data << Var;
    return Out;
}

//
// KIoBuffer serializer declaration
//
// Implementation in .cpp file
//
KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KIoBuffer::SPtr& Buf
    );

//
// KBuffer serializer declaration
//
// Implementation in .cpp file
//
KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KBuffer::SPtr& Buf
    );

//
// Serializer for KIDomNode and KIMutableDomNode
//
// Implementation in .cpp file
//

class KIDomNode;
class KIMutableDomNode;

KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KSharedPtr<KIDomNode>& Dom
    );

KOutChannel& operator<<(
    __in KOutChannel& Out,
    __in KSharedPtr<KIMutableDomNode>& Dom
    );

// KInChannel
//
// An abstraction of an inbound network channel.  This is the mirror
// image of the KOutChannel.
//
struct KInChannel : public KObject<KInChannel>
{
    using KObject::SetStatus;

    KInChannel(KAllocator& Alloc)
		:	_Allocator(Alloc),
			_DeferredQ(Alloc),
			Meta(Alloc),
			Data(Alloc)
    {
        UserContext = nullptr;
    }

    KMemChannel     Meta;           // No underscore because it is directly addressable
    KMemChannel     Data;           //

    VOID  Enqueue(__in KMemRef const & MemRef);
    VOID  Log(__in NTSTATUS) const;
    PVOID UserContext;

    VOID
    ResetCursors()
    {
        Meta.ResetCursor();
        Data.ResetCursor();
        _DeferredQ.ResetCursor();
    }

    // Reads the first 16 bytes of Data and then resets the cursor.
    // Fails if the channel doesn't have at least 16 bytes in it.
    //
    NTSTATUS
    ReadMessageId(
        __out GUID& Id
        ) const;

	KAllocator&
	Allocator()
	{
		return _Allocator;
	}

    KQueue<KMemRef> _DeferredQ;      // Read back large blocks which have been received

private:
    K_DENY_COPY(KInChannel);

    friend K_TRANSFER_HELPER;
    friend class KDeserializer;

	KAllocator&		_Allocator;
};

//
// Default deserializer for most POD types
//
template <typename T>
inline KInChannel& operator >>(
    __in KInChannel& In,
    __in T& Val
    )
{
    In.Data.Read(sizeof(T), &Val);
    return In;
}

//
//  Default deserializer for blobs.
//
//  The caller must allocate
//  the receiving buffer for the 'Ref' object.
//
inline KInChannel& operator >>(
    __in KInChannel& In,
    __in KMemRef& MRef
    )
{
    In.Data.Read(MRef._Size, MRef._Address, &MRef._Param);
    return In;
}

inline KInChannel& operator >>(
    __in KInChannel& In,
    __in KMemRef&& MRef
    )
{
    In.Data.Read(MRef._Size, MRef._Address, &MRef._Param);
    return In;
}

//
//  Default deserializer for blobs.
//
//  The caller must allocate
//  the receiving buffer for the 'Ref' object.
//
inline KMemChannel& operator >>(
    __in KMemChannel& Ch,
    __in KMemRef& MRef
    )
{
    Ch.Read(MRef._Size, MRef._Address, &MRef._Param);
    return Ch;
}


//
//  Deserializer for PUNICODE_STRING
//
//  This implementation allocates the space for the string.
//
inline KMemChannel&
operator >> (
    __in  KMemChannel& Ch,
    __out PUNICODE_STRING Str
    )
{
    USHORT Length = 0;
    Ch.Read(sizeof(USHORT), &Length);
    if (Length)
    {
       // We will superimpose a null terminator.
       //
       Str->MaximumLength = Length + 2;
       Str->Length = Length;

	   HRESULT hr;
	   USHORT result;
	   hr = UShortAdd(Length, 2, &result);
	   KInvariant(SUCCEEDED(hr));
	   Str->Buffer = PWSTR(_newArray<UCHAR>(KTL_TAG_SERIAL, Ch.Allocator(), result));
       RtlZeroMemory(Str->Buffer, Length + 2);

       if (Str->Buffer == nullptr)
       {
            Ch.Log(STATUS_INSUFFICIENT_RESOURCES);
            return Ch;
       }
       Ch.Read(Length, Str->Buffer);
    }
    else
    {
        Str->Length = 0;
        Str->Buffer = nullptr;
        Str->MaximumLength = 0;
    }
    return Ch;
}


inline KInChannel&
operator >>(
    __in KInChannel& In,
    __in PUNICODE_STRING Str
    )
{
    In.Data >> Str;
    return In;
}

inline KInChannel&
operator >>(
    __in KInChannel& In,
    __in KWString& Str
    )
{
    UNICODE_STRING Tmp;
    In.Data >> &Tmp;
    Str.Acquire(Tmp);
    return In;
}



// Support simple POD types for Meta & Ref streams.
//
template <typename T>
inline KMemChannel& operator >>(
    __in KMemChannel& Ch,
    __in const T& Val
    )
{
    Ch.Read(sizeof(T), PVOID(&Val));
    return Ch;
};

//
// Default deserializer for POD pointer types.
//
// The target memory must already exist.
//
template <typename T>
inline KInChannel& operator >>(
    __in    KInChannel& In,
    __inout T* Val
    )
{
    BOOLEAN ObjectPresent = FALSE;
    In >> ObjectPresent;
    if (ObjectPresent && Val)
    {
        In.Data.Read(sizeof(T), Val);
    }
    return In;
}

//
//  Default KSharedPtr<T> deserializer.
//
template <typename T>
inline KInChannel& operator >> (
    __in KInChannel& In,
    __in KSharedPtr<T>& Ptr
    )
{
    // Read the stream to see if there is an object there or not.
    BOOLEAN ObjectPresent = FALSE;
    In >> ObjectPresent;
    if (ObjectPresent)
    {
        T* NewPtr = _new(KTL_TAG_SERIAL, In.Allocator()) T();
        if (!NewPtr)
        {
            In.Log(STATUS_INSUFFICIENT_RESOURCES);
            return In;
        }
        Ptr = NewPtr;
        In >> *Ptr;
    }
    else
    {
        Ptr = NULL;
    }
    return In;
}

//
//  Default KArray<T> deserializer
//
//
template <typename T>
inline KInChannel& operator >> (
    __in KInChannel& In,
    __in KArray<T>& Array
    )
{
    ULONG Count;
    In >> Count;
    if (Count)
    {
        for (ULONG i = 0; i < Count; i++)
        {
            T tmp;
            In >> tmp;
            Array.Append(tmp);  // TBD: Record error if failed
        }
    }
    return In;
}

KMemChannel& operator >>(
    __in    KMemChannel& In,
    __inout KVariant& Var
    );

//
// Deserializer for KVariant
//
inline KInChannel& operator>>(
    __in    KInChannel& In,
    __inout KVariant& Var
    )
{
    In.Data >> Var;
    return In;
}

//
//  Deserializer for KMessageGUID.
//
//  This deserializes and verifies that the specified GUID was found
//  in the input channel.
//
inline KInChannel& operator>>(
    __in KInChannel& In,
    __in KMessageGUID const & Check
    )
{
    KGuid Test;
    In >> Test;
    if (Test != Check._GuidVal)
    {
        In.Log(STATUS_OBJECT_TYPE_MISMATCH);
    }
    return In;
}

//
// KIoBuffer deserializer declaration
//
// Implementation in .cpp file
//
KInChannel& operator>>(
    __in KInChannel& In,
    __in KIoBuffer::SPtr& Buf
    );

//
// KBuffer deserializer declaration
//
// Implementation in .cpp file
//
KInChannel& operator>>(
    __in KInChannel& In,
    __in KBuffer::SPtr& Buf
    );

//
// Deserializer for KIDomNode and KIMutableDomNode
//
// Implementation in .cpp file
//

KInChannel& operator>>(
    __in    KInChannel& In,
    __inout KSharedPtr<KIDomNode>& Dom
    );

KInChannel& operator>>(
    __in    KInChannel& In,
    __inout KSharedPtr<KIMutableDomNode>& Dom
    );

//
//  _KIoChannelTransfer
//
//  This is an internal system helper which does a direct transfer from KOutChannel to a
//  corresponding KInChannel and is used for testing, in-memory transport simulation, etc.
//
NTSTATUS
_KIoChannelTransfer(
    __in KInChannel&  Dest,
    __in KOutChannel& Src,
    __in KAllocator& Alloc
    );

// _KMemChannelTransfer
//
// This is an internal system helper which transfers ownership of memory blocks between
// two KMemChannels.
//
NTSTATUS
_KMemChannelTransfer(
    __in KMemChannel& Dest,
    __in KMemChannel& Src,
    __in KAllocator& Alloc
    );


typedef enum { eBlockTypeNull = 0, eBlockTypeMeta = 1, eBlockTypeData = 2, eBlockTypeDeferred = 3 } SerialBlockType;


// KSerializer
//
// Rootmost serialization object
//
class KSerializer : public KShared<KSerializer>, public KObject<KSerializer>
{
public:
    typedef KSharedPtr<KSerializer> SPtr;
    typedef KUniquePtr<KSerializer> UPtr;

    KSerializer();
   ~KSerializer();

    // Serialize
    //
    // Causes a serialization run-down of the specfied object.  This recursively invokes
    // all available serialization sequences for the object and its children.
    //
    template <class T>
    NTSTATUS
    Serialize(
        __in T& Object
        )
        {
            _OutChannel << Object;
            return _OutChannel.Status();
        }

    // GetSizes
    //
    // Returns the total number of bytes of each type of serialization data.
    //
    // Return value:
    //    STATUS_SUCCESS
    //    STATUS_NO_DATA_DETECTED if this is called before serialization.
    //
    NTSTATUS GetSizes(
        __out ULONG& MetaBytes,
        __out ULONG& DataBytes,
        __out ULONG& DeferredBytes
        );

    // Reset
    //
    // After serialization is complete, this is called to start an iteration over
    // all the serialized data.   In practice, the caller should use the eBlockTypeMeta
    // first and get all the metadata blocks.  Next, the caller should retrieve
    // the eBlockTypeData blocks.  Finally, the caller retrieves the eBlockTypeDeferred
    // blocks.
    //
    // There is no formal hook between this class and any signals to the objects
    // being deserialized so that they can release resources.  That task should be
    // handled by higher-level objects, such as network/memory transports, etc.
    //
    // Reset wipes all cursor information for all block types.  It is not possible
    // to maintain separate iteration positions on each of the block types.
    //
    //
    // Parameters:
    //
    //  Filter      The block type over which to iterate using NextBlock.
    //
    // Return values:
    //      STATUS_SUCCESS             The reset was completed.
    //      STATUS_INVALID_PARAMTER_1  Invalid value for Filter specified.
    //
    NTSTATUS
    Reset(
        __in SerialBlockType Filter
        );

    // KSerializer::NextBlock
    //
    // Retrieves the next block of memory in the serialzation sequence.
    // The caller must COPY this memory to a network transport, file
    // or other medium.
    //
    // Parameters:
    //      BlockRef        Receives information about the block.
    //                      The KMemRef._Param field is also set
    //                      to the block type.
    // Return values:
    //  STATUS_SUCCESS          A memory reference was returned.
    //  STATUS_NO_MORE_ENTRIES  When there are no more blocks left in the iteration.
    //
    NTSTATUS
    NextBlock(
        __out KMemRef& BlockRef
        );



    // HasDeferredBlocks
    //
    // This should be called to see if the serializer has deferred blocks
    // to serialize or not. If not, then the object was fully 'copied'
    // during serialization and can be released when serialization is completed.
    // If not, then the object cannot be released until network transmission has completed.
    //
    // Return value:
    //
    //    If TRUE, the serialized object contains deferred blocks.
    //    If FALSE, the object has no deferred blocks.
    //
    BOOLEAN
    HasDeferredBlocks();


private:
    ULONG       _Iterator;
    ULONG       _FilterType;
    ULONG       _DataBlockCount;
    ULONG       _MetaBlockCount;
    KOutChannel _OutChannel;
};

///////////////////////////////////////////////////////////////////////////////
//
// KSerializationBaseHeader
//
//
#define KSerialization_Base_Signature       0xBBB000000000000

struct KSerializationBaseHeader
{
    ULONGLONG   _Control;

    ULONG       _MetaLength;
    ULONG       _DataLength;
    ULONG       _DeferredLength;
    ULONG       _Reserved1;
};

// KSerializeToBuffer
//
// This function serializes a small object into a contiguous buffer.
// If the supplied buffer is not large enough to hold the serialized data,
// this function returns STATUS_BUFFER_TOO_SMALL. SerializedBuffer._Param
// is set to the serialized size when the return status is either
// STATUS_SUCCESS or STATUS_BUFFER_TOO_SMALL. One can use a zero-sized buffer
// to learn the serialized size.
//
// Parameters:
//      Object              The object to be serialized.
//      SerializedBuffer    Returns the serialized buffer.
//      Allocator           Supplies the allocator.
//      AllocationTag       Supplies the allocation tag.
//
// Return values:
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_SUCCESS
//      STATUS_BUFFER_TOO_SMALL
//

template <class T>
NTSTATUS
KSerializeToBuffer(
    __in T const & Object,
    __out KMemRef& SerializedBuffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Serialize this object into a KOutChannel.
    //

    KSerializer::SPtr Sz = _new(AllocationTag, Allocator) KSerializer();
    if (!Sz)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Sz->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = Sz->Serialize(Object);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KSerializationBaseHeader Header;
    RtlZeroMemory(&Header, sizeof(Header));

    Header._Control = KSerialization_Base_Signature;

    status = Sz->GetSizes(Header._MetaLength, Header._DataLength, Header._DeferredLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    const ULONG serializedSize = sizeof(Header) + Header._MetaLength + Header._DataLength + Header._DeferredLength;
    SerializedBuffer._Param = serializedSize;

    if (SerializedBuffer._Size < serializedSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Copy the Header.
    //

    PUCHAR p = (PUCHAR)SerializedBuffer._Address;
    UCHAR* const endOfBuffer = (PUCHAR)SerializedBuffer._Address + SerializedBuffer._Size;

    KMemCpySafe(p, (ULONG)(endOfBuffer - p), &Header, sizeof(Header));
    p += sizeof(Header);

    //
    // Copy memory blocks from the serialized channels to the buffer.
    //

    for (Sz->Reset(eBlockTypeMeta);;)
    {
        KMemRef BlockRef;
        status = Sz->NextBlock(BlockRef);
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        if (p + BlockRef._Param <= endOfBuffer)
        {
            KMemCpySafe(p, BlockRef._Param, BlockRef._Address, BlockRef._Param);
            p += BlockRef._Param;
        }
        else
        {
            KAssert(!"Invalid data blocks");
            return STATUS_DATA_ERROR;
        }
    }

    for (Sz->Reset(eBlockTypeData);;)
    {
        KMemRef BlockRef;
        status = Sz->NextBlock(BlockRef);
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        if (p + BlockRef._Param <= endOfBuffer)
        {
            KMemCpySafe(p,  BlockRef._Param, BlockRef._Address, BlockRef._Param);
            p += BlockRef._Param;
        }
        else
        {
            KAssert(!"Invalid data blocks");
            return STATUS_DATA_ERROR;
        }
    }

    for (Sz->Reset(eBlockTypeDeferred);;)
    {
        KMemRef BlockRef;
        status = Sz->NextBlock(BlockRef);
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        if (p + BlockRef._Size <= endOfBuffer)
        {
            KMemCpySafe(p, BlockRef._Size, BlockRef._Address, BlockRef._Size);
            p += BlockRef._Size;
        }
        else
        {
            KAssert(!"Invalid data blocks");
            return STATUS_DATA_ERROR;
        }
    }

    return STATUS_SUCCESS;
}


//
// This is a wrapper on top of KSerializeToBuffer() for KBuffer.
//

template <class T>
NTSTATUS
KSerializeToBuffer(
    __in T const & Object,
    __out KBuffer& SerializedBuffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KMemRef BlockRef;

    //
    // Use a zero-sized memory block ref to get the serialized size.
    //

    BlockRef._Address = nullptr;
    BlockRef._Param = BlockRef._Size = 0;

    status = KSerializeToBuffer(Object, BlockRef, Allocator, AllocationTag);

    //
    // This call must not succeed. There is at least a header in the serialized buffer.
    //
    KFatal(!NT_SUCCESS(status));
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        return status;
    }

    //
    // Set the output buffer to the serialized size.
    //

    status = SerializedBuffer.SetSize(BlockRef._Param, FALSE, AllocationTag);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    BlockRef._Address = SerializedBuffer.GetBuffer();
    BlockRef._Size = SerializedBuffer.QuerySize();

    //
    // Now serialize the object with the allocated buffer.
    //

    status = KSerializeToBuffer(Object, BlockRef, Allocator, AllocationTag);
    return status;
}

//  Type-erased pointers to constructor & deserializer, and destructor
//
typedef NTSTATUS (*PfnKConstruct)(KInChannel& In, VOID*& _This);
typedef KInChannel& (*PfnKDeserialize)(KInChannel& In, VOID* _This);
typedef VOID (*PfnKDestruct)(VOID* _This);
typedef KSharedBase* (*PfnKCastToSharedBase)(VOID* _This);

//  Type-erased pointers to serializer
//  The type UCHAR is used to work around the issue of no VOID& in C++.
//  Since all objects should be aligned at least at UCHAR boundary, it is OK
//  to cast any T& to UCHAR&.
//
typedef KOutChannel& (*PfnKSerialize)(KOutChannel& Out, UCHAR& _This);

// KDeserializer
//
// Rootmost deserialization object.
//
// This object contains no signalling mechanisms to let the caller
// know when the deserialization is complete.  That is the job
// of higher-level objects which use this.
//
// Deserialization is logically complete when Deserialize
// has returned successfully AND when any Deferred blocks have
// been filled with actual data items.
//
class KDeserializer : public KShared<KDeserializer>, public KObject<KDeserializer>
{
public:
    typedef KSharedPtr<KDeserializer> SPtr;
    typedef KUniquePtr<KDeserializer> UPtr;


    KDeserializer();
   ~KDeserializer();

    // LoadBlock
    //
    // After construction, this should be called to load all the
    // blocks for the Meta and Data streams.
    //
    // First, all the eBlockTypeMeta blocks should be loaded
    // in the same order they were rerieved from the KSerializer.
    // Next, all the eBlockTypeData blocks should be loaded.
    //
    // Once these steps are completed, the Deserialize can be called.
    //
    // Parameters:
    //      Type        The block type being loaded.
    //      BlockRef    A reference to a block of memory of the specified type.
    //                  The memory is copied.
    //
    // Return values:
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_SUCCESS
    //
    NTSTATUS LoadBlock(
        __in SerialBlockType Type,
        __in KMemRef const & BlockRef
        );

    // Deserialize
    //
    // Deserializes an object of the specified type using the
    // data previously supplied by calling LoadBlock in the correct
    // sequence.
    //
    // This is actually just "Phase 1" of deserialization.
    // Once this call completes, then there may be deferred
    // blocks that have to be filled.
    //
    template <class T>
    NTSTATUS
    Deserialize(
        __in  PVOID UserContext,
        __out T*& Object
        )
        {
            _InChannel.ResetCursors();

            _InChannel.UserContext = UserContext;
            Object = nullptr;
            NTSTATUS Result = __KConstruct(_InChannel, Object);
            if (!NT_SUCCESS(Result))
            {
                return Result;
            }
            _InChannel >> *Object;
            return _InChannel.Status();
        }

    // UntypedDeserialize
    //
    // Invokes the deserialization process via the original pointers to K_CONSTRUCT
    // and K_DESERIALIZE.  The pointers can be captured via K_CONSTRUCT_REF
    // and K_DESERIALIZE_REF macros.
    //
    // The resulting object must be cast to the
    // correct final type by the caller.
    //
    NTSTATUS
    UntypedDeserialize(
        __in PVOID UserContext,
        __in PfnKConstruct   ConstructorPtr,
        __in PfnKDeserialize DeserializerPtr,
        __out VOID*& TheObject
        );

    // HasDeferredBlocks
    //
    // This should be called to see if the deserialized block has
    // deferred blocks that are waiting to be filled by another
    // pass.
    //
    // Return value:
    //
    //    If TRUE, the caller must call NextDeferredBlock one or more times.
    //    If FALSE, the object should be considered fully deserialized.
    //
    BOOLEAN
    HasDeferredBlocks();

    // NextDeferredBlock
    //
    // If the object has deferred data blocks that need to be filled from
    // an outside source, then calling this repeatedly will yield all
    // the buffer addresses and sizes that need to be filled to complete
    // deserialization.
    //
    // The data to be supplied into these blocks it supplied by an
    // external transport or other medium.
    //
    // On return, KMemRef._Size is how many bytes are needed,
    // and KMemRef._Address is where to put them.
    //
    NTSTATUS
    NextDeferredBlock(
        __out KMemRef& Destination
        );


private:
    KInChannel _InChannel;
};

// KDeserializeFromBuffer
//
// This function deserializes a small object from a contiguous buffer.
// The buffer must have been filled using KSerializeToBuffer().
//
// Parameters:
//      SerializedBuffer    Supplies the serialized buffer. SerializedBuffer._Param must be the serialized size in the buffer.
//      UserContext         The opaque context to be supplied to K_CONSTRUCT() call to the user.
//      Object              The deserialized object.
//      Allocator           Supplies the allocator.
//      AllocationTag       Supplies the allocation tag.
//
// Return values:
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_SUCCESS
//      STATUS_BUFFER_TOO_SMALL
//      STATUS_DATA_ERROR
//

template<class T>
NTSTATUS
KDeserializeFromBuffer(
    __in KMemRef& SerializedBuffer,
    __in  PVOID UserContext,
    __out T*& Object,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    Object = nullptr;

    ULONG ExpectedSize = sizeof(KSerializationBaseHeader);
    if (SerializedBuffer._Param < ExpectedSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    KSerializationBaseHeader const* HeaderPtr = (KSerializationBaseHeader const*)SerializedBuffer._Address;
    ExpectedSize += HeaderPtr->_MetaLength + HeaderPtr->_DataLength + HeaderPtr->_DeferredLength;

    if (SerializedBuffer._Param < ExpectedSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (HeaderPtr->_Control != KSerialization_Base_Signature)
    {
        return STATUS_DATA_ERROR;
    }

    KDeserializer::SPtr Dz = _new(AllocationTag, Allocator) KDeserializer();
    if (!Dz)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Dz->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Load meta data blocks.
    //

    PUCHAR p = (PUCHAR)SerializedBuffer._Address;
    UCHAR* const endOfBuffer = (PUCHAR)SerializedBuffer._Address + SerializedBuffer._Param;

    p += sizeof(KSerializationBaseHeader);  // Skip the header at the beginning of the buffer

    KMemRef BlockRef;

    BlockRef._Address = p;
    BlockRef._Size = BlockRef._Param = HeaderPtr->_MetaLength;

    if ((PUCHAR)BlockRef._Address + BlockRef._Param <= endOfBuffer)
    {
        status = Dz->LoadBlock(eBlockTypeMeta, BlockRef);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        p += BlockRef._Param;
    }
    else
    {
        KAssert(!"Invalid data blocks");
        return STATUS_DATA_ERROR;
    }

    //
    // Load data blocks.
    //

    BlockRef._Address = p;
    BlockRef._Size = BlockRef._Param = HeaderPtr->_DataLength;

    if ((PUCHAR)BlockRef._Address + BlockRef._Param <= endOfBuffer)
    {
        status = Dz->LoadBlock(eBlockTypeData, BlockRef);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        p += BlockRef._Param;
    }
    else
    {
        KAssert(!"Invalid data blocks");
        return STATUS_DATA_ERROR;
    }

    //
    // Deserialize the object.
    //

    status = Dz->Deserialize(UserContext, Object);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Now fill the deferred blocks if any.
    //

    if (Dz->HasDeferredBlocks())
    {
        for (;;)
        {
            status = Dz->NextDeferredBlock(BlockRef);
            if (status == STATUS_NO_MORE_ENTRIES)
            {
               break;
            }

            if (p + BlockRef._Size <= endOfBuffer)
            {
                KMemCpySafe(BlockRef._Address, BlockRef._Size, p, BlockRef._Size);
                p += BlockRef._Size;
            }
            else
            {
                KAssert(!"Invalid data blocks");
                return STATUS_DATA_ERROR;
            }
        }
    }

    return STATUS_SUCCESS;
}

//
// This is a wrapper on top of KDeserializeFromBuffer() for KBuffer.
//

template<class T>
NTSTATUS
KDeserializeFromBuffer(
    __in KBuffer& SerializedBuffer,
    __in  PVOID UserContext,
    __out T*& Object,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KMemRef BlockRef;

    BlockRef._Address = SerializedBuffer.GetBuffer();
    BlockRef._Param = BlockRef._Size = SerializedBuffer.QuerySize();

    status = KDeserializeFromBuffer(BlockRef, UserContext, Object, Allocator, AllocationTag);
    return status;
}

// _KTransferAgent
//
// This is an internal system helper to do an in-memory transfer between a serializer
// and a deserializer.  It is primarily for testing and debugging.
//
//
class _KTransferAgent : public KObject<_KTransferAgent>
{
    K_DENY_COPY(_KTransferAgent);

public:
    _KTransferAgent(KAllocator& Alloc)
		:	_Allocator(Alloc),
			_Meta(Alloc),
			_Data(Alloc),
			_BlockData(Alloc)
	{
	}

   ~_KTransferAgent();

    NTSTATUS
    Read(
        __in KSerializer& Ser
        );

    NTSTATUS
    WritePass1(
        __in KDeserializer& Deser
        );

    // WritePass2
    //
    // Transfers any large blocks from the transfer agent to the Deserialized object.
    // This call may not be necessary.  The caller should check KDeserializer::HasDeferredBlocks
    // to determine if this call needs to be made.
    //
    NTSTATUS
    WritePass2(
        __in KDeserializer& Deser
        );

    VOID
    Cleanup();

    NTSTATUS
    GetMessageId(
        __out KGuid& MsgId
        );

	KAllocator&
	Allocator()
	{
		return _Allocator;
	}

private:
	KAllocator& _Allocator;
    KMemChannel _Meta;
    KMemChannel _Data;
    KMemChannel _BlockData;
};

/////////////////////////////////////////////////////////////////////////////////////////
//
// Helper macros
//


#define K_SERIALIZABLE(CLASS) \
    friend KOutChannel& operator<<(KOutChannel& Out, CLASS&); \
    friend KInChannel& operator>>(KInChannel& In, CLASS&); \
    friend NTSTATUS __KConstruct(KInChannel&, CLASS*&);

// For types which are messages, but not POD types
//
#define K_SERIALIZE_MESSAGE(CLASS, CLASSGUID) \
    KOutChannel& operator<<(KOutChannel& Out, CLASS& This)

#define K_DESERIALIZE_MESSAGE(CLASS, CLASSGUID) \
    KInChannel& operator>>(KInChannel& In, CLASS& This)

// For types which are subtypes, but not messages, and not POD
//
#define K_SERIALIZE(CLASS) \
    KOutChannel& operator<<(KOutChannel& Out, CLASS& This)

#define K_CONSTRUCT(CLASS) \
    NTSTATUS __KConstruct(KInChannel& In, CLASS*& _This)

#define K_DESTRUCT(CLASS) \
    VOID __KDestruct(CLASS* _This)

#define K_DESERIALIZE(CLASS) \
    KInChannel& operator>>(KInChannel& In, CLASS& This)

//
// This is the generic version of destructing a object.
//
template <class T>
VOID __KDestruct(__in_opt T* Object)
{
    _delete<T>(Object);
}

//
// This template casts a KSharedBase derivation to its base class.
//
template <class T>
KSharedBase* __KCastToSharedBase(__in_opt T* Object)
{
    return static_cast<KSharedBase*>(Object);
}

// Helperes to build type-erased references to equivalent void pointer types
//
#define K_CONSTRUCT_REF(CLASS)  \
    PfnKConstruct((NTSTATUS (*)(KInChannel& In, CLASS*& _This))  __KConstruct)

#define K_DESTRUCT_REF(CLASS)  \
    PfnKDestruct((VOID (*)(CLASS* _This))  __KDestruct)

#define K_CAST_TO_SHARED_BASE_REF(CLASS)  \
    PfnKCastToSharedBase((KSharedBase* (*)(CLASS* _This))  __KCastToSharedBase)

#define K_DESERIALIZE_REF(CLASS)  \
    PfnKDeserialize(((KInChannel& (*)(KInChannel& In, CLASS& _This))  operator >>))

#define K_SERIALIZE_REF(CLASS)  \
    PfnKSerialize(((KOutChannel& (*)(KOutChannel& Out, CLASS& _This))  operator <<))


// Helpers for emitting and checking the GUID for the message
//
#define K_EMIT_MESSAGE_GUID(TheGUID) \
    Out << KMessageGUID(TheGUID);

#define K_VERIFY_MESSAGE_GUID(TheGUID) \
    In >> KMessageGUID(TheGUID);


// Predefined for POD messages
//
#define K_SERIALIZE_POD_MESSAGE(CLASSNAME, CLASSGUID)\
  KOutChannel& operator<<(KOutChannel& Out, CLASSNAME& This) \
  {\
    return Out << KMessageGUID(CLASSGUID) << KMemRef(sizeof(This), &This);\
  }

#define K_DESERIALIZE_POD_MESSAGE(CLASSNAME, CLASSGUID)\
  KInChannel& operator>>(KInChannel& In, CLASSNAME& This)\
  {\
     return In >> KMessageGUID(CLASSGUID) >> KMemRef(sizeof(This), &This);\
  }

//
// This struct contains information for serializaing and deserializing a user-defined type.
//

struct KMessageTypeInfo : public KObject<KMessageTypeInfo>, public KShared<KMessageTypeInfo>
{
    K_FORCE_SHARED(KMessageTypeInfo);

public:

    static
    NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out KMessageTypeInfo::SPtr& Result
        );

    //
    // This method captures the serialization and deserialization function pointers for type T.
    //

    template <class T>
    VOID
    SetType(
        __in GUID& MessageTypeId
        )
    {
        _MessageTypeId = MessageTypeId;

        _KConstruct = K_CONSTRUCT_REF(T);
        _KDeserializer = K_DESERIALIZE_REF(T);
        _KSerializer = K_SERIALIZE_REF(T);
        _KDestruct = K_DESTRUCT_REF(T);
        _KCastToSharedBase = nullptr;
    }

    //
    // This method captures the serialization and deserialization function pointers for type T.
    // T must derive from KSharedBase, i.e. a KShared type.
    //

    template <class T>
    VOID
    SetSharedType(
        __in GUID& MessageTypeId
        )
    {
        SetType<T>(MessageTypeId);
        _KCastToSharedBase = K_CAST_TO_SHARED_BASE_REF(T);
    }

    //
    // Unique id of the user-defined type.
    //
    GUID _MessageTypeId;

    //
    // Type-erased serialization function pointers to a user-defined type.
    //
    PfnKConstruct _KConstruct;
    PfnKDeserialize _KDeserializer;
    PfnKSerialize _KSerializer;
    PfnKDestruct _KDestruct;
    PfnKCastToSharedBase _KCastToSharedBase;

    //
    // An opaque user context pointer.
    // This context can be attached to the In channel during construction and deserialization.
    //
    PVOID _UserContext;
};

//
// This table holds serialization and deserialization information for different types.
//

class KMessageTypeInfoTable : public KObject<KMessageTypeInfoTable>, public KShared<KMessageTypeInfoTable>
{
    K_FORCE_SHARED(KMessageTypeInfoTable);

public:

    static
    NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out KMessageTypeInfoTable::SPtr& Result,
        __in ULONG HashTableSize = 43
        );

    template <class T>
    NTSTATUS
    Put(
        __in GUID& MessageTypeId,
        __in_opt PVOID UserContext,
        __in BOOLEAN ForceUpdate = TRUE
        )
    {
        NTSTATUS status = STATUS_SUCCESS;

        KMessageTypeInfo::SPtr entry;
        status = KMessageTypeInfo::Create(_AllocationTag, GetThisAllocator(), entry);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        entry->SetType<T>(MessageTypeId);
        entry->_UserContext = UserContext;

        K_LOCK_BLOCK(_TableLock)
        {
            status = _TypeTable.Put(MessageTypeId, Ktl::Move(entry), ForceUpdate);
        }

        return status;
    }

    template <class T>
    NTSTATUS
    PutSharedType(
        __in GUID& MessageTypeId,
        __in_opt PVOID UserContext,
        __in BOOLEAN ForceUpdate = TRUE
        )
    {
        NTSTATUS status = STATUS_SUCCESS;

        KMessageTypeInfo::SPtr entry;
        status = KMessageTypeInfo::Create(_AllocationTag, GetThisAllocator(), entry);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        entry->SetSharedType<T>(MessageTypeId);
        entry->_UserContext = UserContext;

        K_LOCK_BLOCK(_TableLock)
        {
            status = _TypeTable.Put(MessageTypeId, Ktl::Move(entry), ForceUpdate);
        }

        return status;
    }

    NTSTATUS
    Get(
        __in const GUID& MessageTypeId,
        __out KMessageTypeInfo::SPtr& Entry
        )
    {
        NTSTATUS status = STATUS_SUCCESS;

        K_LOCK_BLOCK(_TableLock)
        {
            status = _TypeTable.Get(MessageTypeId, Entry);
        }

        return status;
    }

    NTSTATUS
    Remove(
        __in GUID& MessageTypeId
        )
    {
        NTSTATUS status = STATUS_SUCCESS;

        K_LOCK_BLOCK(_TableLock)
        {
            status = _TypeTable.Remove(MessageTypeId);
        }

        return status;
    }

private:

    KMessageTypeInfoTable(
        __in ULONG AllocationTag,
        __in ULONG HashTableSize
        );

    const ULONG _AllocationTag;
    KHashTable<GUID, KMessageTypeInfo::SPtr> _TypeTable;
    KSpinLock _TableLock;
};

