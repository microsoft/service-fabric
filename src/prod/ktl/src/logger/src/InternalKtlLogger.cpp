/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    InternalRvdLogger.cpp

    Description:
      Internal misc implementations for the RvdLogger.

    History:
      PengLi, Richhas    12-Mar-2011         Initial version.

--*/

#pragma once
#include "InternalKtlLogger.h"

#pragma prefast(push)
#pragma prefast(disable: 28167, "DevDiv:422165: Prefast not recognizing c++ dtor being called")
#pragma warning (disable:4840)    // Maintain back compat with older compiler


//** RvdLogLsnEntryTracker implementation
RvdLogLsnEntryTracker::RvdLogLsnEntryTracker(__in KAllocator& Allocator)
    :   _Allocator(Allocator),
        _Blocks(FIELD_OFFSET(RvdLogLsnEntryBlock, _BlockLinks)),
        _HeadBlockFirstEntryIx(0),
        _TailBlockLastEntryIx(RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock),
        _HighestLsn(RvdLogLsn::Null()),
        _LowestLsn(RvdLogLsn::Null()),
        _TotalSize(0),
        _NumberOfEntires(0),
        _ReservedBlock(nullptr)
{
}

RvdLogLsnEntryTracker::~RvdLogLsnEntryTracker()
{
    Clear();
}

VOID
RvdLogLsnEntryTracker::Clear()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        RvdLogLsnEntryBlock* blk;
        while ((blk = _Blocks.RemoveHead()) != nullptr)
        {
            _delete(blk);
        }

        _delete(_ReservedBlock);
        _ReservedBlock = nullptr;

        _HeadBlockFirstEntryIx = 0;
        _TailBlockLastEntryIx = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
        _NumberOfEntires = 0;
        _TotalSize = 0;
        _LowestLsn = RvdLogLsn::Null();
        _HighestLsn = RvdLogLsn::Null();
    }
}

NTSTATUS
RvdLogLsnEntryTracker::UnsafeAllocateRvdLogLsnEntryBlock(__out RvdLogLsnEntryBlock*& Result)
{
    RvdLogLsnEntryBlock* newBlk = _ReservedBlock;
    _ReservedBlock = nullptr;

    if (newBlk == nullptr)
    {
        newBlk = _new(KTL_TAG_LOGGER, _Allocator) RvdLogLsnEntryBlock();
        if (newBlk == nullptr)
        {
            Result = nullptr;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    Result = newBlk;
    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogLsnEntryTracker::Load(
    __in RvdLogLsnEntryTracker& Src,
    __in_opt RvdLogLsn *const LowestLsn,
    __in_opt RvdLogLsn *const HighestLsn)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KInvariant(_NumberOfEntires == 0);
        K_LOCK_BLOCK(Src._ThisLock)
        {
            RvdLogLsnEntryBlock*        nextBlk = nullptr;

            while ((nextBlk = Src._Blocks.RemoveHead()) != nullptr)
            {
                _Blocks.AppendTail(nextBlk);
            }

            _HeadBlockFirstEntryIx = Src._HeadBlockFirstEntryIx;
            _TailBlockLastEntryIx = Src._TailBlockLastEntryIx;
            _NumberOfEntires = Src._NumberOfEntires;
            _TotalSize = Src._TotalSize;
            _LowestLsn = Src._LowestLsn;
            _HighestLsn = Src._HighestLsn;
            _ReservedBlock = Src._ReservedBlock;

            // Finish clearing Src
            Src._HeadBlockFirstEntryIx = 0;
            Src._TailBlockLastEntryIx = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
            Src._NumberOfEntires = 0;
            Src._TotalSize = 0;
            Src._LowestLsn = RvdLogLsn::Null();
            Src._HighestLsn = RvdLogLsn::Null();
            Src._ReservedBlock = nullptr;
        }

        if (LowestLsn != nullptr)
        {
            *LowestLsn = _LowestLsn;
        }

        if (HighestLsn != nullptr)
        {
            *HighestLsn = _HighestLsn;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogLsnEntryTracker::AddHigherLsnRecord(
    __in RvdLogLsn Lsn, 
    __in ULONG HeaderAndMetaDataSize, 
    __in ULONG IoBufferSize)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert(Lsn > _HighestLsn);
        BOOLEAN wasEmpty  = _Blocks.IsEmpty();

        if ((wasEmpty) || 
            (_TailBlockLastEntryIx == (RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock - 1)))
        {
            // Empty or last (tail) block is full - allocate addition block at tail
            RvdLogLsnEntryBlock* newBlk;
            NTSTATUS status = UnsafeAllocateRvdLogLsnEntryBlock(newBlk);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
            _Blocks.AppendTail(newBlk);

            // Adjust cursors
            _TailBlockLastEntryIx = MAXULONG;
            if (wasEmpty)
            {
                // First entry at start of only block
                _HeadBlockFirstEntryIx = 0;
                _LowestLsn = Lsn;
            }
        }

        _TailBlockLastEntryIx++;
        RvdLogLsnEntry* entry = &_Blocks.PeekTail()->_Entries[_TailBlockLastEntryIx];
        UnsafeAddRecord(*entry, Lsn, HeaderAndMetaDataSize, IoBufferSize);
        _HighestLsn = Lsn;
    }

    return STATUS_SUCCESS;
}

RvdLogLsn
RvdLogLsnEntryTracker::RemoveHighestLsnRecord()
{
    RvdLogLsn       result;

    K_LOCK_BLOCK(_ThisLock)
    {
        result = _HighestLsn;
        KAssert(!_Blocks.IsEmpty());
        KAssert(_NumberOfEntires > 0);

        RvdLogLsnEntry* oldEntry = &_Blocks.PeekTail()->_Entries[_TailBlockLastEntryIx];
        KAssert(oldEntry->Lsn == _HighestLsn);
        _NumberOfEntires--;
        _TotalSize -= ComputeTotalRecordSizeOnDisk(oldEntry->HeaderAndMetaDataSize, oldEntry->IoBufferSize);
        if (_TailBlockLastEntryIx == 0)
        {
            // Trailing block will need to be freed (or put into _ReservedBlock)
            {
                RvdLogLsnEntryBlock* blk = _Blocks.RemoveTail();
                KAssert(blk != nullptr);
                if (_ReservedBlock == nullptr)
                {
                    _ReservedBlock = blk;
                }
                else
                {
                    _delete(blk);
                }
            }
            _TailBlockLastEntryIx = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock - 1;
        }
        else
        {
            _TailBlockLastEntryIx--;
        }

        if (_NumberOfEntires == 0)
        {
            KAssert(_TotalSize == 0);
            KAssert(_Blocks.IsEmpty());
            _HeadBlockFirstEntryIx = 0;
            _TailBlockLastEntryIx = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
            _HighestLsn = RvdLogLsn::Null();
            _LowestLsn = RvdLogLsn::Null();
        }
        else
        {
            _HighestLsn = _Blocks.PeekTail()->_Entries[_TailBlockLastEntryIx].Lsn;
        }
    }

    return result;
}

NTSTATUS
RvdLogLsnEntryTracker::GuarenteeAddTwoHigherRecords()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if ((_ReservedBlock == nullptr) && 
            (_Blocks.IsEmpty() || (_TailBlockLastEntryIx >= (RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock - 2))))
        {
            return UnsafeAllocateRvdLogLsnEntryBlock(_ReservedBlock);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogLsnEntryTracker::AddLowerLsnRecord(
    __in RvdLogLsn Lsn, 
    __in ULONG HeaderAndMetaDataSize, 
    __in ULONG IoBufferSize)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        KAssert((_NumberOfEntires == 0) || (Lsn < _LowestLsn));
        BOOLEAN wasEmpty  = _Blocks.IsEmpty();

        if ((wasEmpty) || (_HeadBlockFirstEntryIx == 0))
        {
            // Empty or first (head) block is full - allocate addition block at head
            RvdLogLsnEntryBlock* newBlk;
            NTSTATUS status = UnsafeAllocateRvdLogLsnEntryBlock(newBlk);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
            _Blocks.InsertHead(newBlk);

            // Adjust cursors
            _HeadBlockFirstEntryIx = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
            if (wasEmpty)
            {
                // First entry at end of only block
                _TailBlockLastEntryIx = _HeadBlockFirstEntryIx - 1;
                _HighestLsn = Lsn;
            }
        }

        _HeadBlockFirstEntryIx--;
        RvdLogLsnEntry* entry = &_Blocks.PeekHead()->_Entries[_HeadBlockFirstEntryIx];
        UnsafeAddRecord(*entry, Lsn, HeaderAndMetaDataSize, IoBufferSize);
        _LowestLsn = Lsn;
    }

    return STATUS_SUCCESS;
}

VOID
RvdLogLsnEntryTracker::Truncate(__in RvdLogLsn NewLowestLsn)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        RvdLogLsnEntryBlock* blkPtr;
        while ((blkPtr = _Blocks.PeekHead()) != nullptr)
        {
            while ((_NumberOfEntires > 0) && 
                   (_HeadBlockFirstEntryIx < RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock))
            {
                RvdLogLsnEntry* entry = &blkPtr->_Entries[_HeadBlockFirstEntryIx];
                if (entry->Lsn >= NewLowestLsn)
                {
                    _LowestLsn = NewLowestLsn;
                    return;
                }

                // Removed the entry
                _TotalSize -= ComputeTotalRecordSizeOnDisk(entry->HeaderAndMetaDataSize, entry->IoBufferSize);
                _NumberOfEntires--;
                _HeadBlockFirstEntryIx++;
            }

            _Blocks.RemoveHead();
            _delete(blkPtr);
            blkPtr = nullptr;

            _HeadBlockFirstEntryIx = 0;
        }

        // Went empty
        KAssert(_NumberOfEntires == 0);
        KAssert(_TotalSize == 0);
        _LowestLsn = RvdLogLsn::Null();
        _HighestLsn = RvdLogLsn::Null();
    }
}

ULONG
RvdLogLsnEntryTracker::QueryNumberOfRecords()
{
    ULONG result = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        // Truncate operation effects are relected atomically
        result = _NumberOfEntires;
    }

    return result;
}

NTSTATUS
RvdLogLsnEntryTracker::QueryRecord(
    __in ULONG              LogRecordIndex,
    __out RvdLogLsn&        Lsn,
    __out_opt ULONG* const  HeaderAndMetaDataSize,
    __out_opt ULONG* const  IoBufferSize)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (LogRecordIndex >= _NumberOfEntires)
        {
            Lsn = RvdLogLsn::Null();
            if (HeaderAndMetaDataSize != nullptr)
            {
                *HeaderAndMetaDataSize = 0;
            }
            if (IoBufferSize != nullptr)
            {
                *IoBufferSize = 0;
            }
            return STATUS_INVALID_PARAMETER;
        }

        RvdLogLsnEntryBlock*    blkPtr = _Blocks.PeekHead();
        RvdLogLsnEntry*         entry = nullptr;
        ULONG                   entriesInFirstBlk;

        entriesInFirstBlk   = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock - _HeadBlockFirstEntryIx;
        if (LogRecordIndex < entriesInFirstBlk)
        {
            // Requested item is within the first (head) block
            entry = &blkPtr->_Entries[LogRecordIndex + _HeadBlockFirstEntryIx];
        }
        else
        {
            LogRecordIndex -= entriesInFirstBlk;
            blkPtr = _Blocks.Successor(blkPtr);
            KAssert(blkPtr != nullptr);

            // Skip any full blocks not containing the requested item
            ULONG fullsBlocksToSkip = LogRecordIndex / RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
            LogRecordIndex -= (fullsBlocksToSkip * RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock);

            while (fullsBlocksToSkip > 0)
            {
                blkPtr = _Blocks.Successor(blkPtr);
                KAssert(blkPtr != nullptr);
                fullsBlocksToSkip--;
            }

            entry = &blkPtr->_Entries[LogRecordIndex];
        }

        // Return the request item state
        Lsn = entry->Lsn;
        if (HeaderAndMetaDataSize != nullptr)
        {
            *HeaderAndMetaDataSize = entry->HeaderAndMetaDataSize;
        }
        if (IoBufferSize != nullptr)
        {
            *IoBufferSize = entry->IoBufferSize;
        }
    }

    return STATUS_SUCCESS;
}

LONGLONG
RvdLogLsnEntryTracker::QueryTotalSize()
{
    LONGLONG result = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        // Truncate operation effects are relected atomically
        result = _TotalSize;
    }

    return result;
}

NTSTATUS
RvdLogLsnEntryTracker::GetAllRecordLsns(
    __inout ULONG* const                            NumberOfEntries,
    __out_ecount(*NumberOfEntries) RvdLogLsnEntry*  ResultingLsnEntries)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if ((*NumberOfEntries) < _NumberOfEntires)
        {
            *NumberOfEntries = _NumberOfEntires;
            return STATUS_INVALID_PARAMETER;
        }

        RvdLogLsnEntryBlock*    blkPtr = _Blocks.PeekHead();
        ULONG                   entriesInFirstBlk;
        
        entriesInFirstBlk = RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock - _HeadBlockFirstEntryIx;
        if (entriesInFirstBlk > (*NumberOfEntries))
        {
            // Allow for a short (only) block
            entriesInFirstBlk = (*NumberOfEntries);
        }

        if (entriesInFirstBlk > 0)
        {
            KMemCpySafe(
                ResultingLsnEntries,
                entriesInFirstBlk * sizeof(RvdLogLsnEntry),     // entriesInFirstBlk is the corect limit give its computed and limited above
                &blkPtr->_Entries[_HeadBlockFirstEntryIx],
                entriesInFirstBlk * sizeof(RvdLogLsnEntry));
        }

        ULONG entriesToDo = _NumberOfEntires - entriesInFirstBlk;
        ResultingLsnEntries += entriesInFirstBlk;
        blkPtr = _Blocks.Successor(blkPtr);

        while (entriesToDo >= RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock)
        {
            KMemCpySafe(
                ResultingLsnEntries, 
                RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock * sizeof(RvdLogLsnEntry),
                &blkPtr->_Entries[0], 
                RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock * sizeof(RvdLogLsnEntry));

            blkPtr = _Blocks.Successor(blkPtr);
            entriesToDo -= RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
            ResultingLsnEntries += RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock;
        }

        if (entriesToDo > 0)
        {
            KAssert(entriesToDo == (_TailBlockLastEntryIx + 1));
            KMemCpySafe(
                ResultingLsnEntries, 
                RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock * sizeof(RvdLogLsnEntry),
                &blkPtr->_Entries[0],
                entriesToDo * sizeof(RvdLogLsnEntry));
        }
    }

    return STATUS_SUCCESS;
}

//* This routine returns a copy of all log record LSN entries into a supplied KIoBuffer; extending
//  that KIoBuffer as needed.
//
//  Arguments:
//      SegmentSize     - Size that limits the number of entries before PreableSize bytes are skipped.
//      PreambleSize    - Number of bytes to skip at the front of each segment (except for the 1st).
//      IoBuffer        - KIoBuffer[IoBufferOffset on entry] == RvdLogLsnEntry[NumberOfEntries] on
//                        return. If multiple segments are generated (NumberOfSegments > 1), 1)  end of
//                        segments are padded as needed such that a returned entry is not split across
//                        4k page boundary AND PreambleSize space is skipped at the start of each segment
//                        after the first.
//                        
//      IoBufferOffset      - Logical starting into IoBuffer that the first RvdLogLsnEntry will be stored.
//      TotalNumberOfEntries- Returns the number of LSN entries copied into IoBuffer.
//      NumberOfEntriesIn1stSegment - Returns number of entries stored in 1st segment at IoBufferOffset (if any)
//      NumberOfSegments            - Returns total number of segments needed to store the results
//
//  Return Value:
//      STATUS_SUCCESS
//      STATUS_INSUFFICIENT_RESOURCES
//
NTSTATUS
RvdLogLsnEntryTracker::GetAllRecordLsnsIntoIoBuffer(
    __in ULONG          SegmentSize,
    __in ULONG          PreambleSize,
    __in KIoBuffer&     IoBuffer,
    __inout ULONG&      IoBufferOffset,
    __out ULONG&        TotalNumberOfEntries,
    __out ULONG&        NumberOfEntriesIn1stSegment,
    __out ULONG&        NumberOfSegments)
{
    // BUG, richhas, 347, consider merging the math here into something common with 
    //                    RvdAsnIndex::GetAllRecordLsnsIntoIoBuffer() and 
    //                    RvdLogUserStreamCheckPointRecordHeader::ComputeSizeOnDisk().
    //                    Maybe extending KIoBufferStream would be a better choice. TBD.

    ULONG const perSegmentSpace = SegmentSize - PreambleSize;
    ULONG const maxLsnEntriesPerSegment = perSegmentSpace / sizeof(RvdLogLsnEntry);
    ULONG const offsetTo1stSegment = (IoBufferOffset / SegmentSize) * SegmentSize;
    ULONG const offsetOverheadIn1stSegment = IoBufferOffset % SegmentSize;
    ULONG const maxLsnEntriesIn1stSegment = (SegmentSize - offsetOverheadIn1stSegment) / sizeof(RvdLogLsnEntry);

    ULONG       itemsToCopy = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        // Compute the total size needed in IoBuffer to hold all of this instance's 
        // Lsn records plus any data represented by the passed IoBufferOffset value.
        ULONG const numberOfLsnsIn1stSegment = __min(maxLsnEntriesIn1stSegment, _NumberOfEntires);
        ULONG const fullSegmentsForLsnEntries = (_NumberOfEntires - numberOfLsnsIn1stSegment) / maxLsnEntriesPerSegment;
        ULONG const numberOfLsnsInLastSegment = 
            _NumberOfEntires - ((fullSegmentsForLsnEntries * maxLsnEntriesPerSegment) + numberOfLsnsIn1stSegment);

        ULONG const totalSegmentsRequired = 
            ((numberOfLsnsIn1stSegment > 0) ? 1 : 0) + fullSegmentsForLsnEntries + ((numberOfLsnsInLastSegment > 0) ? 1 : 0);

        ULONG const spaceNeededIn1stSegment = (numberOfLsnsIn1stSegment * sizeof(RvdLogLsnEntry));
        ULONG const wasteIn1stSegment = (fullSegmentsForLsnEntries + numberOfLsnsInLastSegment > 0) 
                                                                ? (SegmentSize - spaceNeededIn1stSegment) 
                                                                : offsetOverheadIn1stSegment;
        ULONG const startingOffset = IoBufferOffset;

        // Guarentee IoBuffer is large enough and return __out values
        {   
            KAssert(spaceNeededIn1stSegment <= perSegmentSpace);

            ULONG       totalSizeOfIoBuffer = 
                            offsetTo1stSegment +
                            (spaceNeededIn1stSegment + wasteIn1stSegment) +
                            (fullSegmentsForLsnEntries * SegmentSize) + 
                            ((numberOfLsnsInLastSegment * sizeof(RvdLogLsnEntry)) + ((numberOfLsnsInLastSegment > 0) 
                                                                                                ? PreambleSize 
                                                                                                : 0));

            // Return results
            TotalNumberOfEntries = _NumberOfEntires;
            IoBufferOffset = totalSizeOfIoBuffer;
            NumberOfEntriesIn1stSegment = numberOfLsnsIn1stSegment;
            NumberOfSegments = totalSegmentsRequired;

            totalSizeOfIoBuffer = RvdDiskLogConstants::RoundUpToBlockBoundary(totalSizeOfIoBuffer);

            if (totalSizeOfIoBuffer > IoBuffer.QuerySize())
            {
                // Must extend the IoBuffer to contain an addition of (totalSizeOfIoBuffer - IoBuffer.QuerySize());
                KIoBufferElement::SPtr  newElement;
                void*                   newBuffer;

                NTSTATUS status = KIoBufferElement::CreateNew(
                    totalSizeOfIoBuffer - IoBuffer.QuerySize(),
                    newElement,
                    newBuffer,
                    _Allocator,
                    KTL_TAG_LOGGER);

                if (!NT_SUCCESS(status))
                {
                    TotalNumberOfEntries = 0;
                    NumberOfEntriesIn1stSegment = 0;
                    NumberOfSegments = 0;
                    IoBufferOffset = startingOffset;
                    return status;
                }

                IoBuffer.AddIoBufferElement(*newElement);
            }
        }

        itemsToCopy = _NumberOfEntires;

        // Move items into IoBuffer
        if (itemsToCopy > 0)
        {
            RvdLogLsnEntryBlock*    currentBlkPtr;
            UCHAR*                  currentBlkBytePtr;
            ULONG                   currentBlkLength;
            KIoBufferStream         ioBufferStream(IoBuffer, startingOffset);
            ULONG                   remainingSegmentSpace = spaceNeededIn1stSegment;

            // Intital conditions
            currentBlkPtr = _Blocks.PeekHead();
            currentBlkBytePtr = (UCHAR*)&currentBlkPtr->_Entries[_HeadBlockFirstEntryIx];
            currentBlkLength = __min(
                (itemsToCopy *  sizeof(RvdLogLsnEntry)), 
                ((RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock - _HeadBlockFirstEntryIx) * sizeof(RvdLogLsnEntry)));

#pragma warning(disable:4127)   // C4127: conditional expression is constant
            while (TRUE)
            {
                ULONG sizeToDo = __min(remainingSegmentSpace, currentBlkLength);
                KAssert((sizeToDo % sizeof(RvdLogLsnEntry)) == 0);

                if (!ioBufferStream.Put(currentBlkBytePtr, sizeToDo))
                {
                    KInvariant(FALSE);
                }

                KAssert(currentBlkLength >= sizeToDo);
                currentBlkLength -= sizeToDo;
                currentBlkBytePtr += sizeToDo;

                KAssert(remainingSegmentSpace >= sizeToDo);
                remainingSegmentSpace -= sizeToDo;

                itemsToCopy -= sizeToDo / sizeof(RvdLogLsnEntry);
                sizeToDo = 0;           // Value absorbed

                if (itemsToCopy > 0)
                {
                    // More to do
                    if (remainingSegmentSpace == 0)
                    {
                        // Move to the next segment
                        KAssert((currentBlkLength % sizeof(RvdLogLsnEntry)) == 0);      // Entries do not split across segments

                        // Need to move IoBuffer Cursor fwd to the next Block boundary + segment Preamble overhead
                        ULONG       pos = ioBufferStream.GetPosition();
                        KInvariant(ioBufferStream.Skip((RvdDiskLogConstants::RoundUpToBlockBoundary(pos) - pos) + PreambleSize));

                        // Reset the available space in the current segment; limited to itemsToCopy
                        remainingSegmentSpace = __min(
                            (maxLsnEntriesPerSegment * sizeof(RvdLogLsnEntry)), 
                            (itemsToCopy * sizeof(RvdLogLsnEntry)));
                    }

                    if (currentBlkLength == 0)
                    {
                        // out of source space - move to next RvdLogLsnEntryBlock
                        currentBlkPtr = _Blocks.Successor(currentBlkPtr);
                        KAssert(currentBlkPtr != nullptr);

                        currentBlkBytePtr = (UCHAR*)(&currentBlkPtr->_Entries[0]);
                        currentBlkLength = __min(
                            (itemsToCopy *  sizeof(RvdLogLsnEntry)), 
                            (sizeof(RvdLogLsnEntry) * RvdLogLsnEntryBlock::_NumberOfEntriesPerBlock));
                    }
                }
                else break;
            }
        }
    }

    return STATUS_SUCCESS;
}

//** RvdAsnIndex implementation
RvdAsnIndexEntry::SavedState::SavedState()
    :   _Disposition(RvdLogStream::RecordDisposition::eDispositionNone),
        _MappingEntry(RvdLogAsn::Null(), 0, RvdLogLsn::Null(), 0)
{
}

RvdAsnIndexEntry::~RvdAsnIndexEntry()
{
}

VOID 
RvdAsnIndexEntry::Save(__out SavedState& To)
{
    To._Disposition = _Disposition;
    To._MappingEntry = _MappingEntry;
    To._LowestLsnOfHigherASNs = _LowestLsnOfHigherASNs;
}

VOID 
RvdAsnIndexEntry::Restore(__in SavedState& From)
{
    _Disposition = From._Disposition;
    _MappingEntry = From._MappingEntry;
    _LowestLsnOfHigherASNs = From._LowestLsnOfHigherASNs;
}

LONG
RvdAsnIndexEntry::Comparator(__in RvdAsnIndexEntry& Left, __in RvdAsnIndexEntry& Right)
{
    if (Left._MappingEntry.RecordAsn < Right._MappingEntry.RecordAsn)
    {
        return -1;
    }
    if (Left._MappingEntry.RecordAsn > Right._MappingEntry.RecordAsn)
    {
        return 1;
    }
    return 0;
}

ULONG
RvdAsnIndexEntry::GetLinksOffset()
{
    return FIELD_OFFSET(RvdAsnIndexEntry, _AsnIndexEntryLinks);
}

RvdAsnIndexEntry::RvdAsnIndexEntry(__in RvdLogAsn Asn, __in ULONGLONG Version, __in ULONG IoBufferSize)
    :   _MappingEntry(Asn, Version, RvdLogLsn::Null(), IoBufferSize),
        _Disposition(RvdLogStream::RecordDisposition::eDispositionNone)
{
    #if DBG
        DebugContext = 0;
    #endif
}

RvdAsnIndexEntry::RvdAsnIndexEntry(
    __in RvdLogAsn Asn, 
    __in ULONGLONG Version, 
    __in ULONG IoBufferSize, 
    __in RvdLogStream::RecordDisposition Disposition,
    __in RvdLogLsn Lsn)
    :   _MappingEntry(Asn, Version, Lsn, IoBufferSize),
        _Disposition(Disposition)
{
}


RvdAsnIndex::RvdAsnIndex(__in KAllocator& Allocator)
    :   _Table(
            RvdAsnIndexEntry::GetLinksOffset(), 
            KNodeTable<RvdAsnIndexEntry>::CompareFunction(&RvdAsnIndexEntry::Comparator))
{
    _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] = 0;
    _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] = 0;
    _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted] = 0;
    _LookupKey = _new(KTL_TAG_LOGGER, Allocator) RvdAsnIndexEntry(RvdLogAsn::Min(), 0, 0);
    if (_LookupKey == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
}

RvdAsnIndex::~RvdAsnIndex()
{
    Clear();
}

VOID
RvdAsnIndex::Clear()
{
    K_LOCK_BLOCK(_ThisLock)
    {
        RvdAsnIndexEntry*   entry = nullptr;

        while ((entry = _Table.First()) != nullptr)
        {
            _Table.Remove(*entry);
            entry->Release();   // Reverse the #1 AddRef in UnsafeAddOrUpdate() below
        }

        _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] = 0;
        _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] = 0;
        _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted] = 0;
    }
}

NTSTATUS
RvdAsnIndex::Load( __in RvdAsnIndex& Src)
{
    // BUG, richhas, xxxxx, Consider if this needed to be safe or if "unsafe" semantics work
    K_LOCK_BLOCK(_ThisLock)
    {
        K_LOCK_BLOCK(Src._ThisLock)
        {
            KInvariant(_Table.Count() == 0);

            RvdAsnIndexEntry::SPtr  nextEntry;
            while ((nextEntry = Src._Table.First()) != nullptr)
            {
                Src._Table.Remove(*nextEntry.RawPtr());

                RvdAsnIndexEntry::SPtr          resultIndexEntry;
                RvdAsnIndexEntry::SavedState    previousIndexEntryValue;
                BOOLEAN                         previousIndexEntryWasStored;

                ULONGLONG dontCare = 0;
                NTSTATUS status = UnsafeAddOrUpdate(
                    nextEntry,
                    resultIndexEntry,
                    previousIndexEntryValue,
                    previousIndexEntryWasStored,
                    dontCare);

                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                KInvariant(!previousIndexEntryWasStored);

                // Reverse the #1 AddRef in UnsafeAddOrUpdate() below - extra AddRef() due
                // not using "public" removal API
                KInvariant(nextEntry->Release() > 0);   
            }

            Src._CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] = 0;
            Src._CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] = 0;
            Src._CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted] = 0;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdAsnIndex::AddOrUpdate(
    __in RvdAsnIndexEntry::SPtr& IndexEntry, 
    __out RvdAsnIndexEntry::SPtr& ResultIndexEntry,
    __out RvdAsnIndexEntry::SavedState& PreviousIndexEntryValue,
    __out BOOLEAN& PreviousIndexEntryWasStored,
    __out ULONGLONG& DuplicateRecordAsnVersion)
{
    NTSTATUS        status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_ThisLock)
    {
        status = UnsafeAddOrUpdate(IndexEntry, ResultIndexEntry, PreviousIndexEntryValue, PreviousIndexEntryWasStored, DuplicateRecordAsnVersion);
    }

    return status;
}

NTSTATUS
RvdAsnIndex::UnsafeAddOrUpdate(
    __in RvdAsnIndexEntry::SPtr& IndexEntry, 
    __out RvdAsnIndexEntry::SPtr& ResultIndexEntry,
    __out RvdAsnIndexEntry::SavedState& PreviousIndexEntryValue,
    __out BOOLEAN& PreviousIndexEntryWasStored,
    __out ULONGLONG& DuplicateRecordAsnVersion)
{
    KAssert(!IndexEntry->IsIndexedByAsn());

    //
    // This makes OACR happy. PreviousIndexEntryValue only useful as an
    // out parameter if PreviousIndexentryWasStored returns TRUE.
    //
    PreviousIndexEntryValue;
    
    // BUG, nobertk, xxxxx, consider allowing an optional dup Entry pointer to be returned.
    if (_Table.Insert(*IndexEntry))
    {
        // BUG, richhas, xxxxx, consider a different approach - 2 add refs being done here
        ResultIndexEntry = IndexEntry;
        PreviousIndexEntryWasStored = FALSE;

        IndexEntry->AddRef();                   // Keep add ref'd as long as indexed - #1
        UnsafeAccountForNewItem(IndexEntry->_Disposition);
        UnsafeUpdateLowestLsnOfHigherASNs(*IndexEntry);
    }
    else
    {
        // Do an update
        RvdAsnIndexEntry* currentEntry = _Table.Lookup(*IndexEntry);
        KAssert(currentEntry != nullptr);
        KAssert(currentEntry->_MappingEntry.RecordAsn == IndexEntry->_MappingEntry.RecordAsn);
        KAssert(currentEntry->IsIndexedByAsn());

        if (currentEntry->_MappingEntry.RecordAsnVersion >= IndexEntry->_MappingEntry.RecordAsnVersion)
        {
            DuplicateRecordAsnVersion = currentEntry->_MappingEntry.RecordAsnVersion;
            PreviousIndexEntryWasStored = FALSE;
            ResultIndexEntry.Reset();
            return STATUS_OBJECT_NAME_COLLISION;
        }

        // A newer version is being written - update; save the previous version - to support back out logic
        PreviousIndexEntryWasStored = TRUE;
        currentEntry->Save(PreviousIndexEntryValue);
        UnsafeAccountForChangedDisposition(currentEntry->_Disposition, IndexEntry->_Disposition);

        ResultIndexEntry = currentEntry;
        currentEntry->_MappingEntry.RecordAsnVersion = IndexEntry->_MappingEntry.RecordAsnVersion;
        currentEntry->_Disposition = IndexEntry->_Disposition;
        currentEntry->_MappingEntry.RecordLsn = IndexEntry->_MappingEntry.RecordLsn;
        currentEntry->_MappingEntry.IoBufferSizeHint = IndexEntry->_MappingEntry.IoBufferSizeHint;

        if (PreviousIndexEntryValue.GetDisposition() == RvdLogStream::RecordDisposition::eDispositionNone)
        {
            UnsafeUpdateLowestLsnOfHigherASNs(*IndexEntry);
        }
    }

    return STATUS_SUCCESS;
}

RvdLogLsn
RvdAsnIndex::GetLowestLsnOfHigherASNs(__in RvdLogLsn HighestKnownLsn)
{
    RvdAsnIndexEntry*   entry = nullptr;
    RvdLogLsn lowestLsnOfHigherASNs = RvdLogLsn::Max();
    
    K_LOCK_BLOCK(_ThisLock)
    {
        entry = _Table.First();
        while (entry != nullptr)
        {
            if (entry->GetDisposition() != RvdLogStream::RecordDisposition::eDispositionNone)
            {
                if (entry->_LowestLsnOfHigherASNs < lowestLsnOfHigherASNs)
                {
                    lowestLsnOfHigherASNs = entry->_LowestLsnOfHigherASNs;
                }
            }
            entry = _Table.Next(*entry);
        }
    }

    if (lowestLsnOfHigherASNs == RvdLogLsn::Max())
    {
        return(HighestKnownLsn);
    }

    return(lowestLsnOfHigherASNs.Get() - 1);
}

NTSTATUS
RvdAsnIndex::TryRemoveForDelete(
    __in RvdLogAsn Asn,
    __in ULONGLONG ForVersion,
    __out RvdLogLsn& TruncatableLsn
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    RvdAsnIndexEntry* currentEntry = nullptr;
    RvdAsnIndexEntry* prevEntry = nullptr;

    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        currentEntry = _Table.Lookup(*_LookupKey);
        if (currentEntry != nullptr)
        {
            if (ForVersion == currentEntry->_MappingEntry.RecordAsnVersion)
            {
                KAssert(currentEntry->IsIndexedByAsn());

                // Exclude the _LowestLsnOfHigherASNs
                TruncatableLsn = currentEntry->_LowestLsnOfHigherASNs.Get() - 1;
                prevEntry = _Table.Previous(*currentEntry);

                KInvariant(currentEntry->_Disposition == RvdLogStream::eDispositionPersisted);
                status = STATUS_SUCCESS;
                _Table.Remove(*currentEntry);
                UnsafeAccountForRemovedItem(currentEntry->_Disposition);

                //
                // assign LSN from deleted ASN entry to the previous one if the previous one is less than the current one
                // so that space from the deleted one can be freed when the previous one is truncated or deleted. If this is not done
                // then the space for this deleted one won't get freed until a truncation after its LSN
                //
                if (prevEntry != nullptr)
                {
                    if (prevEntry->_LowestLsnOfHigherASNs < currentEntry->_LowestLsnOfHigherASNs)
                    {
                        prevEntry->_LowestLsnOfHigherASNs = currentEntry->_LowestLsnOfHigherASNs;
                    }
                }
            }
        }
    }

    if (NT_SUCCESS(status) && (currentEntry != nullptr))
    {
        // BUG, richhas, xxxxxx, consider a dedicated trace record type
        KDbgPrintf(
            "RvdAsnIndex::TryRemoveByAsn: ASN: %I64u; V: %I64u; L: %I64i; D: %u; LoHL: %I64i\n",
            currentEntry->GetAsn().Get(),
            currentEntry->GetVersion(),
            currentEntry->GetLsn().Get(),
            currentEntry->GetDisposition(),
            currentEntry->GetLowestLsnOfHigherASNs());

        currentEntry->Release();              // Reverse the #1 AddRef in AddOrUpdate() above
    }
    return(status);
}

void
RvdAsnIndex::TryRemove(
    __in RvdAsnIndexEntry::SPtr& IndexEntry, 
    __in ULONGLONG ForVersion)
{
    BOOLEAN     wasRemoved = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (ForVersion == IndexEntry->_MappingEntry.RecordAsnVersion)
        {
            KAssert(IndexEntry->IsIndexedByAsn());
            wasRemoved = TRUE;
            _Table.Remove(*(IndexEntry.RawPtr()));
            UnsafeAccountForRemovedItem(IndexEntry->_Disposition);
        }
    }

    if (wasRemoved)
    {
        // BUG, richhas, xxxxxx, consider a dedicated trace record type
        KDbgPrintf(
            "RvdAsnIndex::TryRemove: ASN: %I64u; V: %I64u; L: %I64i; D: %u; LoHL: %I64i\n",
            IndexEntry->GetAsn().Get(),
            IndexEntry->GetVersion(),
            IndexEntry->GetLsn().Get(),
            IndexEntry->GetDisposition(),
            IndexEntry->GetLowestLsnOfHigherASNs());

        IndexEntry->Release();              // Reverse the #1 AddRef in AddOrUpdate() above
    }
}

VOID
RvdAsnIndex::Restore(
    __in RvdAsnIndexEntry::SPtr& IndexEntry, 
    __in ULONGLONG ForVersion, 
    __in RvdAsnIndexEntry::SavedState& EntryStateToRestore)
{
    BOOLEAN                         wasRestored = FALSE;
    RvdAsnIndexEntry::SavedState    entryBeforeRestore;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (ForVersion == IndexEntry->_MappingEntry.RecordAsnVersion)
        {
            KAssert(IndexEntry->IsIndexedByAsn());
            KAssert(IndexEntry->GetAsn() == EntryStateToRestore.GetMappingEntry().RecordAsn);

            IndexEntry->Save(entryBeforeRestore);
            wasRestored = TRUE;
            UnsafeAccountForChangedDisposition(IndexEntry->_Disposition, EntryStateToRestore.GetDisposition());
            IndexEntry->Restore(EntryStateToRestore);

            if (entryBeforeRestore.GetDisposition() == RvdLogStream::RecordDisposition::eDispositionNone)
            {
                // This Record's Disposition change may be interesting; capture _LowestLsnOfHigherASNs if so
                UnsafeUpdateLowestLsnOfHigherASNs(*IndexEntry);
            }
        }
    }

    if (wasRestored)
    {
        // BUG, richhas, xxxxxx, consider a dedicated trace record type
        KDbgPrintf(
            "RvdAsnIndex::Restore: ASN: %I64u; Old:{V: %I64u; L: %I64i; D: %u; LoHL: %I64i}; Rest:{V: %I64u; L: %I64i; D: %u; LoHL: %I64i}\n",
            entryBeforeRestore.GetMappingEntry().RecordAsn.Get(),
            entryBeforeRestore.GetMappingEntry().RecordAsnVersion,
            entryBeforeRestore.GetMappingEntry().RecordLsn.Get(),
            entryBeforeRestore.GetDisposition(),
            entryBeforeRestore.GetLowestLsnOfHigherASNs().Get(),
            EntryStateToRestore.GetMappingEntry().RecordAsnVersion,
            EntryStateToRestore.GetMappingEntry().RecordLsn.Get(),
            EntryStateToRestore.GetDisposition(),
            EntryStateToRestore.GetLowestLsnOfHigherASNs().Get());
    }
}

BOOLEAN
RvdAsnIndex::UpdateDisposition(
    __in RvdAsnIndexEntry::SPtr& IndexEntry, 
    __in ULONGLONG ForVersion, 
    __in RvdLogStream::RecordDisposition NewDisposition)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (ForVersion == IndexEntry->_MappingEntry.RecordAsnVersion)
        {
            KAssert(IndexEntry->IsIndexedByAsn());
            UnsafeAccountForChangedDisposition(IndexEntry->_Disposition, NewDisposition);
            RvdLogStream::RecordDisposition oldDisposition = IndexEntry->_Disposition;
            IndexEntry->_Disposition = NewDisposition;

            if (oldDisposition == RvdLogStream::RecordDisposition::eDispositionNone)
            {
                // This Record's Disposition change may be interesting; capture _LowestLsnOfHigherASNs if so
                UnsafeUpdateLowestLsnOfHigherASNs(*IndexEntry);
            }
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
RvdAsnIndex::UpdateLsnAndDisposition(
    __in RvdAsnIndexEntry::SPtr& IndexEntry, 
    __in ULONGLONG ForVersion, 
    __in RvdLogStream::RecordDisposition NewDisposition,
    __in RvdLogLsn NewLsn)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if (ForVersion == IndexEntry->_MappingEntry.RecordAsnVersion)
        {
            KAssert(IndexEntry->IsIndexedByAsn());
            UnsafeAccountForChangedDisposition(IndexEntry->_Disposition, NewDisposition);
            RvdLogStream::RecordDisposition oldDisposition = IndexEntry->_Disposition;
            IndexEntry->_Disposition = NewDisposition;
            IndexEntry->_MappingEntry.RecordLsn = NewLsn;

            if (oldDisposition == RvdLogStream::RecordDisposition::eDispositionNone)
            {
                // This Record's Disposition change may be interesting; capture _LowestLsnOfHigherASNs if so
                UnsafeUpdateLowestLsnOfHigherASNs(*IndexEntry);
            }
            return TRUE;
        }
    }

    return FALSE;
}

VOID
RvdAsnIndex::UnsafeUpdateLowestLsnOfHigherASNs(__in RvdAsnIndexEntry& IndexEntry)
{
    #if DBG
        RvdLogLsn volatile          old_LowestLsnOfHigherASNs = IndexEntry._LowestLsnOfHigherASNs;      // For debugging
        #define VOLATILE volatile
    #else
        #define VOLATILE
    #endif

    if (IndexEntry._Disposition == RvdLogStream::RecordDisposition::eDispositionNone)
    {
        return;
    }

    // Search up in ASN space to find the next written (or being written) record
    RvdAsnIndexEntry* VOLATILE  nextEntryInAsnOrder = _Table.Next(IndexEntry);
    while ((nextEntryInAsnOrder != nullptr) && 
           (nextEntryInAsnOrder->GetDisposition() == RvdLogStream::RecordDisposition::eDispositionNone))
    {
        nextEntryInAsnOrder = _Table.Next(*nextEntryInAsnOrder);
    }

    KAssert((nextEntryInAsnOrder == nullptr) || (nextEntryInAsnOrder->GetAsn() > IndexEntry.GetAsn()));

    // if there is not a higher ASN or the higher ASNs have a low lsn that is a higher LSN than IndexEntry, use
    // IndexEntry's own LSN as its _LowestLsnOfHigherASNs; else propagate the higher ASNs' _LowestLsnOfHigherASNs.
    //
    // The intent is to keep track of the lowest LSN that can be truncated to safely (in lowest->highest ASN space).
    IndexEntry._LowestLsnOfHigherASNs = ((nextEntryInAsnOrder == nullptr) || 
                                         (nextEntryInAsnOrder->_LowestLsnOfHigherASNs > IndexEntry._MappingEntry.RecordLsn))
                                        ? IndexEntry._MappingEntry.RecordLsn 
                                        : nextEntryInAsnOrder->_LowestLsnOfHigherASNs;

    // Update any records (in lower ASN space) that have a _LowestLsnOfHigherASN value greater than the current record's 
    // (represented by IndexEntry). The idea is that knowledge of the lowest record's LSN space is pushed toward lower 
    // ASN descriptors kept in ascending ASN order. This protects records that are in lower LSN space (but higher ASN space) 
    // during truncation - see Truncate() below.
    //
    RvdAsnIndexEntry* VOLATILE  prevEntryInAsnOrder = _Table.Previous(IndexEntry);
    RvdLogLsn                   indexEntry_LowestLsnOfHigherASNs = IndexEntry._LowestLsnOfHigherASNs;              
    while (prevEntryInAsnOrder != nullptr)
    {
        if (prevEntryInAsnOrder->GetDisposition() != RvdLogStream::RecordDisposition::eDispositionNone)
        {
            if (prevEntryInAsnOrder->_LowestLsnOfHigherASNs <= indexEntry_LowestLsnOfHigherASNs)
            {
                // Once we detect any prev record's _LowestLsnOfHigherASNs lower/equal to the current's, we are
                // done because any record below that prev record will have at least a _LowestLsnOfHigherASNs
                // that equal or lower.
                break;
            }

            KInvariant(prevEntryInAsnOrder->_MappingEntry.RecordLsn >= indexEntry_LowestLsnOfHigherASNs);

            // A lower ASN has a higher _LowestLsnOfHigherASNs - fix it.
            prevEntryInAsnOrder->SetLowestLsnOfHigherASNs(indexEntry_LowestLsnOfHigherASNs);
        }

        prevEntryInAsnOrder = _Table.Previous(*prevEntryInAsnOrder);
    }

    #undef VOLATILE
}

// Validate that UnsafeUpdateLowestLsnOfHigherASNs() has maintained the correct _LowestLsnOfHigherASNs values
// across an entire RvdAsnIndex. Formally Validate() must hold for correctness of an RvdAsnIndex to be maintained.
//
// For test usage.
//
BOOLEAN
RvdAsnIndex::Validate()
{
    RvdLogLsn       currentLsnOfHigherASNs(RvdLogLsn::Max());

    K_LOCK_BLOCK(_ThisLock)
    {
        RvdAsnIndexEntry* currentEntryInAsnOrder = _Table.Last();

        while (currentEntryInAsnOrder != nullptr)
        {
            if (currentEntryInAsnOrder->GetDisposition() != RvdLogStream::RecordDisposition::eDispositionNone)
            {
                if (currentEntryInAsnOrder->_LowestLsnOfHigherASNs > currentLsnOfHigherASNs)
                {
                    // Found an ASN entry that does not protect the LSN space of its higher ASNs
                    return FALSE;
                }

                if (currentEntryInAsnOrder->_MappingEntry.RecordLsn < currentEntryInAsnOrder->_LowestLsnOfHigherASNs)
                {
                    // Found an ASN entry that does not protect its own LSN space
                    return FALSE;
                }

                currentLsnOfHigherASNs = currentEntryInAsnOrder->_LowestLsnOfHigherASNs;
            }

            currentEntryInAsnOrder = _Table.Previous(*currentEntryInAsnOrder);
        }
    }

    return TRUE;
}

RvdLogLsn
RvdAsnIndex::Truncate(RvdLogAsn UpToAsn, RvdLogLsn HighestKnownLsn)
{
    RvdLogLsn   truncatableLsn = RvdLogLsn::Null();

    K_LOCK_BLOCK(_ThisLock)
    {
        RvdAsnIndexEntry* entry;

        // Walk the ASN ordered index (list) from lower to higher ASNs truncating all entries below UpToAsn (or 
        // full trucation)
        while ((entry = _Table.First()) != nullptr)
        {
            KAssert(entry->IsIndexedByAsn());

            if (entry->GetAsn() > UpToAsn)
            {
                return truncatableLsn;
            }

            // BUG, richhas, xxxxx, Consider if we should allow an over truncation request.
            // Asn is being truncated - don't allow over truncation from layers above
            KInvariant(entry->_Disposition == RvdLogStream::eDispositionPersisted);

            // _LowestLsnOfHigherASNs must not decrease in ASN space
            KAssert(!entry->_LowestLsnOfHigherASNs.IsNull());
            KAssert(entry->_LowestLsnOfHigherASNs.Get() >= truncatableLsn.Get());

            // Exclude the _LowestLsnOfHigherASNs
            truncatableLsn = entry->_LowestLsnOfHigherASNs.Get() - 1;

            _Table.Remove(*entry);
            UnsafeAccountForRemovedItem(entry->_Disposition);
            entry->Release();                   // Reverse the #1 AddRef in AddOrUpdate() above
        }

        // We have an empty index - return the caller's HighestKnownLsn if that
        // is beyond the last records LSN's _LowestLsnOfHigherASNs. Covers an out
        // of order (in LSN space) at the end of a stream.
        if (HighestKnownLsn > truncatableLsn)
        {
            return HighestKnownLsn;
        }
    }

    return truncatableLsn;
}

//* This routine returns a copy of all AsnLsnMappingEntry(s) into a supplied KIoBuffer in a segmented 
//  fashion; extending that KIoBuffer as needed. Each segment will be the same size (SegmentSize) and
//  will have a preamble (skipped) of PreableSize at the start of all but the first segment - it is
//  assumed that the caller will consider any first preamble needs in the initial IoBufferOffset 
//  passed. Only record metadata w/Dispositions of Pending or Persisted are returned.
//
//  NOTE: Entries will not be split across a segment boundary - unused space 
//
//  Arguments:
//      SegmentSize     - Size that limits the number of entries before PreableSize bytes are skipped.
//      PreambleSize    - Number of bytes to skip at the front of each segment (except for the 1st).
//      IoBuffer        - KIoBuffer - Contents below initial IoBufferOffset value are not disturbed. On
//                                    return IoBuffer has NumberOfEntries copied in the specified 
//                                    segmented format: [IoBufferOffset][Entries][pad][[Preabmle][Entries][pad]]...
//      IoBufferOffset  - Logical starting point in IoBuffer that the first segment's AsnLsnMappingEntry(s) will be stored.
//      NumberOfEntries - Returns the number of LSN entries copied into IoBuffer.
//      TotalSegments   - Returns the total number of segments that AsnLsnMappingEntry were stored in.
//
//  Return Value:
//      STATUS_SUCCESS  - Worked; else not
//
NTSTATUS
RvdAsnIndex::GetAllEntriesIntoIoBuffer(
    __in KAllocator&        Allocator,
    __in ULONG              SegmentSize,
    __in ULONG              PreambleSize,
    __inout KIoBuffer&      IoBuffer,
    __inout ULONG&          IoBufferOffset,
    __out ULONG&            NumberOfEntries,
    __out ULONG&            TotalSegments)
{
    KInvariant((SegmentSize % RvdDiskLogConstants::BlockSize) == 0);
    KInvariant(PreambleSize < RvdDiskLogConstants::BlockSize);

    ULONG const perSegmentSpace = SegmentSize - PreambleSize;
    ULONG const maxAsnEntriesPerSegment = perSegmentSpace / sizeof(AsnLsnMappingEntry);
    ULONG const offsetTo1stSegment = (IoBufferOffset / SegmentSize) * SegmentSize;
    ULONG const offsetOverheadIn1stSegment = IoBufferOffset % SegmentSize;
    ULONG const maxAsnEntriesIn1stSegment = (SegmentSize - offsetOverheadIn1stSegment) / sizeof(AsnLsnMappingEntry);

    ULONG           countOfTableItems = 0;

    K_LOCK_BLOCK(_ThisLock)
    {
        // Compute the total size needed in IoBuffer to hold all of this instance's 
        // Lsn records plus any data represented by the passed *IoBufferOffset value.
        KAssert((_CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] + 
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] + 
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted]) == _Table.Count());

        // NOTE: Both Pending and Persisted records are included here because of the way in which physical writes are
        //       ordered by the StreamWrite pipeline. Records that are pending are in the process of being written and only
        //       a physical disk error can cause them to fail; rendering the log as a whole corrupt. Stream CP records,
        //       which this method supports, should include these Pending ASNs because the resulting CP record will always
        //       fall after these Pending records on disk.
        countOfTableItems = _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] +
                            _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted];

        ULONG const numberOfAsnsIn1stSegment = __min(maxAsnEntriesIn1stSegment, countOfTableItems);
        ULONG const fullSegmentsForAsnEntries = (countOfTableItems - numberOfAsnsIn1stSegment) / maxAsnEntriesPerSegment;
        ULONG const numberOfAsnsInLastSegment = 
            countOfTableItems - ((fullSegmentsForAsnEntries * maxAsnEntriesPerSegment) + numberOfAsnsIn1stSegment);

        ULONG const totalSegmentsRequired = 
            ((numberOfAsnsIn1stSegment > 0) ? 1 : 0) + fullSegmentsForAsnEntries + ((numberOfAsnsInLastSegment > 0) ? 1 : 0);

        ULONG const spaceNeededIn1stSegment = (numberOfAsnsIn1stSegment * sizeof(AsnLsnMappingEntry));
        ULONG const wasteIn1stSegment = (fullSegmentsForAsnEntries + numberOfAsnsInLastSegment > 0) 
                                                        ? (SegmentSize - spaceNeededIn1stSegment) 
                                                        : offsetOverheadIn1stSegment;
        ULONG const startingOffset = IoBufferOffset;

        // Guarentee IoBuffer is large enough and return __out values
        {   
            KAssert(spaceNeededIn1stSegment <= perSegmentSpace);

            ULONG       totalSizeOfIoBuffer = 
                            offsetTo1stSegment +
                            (spaceNeededIn1stSegment + wasteIn1stSegment) +
                            (fullSegmentsForAsnEntries * SegmentSize) + 
                            ((numberOfAsnsInLastSegment * sizeof(AsnLsnMappingEntry)) + ((numberOfAsnsInLastSegment > 0) 
                                                                                                    ? PreambleSize 
                                                                                                    : 0));
            // Return results
            NumberOfEntries = countOfTableItems;
            TotalSegments = totalSegmentsRequired;
            IoBufferOffset = totalSizeOfIoBuffer;

            totalSizeOfIoBuffer = RvdDiskLogConstants::RoundUpToBlockBoundary(totalSizeOfIoBuffer);

            if (totalSizeOfIoBuffer > IoBuffer.QuerySize())
            {
                // Must extend the IoBuffer to contain an addition of (totalSizeOfIoBuffer - IoBuffer.QuerySize());
                KIoBufferElement::SPtr  newElement;
                void*                   newBuffer;

                NTSTATUS status = KIoBufferElement::CreateNew(
                    totalSizeOfIoBuffer - IoBuffer.QuerySize(),
                    newElement,
                    newBuffer,
                    Allocator,
                    KTL_TAG_LOGGER);

                if (!NT_SUCCESS(status))
                {
                    NumberOfEntries = 0;
                    IoBufferOffset = startingOffset;
                    TotalSegments = 0;
                    return status;
                }

                IoBuffer.AddIoBufferElement(*newElement);
            }
        }

        // Move items into IoBuffer
        if (countOfTableItems > 0)
        {
            RvdAsnIndexEntry*       currentBlkPtr = _Table.First();
            KIoBufferStream         ioBufferStream(IoBuffer, startingOffset);
            ULONG                   currentSegmentRemainingSpace = spaceNeededIn1stSegment;

#pragma warning(disable:4127)   // C4127: conditional expression is constant
            while (TRUE)
            {
                while ((currentBlkPtr != nullptr) && !UnsafeIsAllocatedLsnDisposition(currentBlkPtr->_Disposition))
                {
                    currentBlkPtr = _Table.Next(*currentBlkPtr);
                }
                KInvariant(currentBlkPtr != nullptr);

                if (!ioBufferStream.Put(currentBlkPtr->_MappingEntry))
                {
                    KInvariant(FALSE);
                }

                currentSegmentRemainingSpace -= sizeof(AsnLsnMappingEntry);
                countOfTableItems--;

                if (countOfTableItems > 0)
                {
                    // More to do
                    if (currentSegmentRemainingSpace == 0)
                    {
                        // Move to the next segment

                        // Need to move IoBuffer Cursor fwd to the next Block boundary + segment Preamble overhead
                        ULONG       pos = ioBufferStream.GetPosition();
                        KInvariant(ioBufferStream.Skip((RvdDiskLogConstants::RoundUpToBlockBoundary(pos) - pos) + PreambleSize));

                        // Reset the available space in the current segment; limited to countOfTableItems
                        currentSegmentRemainingSpace = __min(
                            (maxAsnEntriesPerSegment * sizeof(AsnLsnMappingEntry)), 
                            (countOfTableItems * sizeof(AsnLsnMappingEntry)));
                    }

                    currentBlkPtr = _Table.Next(*currentBlkPtr);
                    KInvariant(currentBlkPtr != nullptr);
                }
                else break;
            }
        }
    }

    KAssert(countOfTableItems == 0);
    return STATUS_SUCCESS;
}

NTSTATUS
RvdAsnIndex::GetEntries(
    __in RvdLogAsn LowestAsn,
    __in RvdLogAsn HighestAsn,
    __in KArray<RvdLogStream::RecordMetadata>& ResultsArray)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(LowestAsn);
        RvdAsnIndexEntry* currentEntry = _Table.LookupEqualOrNext(*_LookupKey);

        while ((currentEntry != nullptr) && (currentEntry->GetAsn() <= HighestAsn))
        {
            RvdLogStream::RecordMetadata    item;
            item.Asn = currentEntry->GetAsn();
            item.Disposition = currentEntry->GetDisposition();
            item.Size = currentEntry->GetIoBufferSizeHint();
            item.Version = currentEntry->GetVersion();
            item.Debug_LsnValue = currentEntry->GetLsn().Get();

            NTSTATUS status = ResultsArray.Append(item);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            currentEntry = _Table.Next(*currentEntry);
        }
    }

    return STATUS_SUCCESS;
}

VOID
RvdAsnIndex::GetAsnRange(__out RvdLogAsn& LowestAsn, __out RvdLogAsn& HighestAsn, __in RvdLogAsn& HighestAsnObserved)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        ULONG count = _Table.Count();
        if (count == 0)
        {
            LowestAsn = HighestAsnObserved;
            HighestAsn = HighestAsnObserved;
            return;
        }

        LowestAsn = _Table.First()->GetAsn();
        HighestAsn = _Table.Last()->GetAsn();
    }
}

VOID
RvdAsnIndex::GetAsnInformation(
    __in RvdLogAsn                          Asn, 
    __out ULONGLONG&                        Version, 
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry* currentEntry = _Table.Lookup(*_LookupKey);
        if (currentEntry != nullptr)
        {
            Version = currentEntry->GetVersion();
            RecordDisposition = currentEntry->GetDisposition();
            CurrentLsn = currentEntry->GetLsn();
            IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
            return;
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}

VOID
RvdAsnIndex::GetNextAsnInformation(
    __inout RvdLogAsn&                      Asn, 
    __out ULONGLONG&                        Version, 
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.Lookup(*_LookupKey);
        if (currentEntry != nullptr)
        {
            currentEntry = UnsafeNext(currentEntry);
            if (currentEntry != nullptr)
            {
                Asn = currentEntry->GetAsn();
                Version = currentEntry->GetVersion();
                RecordDisposition = currentEntry->GetDisposition();
                CurrentLsn = currentEntry->GetLsn();
                IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
                return;
            }
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}

VOID
RvdAsnIndex::GetNextAsnInformationFromNearest(
    __inout RvdLogAsn&                      Asn, 
    __out ULONGLONG&                        Version, 
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.LookupEqualOrPrevious(*_LookupKey);
        if (currentEntry != nullptr)
        {
            currentEntry = UnsafeNext(currentEntry);
            if (currentEntry != nullptr)
            {
                Asn = currentEntry->GetAsn();
                Version = currentEntry->GetVersion();
                RecordDisposition = currentEntry->GetDisposition();
                CurrentLsn = currentEntry->GetLsn();
                IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
                return;
            }
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}

VOID
RvdAsnIndex::GetNextFromSpecificAsnInformation(
    __inout RvdLogAsn&                      Asn,
    __out ULONGLONG&                        Version,
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.LookupEqualOrNext(*_LookupKey);
        if (currentEntry != nullptr)
        {
            if (currentEntry->GetAsn() == Asn)
            {
                currentEntry = UnsafeNext(currentEntry);
            }

            if (currentEntry != nullptr)
            {
                Asn = currentEntry->GetAsn();
                Version = currentEntry->GetVersion();
                RecordDisposition = currentEntry->GetDisposition();
                CurrentLsn = currentEntry->GetLsn();
                IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
                return;
            }
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}


VOID
RvdAsnIndex::GetPreviousAsnInformation(
    __inout RvdLogAsn&                      Asn, 
    __out ULONGLONG&                        Version, 
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.Lookup(*_LookupKey);
        if (currentEntry != nullptr)
        {
            currentEntry = UnsafePrev(currentEntry);
            if (currentEntry != nullptr)
            {           
                Asn = currentEntry->GetAsn();
                Version = currentEntry->GetVersion();
                RecordDisposition = currentEntry->GetDisposition();
                CurrentLsn = currentEntry->GetLsn();
                IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
                return;
            }
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}

VOID
RvdAsnIndex::GetPreviousAsnInformationFromNearest(
    __inout RvdLogAsn&                      Asn, 
    __out ULONGLONG&                        Version, 
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.LookupEqualOrNext(*_LookupKey);
        if (currentEntry != nullptr)
        {
            currentEntry = UnsafePrev(currentEntry);
            if (currentEntry != nullptr)
            {           
                Asn = currentEntry->GetAsn();
                Version = currentEntry->GetVersion();
                RecordDisposition = currentEntry->GetDisposition();
                CurrentLsn = currentEntry->GetLsn();
                IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
                return;
            }
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}

VOID
RvdAsnIndex::GetPreviousFromSpecificAsnInformation(
    __inout RvdLogAsn&                      Asn,
    __out ULONGLONG&                        Version,
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.LookupEqualOrPrevious(*_LookupKey);
        if (currentEntry != nullptr)
        {
            if (currentEntry->GetAsn() == Asn)
            {
                currentEntry = UnsafePrev(currentEntry);
            }

            if (currentEntry != nullptr)
            {
                Asn = currentEntry->GetAsn();
                Version = currentEntry->GetVersion();
                RecordDisposition = currentEntry->GetDisposition();
                CurrentLsn = currentEntry->GetLsn();
                IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
                return;
            }
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}


VOID
RvdAsnIndex::GetContainingAsnInformation(
    __inout RvdLogAsn&                      Asn, 
    __out ULONGLONG&                        Version, 
    __out RvdLogStream::RecordDisposition&  RecordDisposition,
    __out RvdLogLsn&                        CurrentLsn,
    __out ULONG&                            IoBufferSizeHint)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry* currentEntry = _Table.LookupEqualOrPrevious(*_LookupKey);
        if (currentEntry != nullptr)
        {
            Asn = currentEntry->GetAsn();
            Version = currentEntry->GetVersion();
            RecordDisposition = currentEntry->GetDisposition();
            CurrentLsn = currentEntry->GetLsn();
            IoBufferSizeHint = currentEntry->GetIoBufferSizeHint();
            return;
        }
    }

    Version = 0;
    RecordDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    CurrentLsn = RvdLogLsn::Null();
    IoBufferSizeHint = 0;
}

BOOLEAN
RvdAsnIndex::CheckForRecordsWithHigherVersion(
    __in RvdLogAsn Asn,
    __in ULONGLONG Version,
    __out ULONGLONG& HigherVersion
    )
{
    K_LOCK_BLOCK(_ThisLock)
    {
        _LookupKey->SetAsn(Asn);
        RvdAsnIndexEntry::SPtr currentEntry = _Table.LookupEqualOrPrevious(*_LookupKey);
        while (currentEntry != nullptr)
        {
            if (currentEntry->GetVersion() > Version)
            {
                HigherVersion = currentEntry->GetVersion();
                return(TRUE);
            }
            currentEntry = UnsafePrev(currentEntry);
        }
    }

    return(FALSE);
}

RvdAsnIndexEntry::SPtr
RvdAsnIndex::UnsafeFirst()
{
    RvdAsnIndexEntry::SPtr result = _Table.First();
    return result;
}

RvdAsnIndexEntry::SPtr
RvdAsnIndex::UnsafeLast()
{
    RvdAsnIndexEntry::SPtr result = _Table.Last();
    return result;
}

RvdAsnIndexEntry::SPtr
RvdAsnIndex::UnsafeNext(__in RvdAsnIndexEntry::SPtr& Current)
{
    RvdAsnIndexEntry::SPtr result = _Table.Next(*Current);
    return result;
}

RvdAsnIndexEntry::SPtr
RvdAsnIndex::UnsafePrev(__in RvdAsnIndexEntry::SPtr& Current)
{
    RvdAsnIndexEntry::SPtr result = _Table.Previous(*Current);
    return result;
}




//*** LogStreamDescriptor implementation
LogStreamDescriptor::LogStreamDescriptor(
    __in KAllocator& Allocator, 
    __in RvdLogStreamInformation& StreamInfo)
    :   State(OpenState::Closed),
        SerialNumber(0),
        TruncationPoint(RvdLogAsn::Null()),
        MaxAsnWritten(RvdLogAsn::Null()),
        HighestLsn(RvdLogLsn::Null()),
        LastCheckPointLsn(RvdLogLsn::Null()),
        Info(StreamInfo),
        LsnIndex(Allocator),
        AsnIndex(Allocator),
        StreamTruncateWriteOpsRequested(0)
{
    SetConstructorStatus(AsnIndex.Status());
}

NTSTATUS
LogStreamDescriptor::BuildStreamCheckPointRecordTables(
    __in KAllocator& Allocator, 
    __in KIoBuffer& RecordBuffer,
    __in RvdLogConfig& Config,
    __out ULONG& SizeWrittenInRecordBuffer)
//
//  Builds the RvdLogUserStreamCheckPointRecordHeader.AsnLsnMappingArray and LsnEntryArray
//  arrays in the passed KIoBuffer; also sets NumberOfAsnMappingEntries and NumberOfLsnEntries 
//  on successful exit. RecordBuffer is extended via Allocator as needed.
//
{
    ULONG       numberOfSegments = 0;
    ULONG       numberOfAsnEntries = 0;
    ULONG       numberOfLsnEntries = 0;
    ULONG       offsetIntoBuffer = 0;
    ULONG       segmentSize = 0;
    ULONG       maxAsnEntriesPerSegment = 0;
    ULONG       maxSpaceAvailableInSegment = 0;
    BOOLEAN     doingMultiSegment = FALSE;
    NTSTATUS    status = STATUS_SUCCESS;
    BOOLEAN     sharedAsnAndLsnSegment = TRUE;
    
    SizeWrittenInRecordBuffer = 0;

    do
    {
        RvdLogUserStreamCheckPointRecordHeader::ComputeSizeOnDisk(
            Config,
            AsnIndex.GetNumberOfEntries(), 
            LsnIndex.QueryNumberOfRecords(), 
            numberOfSegments);

        // If numberOfSegments > 1, then we'll use the segmented record format where each segment is 1/2 the size of 
        // the max UserStreamCheckPointRecord - this is a guess in that no locks are held. If we guess wrong, which
        // almost never happens, we'll redo the work. The only risk is that two records could be generated in the
        // worst case where one is all that's needed.
        doingMultiSegment = (numberOfSegments > 1);
        segmentSize = (doingMultiSegment) 
                                ? RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(Config) 
                                : RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(Config);
        maxSpaceAvailableInSegment = segmentSize - sizeof(RvdLogUserStreamCheckPointRecordHeader);
        maxAsnEntriesPerSegment = maxSpaceAvailableInSegment / sizeof(AsnLsnMappingEntry);
        offsetIntoBuffer = FIELD_OFFSET(RvdLogUserStreamCheckPointRecordHeader, AsnLsnMappingArray[0]);

        // Generate ASN table into record array
        status = AsnIndex.GetAllEntriesIntoIoBuffer(
            Allocator, 
            segmentSize,
            offsetIntoBuffer,       // Segment preamble size: header
            RecordBuffer,
            offsetIntoBuffer, 
            numberOfAsnEntries,
            numberOfSegments);
    
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        KAssert(numberOfSegments > 0);

        // At this point: offsetIntoBuffer logically points to the next location in RecordBuffer that could have LsnIndex
        //                state stored. It is possible in a corner case that this could be the exact start of another
        //                segment in a multi-segment case. In this case we need to take note of this fact when computing
        //                the total number of segments below.

        if (doingMultiSegment && 
            ((offsetIntoBuffer % RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(Config)) == 0))
        {
            // Advance the "cursor" into the buffer to the start of next segment
            offsetIntoBuffer += FIELD_OFFSET(RvdLogUserStreamCheckPointRecordHeader, AsnLsnMappingArray[0]);
            sharedAsnAndLsnSegment = FALSE;
        }

        // Snap the LSN entries
        ULONG       numberOf1stSegEntries = 0;
        ULONG       totalLsnIndexSegments = 0;

        status = LsnIndex.GetAllRecordLsnsIntoIoBuffer(
            segmentSize, 
            FIELD_OFFSET(RvdLogUserStreamCheckPointRecordHeader, AsnLsnMappingArray[0]),    // Preamble size 
            RecordBuffer, 
            offsetIntoBuffer, 
            numberOfLsnEntries, 
            numberOf1stSegEntries, 
            totalLsnIndexSegments);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        // Add in the number of additional segments beyond numberOfSegments needed for the AsnLsnMappingEntrys
        numberOfSegments += (numberOf1stSegEntries > 0) ? (totalLsnIndexSegments - 1) : totalLsnIndexSegments;
        if (! sharedAsnAndLsnSegment)
        {
            //
            // In the case where the ASN indexes completely fill a segment we need to add back that segment in
            // the total segment count
            //
            numberOfSegments++;
        }

    }                       /* Retry if single part buffer size exceeded */
    while (!doingMultiSegment && 
          (offsetIntoBuffer >= RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(Config)));

    SizeWrittenInRecordBuffer = offsetIntoBuffer;       // Return size of overall space used in RecordBuffer
    
    //* Next all RvdLogUserStreamCheckPointRecordHeader(s) in RecordBuffer must have their NumberOfAsnMappingEntries, 
    //  NumberOfLsnEntries, NumberOfSegments, and SegmentNumber fields set.
    KIoBufferStream         stream(RecordBuffer, 0);

    for (ULONG segment = 0; segment < numberOfSegments; segment++)
    {
        KAssert(stream.GetBufferPointerRange() >= sizeof(RvdLogUserStreamCheckPointRecordHeader));
        RvdLogUserStreamCheckPointRecordHeader* const hdr = (RvdLogUserStreamCheckPointRecordHeader*)stream.GetBufferPointer();

        hdr->SegmentNumber = segment;
        hdr->NumberOfSegments = numberOfSegments;

        if (numberOfAsnEntries >= maxAsnEntriesPerSegment)
        {
            // AsnLsnMappingEntrys-only segment
            hdr->NumberOfAsnMappingEntries = maxAsnEntriesPerSegment;
            hdr->NumberOfLsnEntries = 0;
            numberOfAsnEntries -= maxAsnEntriesPerSegment;
        }
        else
        {
            // partial/empty AsnLsnMappingEntrys segment
            hdr->NumberOfAsnMappingEntries = numberOfAsnEntries;
            numberOfAsnEntries = 0;

            ULONG const maxLsnEntriesInSegment =    (maxSpaceAvailableInSegment - 
                                                        (hdr->NumberOfAsnMappingEntries * sizeof(AsnLsnMappingEntry))) / 
                                                    sizeof(RvdLogLsnEntry);

            if (numberOfLsnEntries >= maxLsnEntriesInSegment)
            {
                hdr->NumberOfLsnEntries = maxLsnEntriesInSegment;
                numberOfLsnEntries -= maxLsnEntriesInSegment;
            }
            else
            {
                hdr->NumberOfLsnEntries = numberOfLsnEntries;
                numberOfLsnEntries = 0;
            }
        }

        if ((numberOfLsnEntries + numberOfLsnEntries) > 0)
        {
            // There's another segment - skip to its beginning
            KInvariant(stream.Skip(segmentSize));
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN
LogStreamDescriptor::Truncate(__in RvdLogAsn UpToAsn)
{
    TruncationPoint.SetIfLarger(UpToAsn);
    RvdLogLsn minStreamLsnTruncationPoint = AsnIndex.Truncate(TruncationPoint, Info.NextLsn);
    return LsnTruncationPointForStream.SetIfLarger(minStreamLsnTruncationPoint);
}

VOID
LogStreamDescriptor::GetAsnRange(__out RvdLogAsn& LowestAsn, __out RvdLogAsn& HighestAsn)
{
    AsnIndex.GetAsnRange(LowestAsn, HighestAsn, MaxAsnWritten);
}




//** LogState class implementation
NTSTATUS
LogState::Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out LogState::SPtr& Result)
{
    Result = _new(AllocationTag, Allocator) LogState();
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return Result->Status();
}

LogState::LogState()
    :   _LogFileSize(0),
        _LogFileLsnSpace(0),
        _MaxCheckPointLogRecordSize(0),
        _LogFileFreeSpace(0),
        _NumberOfStreams(0),
        _StreamDescs(GetThisAllocator(), 0)
{
}

LogState::~LogState()
{
}


// Common global utilities
VOID
RvdLogUtility::GuidToAcsii(__in const GUID& Guid, __out_bcount(38) UCHAR* ResultArray)
{
    struct HexPair
    {
        static VOID
        ToAscii(UCHAR Value, UCHAR* OutputPtr)
        {
            static UCHAR        printableChars[] = "0123456789ABCDEF";

            OutputPtr[0] = printableChars[((Value & 0xF0) >> 4)];
            OutputPtr[1] = printableChars[(Value & 0x0F)];
        }
    };

    ResultArray[0] = '{';

    HexPair::ToAscii((UCHAR)(Guid.Data1 >> 24), &ResultArray[1+0]);
    HexPair::ToAscii((UCHAR)(Guid.Data1 >> 16), &ResultArray[1+2]);
    HexPair::ToAscii((UCHAR)(Guid.Data1 >> 8), &ResultArray[1+4]);
    HexPair::ToAscii((UCHAR)(Guid.Data1 >> 0), &ResultArray[1+6]);
    ResultArray[1+8] = '-';

    HexPair::ToAscii((UCHAR)(Guid.Data2 >> 8), &ResultArray[10+0]);
    HexPair::ToAscii((UCHAR)(Guid.Data2 >> 0), &ResultArray[10+2]);
    ResultArray[10+4] = '-';

    HexPair::ToAscii((UCHAR)(Guid.Data3 >> 8), &ResultArray[15+0]);
    HexPair::ToAscii((UCHAR)(Guid.Data3 >> 0), &ResultArray[15+2]);
    ResultArray[15+4] = '-';

    HexPair::ToAscii(Guid.Data4[0], &ResultArray[20+0]);
    HexPair::ToAscii(Guid.Data4[1], &ResultArray[20+2]);
    ResultArray[20+4] = '-';

    HexPair::ToAscii(Guid.Data4[2], &ResultArray[25+0]);
    HexPair::ToAscii(Guid.Data4[3], &ResultArray[25+2]);
    HexPair::ToAscii(Guid.Data4[4], &ResultArray[25+4]);
    HexPair::ToAscii(Guid.Data4[5], &ResultArray[25+6]);
    HexPair::ToAscii(Guid.Data4[6], &ResultArray[25+8]);
    HexPair::ToAscii(Guid.Data4[7], &ResultArray[25+10]);

    ResultArray[25+12] = '}';
}

RvdLogUtility::AsciiGuid::AsciiGuid()
{
    Init();
}

RvdLogUtility::AsciiGuid::AsciiGuid(__in const GUID& Guid)
{
    Init();
    Set(Guid);
}

LPCSTR
RvdLogUtility::AsciiGuid::Get()
{
    return (LPCSTR)&_Value[0];
}

VOID
RvdLogUtility::AsciiGuid::Set(__in const GUID& Guid)
{
    GuidToAcsii(Guid, &_Value[0]);
}

VOID
RvdLogUtility::AsciiGuid::Init()
{
    KMemCpySafe(&_Value[0], sizeof(_Value), "{12345678-1234-1234-1234-123456789012}", sizeof(_Value));
}


RvdLogUtility::AsciiTwinGuid::AsciiTwinGuid()
{
    Init();
}

RvdLogUtility::AsciiTwinGuid::AsciiTwinGuid(__in const GUID& Guid0, __in const GUID& Guid1)
{
    Init();
    Set(Guid0, Guid1);
}

LPCSTR
RvdLogUtility::AsciiTwinGuid::Get()
{
    return (LPCSTR)&_Value[0];
}

VOID
RvdLogUtility::AsciiTwinGuid::Set(__in const GUID& Guid0, __in const GUID& Guid1)
{
    GuidToAcsii(Guid0, &_Value[0]);
    GuidToAcsii(Guid1, &_Value[RvdLogUtility::AsciiGuidLength + 1]);
}

VOID
RvdLogUtility::AsciiTwinGuid::Init()
{
    KMemCpySafe(
        &_Value[0], 
        sizeof(_Value), 
        "{12345678-1234-1234-1234-123456789012}:{12345678-1234-1234-1234-123456789012}", 
        sizeof(_Value));
}


#pragma prefast(pop)

VOID AsyncParseLogFilename::StartParseFilename(
    __in KWString& Filename,
    __out KGuid& DiskId,
    __out KWString& RootPath,
    __out KWString& RelativePath,
    __out RvdLogId& LogId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _Filename = Filename;
    _DiskId = &DiskId;
    _RootPath = &RootPath;
    _RelativePath = &RelativePath;
    _LogId = &LogId;

    Start(ParentAsyncContext, CallbackPtr);
    // continued at OnStart
}

VOID AsyncParseLogFilename::OnStart()
{
    // continued from StartParseFilename

    NTSTATUS status;

    status = _Filename.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    KWString        rootPath(GetThisAllocator());
    KWString        rootRelativePath(GetThisAllocator());
    
    status = KVolumeNamespace::SplitAndNormalizeObjectPath(_Filename, rootPath, rootRelativePath);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Disallow use of reserved directory
    //
    if (RvdDiskLogConstants::IsReservedDirectory(KStringView((UNICODE_STRING)rootRelativePath)))
    {
        status = STATUS_ACCESS_DENIED;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
#if !defined(PLATFORM_UNIX)
    //
    // return back the component parts of the pathname
    //
    *_RootPath = rootPath;
    status = _RootPath->Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    *_RelativePath = rootRelativePath;
    status = _RelativePath->Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Convert the rootPath into the appropriate disk id
    //
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(
            rootPath,
            GetThisAllocator(),
            *_DiskId,
            KAsyncContextBase::CompletionCallback(this, &AsyncParseLogFilename::OnQueryStableVolumeId), 
            this);

    if (!K_ASYNC_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    // Continued @ OnQueryStableVolumeId
#else
    *_DiskId = RvdDiskLogConstants::HardCodedVolumeGuid();
    
    *_RelativePath = _Filename;
    status = _RelativePath->Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    OnStartContinue();
    // Continued @ OnStartContinue
#endif
}

// Continued from OnStartOpenByPath
VOID 
AsyncParseLogFilename::OnQueryStableVolumeId(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS        status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }    

    OnStartContinue();
    // Continued @ OnStartContinue
}


// Continued from OnStartOpenByPath
VOID 
AsyncParseLogFilename::OnStartContinue()
{
    NTSTATUS status;
    //
    // Open the log file to validate the header and also to extract the log id
    //
    status = KIoBuffer::CreateSimple(
                        RvdDiskLogConstants::MasterBlockSize, 
                        _LogFileHeaderBuffer, 
                        (void*&)_LogFileHeader, 
                        GetThisAllocator());

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    KWString        filePath(GetThisAllocator());
    
#if !defined(PLATFORM_UNIX)
    //
    // rebuild the filename using the disk id instead of drive letter
    //
    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(
                            *_DiskId,
                            filePath);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    filePath += *_RelativePath;
#else
    filePath = _Filename;
#endif

    status = filePath.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
        
    //
    // Open the logfile so that we can read the log id for it
    //
    ULONG createOptions = static_cast<ULONG>(KBlockFile::CreateOptions::eShareRead) +
                          static_cast<ULONG>(KBlockFile::CreateOptions::eShareWrite) +
                          static_cast<ULONG>(KBlockFile::CreateOptions::eShareDelete) +
                          static_cast<ULONG>(KBlockFile::CreateOptions::eReadOnly);
    
    status = KBlockFile::Create(
        filePath,
        FALSE,
        KBlockFile::CreateDisposition::eOpenExisting,
        static_cast<KBlockFile::CreateOptions>(createOptions),
        _LogFileHandle,
        KAsyncContextBase::CompletionCallback(this, &AsyncParseLogFilename::OnLogFileAliasOpenComplete),
        this,
        GetThisAllocator());

    if (!K_ASYNC_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    // Continued @ OnLogFileAliasOpenComplete
}

 
// Continued from StartContinue()
VOID 
AsyncParseLogFilename::OnLogFileAliasOpenComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS        status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    //
    // File opened - try and read the header
    //
    status = _LogFileHandle->Transfer(
        KBlockFile::IoPriority::eForeground,
        KBlockFile::TransferType::eRead,
        0,
        *_LogFileHeaderBuffer,
        KAsyncContextBase::CompletionCallback(this, &AsyncParseLogFilename::OnLogFileHeaderReadComplete),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        _LogFileHandle->Close();
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    // Continued @ OnLogFileHeaderReadComplete
}


// Continued from OnLogFileAliasOpenComplete()
VOID 
AsyncParseLogFilename::OnLogFileHeaderReadComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    _LogFileHandle->Close();

    NTSTATUS        status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Validate the header and extract the LogId and continued the normal open
    //
    KAssert(_LogFileHeaderBuffer->First()->GetBuffer() == _LogFileHeader);
    ULONGLONG savedChksum = _LogFileHeader->ThisBlockChecksum;
    _LogFileHeader->ThisBlockChecksum = 0;
    if (KChecksum::Crc64(_LogFileHeader, RvdDiskLogConstants::MasterBlockSize, 0) != savedChksum)
    {
        //
        // CONSIDER: Should we try to read the trailer to see if its checksum is ok ?
        //
        KDbgErrorWData(0, "Incorrect header checksum for log container file", K_STATUS_LOG_STRUCTURE_FAULT, 0, 0, 0, 0);
        Complete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }
    
    //
    // We have what appears to be a good log file - get the LogId and complete
    //
    *_LogId = _LogFileHeader->LogId;
    Complete(STATUS_SUCCESS);
}

VOID
AsyncParseLogFilename::OnCompleted()
{
    _LogFileHandle = nullptr;
    _LogFileHeaderBuffer = nullptr;
}

NTSTATUS
AsyncParseLogFilename::Create(
    __in ULONG AllocationTag, 
    __in KAllocator& Allocator, 
    __out AsyncParseLogFilename::SPtr& Result
    )
{
    NTSTATUS status;
    AsyncParseLogFilename::SPtr result;

    result = _new(AllocationTag, Allocator) AsyncParseLogFilename();
    if (result == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, 0, 0, 0);
        return(status);
    }

    status = result->Status();
    if (! NT_SUCCESS(status))
    {
        KDbgErrorWStatus(0, "AsyncParseLogFilename failed constructor", status);
        return(status);
    }

    Result = Ktl::Move(result);
    return(STATUS_SUCCESS);
}

AsyncParseLogFilename::AsyncParseLogFilename() :
    _Filename(GetThisAllocator())
{
}

AsyncParseLogFilename::~AsyncParseLogFilename()
{
}
