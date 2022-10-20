/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kbitmap.h

Abstract:

    This file defines a kbitmap class.  This class does not own its own allocation.  This is just a lightweight wrapper
    over simple bitmap functionality, leveraging the RTL_BITMAP package.  For a smart pointer based buffer allocator to use
    in conjunction with this, if desired, see KBuffer.

Author:

    Norbert P. Kusters (norbertk) 7-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KBitmap : public KObject<KBitmap> {

    public:

        static
        ULONG
        QueryRequiredBufferSize(
            __in ULONG NumberOfBits
            );

        ~KBitmap(
            );

        NOFAIL
        KBitmap(
            );

        FAILABLE
        KBitmap(
            __inout_bcount(BitmapBufferSize) VOID* BitmapBuffer,
            __in ULONG BitmapBufferSize,
            __in ULONG NumberOfBits
            );

        NTSTATUS
        Reuse(
            __inout_bcount(BitmapBufferSize) VOID* BitmapBuffer,
            __in ULONG BitmapBufferSize,
            __in ULONG NumberOfBits
            );

        VOID*
        GetBuffer(
            );

        ULONG
        QueryBufferSize(
            );

        ULONG
        QueryNumberOfBits(
            );

        VOID
        ClearBits(
            __in ULONG StartingBit,
            __in ULONG NumberOfBits
            );

        VOID
        SetBits(
            __in ULONG StartingBit,
            __in ULONG NumberOfBits
            );

        BOOLEAN
        CheckBit(
            __in ULONG BitNumber
            );

        VOID
        ClearAllBits(
            );

        VOID
        SetAllBits(
            );

        ULONG
        QueryNumberOfSetBits(
            );

        BOOLEAN
        AreBitsClear(
            __in ULONG StartingBit,
            __in ULONG NumberOfBits
            );

        BOOLEAN
        AreBitsSet(
            __in ULONG StartingBit,
            __in ULONG NumberOfBits
            );

        ULONG
        QueryRunLength(
            __in ULONG StartingBit,
            __out BOOLEAN& BitValue,
            __in_opt ULONG MaxRunLength = 0
            );

        ULONG
        QueryPrecedingRunLength(
            __in ULONG StartingBit,
            __out BOOLEAN& IsAllocated,
            __in_opt ULONG MaxRunLength = 0
            );

    private:

        K_DENY_COPY(KBitmap);

        PUCHAR _Buffer;
        ULONG _BufferSize;
        ULONG _Bits;

};

class KAllocationBitmap : public KObject<KAllocationBitmap>, public KShared<KAllocationBitmap> {

    K_FORCE_SHARED(KAllocationBitmap);

    FAILABLE
    KAllocationBitmap(
        __in ULONG NumberOfBits,
        __in ULONG AllocationTag
        );

    FAILABLE
    KAllocationBitmap(
        __in ULONG NumberOfBits,
        __inout KIoBufferElement& IoBufferElement,
        __in ULONG AllocationTag
        );

    public:

        static
        NTSTATUS
        Create(
            __in ULONG NumberOfBits,
            __out KAllocationBitmap::SPtr& AllocatorBitmap,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_BITMAP
            );

        static
        NTSTATUS
        CreateFromIoBufferElement(
            __in ULONG NumberOfBits,
            __inout KIoBufferElement& IoBufferElement,
            __out KAllocationBitmap::SPtr& AllocatorBitmap,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_BITMAP
            );

        ULONG
        QueryNumberOfBits(
            );

        BOOLEAN
        Allocate(
            __in ULONG NumberOfBits,
            __out ULONG& StartBit
            );

        VOID
        Free(
            __in ULONG StartBit,
            __in ULONG NumberOfBits
            );

        VOID
        SetAsAllocated(
            __in ULONG StartBit,
            __in ULONG NumberOfBits
            );

        VOID
        SetAsFree(
            __in ULONG StartBit,
            __in ULONG NumberOfBits
            );

        BOOLEAN
        CheckIfFree(
            __in ULONG StartBit,
            __in ULONG NumberOfBits
            );

        BOOLEAN
        CheckIfAllocated(
            __in ULONG StartBit,
            __in ULONG NumberOfBits
            );

        ULONG
        QueryNumFreeBits(
            );

        ULONG
        QueryRunLength(
            __in ULONG StartingBit,
            __out BOOLEAN& IsAllocated,
            __in_opt ULONG MaxRunLength = 0
            );

        ULONG
        QueryPrecedingRunLength(
            __in ULONG StartingBit,
            __out BOOLEAN& IsAllocated,
            __in_opt ULONG MaxRunLength = 0
            );

        BOOLEAN
        Verify(
            );

    private:

        struct BitmapRun {
            KListEntry ListEntry;
            KTableEntry StartSort;
            KTableEntry LengthSort;
            ULONG Start;
            ULONG Length;
            ULONG LargestGapLength;
        };

        typedef KNodeList<BitmapRun> BitmapRunList;

        typedef KNodeTable<BitmapRun> BitmapRunTable;

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __in ULONG NumberOfBits,
            __in ULONG AllocationTag
            );

        NTSTATUS
        Initialize(
            __in ULONG NumberOfBits,
            __inout KIoBufferElement& IoBufferElement,
            __in ULONG AllocationTag
            );

        NTSTATUS
        CreateFreeList(
            );

        VOID
        SetupRunsList(
            __in_opt BitmapRun* ScanStartRun,
            __in ULONG ScanStart,
            __in ULONG KnownLargestRun
            );

        VOID
        FreeWorker(
            __in ULONG StartBit,
            __in ULONG NumberOfBits,
            __in BOOLEAN BitsMayNotBeAllocated
            );

        VOID
        SetAllocatedAsFree(
            __in ULONG StartBit,
            __in ULONG NumberOfBits
            );

        ULONG
        LongestRun(
            __in ULONG Start,
            __in ULONG End,
            __in ULONG UpperBoundForLargestRun
            );

        VOID
        SetFreeAsAllocated(
            __in ULONG  StartBit,
            __in ULONG  NumberOfBits
            );

        static
        LONG
        StartCompare(
            __in BitmapRun& First,
            __in BitmapRun& Second
            );

        static
        LONG
        LengthCompare(
            __in BitmapRun& First,
            __in BitmapRun& Second
            );

        KBuffer::SPtr _Buffer;
        KIoBufferElement::SPtr _IoBufferElement;
        KBitmap _Bitmap;
        ULONG _NumberOfFreeListBitmapRuns;
        BitmapRun* _FreeListBitmapRuns;
        BitmapRunList _FreeList;
        BitmapRunTable::CompareFunction _StartCompare;
        BitmapRunTable::CompareFunction _LengthCompare;
        BitmapRunTable _StartSorted;
        BitmapRunTable _LengthSorted;
        ULONG _FreeBits;

};
