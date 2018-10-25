/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KMemChannel

    Description:
      KTL generic I/O memory channel

      Abstractions and implementations for serialization channels.

    History:
      raymcc, mattklup       11-Feb-2011         Initial version
      raymcc                 30-Mar-2011         Updates for variable sized pages
      raymcc                 28-Apr-2011         SetCursor fix, error model, asserts

--*/


#pragma once

class KMemChannel : public KObject<KMemChannel>
{
    K_DENY_COPY(KMemChannel);

public:
    typedef KUniquePtr<KMemChannel> UPtr;

    // General note on failure conditions.
    //
    // When any of the APIs below (except for Reset()) encounter a failure condition, that status is latched and can be
    // detected via Status(). Once a current KMemChannel is in a latched error state (!NT_SUCCESS(Status()), all other APIs
    // that return an NTSTATUS will return Status(). The only way to clear such a latched state is via a call to one of the
    // Reset() methods.
    //
    // By default a short read is not an error. This default behavior can be turned off and on via SetReadFullRequired().

    // This enum is used with the SetCursor() method.
    //
    enum
    {
        eFromBegin     =  1,
        eBeforeEnd     =  2,      // End == first byte after the last byte written, not the last byte
        eAfterCurrent  =  3,
        eBeforeCurrent =  4
    };

    //
    // KMemChannel
    //
    // Parameters:
    //      FirstBlockSize      The size of the initial block. This is not allocated until the first write.
    //      MinGrowBy           The minimum size to grow the channel if the write won't fit within the current
    //                          channel size.
    //      MaxGrowBy           The maximum size to grow the channel when a new write occurs that doesn't fit.
    //                          Writes larger than this will still succeed, but they will allocate multiple times
    //                          internally until the write succeeds.
    //
    KMemChannel(
		__in KAllocator& Allocator,
        __in ULONG   FirstBlockSize   = 0x1000-KAllocatorSupport::MaxAllocationBlockOverhead,
        __in ULONG   MinGrowBy        = 0x1000-KAllocatorSupport::MaxAllocationBlockOverhead,
        __in ULONG   MaxGrowBy        = 0x1000-KAllocatorSupport::MaxAllocationBlockOverhead
        );

   ~KMemChannel();

   // Allow control of end of stream indication behavior. 
   void SetReadFullRequired(BOOLEAN OnOrOff)
   {
       _TreatShortReadsAsError = OnOrOff;
   }

    // DetachOnDestruct
    //
    // Signals that the memory should not be deallocated during destruction.  This occurs when a user
    // supplies externally-owned blocks to StartCapture()/Capture() but wants to retain ownership of
    // the blocks after using KMemChannel.
    //
    void DetachOnDestruct()
    {
        _DeallocateOnDestruct = FALSE;
    }

    //  Reset
    //
    //  Releases all internal buffers except the first page, resets all
    //  cursors to zero.  Brings the object back to its 'constructed' state.
    //
    VOID
    Reset();

    //  Reset (overload)
    //
    //  This overload has the same semantics as the constructor. It deallocates
    //  any existing buffers, zeros internal values and
    //
    VOID
    Reset(
        __in ULONG   FirstBlockSize,
        __in ULONG   MinGrowBy,
        __in ULONG   MaxGrowBy
        );


    // Size
    //
    // Returns the current size of the channel, i.e.,
    // the total max bytes written so far, irrespective of internal allocation sizes.
    //
    ULONG Size() const
    {
        return _TotalSize;
    }


    // SetCursor
    //
    // Seeks to a relative position within the channel.
    //
    // Parameters:
    //      MoveValue       The adjustment to the current cursor position.
    //      MoveMethod      Relative to enumeration type listed above
    //
    // Return value:
    //      STATUS_SUCCESS              The move happened
    //      STATUS_INVALID_PARAMETER    MoveMethod was incorrect or
    //                                  The value would have moved the cursor
    //                                  past the end or the beginning.
    //
    NTSTATUS SetCursor(
        __in ULONG MoveValue,
        __in ULONG MoveMethod
        );

    // ResetCursor
    //
    // Since most seeks are back to offset 0, this is a simple pseudonym for that.
    //
    NTSTATUS
    ResetCursor()
    {
        return SetCursor(0, eFromBegin);
    }

    // Remaining
    //
    // The number of previously written bytes remaining aheadof the current
    // cursor position before end-of-file is encountered.
    //
    ULONG
    Remaining() const
    {
        return _TotalSize - _LogicalCursor;
    }

    // Eof
    //
    // Simple test to see if the cursor is at the end of the file/channel.
    // (The next writable position)
    //
    BOOLEAN
    Eof() const
    {
        return _TotalSize == _LogicalCursor;
    }

    // GetCursor
    //
    // Retrieves the current cursor position (zero-origin).
    //
    ULONG
    GetCursor() const
    {
        return _LogicalCursor;
    }


    // Remote this at some point
    VOID Log(__in NTSTATUS) const ;

    // Write
    //
    // Writes the specified bytes into the internal buffer set.
    // This will span the current block and dynamically allocate
    // more memory, as required, to complete the write.
    //
    // This method is atomic and will not fail after partially writing.
    //
    // Parameters:
    //      Size            The size of the block to write in bytes.
    //      Buffer          The source of the write.
    //
    // Return values:
    //      STATUS_SUCCESS  The write was completed successfully.
    //      STATUS_INSUFFICIENT_RESOURCES  Memory could not be allocated
    //                                     to complete the write.
    //
    inline NTSTATUS Write(
        __in ULONG ToCopy,
        __in const VOID* Buffer
        )
    {
        if (!NT_SUCCESS(Status()))
        {
            return Status();
        }

        if (_RemainingInBlock >= ToCopy)
        {
            KMemCpySafe(_CurrentPtr, _RemainingInBlock, Buffer, ToCopy);
            _CurrentPtr += ToCopy;
            _RemainingInBlock -= ToCopy;
            _LogicalCursor += ToCopy;
            if (_LogicalCursor >= _TotalSize)
            {
                _TotalSize = _LogicalCursor;
            }
        }
        else
        {
            return SplitWrite(ToCopy, Buffer);
        }
        return STATUS_SUCCESS;
    }

    // Read
    //
    // Copies the memory from the current position into the destination.
    // If the memory needs to be read without copying, then use Reset/Next
    // to access the underlying buffers.
    //
    // Parameters:
    //      Count           Number of bytes to be read.
    //      Destination     Where to put them
    //      ActuallyRead    Receives the number of bytes read, which may
    //                      be less than requested if there is an attempt
    //                      to read past the end of the buffer.
    //
    // Return values:
    //      STATUS_SUCCESS           In all cases where at least some bytes were read.
    //      STATUS_END_OF_MEDIA      In attemps to read past the end.
    //      STATUS_INVALID_PARAMETER One or more invalid parameters.
    //      K_STATUS_COULD_NOT_READ_STREAM - Iif SetReadFullRequired(TRUE) was called.
    //
    // Note: An attempt to read past end-of-file returns STATUS_SUCCESS if
    //       the number of bytes read is larger than zero, even if it is less than
    //       the total requested.  If a read was attempted starting at end of file position
    //       then STATUS_END_OF_MEDIA is returned.
    //
    NTSTATUS Read(
        __in      ULONG Count,
        __in      VOID *Destination,
        __out_opt ULONG* ActuallyRead = nullptr
        )
    {
        if (!NT_SUCCESS(Status()))
        {
            return Status();
        }

        if (_LogicalCursor == _TotalSize)
        {
            if (ActuallyRead)
            {
                *ActuallyRead = 0;
            }

            if (_TreatShortReadsAsError)
            {
                SetStatus(K_STATUS_COULD_NOT_READ_STREAM);
                return K_STATUS_COULD_NOT_READ_STREAM;
            }
            return STATUS_END_OF_MEDIA;
        }
        if (_RemainingInBlock >= Count)
        {
            ULONG Available = _TotalSize - _LogicalCursor;
            if (Available < Count)
            {
                if (_TreatShortReadsAsError)
                {
                    SetStatus(K_STATUS_COULD_NOT_READ_STREAM);
                    return K_STATUS_COULD_NOT_READ_STREAM;
                }
                Count = Available;
            }
            KMemCpySafe(Destination, Available, _CurrentPtr, Count);
            _CurrentPtr += Count;
            _RemainingInBlock -= Count;
            _LogicalCursor += Count;
            if (ActuallyRead)
            {
                *ActuallyRead = Count;
            }
        }
        else
        {
            return SplitRead(Count, Destination, ActuallyRead);
        }

        return STATUS_SUCCESS;
    }

    // GetBlockCount
    //
    // Returns the size of the internal _Buffers array, which is the same as
    // the number of blocks.
    //
    // This MUST be called before calling GetBlock, since it does some internal
    // normalization of the block _Param values for GetBlock to return correct
    // values.
    //
    // The channel should not be written to while doing a GetBlockCount/GetBlock
    // sequence.
    //
    ULONG
    GetBlockCount() const;

    // GetBlock
    //
    // Returns a reference to an internal block. This can be used
    // to acquire all existing memory for the channel if followed
    // by a DetachMemory call.
    //
    // You MUST call GetBlockCount() prior to calling this, since
    // GetBlockCount does normalization as well as returning
    // the number of blocks.
    //
    // Parameters:
    //      Block       Receives the block size. Since the last block
    //                  is always fully allocated but not may be fully
    //                  written. This is indicatead in the _Param field
    //                  of the block
    //
    // Note that the Block._Param is also set to the number of bytes used
    // in any given block.  _Param can only be less than the block size
    // on the last block.
    //
    //
    NTSTATUS
    GetBlock(
        __in  ULONG BlockIndex,
        __out KMemRef& Block
        );


    // StartCapture
    //
    // Call this to start a 'capture' sequence where KMemRef
    // blocks are directly acquired.
    //
    NTSTATUS StartCapture();

    // Capture
    //
    // Captures a memory block to the internal _Buffers array.
    // The block size must match the constructed block size.
    //
    // Parameters:
    //   Memory     The block to be captured.  The last block must
    //              have the KMemRef._Param field set to the actual
    //              number of bytes used, since partial blocks
    //              at the end of the steram are the norm.
    //   LastBlock  Set to TRUE if this is the last block.
    //
    NTSTATUS Capture(
        __in KMemRef const & Memory
        );

    // EndCapture
    //
    // Prepares the KMemCHannel for normal read/write use
    // after a capture sequence.
    //
    NTSTATUS EndCapture();

    // DetachMemory
    //
    // Detaches the object from existing memory
    // and reinitializes it.  To prevent leaks, the blocks
    // should have been acquire by some other owner.
    //
    VOID DetachMemory()
    {
		_Buffers.Clear();
        Initialize();
    }

	KAllocator&
	Allocator()
	{
		return _Allocator;
	}

private:
    // Methods

    // SplitWrite
    //
    // This is used when the amount to be written exceeds the current
    // block and spills into a new block.
    //
    NTSTATUS SplitWrite(
        __in ULONG Size,
        __in const VOID* Buffer
        );

    // SplitRead
    //
    // This is used when the read has to span more than one block
    NTSTATUS SplitRead(
        __in  ULONG Count,
        __in  VOID *Destination,
        __out ULONG* ActuallyRead
        );

    // Startup
    NTSTATUS Initialize();

    // Grow.
    //
    NTSTATUS Grow(
        __in ULONG Hint
        );

    // Cleanup.
    void Cleanup();

    // SelectBlock
    //
    // Activates the specified block as the current block
    // The cursor is set to the first position in this block.
    //
    NTSTATUS SelectBlock(
        __in ULONG Block
        );

private:
    // Data members
	KAllocator&		  _Allocator;
    KArray<KMemRef>   _Buffers;                 // The set of buffers making up the channel
    ULONG             _MinGrowBy;               // Grow by at least this amount
    ULONG             _MaxGrowBy;               // ...but no more than this amount
    ULONG             _FirstBlockSize;          // Size of first block
    LONG              _CurrentBlockIndex;       // Which element in _Buffers we are working with
    UCHAR*            _CurrentPtr;              // Pointer to next write position
    ULONG             _RemainingInBlock;        // Offset in current block
    ULONG             _TotalSize;               // Size of channel
    ULONG             _LogicalCursor;           // Logical cursor position
    ULONG             _AllocGranularity;        // Granularity of allocations
    BOOLEAN           _DeallocateOnDestruct;    // Whether to deallocate memory during destructor or not
    BOOLEAN           _TreatShortReadsAsError;  // If true short reads will result in a return/latched status of
                                                // K_STATUS_COULD_NOT_READ_STREAM
};
