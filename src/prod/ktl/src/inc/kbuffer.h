/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kbuffer.h

Abstract:

    This file defines the KBuffer class.

Author:

    Norbert P. Kusters (norbertk) 7-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

#pragma once

class KBuffer : public KObject<KBuffer>, public KShared<KBuffer> 
{
    K_FORCE_SHARED_WITH_INHERITANCE(KBuffer);

public:
    /*++
            Description:

            This routine creates a KBuffer of the requested size and returns a smart pointer to it.

        Arguments:

            Size            - Supplies the needed size of the KBuffer.

            Buffer          - Returns the newly created KBuffer.

            AllocationTag   - Supplies the allocation tag.

            Allocator       - Supplies, optionally, the allocator.

        Return Value:

            NTSTATUS

    --*/
    static
    NTSTATUS
    Create(
        __in ULONG Size,
        __out KBuffer::SPtr& Buffer,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        );

    /*++
        Description:

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
    static
    NTSTATUS
    Create(
        __in KUniquePtr<UCHAR>& AllocationToUse, 
        __out KBuffer::SPtr& Buffer,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        );

        /*++
            Description:

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
    static
    NTSTATUS
    CreateOrCopyFrom(
        __inout KBuffer::SPtr& TargetBuffer,        
        __in KBuffer const & SourceBuffer,
        __in ULONG SourceOffset,
        __in ULONG Length,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        );

    /*++
        Description:

            This routine copies the given raw string of bytes to the given target buffer, setting the target buffer size to exactly
            the size of SourceBuffer.  If the target buffer doesn't exist, it is created.

        Arguments:

            TargetBuffer    - Supplies the target buffer.

            SourceBuffer    - Supplies the source buffer.

            Allocator           - Supplies the allocator.

            AllocationTag       - Supplies the allocation tag.

        Return Value:

            STATUS_INSUFFICIENT_RESOURCES

            STATUS_SUCCESS

    --*/
    static
    NTSTATUS
    CreateOrCopy(
        __inout KBuffer::SPtr& TargetBuffer,
        __in_opt KBuffer const * SourceBuffer,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        );

    /*++
        Description:

            This routine copies the given raw string of bytes to the given target buffer, setting the target buffer size to exactly
            the size of SourceBufferSize.  If the target buffer doesn't exist, it is created.

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
    static
    NTSTATUS
    CreateOrCopy(
        __inout KBuffer::SPtr& TargetBuffer,
        __in_bcount(SourceBufferSize) const VOID* SourceBuffer,
        __in ULONG SourceBufferSize,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        );

    virtual NTSTATUS
    SetSize(
        __in ULONG NewSize,
        __in BOOLEAN PreserveOldContents = FALSE,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        ) = 0;

    virtual VOID const *
    GetBuffer() const = 0;

    virtual VOID*
    GetBuffer() = 0;

    virtual ULONG
    QuerySize() const = 0;

    virtual VOID
    Zero() = 0;

    virtual VOID
    Zero(
        __in ULONG Offset,
        __in ULONG Length
        ) = 0;

    virtual VOID
    CopyFrom(
        __in ULONG TargetOffset,
        __in KBuffer const & SourceBuffer,
        __in ULONG SourceOffset,
        __in ULONG Length
        ) = 0;

    virtual NTSTATUS
    CloneFrom(
        __in KBuffer const & SourceBuffer,
        __in ULONG AllocationTag = KTL_TAG_BUFFER
        ) = 0;        

    virtual VOID
    Move(
        __in ULONG TargetOffset,
        __in ULONG SourceOffset,
        __in ULONG Length
        ) = 0;

    virtual BOOLEAN 
    operator==(
        __in KBuffer const& Right
        ) = 0;
        
    virtual BOOLEAN 
    operator!=(
        __in KBuffer const & Right
        ) = 0;
};
