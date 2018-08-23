/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kiobuffer.h

Abstract:

    This file defines the KIoBuffer class.

Author:

    Norbert P. Kusters (norbertk) 1-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KIoBuffer;
class KIoBufferStandard;
class KIoBufferView;

class KIoBufferElement : public KShared<KIoBufferElement>, public KObject<KIoBufferElement> {

    K_FORCE_SHARED_WITH_INHERITANCE(KIoBufferElement);

    public:

        static
        NTSTATUS
        CreateNew(
            __in ULONG Size,
            __out KIoBufferElement::SPtr& IoBufferElement,
            _Outref_result_bytebuffer_(Size) VOID*& Buffer,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        static
        NTSTATUS
        CreateReference(
            __inout KIoBufferElement& SourceIoBufferElement,
            __in ULONG Offset,
            __in ULONG Size,
            __out KIoBufferElement::SPtr& IoBufferElement,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );
        
        const VOID*
        GetBuffer(
            );

        ULONG
        QuerySize(
            ) const;

        BOOLEAN
        IsReference(
            );

#if KTL_USER_MODE
        NTSTATUS
        SetPageAccess(
            __in DWORD ProtectFlag
        );
#endif
        
    protected:

        FAILABLE
        KIoBufferElement(
            __in ULONG Size,
            __in ULONG AllocationTag
            );

        NOFAIL
        KIoBufferElement(
            __inout KIoBufferElement& SourceIoBufferElement,
            __in ULONG Offset,
            __in ULONG Size
            );

        NOFAIL
        KIoBufferElement(
            __in PVOID Source,
            __in ULONG Size
            );          
        
        virtual
        VOID*
        PageAlignedAllocate(
            __in ULONG Size,
            __in ULONG AllocationTag
            );

        virtual
        VOID
        PageAlignedFree(
            __in VOID* Buffer,
            __in ULONG Size
            );

    private:

        static
        NTSTATUS
        CreateEmptyReference(
            __out KIoBufferElement::SPtr& IoBufferElement,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        NTSTATUS
        InitializeNew(
            __in ULONG Size,
            __in ULONG AllocationTag
            );

        VOID
        InitializeReference(
            __inout KIoBufferElement& SourceIoBufferElement,
            __in ULONG Offset,
            __in ULONG Size,
            __in BOOLEAN IsViewMode
            );

        VOID
        InitializeEmptyReference(
            );

        VOID
        SetReference(
            __in KIoBufferElement& SourceIoBufferElement,
            __in ULONG Offset,
            __in ULONG Size
            );

        VOID
        ClearReference(
            );

        friend class KIoBuffer;
        friend class KIoBufferStandard;
        friend class KIoBufferView;

        KIoBufferElement::SPtr _SourceIoBufferElement;
        KIoBufferElement::SPtr _Link;
        BOOLEAN _InIoBuffer;

    protected:
        VOID* _Buffer;
        ULONG _Size;
        BOOLEAN _FreeBuffer;
        BOOLEAN _IsReference;
#if KTL_USER_MODE
#if defined(PLATFORM_UNIX)      
        BOOLEAN _PageAccessChanged;
#endif
#endif
};

class KIoBuffer : public KShared<KIoBuffer>, public KObject<KIoBuffer> {

    K_FORCE_SHARED_WITH_INHERITANCE(KIoBuffer);

    public:

        static
        NTSTATUS
        CreateEmpty(
            __out KIoBuffer::SPtr& IoBuffer,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        static
        NTSTATUS
        CreateSimple(
            __in ULONG Size,
            __out KIoBuffer::SPtr& IoBuffer,
            _Outref_result_bytebuffer_(Size) VOID*& Buffer,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        virtual
        NTSTATUS
        CreateAlias(
            __out KIoBuffer::SPtr& IoBuffer
            );

        virtual
        VOID
        AddIoBufferElement(
            __in KIoBufferElement& IoBufferElement
            ) = 0;

        virtual
        BOOLEAN
        RemoveIoBufferElement(
            __out KIoBufferElement::SPtr& IoBufferElement
            ) = 0;

        virtual
        ULONG
        QueryNumberOfIoBufferElements(
            ) = 0;

        virtual
        ULONG
        QuerySize(
            ) const = 0;

        virtual
        KIoBufferElement*
        First(
            ) = 0;

        virtual
        KIoBufferElement*
        Next(
            __in KIoBufferElement& IoBufferElement
            ) = 0;

        virtual
        VOID
        Clear(
            ) = 0;

        NTSTATUS
        AddIoBufferElementReference(
            __in KIoBufferElement& SourceIoBufferElement,
            __in ULONG SourceStartingOffset,
            __in ULONG Size,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        NTSTATUS
        AddIoBufferReference(
            __inout KIoBuffer& SourceIoBuffer,
            __in ULONG SourceStartingOffset,
            __in ULONG Size,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        VOID
        AddIoBuffer(
            __inout KIoBuffer& IoBuffer
            );

        PVOID GetContiguousPointer(
            __inout KIoBufferElement::SPtr& ElementHint,
            __inout ULONG& PositionHint,
            __in ULONG Offset,
            __in ULONG Size
            );              
};

class KIoBufferView : public KIoBuffer {

    K_FORCE_SHARED(KIoBufferView);

    FAILABLE
    KIoBufferView(
        __in ULONG AllocationTag
        );

    public:

        static
        NTSTATUS
        Create(
            __out KIoBufferView::SPtr& IoBufferView,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
            );

        VOID
        SetView(
            __in KIoBuffer& IoBuffer,
            __in ULONG Offset,
            __in ULONG Length
            );

        VOID
        AddIoBufferElement(
            __in KIoBufferElement& IoBufferElement
            );

        BOOLEAN
        RemoveIoBufferElement(
            __out KIoBufferElement::SPtr& IoBufferElement
            );

        ULONG
        QueryNumberOfIoBufferElements(
            );

        ULONG
        QuerySize(
            ) const;

        KIoBufferElement*
        First(
            );

        KIoBufferElement*
        Next(
            __in KIoBufferElement& IoBufferElement
            );

        VOID
        Clear(
            );

    private:

        NTSTATUS
        Initialize(
            __in ULONG AllocationTag
            );

        KIoBuffer::SPtr _IoBuffer;
        KIoBufferElement::SPtr _First;
        KIoBufferElement::SPtr _Last;

        ULONG _NumberOfIoBufferElements;
        ULONG _Size;

};


//** Utility class to ease access to KIoBuffer (segmented) contents
//
//  Description:
//      This class implements a stream abstraction over the contents of a KIoBuffer instance.
//      It maintains a current location (in terms of offset) into the contents. There are 
//      primitives for advancing the cursor (Skip), positioning it (PostionTo), and fetching
//      the current position (GetPosition).
//
//      The contents can be interated over via the Iterate() method and a caller supplied callback
//      KDelegate (InterationCallback). In addition, there are methods for fetching and storing 
//      into and from the stream (Pull and Put). In the cases of Pull and Put, both raw byte and 
//      strong type-safe access is supported. 
//
//      To support the corner cases where raw access to the underlying buffer contents is desired,
//      GetBufferPointer() and GetBufferPointerRange() may be used.
//
//      There are also Reset() and Reuse() methods.
//      
class KIoBufferStream
{
public:
    __forceinline
    KIoBufferStream()
    {
        Reset();
    }

    __forceinline
    KIoBufferStream(KIoBuffer& IoBuffer, ULONG InitialOffset = 0, ULONG IoBufferLimit = MAXULONG)
    {
        KInvariant(Reuse(IoBuffer, InitialOffset, IoBufferLimit));
    }

    //** Disassocate a KIoBufferStream with a KIoBuffer
    __forceinline
    VOID
    Reset()
    {
        _Buffer.Reset();
        InternalClear();
    }

    //** Associate a KIoBufferStream with a KIoBuffer (and optionally set its cursor - defaulted to 0)
    BOOLEAN
    Reuse(KIoBuffer& IoBuffer, ULONG InitialOffset = 0, ULONG IoBufferLimit = MAXULONG);

    //** Position the cursor of a KIoBufferStream to a specific offset in the assocated KIoBuffer.
    //
    //   Returns: FALSE  - If the location is out of range for the KIoBuffer OR there is not an assocated
    //                     KIoBuffer.
    //
    BOOLEAN
    PositionTo(ULONG Offset);

    //** Return the current position of a KIoBufferStream
    __forceinline 
    ULONG
    GetPosition() const
    {
        return _Position;
    }

    //** User provided callback delegate to the Iterate() method.
    //  
    //   Description:
    //      For each continous fragment of memory interated over for a given Iterate() call,
    //      this caller provided delegate is invoked.
    //
    //  Parameters:
    //      IoBufferFragement   - pointer to the base of the continous memory fragment.
    //      FragmentSize        - size of the current memory fragment.
    //
    //  Returns:
    //      !NT_SUCCESS         - Aborts the current Iterate() and the status is returned to the 
    //                            Iterate() caller.
    //
    typedef KDelegate<NTSTATUS(
        __in void* IoBufferFragment,
        __inout ULONG& FragmentSize
    )> InterationCallback;

    //** Iterate over the underlying KIoBuffer contents from the current position to the
    //   interval provided (InterationSize) invoking a caller provided delegate for each
    //   continous memory fragment within that interval.
    //
    //  Parameters:
    //      Callback        - User callback to invoke for each fragment.
    //      InterationSize  - Limit on the interation interval [currectPos, currectPos + InterationSize -1]
    //
    //  Returns:
    //      STATUS_SUCCESS          - Interval interated over - Current Position += InterationSize.
    //      STATUS_ACCESS_VIOLATION - Range error OR no KIoBuffer assocated with the KIoBufferStream.
    //      <any failure from Callback invocations>
    //
    NTSTATUS
    Iterate(InterationCallback Callback, ULONG InterationSize);

    //** Advance the KIoBufferStream cursor by a specifc (LengthToSkip) amount
    //
    //  Returns:    FALSE - Out of range OR no KIoBuffer assocated with the KIoBufferStream.
    //
    BOOLEAN
    Skip(ULONG LengthToSkip);

    //** Pull an intance of type-T from the stream; copying it to a given location (Result).
    //
    //  Returns:    
    //      TRUE    - Result is set with the desired contents and Current Position += sizeof(T)
    //      FALSE   - Out of range OR no KIoBuffer assocated with the KIoBufferStream.
    //
    template<typename T>
    BOOLEAN
    Pull(__out T& Result)
    {
        return Pull(&Result, sizeof(T));
    }

    //** Pull a block of memory of a given size (ResultSize) from the stream; copying it to a given 
    //   location (ResultPtr).
    //
    //  Returns:    
    //      TRUE    - The desired contents are retrieved from Current Location and stored at *ResultPtr 
    //                and Current Position += ResultSize.
    //
    //      FALSE   - Out of range OR no KIoBuffer assocated with the KIoBufferStream.
    //
    BOOLEAN
    Pull(__out void *const ResultPtr, __in ULONG ResultSize);

    //** Store an intance of type-T into the stream; copying it from a given location (Src).
    //
    //  Returns:    
    //      TRUE    - Src is stored at Current Position and Current Position += sizeof(T)
    //      FALSE   - Out of range OR no KIoBuffer assocated with the KIoBufferStream.
    //
    template<typename T>
    BOOLEAN
    Put(__in T& Src)
    {
        return Put(&Src, sizeof(T));
    }

    //** Store a block of memory of a given size (Size) into the stream; copying it from a given 
    //   location (SrcPtr).
    //
    //  Returns:    
    //      TRUE    - The desired contents are stored at Current Location and Current Position += Size.
    //
    //      FALSE   - Out of range OR no KIoBuffer assocated with the KIoBufferStream.
    //
    BOOLEAN
    Put(__in void const *const SrcPtr, __in ULONG Size);

    //** Get access to the memory in a KIoBufferStream's current position.
    //
    //  Returns:
    //      nullptr - No KIoBuffer assocated with the KIoBufferStream.
    //
    __forceinline
    void*
    GetBufferPointer()
    {
        return _CurrentElementBufferBase + _CurrentElementOffset;
    }

    //** Return the limit (size) of the underlying memory at a KIoBufferStream's current position.
    //
    //  Returns:
    //      0 - No KIoBuffer assocated with the KIoBufferStream.
    //
    __forceinline
    ULONG
    GetBufferPointerRange() const
    {
        return _CurrentElementSize - _CurrentElementOffset;
    }


    //** Utility function to copy a block of a given size (Size) from a KIoBuffer (IoBuffer) at
    //   a specified offset (SrcOffset) to a provided memory location (ResultPtr)
    //   
    //  Returns:    
    //      FALSE   - Out of range
    //
    static
    BOOLEAN
    CopyFrom(__in KIoBuffer const & IoBuffer, __in ULONG SrcOffset, __in ULONG Size, __out_bcount(Size) void *const ResultPtr);

    //** Utility function to copy a block of a given size (Size) to a KIoBuffer (IoBuffer) at
    //   a specified offset (DestOffset) to a provided memory location (SrcPtr)
    //   
    //  Returns:    
    //      FALSE   - Out of range
    //
    static
    BOOLEAN
    CopyTo(__in KIoBuffer& IoBuffer, __in ULONG DestOffset, __in ULONG Size, __in_bcount(Size) void const *const SrcPtr);

private:
    NTSTATUS
    MoveToBufferIterator(void* IoBufferFragment, ULONG& FragmentSize);

    NTSTATUS
    MoveFromBufferIterator(void* IoBufferFragment, ULONG& FragmentSize);

    VOID
    InternalClear();

private:
    KIoBuffer::SPtr         _Buffer;
    KIoBufferElement::SPtr  _CurrentElement;
    ULONG                   _CurrentElementOffset;
    ULONG                   _CurrentElementSize;
    UCHAR*                  _CurrentElementBufferBase;
    ULONG                   _Position;
    ULONG                   _MaxPosition;

    UCHAR*                  _MoveInteratorState;
};

VOID __inline
KIoBufferStream::InternalClear()
{
    _CurrentElement.Reset();
    _CurrentElementBufferBase = nullptr;
    _CurrentElementOffset = 0;
    _CurrentElementOffset = 0;
    _CurrentElementSize = 0;
}

BOOLEAN __forceinline
KIoBufferStream::Reuse(KIoBuffer& IoBuffer, ULONG InitialOffset, ULONG IoBufferLimit)
{
    if (InitialOffset > IoBufferLimit)
    {
        return FALSE;
    }

    if (IoBuffer.QueryNumberOfIoBufferElements() == 1)
    {
        // Optimize for single buffer case
        _Buffer = &IoBuffer;
        _CurrentElement = IoBuffer.First();
        _CurrentElementOffset = InitialOffset;
        _CurrentElementSize = __min(_CurrentElement->QuerySize(), ((IoBufferLimit == MAXULONG) ? MAXULONG : IoBufferLimit + 1));
        _MaxPosition = _CurrentElementSize - 1;
        _CurrentElementBufferBase = (UCHAR*)_CurrentElement->GetBuffer();
        _Position = InitialOffset;
        return (InitialOffset < _CurrentElementSize);
    }

    _Buffer = &IoBuffer;
    InternalClear();
    _MaxPosition = __min((IoBuffer.QuerySize() - 1), IoBufferLimit);
    return PositionTo(InitialOffset);
}

