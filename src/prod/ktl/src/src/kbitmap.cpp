/*++

Module Name:

    kbitmap.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KBitmap object.

Author:

    Norbert P. Kusters (norbertk) 7-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>


/*++
    
Note:

    Bits are read starting with the lowest-order bit 0x1 as the first bit position.

Warning:

    Mask(Bits) has undefined behavior at Bits >= 64.

    RangeMask(Start, Length) has undefined behavior at Length <= 0.

--*/

#define Mask(Bits) ((1ULL << Bits) - 1ULL)
#define RangeMask(Start, Length) ((ULONGLONG_MAX >> (64 - Length)) << Start)
#define NumberOfBytes(Bits) (Bits / CHAR_BIT)
#define RemainderBits(Bits) (Bits & (CHAR_BIT - 1))
typedef ULONGLONG KBitmapChunkType;
static const ULONG g_BytesPerChunk = sizeof(KBitmapChunkType);
static const ULONG g_BitsPerChunk = g_BytesPerChunk * CHAR_BIT;


ULONG
KBitmap::QueryRequiredBufferSize(
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine returns the required buffer size, according to the internal alignment requirement.

Arguments:

    NumberOfBits    - Supplies the number of bits in the bitmap.

Return Value:

    The minimum size of the bitmap, in bytes.

--*/

{
    return ((NumberOfBits + g_BitsPerChunk - 1)/g_BitsPerChunk)*g_BytesPerChunk;
}

KBitmap::~KBitmap(
    )
{
}

KBitmap::KBitmap(
    )
{
    _Buffer = NULL;
    _BufferSize = 0;
    _Bits = 0;
}

KBitmap::KBitmap(
    __inout_bcount(BitmapBufferSize) VOID* BitmapBuffer,
    __in ULONG BitmapBufferSize,
    __in ULONG NumberOfBits
    )
{
    SetConstructorStatus(Reuse(BitmapBuffer, BitmapBufferSize, NumberOfBits));
}

NTSTATUS
KBitmap::Reuse(
    __inout_bcount(BitmapBufferSize) VOID* BitmapBuffer,
    __in ULONG BitmapBufferSize,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine initializes a bitmap.

Arguments:

    BitmapBuffer        - Supplies the bitmap buffer.  This buffer should be at least aligned to sizeof(ULONGLONG).

    BitmapBufferSize    - Supplies the size, in bytes, of the bitmap buffer.  This should be a multiple of sizeof(ULONGLONG).

    NumberOfBits        - Supplies the number of bits in the bitmap.  This needs to fit within the buffer supplied.

Return Value:

    NTSTATUS

--*/

{
    ULONG_PTR bitmapPointerValue;
    ULONG neededBufferSize;

    bitmapPointerValue = (ULONG_PTR) BitmapBuffer;

    if (bitmapPointerValue % g_BytesPerChunk) {

        //
        // Bitmap buffer must be aligned to chunk size (64 bits, or 8 bytes).
        //

        KAssert(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    neededBufferSize = QueryRequiredBufferSize(NumberOfBits);

    if (BitmapBufferSize < neededBufferSize) {
        KAssert(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    _Buffer = reinterpret_cast<PUCHAR>(BitmapBuffer);
    _BufferSize = BitmapBufferSize;
    _Bits = NumberOfBits;

    return STATUS_SUCCESS;
}

VOID*
KBitmap::GetBuffer(
    )
{
    return _Buffer;
}

ULONG
KBitmap::QueryBufferSize(
    )
{
    return _BufferSize;
}

ULONG
KBitmap::QueryNumberOfBits(
    )
{
    return _Bits;
}

VOID
KBitmap::ClearBits(
    __in ULONG StartingBit,
    __in ULONG NumberOfBits
    )
{
    KInvariant(StartingBit < _Bits);
    KInvariant(NumberOfBits <= _Bits - StartingBit);

    PUCHAR CurrentPosition = _Buffer + NumberOfBytes(StartingBit);
    StartingBit = RemainderBits(StartingBit);

    //
    // If only updating one byte, clear the bits and exit.
    //

    if (StartingBit + NumberOfBits <= CHAR_BIT) {
        if (NumberOfBits > 0) {
            *CurrentPosition &= ~RangeMask(StartingBit, NumberOfBits);
        }
        return;
    }
    
    //
    // We are now handling multiple bytes.
    // If the starting bit is not byte-aligned, mask and clear the first byte separately.
    //

    if (StartingBit > 0) {
        *CurrentPosition &= Mask(StartingBit);
        ++CurrentPosition;
        NumberOfBits -= CHAR_BIT - StartingBit;
    }

    //
    // Clear the full bytes in the middle.
    //

    if (NumberOfBits > CHAR_BIT) {
        ULONG NumberOfBytes = NumberOfBytes(NumberOfBits);
        memset(CurrentPosition, 0, NumberOfBytes);
        CurrentPosition += NumberOfBytes;
        NumberOfBits = RemainderBits(NumberOfBits);
    }

    //
    // Clear any trailing bits.
    //

    if (NumberOfBits > 0) {
        *CurrentPosition &= ~Mask(NumberOfBits);
    }
}

VOID
KBitmap::SetBits(
    __in ULONG StartingBit,
    __in ULONG NumberOfBits
    )
{
    KInvariant(StartingBit < _Bits);
    KInvariant(NumberOfBits <= _Bits - StartingBit);

    PUCHAR CurrentPosition = _Buffer + (NumberOfBytes(StartingBit));
    StartingBit = RemainderBits(StartingBit);

    //
    // If only updating one byte, set the bits and exit.
    //

    if (StartingBit + NumberOfBits <= CHAR_BIT) {
        if (NumberOfBits > 0) {
            *CurrentPosition |= RangeMask(StartingBit, NumberOfBits);
        }
        return;
    }

    //
    // If the starting bit is not byte-aligned, mask and set the partial byte.
    //

    if (StartingBit > 0) {
        *CurrentPosition |= ~Mask(StartingBit);
        ++CurrentPosition;
        NumberOfBits -= CHAR_BIT - StartingBit;
    }

    //
    // Set the full bytes in the middle.
    //

    if (NumberOfBits > CHAR_BIT) {
        ULONG NumberOfBytes = NumberOfBytes(NumberOfBits);
        memset(CurrentPosition, 0xFF, NumberOfBytes);
        CurrentPosition += NumberOfBytes;
        NumberOfBits = RemainderBits(NumberOfBits);
    }

    //
    // Set any trailing bits.
    //

    if (NumberOfBits > 0) {
        *CurrentPosition |= Mask(NumberOfBits);
    }
}

BOOLEAN
KBitmap::CheckBit(
    __in ULONG BitNumber
    )
{
    KInvariant(BitNumber < _Bits);
    return (_Buffer[NumberOfBytes(BitNumber)] & (1 << RemainderBits(BitNumber))) != 0;
}

VOID
KBitmap::ClearAllBits(
    )
{
    ClearBits(0, _Bits);
}

VOID
KBitmap::SetAllBits(
    )
{
    SetBits(0, _Bits);
}

ULONG
KBitmap::QueryNumberOfSetBits(
    )
{
    //
    // Look-up table of the Hamming weight for every byte-size bit combination.
    //

    static const UCHAR SetBitsMap[] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

    volatile PUCHAR CurrentPosition;
    PUCHAR EndPosition;
    KBitmapChunkType Chunk;
    ULONG NumberOfChunks = _Bits / g_BitsPerChunk;
    ULONG TrailingBytes = NumberOfBytes(_Bits) - (NumberOfChunks * g_BytesPerChunk);
    ULONG TrailingBits = RemainderBits(_Bits);
    ULONG TotalSet = 0;

    //
    // Count the number of set bits for every full chunk.
    //

    CurrentPosition = _Buffer;
    EndPosition = CurrentPosition + (NumberOfChunks * g_BytesPerChunk);
    while (CurrentPosition != EndPosition) {
        Chunk = *(reinterpret_cast<volatile KBitmapChunkType*>(CurrentPosition));
        CurrentPosition += g_BytesPerChunk;
        TotalSet += SetBitsMap[Chunk & 0xFF] +
                    SetBitsMap[(Chunk >> 8) & 0xFF] +
                    SetBitsMap[(Chunk >> 16) & 0xFF] +
                    SetBitsMap[(Chunk >> 24) & 0xFF] +
                    SetBitsMap[(Chunk >> 32) & 0xFF] +
                    SetBitsMap[(Chunk >> 40) & 0xFF] +
                    SetBitsMap[(Chunk >> 48) & 0xFF] +
                    SetBitsMap[Chunk >> 56];
    }

    //
    // If there are no tail bits to count, return the result.
    //

    if (TrailingBytes + TrailingBits == 0) {
        return TotalSet;
    }

    //
    // Count the number of set bits for every full byte.
    //

    EndPosition += TrailingBytes;
    while (CurrentPosition != EndPosition) {
        TotalSet += SetBitsMap[*CurrentPosition++];
    }

    //
    // Count the number of set bits in the trailing partial byte, if any.
    //

    if (TrailingBits > 0) {
        TotalSet += SetBitsMap[*CurrentPosition & Mask(TrailingBits)];
    }

    return TotalSet;
}

FORCEINLINE
BOOLEAN
AreBitsOkay(
    __in PULONGLONG BitMapBuffer,
    __in ULONG BitMapSize,
    __in ULONG StartingIndex,
    __in ULONG NumberOfBits,
    __in BOOLEAN Set
    )
{
    KBitmapChunkType * CurrentChunk = BitMapBuffer + (StartingIndex / g_BitsPerChunk);
    KBitmapChunkType * LastChunk = BitMapBuffer + ((StartingIndex + NumberOfBits) / g_BitsPerChunk) - 1;
    KBitmapChunkType ChunkMask;
    KBitmapChunkType Expected;
    KBitmapChunkType Actual;
    ULONG EndingIndex = StartingIndex + NumberOfBits;

    //
    // Make sure that the specified range is contained within the bitmap,
    // and that the EndingIndex did not overflow.
    //

    if (BitMapSize < EndingIndex ||
        EndingIndex < StartingIndex) {
        return FALSE;
    }

    //
    // KAllocationBitMap fix.
    //

    if (NumberOfBits == 0) {
        return FALSE;
    }

    //
    // Trim the chunk indices. We only need the chunk offsets now.
    //

    StartingIndex &= (g_BitsPerChunk - 1);
    EndingIndex &= (g_BitsPerChunk - 1);

    //
    // If there is a single chunk, mask and test the chunk and return the result.
    //
    
    if (StartingIndex + NumberOfBits <= g_BitsPerChunk) {
        ChunkMask = RangeMask(StartingIndex, NumberOfBits);
        Expected = Set ? ChunkMask : 0;
        Actual = *CurrentChunk & ChunkMask;
        return Actual == Expected;
    }
    
    //
    // Mask and test the first chunk.
    //

    ChunkMask = ~Mask(StartingIndex);
    Expected = Set ? ChunkMask : 0;
    Actual = *CurrentChunk & ChunkMask;
    if (Actual != Expected) {
        return FALSE;
    }
    
    //
    // Test the middle chunks.
    //

    ++CurrentChunk;
    ChunkMask = ULONGLONG_MAX;
    Expected = Set ? ChunkMask : 0;
    for (; CurrentChunk <= LastChunk; ++CurrentChunk) {
        Actual = *CurrentChunk & ChunkMask;
        if (Actual != Expected) {
            return FALSE;
        }
    }

    //
    // Mask and test the last chunk.
    //

    ChunkMask = Mask(EndingIndex);
    Expected = Set ? ChunkMask : 0;
    Actual = *CurrentChunk & ChunkMask;
    return Actual == Expected;
}

BOOLEAN
KBitmap::AreBitsClear(
    __in ULONG StartingBit,
    __in ULONG NumberOfBits
    )
{
    return AreBitsOkay(reinterpret_cast<PULONGLONG>(_Buffer), _Bits, StartingBit, NumberOfBits, FALSE);
}

BOOLEAN
KBitmap::AreBitsSet(
    __in ULONG StartingBit,
    __in ULONG NumberOfBits
    )
{
    return AreBitsOkay(reinterpret_cast<PULONGLONG>(_Buffer), _Bits, StartingBit, NumberOfBits, TRUE);
}

ULONG
KBitmap::QueryRunLength(
    __in ULONG StartingBit,
    __out BOOLEAN& BitValue,
    __in_opt ULONG MaxRunLength
    )

/*++

Routine Description:

    This routine will return the length and type of the run at the given starting bit.

Arguments:

    StartingBit     - Supplies the bit number to query the run length of.

    BitValue        - Returns whether the run is a run of zeros or a run of ones.

    MaxRunLength    - Supplies, optionally, the maximum run length that this function will return.

Return Value:

    The number of bits in the run.

--*/

{
    ULONG r = 0;
    ULONG runLengthLimit;
    ULONG numBitsInFirstChunk;
    ULONG startChunk;
    KBitmapChunkType fullBits;
    KBitmapChunkType* p;
    ULONG i;

    if (StartingBit >= _Bits) {

        //
        // The start bit is out of range.  The run length is zero.
        //

        return 0;
    }

    //
    // Compute the largest possible answer and set that into 'MaxRunLength'.
    //

    runLengthLimit = _Bits - StartingBit;

    if (MaxRunLength) {
        if (MaxRunLength > runLengthLimit) {
            MaxRunLength = runLengthLimit;
        }
    } else {
        MaxRunLength = runLengthLimit;
    }

    //
    // Figure out how many bits to individually check in the first chunk.
    //

    numBitsInFirstChunk = StartingBit%g_BitsPerChunk;

    if (numBitsInFirstChunk) {
        numBitsInFirstChunk = g_BitsPerChunk - numBitsInFirstChunk;
    }

    //
    // Check the first chunk bit by bit.
    //

    BitValue = CheckBit(StartingBit);
    for (i = 0; i < numBitsInFirstChunk; i++) {
        if (CheckBit(StartingBit + i) != BitValue) {
            return r;
        }
        r++;
        if (r >= MaxRunLength) {
            return MaxRunLength;
        }
    }

    //
    // Figure the index of the first whole chunk that will be checked.
    //

    startChunk = (StartingBit + g_BitsPerChunk - 1)/g_BitsPerChunk;

    //
    // Set up the loop by getting a chunk pointer to the bitmap and by knowing the type of the run that will be returned.
    //

    p = (KBitmapChunkType*) _Buffer;

    //
    // Check for whole chunks.
    //

    if (BitValue) {
        fullBits = MAXULONGLONG;
    } else {
        fullBits = 0;
    }

    for (i = startChunk; ; i++) {
        if (p[i] != fullBits) {
            break;
        }
        r += g_BitsPerChunk;
        if (r >= MaxRunLength) {
            return MaxRunLength;
        }
    }

    //
    // The final chunk that was checked was mixed.  Check it again, bit by bit.
    //

    StartingBit = i*g_BitsPerChunk;

    for (i = 0; i < g_BitsPerChunk; i++) {
        if (CheckBit(StartingBit + i) != BitValue) {
            break;
        }
        r++;
        if (r >= MaxRunLength) {
            return MaxRunLength;
        }
    }

    return r;
}

ULONG
KBitmap::QueryPrecedingRunLength(
    __in ULONG StartingBit,
    __out BOOLEAN& BitValue,
    __in_opt ULONG MaxRunLength
    )

/*++

Routine Description:

    This routine will return the length and type of the run that immediately precedes the given starting bit, going backwards.

Arguments:

    StartingBit     - Supplies the bit number.  The run returned immediately precedes this bit.

    BitValue        - Returns whether the run is a run of zeros or a run of ones.

    MaxRunLength    - Supplies, optionally, the maximum run length that this function will return.

Return Value:

    The number of bits in the run that immediately precedes the given start bit.

--*/

{
    ULONG r = 0;
    ULONG runLengthLimit;
    ULONG numBitsInFirstChunk;
    ULONG i;
    ULONG startChunk;
    KBitmapChunkType* p;
    KBitmapChunkType fullBits;

    if (StartingBit > _Bits) {

        //
        // The start bit is out of range.  The run length is zero.
        //

        return 0;
    }

    if (!StartingBit) {

        //
        // There aren't any bits that precedes this one.
        //

        return 0;
    }

    //
    // Compute the largest possible answer and set that into 'MaxRunLength'.
    //

    runLengthLimit = StartingBit;

    if (MaxRunLength) {
        if (MaxRunLength > runLengthLimit) {
            MaxRunLength = runLengthLimit;
        }
    } else {
        MaxRunLength = runLengthLimit;
    }

    //
    // Figure out how many bits to individually check in the first chunk.
    //

    numBitsInFirstChunk = StartingBit%g_BitsPerChunk;

    //
    // Check the first chunk bit by bit.
    //

    BitValue = CheckBit(StartingBit - 1);
    for (i = 1; i <= numBitsInFirstChunk; i++) {
        if (CheckBit(StartingBit - i) != BitValue) {
            return r;
        }
        r++;
        if (r >= MaxRunLength) {
            return MaxRunLength;
        }
    }

    //
    // Figure the index of the first whole chunk that will be checked.
    //

    startChunk = StartingBit/g_BitsPerChunk - 1;

    //
    // Set up the loop by getting a chunk pointer to the bitmap and by knowing the type of the run that will be returned.
    //

    p = (KBitmapChunkType*) _Buffer;

    //
    // Check for whole chunks.
    //

    if (BitValue) {
        fullBits = MAXULONGLONG;
    } else {
        fullBits = 0;
    }

    for (i = startChunk; ; i--) {
        if (p[i] != fullBits) {
            break;
        }
        r += g_BitsPerChunk;
        if (r >= MaxRunLength) {
            return MaxRunLength;
        }
    }

    //
    // The final chunk that was checked was mixed.  Check it again, bit by bit.
    //

    StartingBit = (i + 1)*g_BitsPerChunk;

    for (i = 1; i <= g_BitsPerChunk; i++) {
        if (CheckBit(StartingBit - i) != BitValue) {
            break;
        }
        r++;
        if (r >= MaxRunLength) {
            return MaxRunLength;
        }
    }

    return r;
}

KAllocationBitmap::~KAllocationBitmap(
    )
{
    Cleanup();
}

KAllocationBitmap::KAllocationBitmap(
    __in ULONG NumberOfBits,
    __in ULONG AllocationTag
    ) :
    _FreeList(FIELD_OFFSET(BitmapRun, ListEntry)),
    _StartCompare(&KAllocationBitmap::StartCompare),
    _LengthCompare(&KAllocationBitmap::LengthCompare),
    _StartSorted(FIELD_OFFSET(BitmapRun, StartSort), _StartCompare),
    _LengthSorted(FIELD_OFFSET(BitmapRun, LengthSort), _LengthCompare)
{
    Zero();
    SetConstructorStatus(Initialize(NumberOfBits, AllocationTag));
}

KAllocationBitmap::KAllocationBitmap(
    __in ULONG NumberOfBits,
    __inout KIoBufferElement& IoBufferElement,
    __in ULONG AllocationTag
    ) :
    _FreeList(FIELD_OFFSET(BitmapRun, ListEntry)),
    _StartCompare(&KAllocationBitmap::StartCompare),
    _LengthCompare(&KAllocationBitmap::LengthCompare),
    _StartSorted(FIELD_OFFSET(BitmapRun, StartSort), _StartCompare),
    _LengthSorted(FIELD_OFFSET(BitmapRun, LengthSort), _LengthCompare)
{
    Zero();
    SetConstructorStatus(Initialize(NumberOfBits, IoBufferElement, AllocationTag));
}

NTSTATUS
KAllocationBitmap::Create(
    __in ULONG NumberOfBits,
    __out KAllocationBitmap::SPtr& AllocatorBitmap,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KAllocationBitmap* bitmap;
    NTSTATUS status;

    bitmap = _new(AllocationTag, Allocator) KAllocationBitmap(NumberOfBits, AllocationTag);

    if (!bitmap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = bitmap->Status();

    if (!NT_SUCCESS(status)) {
        _delete(bitmap);
        return status;
    }

    AllocatorBitmap = bitmap;

    return STATUS_SUCCESS;
}

NTSTATUS
KAllocationBitmap::CreateFromIoBufferElement(
    __in ULONG NumberOfBits,
    __inout KIoBufferElement& IoBufferElement,
    __out KAllocationBitmap::SPtr& AllocatorBitmap,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KAllocationBitmap* bitmap;
    NTSTATUS status;

    bitmap = _new(AllocationTag, Allocator) KAllocationBitmap(NumberOfBits, IoBufferElement, AllocationTag);

    if (!bitmap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = bitmap->Status();

    if (!NT_SUCCESS(status)) {
        _delete(bitmap);
        return status;
    }

    AllocatorBitmap = bitmap;

    return STATUS_SUCCESS;
}

ULONG
KAllocationBitmap::QueryNumberOfBits(
    )
{
    return _Bitmap.QueryNumberOfBits();
}

BOOLEAN
KAllocationBitmap::Allocate(
    __in ULONG NumberOfBits,
    __out ULONG& StartBit
    )

/*++

Routine Description:

    This routine allocates a run of bits of the given length.  The basic invariant of the list is maintained.
    The invariant states that the first run in the list is the first free run in the bitmap.
    Every successive entry in the points to a run that is the first run that is strictly greater than its
    predecessory.  Finally, the 'largestgap' field demarks the largest run between this run and the successor run.

    Therefore we have the following invariants to maintain for all runs r(i) in the list:

        1.  r(i)->Start < r(i+1)->Start, for all i in [1,n)
        2.  r(i)->Length < r(i+1)->Length, for all i in [1,n) *and* r(1)->Length > 0
        3.  r(i)->LargestGapLength <= r(i)->Length, for all i in [1, n]
        4.  r(i)->LargestGapLength is exactly the length of the largest run in (r(i)->Start + r(i)->Length, r(i+1)->Start)

Arguments:

    NumberOfBits    - Supplies the number of bits to allocate.

    StartBit        - Returns the first bit of the allocation.

Return Value:

    FALSE   - The allocation failed.  There isn't sufficient free space for this allocation.

    TRUE    - The allocation succeeded.

--*/

{
    BitmapRun searchKey;
    BitmapRun* run;
    BitmapRun* prev;
    ULONG prevLength;
    ULONG prevGap;
    ULONG largest;

    StartBit = 0;

    if (!NumberOfBits) {

        //
        // Allocating 0 bits always succeeds.
        //

        return TRUE;
    }

    //
    // Find a run that is sufficiently large for the request.
    //

    searchKey.Length = NumberOfBits;

    run = _LengthSorted.LookupEqualOrNext(searchKey);

    if (!run) {

        //
        // There isn't a run that will accomodate this allocation request.  Quit.
        //

        return FALSE;
    }

    //
    // Capture that allocation from this run and modify the length of the run accordingly.
    //

    StartBit = run->Start;
    run->Start += NumberOfBits;
    run->Length -= NumberOfBits;
    _Bitmap.SetBits(StartBit, NumberOfBits);
    _FreeBits -= NumberOfBits;

    //
    // Now do the fixup of the list.  Consider all the cases.
    //

    prev = _LengthSorted.Previous(*run);
    prevLength = prev ? prev->Length : 0;
    prevGap = prev ? prev->LargestGapLength : 0;

    if (prevLength >= run->Length) {

        //
        // In this case, this run must be taken out of the list because its length is not longer than its predecessor which
        // violates invariant #2 above.
        //

        _LengthSorted.Remove(*run);
        _StartSorted.Remove(*run);
        _FreeList.InsertHead(run);

        //
        // The known largest gap between prev and next is the max of prev->LargestGapLength, run->Length, and
        // run->LargestGapLength.
        //

        largest = prevGap;
        if (largest < run->Length) {
            largest = run->Length;
        }
        if (largest < run->LargestGapLength) {
            largest = run->LargestGapLength;
        }

        if (largest > prevLength) {

            //
            // In this case invariant #3 is violated.  We must scan the (run, next) range runs that are larger than prev->Length.
            // To set this up properly, first set the correct 'LargestGapLength' on 'prev' to cover the range (prev, run].
            //

            if (prev && prevGap < run->Length) {
                prev->LargestGapLength = run->Length;
            }

            //
            // Now, scan (run, next).  The largest possible run in this range is known to be 'run->LargestGapLength'.
            //

            SetupRunsList(prev, run->Start + run->Length, run->LargestGapLength);

        } else {

            //
            // Otherwise no scan is required, just set the correct largest gap length field.
            //

            if (prev) {
                prev->LargestGapLength = largest;
            }
        }

    } else if (run->LargestGapLength > run->Length) {

        //
        // In this case Invariant #2 is fine.  But invariant #3 is violated.  To fix this we must scan (run, next) for
        // runs that are larger than run->Length and insert them.  We happen to know that the largest run in this range is
        // of length 'run->LargestGapLength'.
        //

        largest = run->LargestGapLength;
        run->LargestGapLength = 0;

        SetupRunsList(run, run->Start + run->Length, largest);

    } else {

        //
        // In this case Invariant #2 is fine and invariant #3 is fine.  Nothing to do.
        //

    }

    // KAssert(Verify());

    return TRUE;
}

VOID
KAllocationBitmap::Free(
    __in ULONG StartBit,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine frees the given range of bits.

Arguments:

    StartBit        - Supplies the beginning of the range to free.

    NumberOfBits    - Supplies the number of bits to free.

Return Value:

    None.

--*/

{
    FreeWorker(StartBit, NumberOfBits, FALSE);
}

VOID
KAllocationBitmap::SetAsAllocated(
    __in ULONG StartBit,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine will sets the given bits as 'allocated', regardless of their current state.

Arguments:

    StartBit        - Supplies the beginning of the range.

    NumberOfBits    - Supplies the number of bits in the range.

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG length;
    BOOLEAN bit;

    if (StartBit >= _Bitmap.QueryNumberOfBits()) {

        //
        // A call that start outside of the range is only valid if it doesn't affect any bits.
        //

        KAssert(!NumberOfBits);

        return;
    }

    if (_Bitmap.QueryNumberOfBits() - StartBit < NumberOfBits) {

        //
        // This request starts in range, but then goes out of range.  Such a call is invalid.
        //

        KAssert(FALSE);

        NumberOfBits = _Bitmap.QueryNumberOfBits() - StartBit;
    }

    //
    // Iterate through all of the runs in the given range and call 'SetAllocatedAsFree' on those runs that are set.
    //

    i = 0;
    while (i < NumberOfBits) {

        length = _Bitmap.QueryRunLength(StartBit + i, bit, NumberOfBits - i);

        KInvariant(length);

        if (!bit) {

            //
            // This range is marked as free, make it allocated.
            //

            SetFreeAsAllocated(StartBit + i, length);

        }

        i += length;
    }
}

VOID
KAllocationBitmap::SetAsFree(
    __in ULONG StartBit,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine will sets the given bits as 'free', regardless of their current state.

Arguments:

    StartBit        - Supplies the beginning of the range.

    NumberOfBits    - Supplies the number of bits in the range.

Return Value:

    None.

--*/

{
    FreeWorker(StartBit, NumberOfBits, TRUE);
}

BOOLEAN
KAllocationBitmap::CheckIfFree(
    __in ULONG StartBit,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine figures out whether or not the given range of bits are free.

Arguments:

    StartBit        - Supplies the beginning of the range to check.

    NumberOfBits    - Supplies the number of bits to check.

Return Value:

    FALSE   - Some of the bits are not free.

    TRUE    - All of the bits are free.

--*/

{
    return _Bitmap.AreBitsClear(StartBit, NumberOfBits);
}

BOOLEAN
KAllocationBitmap::CheckIfAllocated(
    __in ULONG StartBit,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine figures out whether or not the given range of bits are allocated.

Arguments:

    StartBit        - Supplies the beginning of the range to check.

    NumberOfBits    - Supplies the number of bits to check.

Return Value:

    FALSE   - Some of the bits are not allocated.

    TRUE    - All of the bits are allocated.

--*/

{
    return _Bitmap.AreBitsSet(StartBit, NumberOfBits);
}

ULONG
KAllocationBitmap::QueryNumFreeBits(
    )

/*++

Routine Description:

    This routine returns the number of free bits.

Arguments:

    None.

Return Value:

    The number of free bits.

--*/

{
    return _FreeBits;
}

ULONG
KAllocationBitmap::QueryRunLength(
    __in ULONG StartingBit,
    __out BOOLEAN& IsAllocated,
    __in_opt ULONG MaxRunLength
    )

/*++

Routine Description:

    This routine returns the run length of the run started by the given bit.

Arguments:

    StartingBit     - Supplies the start bit for the run.

    IsAllocated     - Returns whether or not the run is allocated or free.

    MaxRunLength    - Returns the maximum run length desired and that will be returned as the result of this function.

Return Value:

    The run length.

--*/

{
    return _Bitmap.QueryRunLength(StartingBit, IsAllocated, MaxRunLength);
}

ULONG
KAllocationBitmap::QueryPrecedingRunLength(
    __in ULONG StartingBit,
    __out BOOLEAN& IsAllocated,
    __in_opt ULONG MaxRunLength
    )

/*++

Routine Description:

    This routine returns the run length of the run that precedes the given bit.

Arguments:

    StartingBit     - Supplies the start bit for the run.

    IsAllocated     - Returns whether or not the run is allocated or free.

    MaxRunLength    - Returns the maximum run length desired and that will be returned as the result of this function.

Return Value:

    The run length.

--*/

{
    return _Bitmap.QueryPrecedingRunLength(StartingBit, IsAllocated, MaxRunLength);
}

BOOLEAN
KAllocationBitmap::Verify(
    )

/*++

Routine Description:

    This routine ensures that the list is exactly correct for the current bitmap.  A full bitmap scan is
    made.

Arguments:

    None.

Return Value:

    FALSE   - The list is not correct according to the bitmap.

    TRUE    - The list is correct.

--*/

{
    BOOLEAN r = TRUE;
    BitmapRun* run;
    BitmapRunTable startSorted(FIELD_OFFSET(BitmapRun, StartSort), _StartCompare);
    BOOLEAN b;
    BitmapRun* newRun;

    //
    // Make sure that start-sorted and length-sorted are exactly the same.
    //

    run = _StartSorted.First();
    newRun = _LengthSorted.First();

    for (;;) {
        if (run != newRun) {
            KAssert(FALSE);
            return FALSE;
        }

        if (!run) {
            break;
        }

        newRun = _LengthSorted.Next(*run);
        run = _StartSorted.Next(*run);
    }

    //
    // As a first step save the current table to a local variable.
    //

    while ((run = _StartSorted.First()) != NULL) {
        _StartSorted.Remove(*run);
        _LengthSorted.Remove(*run);
        b = startSorted.Insert(*run);
        if (!b) {
            KAssert(FALSE);
            return FALSE;
        }
    }

    //
    // Setup the list from scratch.
    //

    SetupRunsList(NULL, 0, 0);

    //
    // Now go through and make sure that the lists are the same.
    //

    run = startSorted.First();
    newRun = _StartSorted.First();

    for (;;) {

        if (!run) {
            if (newRun) {
                r = FALSE;
                KAssert(FALSE);
            }
            break;
        }

        if (!newRun) {
            r = FALSE;
            KAssert(FALSE);
            break;
        }

        if (run->Start != newRun->Start || run->Length != newRun->Length || run->LargestGapLength != newRun->LargestGapLength) {
            r = FALSE;
            KAssert(FALSE);
            break;
        }

        run = startSorted.Next(*run);
        newRun = _StartSorted.Next(*newRun);

    }

    //
    // Return items to the free list.
    //

    while ((run = startSorted.First()) != NULL){
        startSorted.Remove(*run);
        _FreeList.InsertHead(run);
    }

    return r;
}

VOID
KAllocationBitmap::Zero(
    )
{
    _NumberOfFreeListBitmapRuns = 0;
    _FreeListBitmapRuns = NULL;
    _FreeBits = 0;
}

VOID
KAllocationBitmap::Cleanup(
    )
{
    BitmapRun* run;

    for (;;) {

        run = _FreeList.RemoveHead();

        if (!run) {
            break;
        }
    }

    for (;;) {

        run = _StartSorted.First();

        if (!run) {
            break;
        }

        _StartSorted.Remove(*run);
    }

    for (;;) {

        run = _LengthSorted.First();

        if (!run) {
            break;
        }

        _LengthSorted.Remove(*run);
    }

    _deleteArray(_FreeListBitmapRuns);
}

NTSTATUS
KAllocationBitmap::Initialize(
    __in ULONG NumberOfBits,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine initializes a new, completely free, bitmap of the given size.

Arguments:

    NumberOfBits    - Supplies the number of bits.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    ULONG bitmapSize;
    NTSTATUS status;

    //
    // The bitmap size must be properly aligned.
    //

    bitmapSize = KBitmap::QueryRequiredBufferSize(NumberOfBits);

    //
    // Allocate an IO buffer element that is sufficiently large for the bitmap.
    //

    status = KBuffer::Create(bitmapSize, _Buffer, GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Zero the memory.
    //

    _Buffer->Zero();

    //
    // Initialize the bitmap object.
    //

    _Bitmap.Reuse(_Buffer->GetBuffer(), bitmapSize, NumberOfBits);

    //
    // Create the free list for this bitmap.
    //

    status = CreateFreeList();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Setup the runs list.
    //

    SetupRunsList(NULL, 0, 0);

    //
    // All of the bits are free.
    //

    _FreeBits = NumberOfBits;

    return STATUS_SUCCESS;
}

NTSTATUS
KAllocationBitmap::Initialize(
    __in ULONG NumberOfBits,
    __inout KIoBufferElement& IoBufferElement,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine initializes a bitmap of the given size with the given initial state fas provided by the IO buffer element.

Arguments:

    NumberOfBits    - Supplies the number of bits.

    IoBufferElement - Supplies the bitmap buffer to use with its given initial state.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    ULONG bitmapSize;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(AllocationTag);

    //
    // The bitmap size must be properly aligned.
    //

    bitmapSize = KBitmap::QueryRequiredBufferSize(NumberOfBits);

    //
    // Make sure that the given buffer sufficiently large.
    //

    if (IoBufferElement.QuerySize() < bitmapSize) {
        KAssert(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Keep a reference to the IO buffer element.
    //

    _IoBufferElement = &IoBufferElement;

    //
    // Initialize the bitmap object.
    //

    _Bitmap.Reuse((VOID*) _IoBufferElement->GetBuffer(), bitmapSize, NumberOfBits);

    //
    // Create the free list for this bitmap.
    //

    status = CreateFreeList();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Setup the runs list.
    //

    SetupRunsList(NULL, 0, 0);

    //
    // Compute the number of free bits.
    //

    _FreeBits = NumberOfBits - _Bitmap.QueryNumberOfSetBits();

    return STATUS_SUCCESS;
}

NTSTATUS
KAllocationBitmap::CreateFreeList(
    )

/*++

Routine Description:

    This routine creates the list of free runs for use with this bitmap.  The free list is created to be sufficiently large so
    as to be enough for any bitmap pattern of the size of this bitmap.  Given the invariants described in the comment block
    for Allocate, you can show that the maximum number of runs 'r' needed for a bitmap with 'n' bits must be chosen so that:

        1 + sum(i + 1, i=2..r) >= n

    When solving yields:

        (r + 1)(r + 2) >= 2n + 4

    From there this routine allocates a single buffer that is sufficiently large and then carves out a free list.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    ULONGLONG target;
    ULONGLONG r;
    ULONGLONG source;
    ULONG i;

    target = _Bitmap.QueryNumberOfBits();
    target *= 2;
    target += 4;

    //
    // Double r until we get a value that is large enough.
    //

    r = 1;

    for (;;) {

        source = (r + 1)*(r + 2);

        if (source >= target) {
            break;
        }

        r *= 2;
    }

    //
    // Allocate a single buffer of sufficient size.  Double the required buffer size to accomodate 'Verify'.
    //

    // BUG: RLH Possible Overflow
    _NumberOfFreeListBitmapRuns = (ULONG) r*2;
    _FreeListBitmapRuns = _newArray<BitmapRun>(KTL_TAG_BITMAP, GetThisAllocator(), _NumberOfFreeListBitmapRuns);

    if (!_FreeListBitmapRuns) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Carve out the needed free runs from that large buffer.
    //

    for (i = 0; i < _NumberOfFreeListBitmapRuns; i++) {
        _FreeList.AppendTail(&_FreeListBitmapRuns[i]);
    }

    return STATUS_SUCCESS;
}

VOID
KAllocationBitmap::SetupRunsList(
    __in_opt BitmapRun* ScanStartRun,
    __in ULONG ScanStart,
    __in ULONG KnownLargestRun
    )

/*++

Routine Description:

    This routine sets up the increasing runs list, between 2 points in the bitmap.

Arguments:

    ScanStartRun    - Supplies the run to start scanning from.

    ScanStart       - Supplies the bit number to start scanning from.

    KnownLargestRun - Supplies the known largest run in the range up to the next run.

Return Value:

    None.

--*/

{
    BitmapRun* run;
    ULONG end;
    ULONG i;
    ULONG length;
    BOOLEAN bit;
    BitmapRun* newRun;
    BOOLEAN b;

    if (ScanStartRun) {
        run = _StartSorted.Next(*ScanStartRun);
    } else {
        run = _StartSorted.First();
    }

    if (run) {
        end = run->Start;
    } else {
        end = _Bitmap.QueryNumberOfBits();
    }

    run = ScanStartRun;
    i = ScanStart;

    for (;;) {

        if (run && KnownLargestRun && run->LargestGapLength == KnownLargestRun) {

            //
            // There are no runs greater than this to be found.  We can exit.
            //

            break;
        }

        if (i >= end) {

            //
            // We're beyond the end of the scan.  Exit.
            //

            break;
        }

        //
        // Find the next free run, starting at i.
        //

        length = _Bitmap.QueryRunLength(i, bit, end - i);

        KInvariant(length);

        if (bit) {

            //
            // Skip over this allocated part.
            //

            i += length;

            if (i >= end) {

                //
                // We're beyond the end of the scan.  Exit.
                //

                break;
            }

            //
            // Scan now for a free run.
            //

            length = _Bitmap.QueryRunLength(i, bit, end - i);

            KInvariant(length);
            KInvariant(!bit);
        }

        if (!run || length > run->Length) {

            //
            // We have a new run to add.
            //

            newRun = _FreeList.RemoveHead();
            KInvariant(newRun);

            newRun->Start = i;
            newRun->Length = length;
            newRun->LargestGapLength = 0;

            b = _StartSorted.Insert(*newRun);
            KInvariant(b);

            b = _LengthSorted.Insert(*newRun);
            KInvariant(b);

            //
            // Resume the scan with this new run as most recent.
            //

            run = newRun;

        } else if (length > run->LargestGapLength) {

            //
            // A larger, largest-gap was found.  Remember this new maximum.
            //

            run->LargestGapLength = length;
        }

        i += length;
    }
}

VOID
KAllocationBitmap::FreeWorker(
    __in ULONG StartBit,
    __in ULONG NumberOfBits,
    __in BOOLEAN BitsMayNotBeAllocated
    )

/*++

Routine Description:

    This routine frees the given set of bits.  The routine will KAssert if the bits are not allocated and if 'BitsAreAllocated'
    is set.

Arguments:

    StartBit                - Supplies the start of the run to mark as free.

    NumberOfBits            - Supplies the length of the run to mark as free.

    BitsMayNotBeAllocated   - Supplies that not all bits are necessarily allocated.

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG length;
    BOOLEAN bit;

    if (StartBit >= _Bitmap.QueryNumberOfBits()) {

        //
        // A call that start outside of the range is only valid if it doesn't affect any bits.
        //

        KAssert(!NumberOfBits);

        return;
    }

    if (_Bitmap.QueryNumberOfBits() - StartBit < NumberOfBits) {

        //
        // This request starts in range, but then goes out of range.  Such a call is invalid.
        //

        KAssert(FALSE);

        NumberOfBits = _Bitmap.QueryNumberOfBits() - StartBit;
    }

    //
    // Iterate through all of the runs in the given range and call 'SetAllocatedAsFree' on those runs that are set.
    //

    i = 0;
    while (i < NumberOfBits) {

        length = _Bitmap.QueryRunLength(StartBit + i, bit, NumberOfBits - i);

        KInvariant(length);

        if (bit) {

            //
            // This range is marked as allocated.
            //

            SetAllocatedAsFree(StartBit + i, length);

        } else {

            //
            // There is a range that is not allocated here.  Is this expected?
            //

            KInvariant(BitsMayNotBeAllocated);
        }

        i += length;
    }
}

VOID
KAllocationBitmap::SetAllocatedAsFree(
    __in ULONG StartBit,
    __in ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine marks allocated bits as free.

Arguments:

    StartBit        - Supplies the start of the run to mark as free.

    NumberOfBits    - Supplies the length of the run to mark as free.

Return Value:

    None.

--*/

{
    BitmapRun* pendingLengthInsert = NULL;
    BitmapRun searchKey;
    BitmapRun* run;
    BitmapRun* prev;
    ULONG start;
    ULONG length;
    BOOLEAN bit;
    ULONG end;
    ULONG delta;
    ULONG n;
    ULONG largestGap;
    BitmapRun* newRun;
    BitmapRun* next;
    BOOLEAN b;

    if (!NumberOfBits) {

        //
        // Nothing to do.
        //

        return;
    }

    //
    // Start off by clearing the bits in the bitmap and adjusting the free bits count.
    //

    _Bitmap.ClearBits(StartBit, NumberOfBits);

    _FreeBits += NumberOfBits;

    //
    // Look for the first run in the list that is greater than the run being freed.  Since the run being
    // freed was allocated, it could not be part of the list.  So, lookup-equal-or-next can only return a greater value.
    //

    searchKey.Start = StartBit;

    run = _StartSorted.LookupEqualOrNext(searchKey);

    KInvariant(!run || run->Start > StartBit);

    //
    // Find the run that is the greatest one in the list that is less than the run being freed.
    //

    prev = run ? _StartSorted.Previous(*run) : _StartSorted.Last();

    //
    // Set 'start' to be the beginning of the free run formed by this free command.
    //

    if (!prev) {

        //
        // This free run comes before the formerly first free run.  The beginning of the entire free run is simply the beginning
        // of the run being freed.
        //

        start = StartBit;

    } else if (prev->Start + prev->Length == StartBit) {

        //
        // In this case the run in the list is directly adjacent to this new free run, so the beginning
        // of the entire free run is just the beginning of the free run in the list.
        //

        start = prev->Start;

    } else {

        //
        // Otherwise scan backwards to find the beginning of the run.
        //

        start = StartBit;
        if (start && !_Bitmap.CheckBit(start - 1)) {
            length = _Bitmap.QueryPrecedingRunLength(start, bit);
            KInvariant(!bit);
            start -= length;
        }
    }

    //
    // Set 'end' to the end of the free run formed by this free command.
    //

    if (run && StartBit + NumberOfBits == run->Start) {

        //
        // In this case the new free joins the run that comes after it.  And so the end of the free run formed by this free
        // is just the end of the next run in the list.
        //

        end = run->Start + run->Length;

    } else {

        //
        // Otherwise scan forwards to find the end of the run.
        //

        end = StartBit + NumberOfBits;
        if (end < _Bitmap.QueryNumberOfBits() && !_Bitmap.CheckBit(end)) {
            length = _Bitmap.QueryRunLength(end, bit);
            KInvariant(!bit);
            end += length;
        }
    }

    //
    // For this next part fix up the list and the 'largestgap' fields without necessarily keeping
    // the list monotonic.  So after this step Invariant #3 will be correct.
    //

    if (prev && prev->Start == start) {

        //
        // This is the case where the preceding run merges with the new run.  Set the preceding length accordingly.
        //

        prev->Length = end - prev->Start;

        if (run && run->Start < end) {

            //
            // This is the case where the next run also merges with the new run.  We can take the next run out of the list.
            // Also, the largest gap corresponds simply to the largest gap that came after the next run.
            //

            _StartSorted.Remove(*run);
            _LengthSorted.Remove(*run);
            _FreeList.InsertHead(run);

            prev->LargestGapLength = run->LargestGapLength;

            //
            // Reset the next run pointer to the next run.
            //

            run = _StartSorted.Next(*prev);

        } else {

            //
            // In this case the next run is separate from the new run.  There is a case where the new end consumed a formerly
            // largest gap.  We need to rescan for this case to find the new largest gap, knowing that it is no larger than
            // the former largest gap.
            //

            if (end - (StartBit + NumberOfBits) == prev->LargestGapLength && prev->LargestGapLength) {
                prev->LargestGapLength = LongestRun(end, run ? run->Start : _Bitmap.QueryNumberOfBits(), prev->LargestGapLength);
            }
        }

    } else if (run && run->Start < end) {

        //
        // In this case the new run merges with the next run, but is separate from the preceding run.  Adjust the next run
        // to include the freed run.
        //

        delta = run->Start - start;
        run->Start -= delta;
        run->Length += delta;

        //
        // There is a case where the new start consumed a formerly largest gap of the preceding run.  We need to rescan
        // to establish the new largest gap for the preceding run, knowing that it is no larger than the former largest gap.
        //

        if (prev && StartBit - start == prev->LargestGapLength && prev->LargestGapLength) {
            prev->LargestGapLength = LongestRun(prev->Start + prev->Length, run->Start, prev->LargestGapLength);
        }

        //
        // Now set the run that just consumed the new free as the preceding run.
        //

        prev = run;
        run = _StartSorted.Next(*prev);

    } else {

        //
        // In this case the new run does not merge with either the preceding run nor with the next run.
        // Let 'n' be the length of the new run.
        //

        n = end - start;

        if (!prev) {

            //
            // In this case there is no preceding run, which makes this run the first in the list.  This also means
            // that there are no free runs at all between this one and the next one in the list so we can set the
            // 'largestgap' to 0.
            //

            prev = _FreeList.RemoveHead();

            prev->Start = start;
            prev->Length = n;
            prev->LargestGapLength = 0;

            b = _StartSorted.Insert(*prev);
            KInvariant(b);

            pendingLengthInsert = prev;

        } else if (prev->Length < n) {

            //
            // In this case the length of the preceding run is less than the length of this run.  So, we can do the insert.
            // The largest gap for the preceding run may have been reduced because this run may end up being in the way
            // of its formerly largest gap run.
            //

            largestGap = prev->LargestGapLength;
            if (largestGap) {
                prev->LargestGapLength = LongestRun(prev->Start + prev->Length, start, largestGap);
            }

            newRun = _FreeList.RemoveHead();

            newRun->Start = start;
            newRun->Length = n;

            if (largestGap) {
                newRun->LargestGapLength = LongestRun(end, run ? run->Start : _Bitmap.QueryNumberOfBits(), largestGap);
            } else {
                newRun->LargestGapLength = 0;
            }

            b = _StartSorted.Insert(*newRun);
            KInvariant(b);

            pendingLengthInsert = newRun;

            prev = newRun;

        } else if (prev->LargestGapLength < n) {

            //
            // This is the case where this free run will not be inserted into the list.  But, the largest gap field of the
            // preceding run is affected because a new longer free run has been created.
            //

            prev->LargestGapLength = n;

        }
    }

    //
    // At this point all gap fields are less than the length fields as required by invariant #3.  We can just
    // remove any runs that violate invariant #2.
    //

    while (run && run->Length <= prev->Length) {

        if (run->Length > prev->LargestGapLength) {
            prev->LargestGapLength = run->Length;
        }

        next = _StartSorted.Next(*run);
        _StartSorted.Remove(*run);
        _LengthSorted.Remove(*run);
        _FreeList.InsertHead(run);

        run = next;
    }

    //
    // It may be that the insertion of 'prev' into the length table has been delayed until the length table was cleared
    // of conflicting items.  Check for that here and make the length table correct.
    //

    if (pendingLengthInsert) {
        _LengthSorted.Insert(*pendingLengthInsert);
    }

    // KAssert(Verify());
}

ULONG
KAllocationBitmap::LongestRun(
    __in ULONG Start,
    __in ULONG End,
    __in ULONG UpperBoundForLargestRun
    )

/*++

Routine Description:

    This routine finds the longest free run in the given range of the bitmap.

Arguments:

    Start                   - Supplies the start position to start the search.

    End                     - Supplies the end postion to end the search.

    UpperBoundForLargestRun - Supplies a known maximum for the largest free run.

Return Value:

    The length of the longest run.

--*/

{
    ULONG r;
    ULONG i;
    ULONG length;
    BOOLEAN bit;

    r = 0;
    i = Start;
    while (i < End) {

        length = _Bitmap.QueryRunLength(i, bit, End - i);

        if (!bit && length > r) {
            r = length;
            if (r == UpperBoundForLargestRun) {
                break;
            }
        }

        i += length;
    }

    return r;
}

VOID
KAllocationBitmap::SetFreeAsAllocated(
    __in ULONG  StartBit,
    __in ULONG  NumberOfBits
    )

/*++

Routine Description:

    This routine marks free bits as allocated.

Arguments:

    StartBit        - Supplies the start of the run to mark as allocated.

    NumberOfBits    - Supplies the length of the run to mark as allocated.

Return Value:

    None.

--*/

{
    BOOLEAN intersectionFound = FALSE;
    BitmapRun searchKey;
    BitmapRun* run;
    BitmapRun* prev;
    ULONG scanStart;
    ULONG oldEnd;
    ULONG newEnd;
    ULONG largest;
    ULONG length;
    BOOLEAN bit;
    ULONG start;
    ULONG end;

    //
    // Check to see if there is an intersection between what is going to be freed and one of the runs.
    // Set 'prev' to the run that comes before the range being freed.
    // Set 'run' to the run that comes after the range being freed or the intersecting run, if there is one.
    //

    searchKey.Start = StartBit;

    run = _StartSorted.LookupEqualOrPrevious(searchKey);

    if (run) {
        if (StartBit >= run->Start + run->Length) {
            prev = run;
            run = _StartSorted.Next(*run);
        } else {
            prev = _StartSorted.Previous(*run);
            intersectionFound = TRUE;
        }
    } else {
        prev = NULL;
        run = _StartSorted.First();
        KInvariant(run);
    }

    //
    // Set the bits the way they should go.
    //

    _Bitmap.SetBits(StartBit, NumberOfBits);
    _FreeBits -= NumberOfBits;

    //
    // Consider both cases where the intersection was found and not found.
    //

    if (intersectionFound) {

        if (StartBit == run->Start) {

            //
            // In this case the part being marked allocated coincides with the beginning of the run.  The run must
            // necessarily be at least as long as what is being freed.  Reduce the length of the run.  Remember
            // that a scan is needed starting at the end of the run.
            //

            KInvariant(run->Length >= NumberOfBits);

            run->Length -= NumberOfBits;
            run->Start += NumberOfBits;

            scanStart = run->Start + run->Length;

        } else {

            //
            // In this case the run keeps its beginning and then might be split.  The start of the allocate is necessarily
            // greater than the start of the run.  The end of the allocate is necessarily within the run.  Remember the
            // old end of the run and the new end of the run.  Allow that the run's largest gap might be greater now that
            // a free part of the original run has been split off into the space between this run and the next run.
            // Remember that the scan is needed starting at the end of allocation.
            //

            KInvariant(StartBit > run->Start);
            KInvariant(StartBit + NumberOfBits <= run->Start + run->Length);

            oldEnd = run->Start + run->Length;
            run->Length = StartBit - run->Start;
            newEnd = run->Start + run->Length;

            KInvariant(oldEnd > newEnd);
            largest = oldEnd - newEnd;

            KInvariant(largest >= NumberOfBits);
            largest -= NumberOfBits;

            if (largest > run->LargestGapLength) {
                run->LargestGapLength = largest;
            }

            scanStart = StartBit + NumberOfBits;
        }

        if ((prev && prev->Length >= run->Length) || (!prev && run->Length == 0)) {

            //
            // In this case, 'run' is not correctly ordered any more.  So, take it out of the tables.  Do the scan up to the
            // next run.
            //

            _StartSorted.Remove(*run);
            _LengthSorted.Remove(*run);
            _FreeList.InsertHead(run);

            //
            // The scan will start after the run, so consider the run for the prev->LargestGap field.
            //

            if (prev && prev->LargestGapLength < run->Length) {
                prev->LargestGapLength = run->Length;
            }

            //
            // Now figure out what the largest gap is between the start of a scan and the next run.
            //

            largest = prev ? prev->LargestGapLength : 0;
            if (largest < run->LargestGapLength) {
                largest = run->LargestGapLength;
            }

            //
            // If the largest gap is larger than the existing length then scan, otherwise just remember the largest gap.
            //

            if (largest > (prev ? prev->Length : 0)) {
                SetupRunsList(prev, scanStart, largest);
            } else if (prev) {
                prev->LargestGapLength = largest;
            }

        } else if (run->Length < run->LargestGapLength) {

            //
            // In this case the length is larger than its predecessor, but there is still a run in between this run and
            // the next that needs to be added to the list.
            //

            largest = run->LargestGapLength;
            run->LargestGapLength = 0;

            SetupRunsList(run, scanStart, largest);
        }

    } else {

        //
        // In this case there is no intersection.  We cannot come before the first free run.
        //

        KInvariant(prev);

        //
        // Find the 'start' and 'end' of the free run that contains this new allocation.
        //

        start = StartBit;
        if (start && !_Bitmap.CheckBit(start - 1)) {
            length = _Bitmap.QueryPrecedingRunLength(start, bit);
            KInvariant(!bit);
            KInvariant(length);
            KInvariant(length <= StartBit);
            start -= length;
        }

        end = StartBit + NumberOfBits;
        if (end < _Bitmap.QueryNumberOfBits() && !_Bitmap.CheckBit(end)) {
            length = _Bitmap.QueryRunLength(end, bit);
            KInvariant(!bit);
            KInvariant(length);
            end += length;
            KInvariant(end <= _Bitmap.QueryNumberOfBits());
        }

        //
        // If this former run was greater or equal to the largest gap, then it may be that the largest gap is smaller now.
        //

        if (end - start >= prev->LargestGapLength) {

            largest = prev->LargestGapLength;
            prev->LargestGapLength = 0;

            SetupRunsList(prev, prev->Start + prev->Length, largest);
        }
    }

    // KAssert(Verify());
}

LONG
KAllocationBitmap::StartCompare(
    __in BitmapRun& First,
    __in BitmapRun& Second
    )

/*++

Routine Description:

    This routine compares the 2 bitmap runs by start value.

Arguments:

    First   - Supplies the first run to compare.

    Second  - Supplies the second run to compare.

Return Value:

    <0  - The first run's start value is less than the second's.

    0   - Both runs have equal start values.

    >0  - The first run's start value is greater than the second's.

--*/

{
    if (First.Start < Second.Start) {
        return -1;
    }
    if (First.Start > Second.Start) {
        return 1;
    }
    return 0;
}

LONG
KAllocationBitmap::LengthCompare(
    __in BitmapRun& First,
    __in BitmapRun& Second
    )

/*++

Routine Description:

    This routine compares the 2 bitmap runs by length value.

Arguments:

    First   - Supplies the first run to compare.

    Second  - Supplies the second run to compare.

Return Value:

    <0  - The first run's length value is less than the second's.

    0   - Both runs have equal length values.

    >0  - The first run's length value is greater than the second's.

--*/

{
    if (First.Length < Second.Length) {
        return -1;
    }
    if (First.Length > Second.Length) {
        return 1;
    }
    return 0;
}
