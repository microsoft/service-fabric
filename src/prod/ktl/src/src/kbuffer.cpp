/*++

Module Name:

    kbuffer.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KBuffer object.

Author:

    Norbert P. Kusters (norbertk) 7-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>


class KBufferStandard : public KBuffer 
{
    K_FORCE_SHARED(KBufferStandard);

    friend class KBuffer;

    FAILABLE
    KBufferStandard(
        __in ULONG Size,
        __in ULONG AllocationTag
        );

    NOFAIL
    KBufferStandard(__in KUniquePtr<UCHAR>& AllocationToUse);

private:
    NTSTATUS
    SetSize(
        __in ULONG NewSize,
        __in BOOLEAN PreserveOldContents = FALSE,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        ) override;

    VOID const *
    GetBuffer() const override;

    VOID*
    GetBuffer() override;

    ULONG
    QuerySize() const override;

    VOID
    Zero() override;

    VOID
    Zero(
        __in ULONG Offset,
        __in ULONG Length
        ) override;

    VOID
    CopyFrom(
        __in ULONG TargetOffset,
        __in KBuffer const & SourceBuffer,
        __in ULONG SourceOffset,
        __in ULONG Length
        ) override;

    NTSTATUS
    CloneFrom(
        __in KBuffer const & SourceBuffer,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        ) override;        

    VOID
    Move(
        __in ULONG TargetOffset,
        __in ULONG SourceOffset,
        __in ULONG Length
        ) override;

    BOOLEAN 
    operator==(
        __in KBuffer const & Right
        ) override;
        
    BOOLEAN 
    operator!=(
        __in KBuffer const & Right
        ) override;

private:
    VOID
    ZeroInit();

    VOID
    Cleanup();

    NTSTATUS
    Initialize(
        __in ULONG Size,
        __in BOOLEAN PreserveOldContents,
        __in ULONG AllocationTag
        );

private:
    ULONG _Size;
    ULONG _MaxSize;
    VOID* _Content;
};


KBuffer::~KBuffer()
{
}

KBuffer::KBuffer()
{
}

KBufferStandard::~KBufferStandard()
{
    Cleanup();
}

KBufferStandard::KBufferStandard(
    __in ULONG Size,
    __in ULONG AllocationTag
    )
{
    ZeroInit();
    SetConstructorStatus(Initialize(Size, FALSE, AllocationTag));
}

KBufferStandard::KBufferStandard(__in KUniquePtr<UCHAR>& AllocationToUse)
{
    ZeroInit();

    void*       allocationToUse = AllocationToUse.Detach();
    if (allocationToUse != nullptr)
    {
        KFatal(&GetThisKtlSystem() == &KAllocatorSupport::GetAllocator(allocationToUse).GetKtlSystem());
        size_t      allocatedSize = KAllocatorSupport::GetAllocatedSize(allocationToUse);

        KFatal(allocatedSize <= MAXULONG);
        _MaxSize = (ULONG)allocatedSize;
        _Size = _MaxSize;
        _Content = allocationToUse;
    }
}

NTSTATUS
KBuffer::Create(
    __in KUniquePtr<UCHAR>& AllocationToUse, 
    __out KBuffer::SPtr& Buffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a KBuffer of the requested from a supplied unique pointer and returns a smart pointer to it.

Arguments:

    AllocationToUse - Underlying memory to use for the newly created KBuffer. Underlying Raw pointer is Detach()ed from
                      the KUniquePtr<> supplied.

    Buffer          - Returns the newly created KBuffer.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies the allocator.

Return Value:

    NTSTATUS

--*/

{
    Buffer = _new(AllocationTag, Allocator) KBufferStandard(AllocationToUse);
    if (!Buffer) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
KBuffer::Create(
    __in ULONG Size,
    __out KBuffer::SPtr& Buffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a KBuffer of the requested size and returns a smart pointer to it.

Arguments:

    Size            - Supplies the needed size of the KBuffer.

    Buffer          - Returns the newly created KBuffer.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies, optionally, the allocator.

Return Value:

    NTSTATUS

--*/

{
    KBuffer* buffer;
    NTSTATUS status;

    buffer = _new(AllocationTag, Allocator) KBufferStandard(Size, AllocationTag);

    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = buffer->Status();

    if (!NT_SUCCESS(status)) {
        _delete(buffer);
        return status;
    }

    Buffer = buffer;

    return STATUS_SUCCESS;
}

NTSTATUS
KBuffer::CreateOrCopyFrom(
    __inout KBuffer::SPtr& TargetBuffer,
    __in KBuffer const & SourceBuffer,
    __in ULONG SourceOffset,
    __in ULONG Length,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a KBuffer that is a copy of the SourceBuffer in the range of [SourceOffset, SourceOffset + Length].
    If TargetBuffer does not exist, it is created first. Otherwise, its size is set to Length.
    Then TargetBuffer is populated with data from SourceBuffer.

Arguments:

    TargetBuffer    - Returns the populated KBuffer.

    SourceBuffer    - Supplies the source buffer.

    SourceOffset    - Supplies the offset in the source buffer in bytes.

    Length          - Supplies the number of bytes to copy.

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies, optionally, the allocator.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    if (!TargetBuffer) {
        status = KBuffer::Create(Length, TargetBuffer, Allocator, AllocationTag);
    } else {
        status = TargetBuffer->SetSize(Length, FALSE, AllocationTag);
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    KAssert(TargetBuffer);
    TargetBuffer->CopyFrom(0, SourceBuffer, SourceOffset, Length);

    return status;
}

NTSTATUS
KBuffer::CreateOrCopy(
    __inout KBuffer::SPtr& TargetBuffer,
    __in_opt KBuffer const * SourceBuffer,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SourceBuffer) {
        status = CreateOrCopy(TargetBuffer, SourceBuffer->GetBuffer(), SourceBuffer->QuerySize(), Allocator, AllocationTag);
    } else if (TargetBuffer) {
        status = TargetBuffer->SetSize(0);
    }

    return status;
}

NTSTATUS
KBuffer::CreateOrCopy(
    __inout KBuffer::SPtr& TargetBuffer,
    __in_bcount(SourceBufferSize) const VOID* SourceBuffer,
    __in ULONG SourceBufferSize,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine copies the given raw string of bytes to the given target buffer, setting the target buffer size to exactly
    the size of the given raw string.  If the target buffer doesn't exist, it is created.

Arguments:

    TargetBuffer    - Supplies the target buffer.

    SourceBuffer    - Supplies the source buffer.

    SourceBufferSize    - Supplies the source buffer size, in bytes.

    Allocator           - Supplies the allocator.

    AllocationTag       - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    NTSTATUS status;

    if (TargetBuffer) {
        status = TargetBuffer->SetSize(SourceBufferSize, FALSE, AllocationTag);
    } else {
        if (SourceBufferSize) {
            status = KBuffer::Create(SourceBufferSize, TargetBuffer, Allocator, AllocationTag);
        } else {
            status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (SourceBufferSize) {
        KMemCpySafe(TargetBuffer->GetBuffer(), SourceBufferSize, SourceBuffer, SourceBufferSize);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KBufferStandard::SetSize(
    __in ULONG NewSize,
    __in BOOLEAN PreserveOldContents,
    __in ULONG AllocationTag
    )
{
    return Initialize(NewSize, PreserveOldContents, AllocationTag);
}

VOID const *
KBufferStandard::GetBuffer() const
{
    return _Content;
}

VOID*
KBufferStandard::GetBuffer()
{
    return _Content;
}

ULONG
KBufferStandard::QuerySize() const
{
    return _Size;
}

VOID
KBufferStandard::Zero()
{
    if (_Size) {
        RtlZeroMemory(_Content, _Size);
    }
}

VOID
KBufferStandard::Zero(
    __in ULONG Offset,
    __in ULONG Length
    )
{
    KFatal(Offset <= _Size);
    KFatal(_Size - Offset >= Length);
    if (Length) {
        RtlZeroMemory((UCHAR*) _Content + Offset, Length);
    }
}

VOID
KBufferStandard::CopyFrom(
    __in ULONG TargetOffset,
    __in KBuffer const & SourceBuffer,
    __in ULONG SourceOffset,
    __in ULONG Length
    )
{
    KInvariant(TargetOffset <= _Size);
    KInvariant(_Size - TargetOffset >= Length);
    KInvariant(SourceOffset <= SourceBuffer.QuerySize());
    KInvariant(SourceBuffer.QuerySize() - SourceOffset >= Length);
    if (Length) 
    {
        KMemCpySafe((UCHAR*)_Content + TargetOffset, Length, (UCHAR*) SourceBuffer.GetBuffer() + SourceOffset, Length);
    }
}

NTSTATUS
KBufferStandard::CloneFrom(
    __in KBuffer const & SourceBuffer,
    __in ULONG AllocationTag
    )
{
    ULONG length = SourceBuffer.QuerySize();
    NTSTATUS status = SetSize(length, FALSE, AllocationTag);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    CopyFrom(0, SourceBuffer, 0, length);
    return STATUS_SUCCESS;
}


VOID
KBufferStandard::Move(
    __in ULONG TargetOffset,
    __in ULONG SourceOffset,
    __in ULONG Length
    )
{
    KFatal(SourceOffset <= _Size);
    KFatal(_Size - SourceOffset >= Length);
    KFatal(TargetOffset <= _Size);
    KFatal(_Size - TargetOffset >= Length);
    if (Length) {
        RtlMoveMemory((UCHAR*) _Content + TargetOffset, (UCHAR*) _Content + SourceOffset, Length);
    }
}

VOID
KBufferStandard::ZeroInit()
{
    _Size = 0;
    _MaxSize = 0;
    _Content = 0;
}

VOID
KBufferStandard::Cleanup()
{
    if (_Content) {
        _deleteArray((UCHAR*)_Content);
        _Content = NULL;
    }
}

NTSTATUS
KBufferStandard::Initialize(
    __in ULONG Size,
    __in BOOLEAN PreserveOldContents,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine initializes this KBuffer.

Arguments:

    Size                - Supplies the needed size of the KBuffer.

    PreserveOldContents - Supplies whether or not the old content needs to be preserved after the resize.

    AllocationTag       - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    VOID* newContent;

    if (Size <= _MaxSize) {
        _Size = Size;
        return STATUS_SUCCESS;
    }

    if (Size) {

        newContent = _newArray<UCHAR>(AllocationTag, GetThisAllocator(), Size);

        if (!newContent) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {
        newContent = NULL;
    }

    if (_Size && PreserveOldContents) {
        KMemCpySafe(newContent, Size, _Content, _Size);
    }

    if (_Content) {
        _deleteArray((UCHAR*)_Content);
    }

    _Size = _MaxSize = Size;
    _Content = newContent;

    return STATUS_SUCCESS;
}

BOOLEAN
KBufferStandard::operator==(
    __in KBuffer const & Right
    )
{
    if (QuerySize() != Right.QuerySize())
    {
        return FALSE;
    }

    size_t n = RtlCompareMemory(GetBuffer(), Right.GetBuffer(), QuerySize());
    if (n != QuerySize())
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
KBufferStandard::operator!=(
    __in KBuffer const & Right
    )
{
    return !(*this == Right);
}
