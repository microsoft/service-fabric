/*++

Module Name:

    kiobuffer.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KIoBuffer object.

Author:

    Norbert P. Kusters (norbertk) 27-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>

#include <ktrace.h>

#include <ktrace.h>

class KIoBufferStandard : public KIoBuffer {

    K_FORCE_SHARED(KIoBufferStandard);

    public:

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
            ) const override;

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

        VOID
        Initialize(
            );

        friend class KIoBuffer;

        KIoBufferElement::SPtr _First;
        KIoBufferElement* _Last;
        ULONG _NumberOfIoBufferElements;
        ULONG _IoBufferSize;

};


KIoBufferElement::~KIoBufferElement(
    )
{
    if (_FreeBuffer) {
#if defined(PLATFORM_UNIX)
        //
        // Windows does not need to change the page protection back as 
        // VirtualFree will take care of it. Linux requires the page
        // protection to be reset.
        //
        if (_PageAccessChanged)
        {
            DWORD oldPageProtection;

            VirtualProtect((PVOID)GetBuffer(),
                           QuerySize(),
                           PAGE_EXECUTE_READWRITE,
                           &oldPageProtection);         
        }
#endif
        PageAlignedFree(_Buffer, _Size);
        _FreeBuffer = FALSE;
    }

    _Buffer = NULL;
    _Size = 0;

    _SourceIoBufferElement = NULL;

    KFatal(!_Link);
    KFatal(!_InIoBuffer);
}

NTSTATUS
KIoBufferElement::CreateNew(
    __in ULONG Size,
    __out KIoBufferElement::SPtr& IoBufferElement,
    __out VOID*& Buffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new KIoBufferElement and returns a smart pointer to this new object.

Arguments:

    Size            - Supplies the size, in bytes, of the new IO buffer element.

    IoBufferElement - Returns a smart pointer to an IO buffer element.

    Buffer          - Returns a writable pointer to the page aligned memory allocated.

    AllocationTag   - Supplies the tag for the allocation.

    Allocator       - Supplies the allocator that will be used for the object allocation.  The actual buffer allocation
                        will be made via a custom page aligned allocator.

Return Value:

    NTSTATUS

--*/

{
    KIoBufferElement* ioBufferElement;
    NTSTATUS status;

    ioBufferElement = _new(AllocationTag, Allocator) KIoBufferElement(Size, AllocationTag);

    if (!ioBufferElement) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ioBufferElement->Status();

    if (!NT_SUCCESS(status)) {
        _delete(ioBufferElement);
        return status;
    }

    IoBufferElement = ioBufferElement;

    Buffer = (VOID*) IoBufferElement->GetBuffer();

    return STATUS_SUCCESS;
}


NTSTATUS
KIoBufferElement::CreateReference(
    __inout KIoBufferElement& SourceIoBufferElement,
    __in ULONG Offset,
    __in ULONG Size,
    __out KIoBufferElement::SPtr& IoBufferElement,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a KIoBufferElement that is a partial reference to another KIoBufferElement and returns a smart
    pointer to this new object.

Arguments:

    SourceIoBufferElement   - Supplies the source IO buffer element that will be referred to.

    Offset                  - Supplies the offset within the source IO buffer element to refer to.

    Size                    - Supplies the size of this IO buffer reference.

    AllocationTag           - Supplies the allocation tag.

    Allocator               - Supplies the allocator to be used for object allocation.

    IoBufferElement         - Returns a smart pointer to an IO buffer element.

Return Value:

    NTSTATUS

--*/

{
    KIoBufferElement* ioBufferElement;
    NTSTATUS status;

    ioBufferElement = _new(AllocationTag, Allocator) KIoBufferElement(SourceIoBufferElement, Offset, Size);

    if (!ioBufferElement) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ioBufferElement->Status();

    if (!NT_SUCCESS(status)) {
        _delete(ioBufferElement);
        return status;
    }

    IoBufferElement = ioBufferElement;

    return STATUS_SUCCESS;
}

NTSTATUS
KIoBufferElement::CreateEmptyReference(
    __out KIoBufferElement::SPtr& IoBufferElement,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates an uninitialized KIoBufferElement that can later be set, and then reset, via the SetReference method.

Arguments:

    IoBufferElement - Returns the IO buffer element.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies, optionally, the allocator.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    KIoBufferElement* ioBufferElement;
    NTSTATUS status;

    ioBufferElement = _new(AllocationTag, Allocator) KIoBufferElement;

    if (!ioBufferElement) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ioBufferElement->Status();

    if (!NT_SUCCESS(status)) {
        _delete(ioBufferElement);
        return status;
    }

    IoBufferElement = ioBufferElement;

    return STATUS_SUCCESS;
}

VOID
KIoBufferElement::SetReference(
    __in KIoBufferElement& SourceIoBufferElement,
    __in ULONG Offset,
    __in ULONG Size
    )

/*++

Routine Description:

    This routine takes this existing empty KIoBufferElement or reference KIoBufferElement and resets the reference to the
    given coordinates.

Arguments:

    SourceIoBufferElement   - Supplies the source IO buffer element.

    Offset                  - Supplies the offset.

    Size                    - Supplies the size.

Return Value:

    None.

--*/

{
    ClearReference();
    InitializeReference(SourceIoBufferElement, Offset, Size, TRUE);
}

VOID
KIoBufferElement::ClearReference(
    )

/*++

Routine Description:

    This routine clears this reference KIoBufferElement so that the object no longer points anywhere or holds any reference
    to any other KIoBufferElement.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // It is a programming error to call this routine on a non-reference type of KIoBufferElement.
    //

    KFatal(_IsReference);

    //
    // It is a programming error to call this routine when the KIoBufferElement is linked into a KIoBuffer.
    //

    KFatal(!_InIoBuffer);

    _Buffer = NULL;
    _Size = 0;
    _SourceIoBufferElement = NULL;
}

const VOID*
KIoBufferElement::GetBuffer(
    )
{
    return _Buffer;
}

ULONG
KIoBufferElement::QuerySize(
    ) const
{
    return _Size;
}

BOOLEAN
KIoBufferElement::IsReference(
    )
{
    return _IsReference ? TRUE : FALSE;
}
#if KTL_USER_MODE

NTSTATUS
KIoBufferElement::SetPageAccess(
    __in DWORD ProtectFlag
)
{
    BOOL b;
    DWORD oldPageProtection;

    if (! _FreeBuffer)
    {
        return(STATUS_INVALID_PARAMETER);
    }
    
    b = VirtualProtect((PVOID)GetBuffer(),
                       QuerySize(),
                       ProtectFlag,
                       &oldPageProtection);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }
#if defined(PLATFORM_UNIX)      
    _PageAccessChanged = TRUE;
#endif
    return(STATUS_SUCCESS);
}
#endif

KIoBufferElement::KIoBufferElement(
    __in ULONG Size,
    __in ULONG AllocationTag
    )
{
    SetConstructorStatus(InitializeNew(Size, AllocationTag));
}

KIoBufferElement::KIoBufferElement(
    __inout KIoBufferElement& SourceIoBufferElement,
    __in ULONG Offset,
    __in ULONG Size
    )
{
    InitializeReference(SourceIoBufferElement, Offset, Size, FALSE);
}

KIoBufferElement::KIoBufferElement(
    __in PVOID Src,
    __in ULONG Size
    )
{
    _Buffer = (UCHAR*) Src;
    _Size = Size;
    _FreeBuffer = FALSE;
    _InIoBuffer = FALSE;

    _IsReference = FALSE;
    _SourceIoBufferElement = NULL;
}

KIoBufferElement::KIoBufferElement(
    )
{
    InitializeEmptyReference();
}

VOID*
KIoBufferElement::PageAlignedAllocate(
    __in ULONG Size,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocate a buffer of the given size on a page aligned boundary.

Arguments:

    Size    - Supplies the needed size of the buffer.

Return Value:

    A pointer to the newly allocated buffer or NULL.

--*/

{
    return GetThisKtlSystem().PageAlignedRawAllocator().AllocWithTag(Size, AllocationTag);
}

VOID
KIoBufferElement::PageAlignedFree(
    __in VOID* Buffer,
    __in ULONG Size
    )

/*++

Routine Description:

    This routine frees a buffer that was allocated with 'PageAlignedAllocate'.

Arguments:

    Buffer  - Supplies the buffer to free.

    Size    - Supplies the size of the buffer to free.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(Size);
    GetThisKtlSystem().PageAlignedRawAllocator().Free(Buffer);
}

NTSTATUS
KIoBufferElement::InitializeNew(
    __in ULONG Size,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine initializes a KIoBufferElement.

Arguments:

    Size    - Supplies needed size of the KIoBufferElement.

Return Value:

    NTSTATUS

--*/

{
    _Size = Size;
    _FreeBuffer = TRUE;
    _IsReference = FALSE;
    _InIoBuffer = FALSE;

#if defined(PLATFORM_UNIX)      
    _PageAccessChanged = FALSE;
#endif
    
    _Buffer = PageAlignedAllocate(Size, AllocationTag);

    if (!_Buffer) {
        _FreeBuffer = FALSE;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

VOID
KIoBufferElement::InitializeReference(
    __inout KIoBufferElement& SourceIoBufferElement,
    __in ULONG Offset,
    __in ULONG Size,
    __in BOOLEAN IsViewMode
    )

/*++

Routine Description:

    This routine initializes this KIoBufferElement by taking a reference to another object and remembering offsets
    within it.

Arguments:

    SourceIoBufferElement   - Supplies the source IO buffer element.

    Offset                  - Supplies the offset, in bytes, for this IO buffer element into the source IO buffer element.

    Size                    - Supplies the size, in bytes, of this IO buffer element.

Return Value:

    None.

--*/

{
    if (Offset >= SourceIoBufferElement.QuerySize()) {
        KFatal(FALSE);
        return;
    }

    if (SourceIoBufferElement.QuerySize() - Offset < Size) {
        KFatal(FALSE);
        return;
    }

    _SourceIoBufferElement = &SourceIoBufferElement;

    _Buffer = (UCHAR*) SourceIoBufferElement.GetBuffer() + Offset;
    _Size = Size;
    _FreeBuffer = FALSE;
    _IsReference = TRUE;
    _InIoBuffer = FALSE;

    //
    // Now make sure the sptr points to a native io buffer element that cannot be cleared.
    //

    if (!IsViewMode) {
        while (_SourceIoBufferElement->IsReference()) {
            _SourceIoBufferElement = _SourceIoBufferElement->_SourceIoBufferElement;
            KFatal(_SourceIoBufferElement);
        }
    }
}

VOID
KIoBufferElement::InitializeEmptyReference(
    )

/*++

Routine Description:

    This routine sets up this object to accept a reference via the 'SetReference' method.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _Buffer = NULL;
    _Size = 0;
    _FreeBuffer = FALSE;
    _IsReference = TRUE;
    _InIoBuffer = FALSE;
}

KIoBuffer::~KIoBuffer(
    )
{
    // Nothing.
}

KIoBuffer::KIoBuffer(
    )
{
    // Nothing.
}

NTSTATUS
KIoBuffer::CreateEmpty(
    __out KIoBuffer::SPtr& IoBuffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates an empty IO buffer and returns a smart pointer to it.

Arguments:

    IoBuffer        - Returns the IO buffer.

    AllocationTag   - Supplies the tag for allocation.

    Allocator       - Supplies, optionally, the allocator for the allocation of the IO buffer structure.

Return Value:

    NTSTATUS

--*/

{
    KIoBuffer* ioBuffer;
    NTSTATUS status;

    ioBuffer = _new(AllocationTag, Allocator) KIoBufferStandard;

    if (!ioBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ioBuffer->Status();

    if (!NT_SUCCESS(status)) {
        _delete(ioBuffer);
        return status;
    }

    IoBuffer = ioBuffer;

    return STATUS_SUCCESS;
}

NTSTATUS
KIoBuffer::CreateSimple(
    __in ULONG Size,
    __out KIoBuffer::SPtr& IoBuffer,
    __out VOID*& Buffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine will create a simple IO buffer that has exactly one IO buffer element of the given size.

Arguments:

    Size            - Supplies the size of the IO buffer element.

    IoBuffer        - Returns the IO buffer.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies, optionally, an allocator.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    KIoBufferElement::SPtr ioBufferElement;

    status = KIoBufferElement::CreateNew(Size, ioBufferElement, Buffer, Allocator, AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KIoBuffer::CreateEmpty(IoBuffer, Allocator, AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    IoBuffer->AddIoBufferElement(*ioBufferElement);

    return STATUS_SUCCESS;
}

NTSTATUS
KIoBuffer::CreateAlias(
    __out KIoBuffer::SPtr& IoBuffer
    )

/*++

Routine Description:

    This routine creates an alias for a KIoBuffer that points at the same
    KIoBufferElements as the this KIoBuffer

Arguments:

    IoBuffer        - Returns the IO buffer.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    KIoBuffer::SPtr ioBuffer;

    IoBuffer = nullptr;
    
    status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    status = ioBuffer->AddIoBufferReference(*this,
                                            0,
                                            QuerySize());                   
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    IoBuffer = Ktl::Move(ioBuffer);
    return STATUS_SUCCESS;
}

NTSTATUS
KIoBuffer::AddIoBufferElementReference(
    __in KIoBufferElement& SourceIoBufferElement,
    __in ULONG SourceStartingOffset,
    __in ULONG Size,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a reference of the given IoBufferElement and adds it to this IO buffer.

Arguments:

    SourceIoBufferElement   - Supplies the IO buffer element that will be referred to when adding a new IO buffer element to this
                                 KIoBuffer.

    SourceStartingOffset    - Supplies the offset within the given source IO buffer element to start the new IO buffer element.

    Size                    - Supplies the size of the new IO buffer.

    AllocationTag           - Supplies the tag for allocation.

    Allocator               - Supplies, optionally, the allocator for the allocation of the IO buffer structure.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    KIoBufferElement::SPtr ioBufferElement;

    status = KIoBufferElement::CreateReference(SourceIoBufferElement, SourceStartingOffset, Size, ioBufferElement,
            GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    AddIoBufferElement(*ioBufferElement);

    return STATUS_SUCCESS;
}

NTSTATUS
KIoBuffer::AddIoBufferReference(
    __inout KIoBuffer& SourceIoBuffer,
    __in ULONG SourceStartingOffset,
    __in ULONG Size,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates adds reference buffers to those in the given source IO buffer to this IO buffer, starting at the given
    relative offset and for the given size.

Arguments:

    SourceIoBuffer          - Supplies the IO buffer that is the source for the new relative IO buffer.

    SourceStartingOffset    - Supplies the offset within the given source IO buffer to start the new IO buffer.

    Size                    - Supplies the size of the new IO buffer.

    AllocationTag           - Supplies the tag for allocation.

    Allocator               - Supplies, optionally, the allocator for the allocation of the IO buffer structure.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    KIoBufferElement* element;
    ULONG size;
    KIoBufferElement::SPtr ioBufferElement;

    if (SourceStartingOffset > SourceIoBuffer.QuerySize()) {
        KFatal(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    if (SourceIoBuffer.QuerySize() - SourceStartingOffset < Size) {
        KFatal(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    for (element = SourceIoBuffer.First(); Size && element; element = SourceIoBuffer.Next(*element)) {

        if (SourceStartingOffset >= element->QuerySize()) {
            SourceStartingOffset -= element->QuerySize();
            continue;
        }

        size = element->QuerySize() - SourceStartingOffset;

        if (size > Size) {
            size = Size;
        }

        status = KIoBufferElement::CreateReference(*element, SourceStartingOffset, size, ioBufferElement, GetThisAllocator(),
                AllocationTag);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        AddIoBufferElement(*ioBufferElement);

        Size -= size;
        SourceStartingOffset = 0;
    }

    return STATUS_SUCCESS;
}

VOID
KIoBuffer::AddIoBuffer(
    __inout KIoBuffer& IoBuffer
    )

/*++

Routine Description:

    This routine will add the given io buffer to the end of this io buffer, leaving the given io buffer empty.

Arguments:

    IoBuffer    - Supplies the io buffer to drain/append to this io buffer.

Return Value:

    None.

--*/

{
    KIoBufferElement::SPtr ioBufferElement;

    while (IoBuffer.RemoveIoBufferElement(ioBufferElement)) {
        AddIoBufferElement(*ioBufferElement);
    }
}

PVOID
KIoBuffer::GetContiguousPointer(
    __inout KIoBufferElement::SPtr& ElementHint,
    __inout ULONG& PositionHint,
    __in ULONG Offset,
    __in ULONG Size
    )
/*++

Routine Description:

    This routine will determine if a subbuffer within a KIoBuffer is
    contiguous and if so then a pointer to it is returned. If not
    contiguos then nullptr is returned.

Arguments:

    ElementHint - Optional KIoBufferElement from which the search starts

    PositionHint - If ElementHint specified then this must be the
                   offset of the element from the beginning of the KIoBuffer

    Offset    - Offset into the KIoBuffer for the start of the subbuffer

    Size      - Number of bytes in the subbuffer
Return Value:

    pointer to a contiguous subbuffer or null if subbuffer is not contiguous

--*/
{
    KIoBufferElement::SPtr ioBufferElement;
    ULONG position, nextPosition = 0, ioSize;

    ioSize = QuerySize();

    if (Offset >= ioSize)
    {
        return(nullptr);
    }

    if (ElementHint)
    {
        ioBufferElement = ElementHint;
        position = PositionHint;
    } else {
        ioBufferElement = First();
        position = 0;
    }

    for (; (ioBufferElement); ioBufferElement = Next(*ioBufferElement))
    {
        nextPosition = position + ioBufferElement->QuerySize();

        if ((Offset >= position) && (((Offset + Size) <= nextPosition) || (nextPosition == ioSize)))
        {
            //
            // If the subbuffer fits in the element or there is part of the subbuffer at the very end of the
            // KIoBuffer, then return it
            //
            ElementHint = ioBufferElement;
            PositionHint = position;
            return(((PUCHAR)ioBufferElement->GetBuffer()) + (Offset - position));
        }

        position = nextPosition;
    }

    return(NULL);
}

KIoBufferStandard::~KIoBufferStandard(
    )
{
    Clear();
}

KIoBufferStandard::KIoBufferStandard(
    )
{
    Initialize();
}

VOID
KIoBufferStandard::AddIoBufferElement(
    __in KIoBufferElement& IoBufferElement
    )

/*++

Routine Description:

    This routine append the given IO buffer element to this IO buffer.

Arguments:

    IoBufferElement - Supplies the IO buffer element to add to the IO buffer.

Return Value:

    None.

--*/

{
    KFatal(!IoBufferElement._InIoBuffer);
    KFatal(!IoBufferElement._Link);

    IoBufferElement._InIoBuffer = TRUE;

    if (_Last) {
        _Last->_Link = &IoBufferElement;
    } else {
        KFatal(!_First);
        _First = &IoBufferElement;
    }

    _Last = &IoBufferElement;
    _NumberOfIoBufferElements++;
    _IoBufferSize += IoBufferElement.QuerySize();
}

BOOLEAN
KIoBufferStandard::RemoveIoBufferElement(
    __out KIoBufferElement::SPtr& IoBufferElement
    )

/*++

Routine Description:

    This routine will remove the first io buffer element from this io buffer and return its pointer.

Arguments:

    IoBufferElement - Returns the io buffer element removed.

Return Value:

    FALSE   - The io buffer is empty.

    TRUE    - Success.

--*/

{
    if (!_First) {
        return FALSE;
    }

    IoBufferElement = _First;

    _First = _First->_Link;

    if (!_First) {
        _Last = NULL;
    }

    KFatal(_NumberOfIoBufferElements);
    _NumberOfIoBufferElements--;

    KFatal(_IoBufferSize >= IoBufferElement->QuerySize());
    _IoBufferSize -= IoBufferElement->QuerySize();

    IoBufferElement->_Link = NULL;
    IoBufferElement->_InIoBuffer = FALSE;

    return TRUE;
}

ULONG
KIoBufferStandard::QueryNumberOfIoBufferElements(
    )
{
    return _NumberOfIoBufferElements;
}

ULONG
KIoBufferStandard::QuerySize(
    ) const
{
    return _IoBufferSize;
}

KIoBufferElement*
KIoBufferStandard::First(
    )
{
    return _First.RawPtr();
}

KIoBufferElement*
KIoBufferStandard::Next(
    __in KIoBufferElement& IoBufferElement
    )
{
    KFatal(IoBufferElement._InIoBuffer);
    return IoBufferElement._Link.RawPtr();
}

VOID
KIoBufferStandard::Clear(
    )

/*++

Routine Description:

    This routine resets the state of a KioBuffer back to the same state as if it was created via CreateEmpty()

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIoBufferElement::SPtr next;

    _Last = NULL;

    while (_First) {
        next = _First->_Link;
        _First->_Link = NULL;
        _First->_InIoBuffer = FALSE;
        _First = next;
    }

    _NumberOfIoBufferElements = 0;
    _IoBufferSize = 0;
}

VOID
KIoBufferStandard::Initialize(
    )
{
    _Last = NULL;
    _NumberOfIoBufferElements = 0;
    _IoBufferSize = 0;
}

KIoBufferView::~KIoBufferView(
    )
{
    // Nothing.
}

KIoBufferView::KIoBufferView(
    __in ULONG AllocationTag
    )
{
    SetConstructorStatus(Initialize(AllocationTag));
}

NTSTATUS
KIoBufferView::Create(
    __out KIoBufferView::SPtr& IoBufferView,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new KIoBufferView object that is initially empty and whose view can be repointed anywhere with the
    'SetView' method.

Arguments:

    IoBufferView    - Returns the newly allocated io buffer view object.

    Allocator       - Supplies the allocator.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    KIoBufferView* ioBuffer;
    NTSTATUS status;

    ioBuffer = _new(AllocationTag, Allocator) KIoBufferView(AllocationTag);

    if (!ioBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ioBuffer->Status();

    if (!NT_SUCCESS(status)) {
        _delete(ioBuffer);
        return status;
    }

    IoBufferView = ioBuffer;

    return STATUS_SUCCESS;
}

VOID
KIoBufferView::SetView(
    __in KIoBuffer& IoBuffer,
    __in ULONG Offset,
    __in ULONG Size
    )

/*++

Routine Description:

    This routine sets the view of this object to point to the given IoBuffer with the given offset and size.

Arguments:

    IoBuffer    - Supplies the IoBuffer.

    Offset      - Supplies the offset.

    Size        - Supplies the size.

Return Value:

    None.

--*/

{
    ULONG i = 0;
    KIoBufferElement* ioBufferElement;
    ULONG size = 0;
    ULONG firstElementSize;

    _IoBuffer = &IoBuffer;

    //
    // Find the first io buffer element.
    //

    for (ioBufferElement = IoBuffer.First(); ioBufferElement; ioBufferElement = IoBuffer.Next(*ioBufferElement)) {
        size = ioBufferElement->QuerySize();
        if (i + size > Offset) {
            break;
        }
        i += size;
    }

    //
    // It is a programming error to give an invalid (offset, size) for this io buffer.
    //

    KFatal(ioBufferElement);

    firstElementSize = i + size - Offset;

    if (firstElementSize > Size) {
        firstElementSize = Size;
    }

    _First->SetReference(*ioBufferElement, Offset - i, firstElementSize);

    _NumberOfIoBufferElements = 1;
    _Size = firstElementSize;       // _Size is for the _First portion in the view

    //
    // If the view is entirely covered by this IO buffer element, set _Last to it.
    //

    KFatal(_Size <= Size);

    if (_Size == Size) {
        _Last->SetReference(*ioBufferElement, Offset - i, firstElementSize);
        return;
    }

    //
    // Continue until the last element is found.
    //

    for (ioBufferElement = IoBuffer.Next(*ioBufferElement); ioBufferElement; ioBufferElement = IoBuffer.Next(*ioBufferElement)) {

        _Size += ioBufferElement->QuerySize();
        _NumberOfIoBufferElements++;

        if (_Size >= Size) {
            break;
        }
    }

    KFatal(ioBufferElement);

    _Last->SetReference(*ioBufferElement, 0, ioBufferElement->QuerySize() - (_Size - Size));

    _Size = Size;
}

VOID
KIoBufferView::AddIoBufferElement(
    __in KIoBufferElement& IoBufferElement
    )
{
    //
    // It is a programming error to extend a view.
    //
    UNREFERENCED_PARAMETER(IoBufferElement);
    KFatal(FALSE);
}

BOOLEAN
KIoBufferView::RemoveIoBufferElement(
    __out KIoBufferElement::SPtr& IoBufferElement
    )
{
    //
    // It is a programming error to reduce a view.
    //

    UNREFERENCED_PARAMETER(IoBufferElement);
    KFatal(FALSE);

    return FALSE;
}

ULONG
KIoBufferView::QueryNumberOfIoBufferElements(
    )
{
    return _NumberOfIoBufferElements;
}

ULONG
KIoBufferView::QuerySize(
    ) const
{
    return _Size;
}

KIoBufferElement*
KIoBufferView::First(
    )
{
    return _First.RawPtr();
}

KIoBufferElement*
KIoBufferView::Next(__in KIoBufferElement& IoBufferElement
    )
{
    KIoBufferElement* ioBufferElement;

    if (_NumberOfIoBufferElements == 1) {
        return NULL;
    }

    if (&IoBufferElement == _Last.RawPtr()) {
        return NULL;
    }

    if (&IoBufferElement == _First.RawPtr()) {
        KFatal(_First->_SourceIoBufferElement->_InIoBuffer);
        ioBufferElement = _IoBuffer->Next(*_First->_SourceIoBufferElement);
    } else {
        ioBufferElement = _IoBuffer->Next(IoBufferElement);
    }

    if (ioBufferElement == _Last->_SourceIoBufferElement.RawPtr()) {
        return _Last.RawPtr();
    }

    return ioBufferElement;
}

VOID
KIoBufferView::Clear(
    )
{
    _IoBuffer = NULL;
    _First->ClearReference();
    _Last->ClearReference();
    _NumberOfIoBufferElements = 0;
    _Size = 0;
}

NTSTATUS
KIoBufferView::Initialize(
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    status = KIoBufferElement::CreateEmptyReference(_First, GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KIoBufferElement::CreateEmptyReference(_Last, GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _NumberOfIoBufferElements = 0;
    _Size = 0;

    return STATUS_SUCCESS;
}


//*** KIoBufferStream implementation
BOOLEAN
KIoBufferStream::PositionTo(ULONG Offset)
{
    if ((_Buffer != nullptr) && (_MaxPosition >= Offset))
    {
        _CurrentElement = _Buffer->First();
        _CurrentElementOffset = 0;
        _Position = 0;
        _CurrentElementSize = __min(_CurrentElement->QuerySize(), (_MaxPosition + 1));
        _CurrentElementBufferBase = (UCHAR*)_CurrentElement->GetBuffer();
        return Skip(Offset);
    }

    return FALSE;
}

NTSTATUS
KIoBufferStream::Iterate(InterationCallback Callback, ULONG InterationSize)
{
    if (InterationSize > 0)
    {
        if (_CurrentElement == nullptr)
        {
            return STATUS_ACCESS_VIOLATION;           // EOS
        }

        #pragma warning(disable:4127)   // C4127: conditional expression is constant
        while (TRUE)
        {
            {
                ULONG   fragmentSize;

                KAssert(_CurrentElementSize >= _CurrentElementOffset);
                fragmentSize = __min(InterationSize, (_CurrentElementSize - _CurrentElementOffset));
                if (fragmentSize > 0)
                {
                    NTSTATUS status = Callback(_CurrentElementBufferBase + _CurrentElementOffset, fragmentSize);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }

                    _CurrentElementOffset += fragmentSize;
                    InterationSize -= fragmentSize;
                    _Position += fragmentSize;
                }
            }

            if ((InterationSize > 0) || ((_CurrentElementOffset == _CurrentElementSize) && (_MaxPosition >= _Position)))
            {
                _CurrentElement = _Buffer->Next(*_CurrentElement);
                _CurrentElementOffset = 0;
                if (_CurrentElement == nullptr)
                {
                    return STATUS_ACCESS_VIOLATION;           // EOS
                }

                _CurrentElementSize = __min(_CurrentElement->QuerySize(), ((_MaxPosition + 1) - _Position));
                _CurrentElementBufferBase = (UCHAR*)_CurrentElement->GetBuffer();
            }
            else break;
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN
KIoBufferStream::Skip(ULONG LengthToSkip)
{
    struct Local
    {
        static NTSTATUS
        NullIterator(void* IoBufferFragment, ULONG& FragmentSize)
        {
            UNREFERENCED_PARAMETER(IoBufferFragment);
            UNREFERENCED_PARAMETER(FragmentSize);
            return STATUS_SUCCESS;
        }
    };

    static InterationCallback  nullIterator(Local::NullIterator);

    return Iterate(nullIterator, LengthToSkip) == STATUS_SUCCESS;
}

BOOLEAN
KIoBufferStream::Pull(__out void *const ResultPtr, __in ULONG ResultSize)
{
    _MoveInteratorState = (UCHAR*)ResultPtr;
    return Iterate(InterationCallback(this, &KIoBufferStream::MoveFromBufferIterator), ResultSize) == STATUS_SUCCESS;
}

BOOLEAN
KIoBufferStream::CopyFrom(__in KIoBuffer const & IoBuffer, __in ULONG SrcOffset, __in ULONG Size, __out_bcount(Size) void *const ResultPtr)
{
    if (IoBuffer.QuerySize() < (SrcOffset + Size))
    {
        return FALSE;       // Out of range request
    }

    // Compute current source location
    KIoBufferStream     src(const_cast<KIoBuffer&>(IoBuffer), SrcOffset);       // NOTE: ctor will fast path for single element KIoBuffers
                                                        //       and code is inlined here
    if (src.GetBufferPointerRange() >= Size)
    {
        // Optimize for moving totally within current element
        __movsb((UCHAR*)ResultPtr, src._CurrentElementBufferBase + src._CurrentElementOffset, Size);
        return TRUE;
    }

    // Will be crossing element boundary
    return src.Pull(ResultPtr, Size);
}

BOOLEAN
KIoBufferStream::Put(__in void const *const SrcPtr, __in ULONG Size)
{
    _MoveInteratorState = (UCHAR*)SrcPtr;
    return Iterate(InterationCallback(this, &KIoBufferStream::MoveToBufferIterator), Size) == STATUS_SUCCESS;
}

BOOLEAN
KIoBufferStream::CopyTo(__in KIoBuffer& IoBuffer, __in ULONG DestOffset, __in ULONG Size, __in_bcount(Size) void const *const SrcPtr)
{
    if (IoBuffer.QuerySize() < (DestOffset + Size))
    {
        return FALSE;       // Out of range request
    }

    // Compute current destination location
    KIoBufferStream     dest(IoBuffer, DestOffset);     // NOTE: ctor will fast path for single element KIoBuffers
                                                        //       and code is inlined here
    if (dest.GetBufferPointerRange() >= Size)
    {
        // Optimize for moving totally within current element
        __movsb(dest._CurrentElementBufferBase + dest._CurrentElementOffset, (UCHAR*)SrcPtr, Size);
        return TRUE;
    }

    // Will be crossing element boundary
    return dest.Put(SrcPtr, Size);
}

NTSTATUS
KIoBufferStream::MoveToBufferIterator(void* IoBufferFragment, ULONG& FragmentSize)
{
    __movsb((UCHAR*)IoBufferFragment, _MoveInteratorState, FragmentSize);
    _MoveInteratorState += FragmentSize;
    return STATUS_SUCCESS;
}

NTSTATUS
KIoBufferStream::MoveFromBufferIterator(void* IoBufferFragment, ULONG& FragmentSize)
{
    __movsb(_MoveInteratorState, (UCHAR*)IoBufferFragment, FragmentSize);
    _MoveInteratorState += FragmentSize;
    return STATUS_SUCCESS;
}
