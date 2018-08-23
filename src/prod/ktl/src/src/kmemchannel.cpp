
/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KMemChannel

    Description:
      KTL generic I/O memory channel.

      This implements a seekable/read/write stream over an arbitrary
      set of discontiguous memory blocks.

    History:
      raymcc        11-Feb-2011         Initial version
      raymcc        30-Mar-2011         Variable size block support, delayed allocation
--*/

#include <ktl.h>


// KMemChannel Constructor
//
//
KMemChannel::KMemChannel(
    __in KAllocator&    Allocator,
    __in ULONG          FirstBlockSize,
    __in ULONG          MinGrowBy,
    __in ULONG          MaxGrowBy
    )
    :   _Allocator(Allocator), _Buffers(Allocator)
{
    // TBD: Add asserts for
    // MinGrowBy to be less than MaxGrowBy
    // and Min to be at least 32

    _DeallocateOnDestruct = TRUE;
    _FirstBlockSize = FirstBlockSize;
    _MinGrowBy = MinGrowBy;
    _MaxGrowBy = MaxGrowBy;
    Initialize();
}


//  KMemChannel::Initialize
//
//  Initializes the object.  Assumes prior cleanup if called
//  from other locations besides the constructor.
//  This method MUST avoid touching the constructor parameter values.
//
NTSTATUS KMemChannel::Initialize()
{
    _CurrentBlockIndex = -1;
    _CurrentPtr = nullptr;
    _RemainingInBlock = 0;
    _TotalSize = 0;
    _LogicalCursor = 0;
    _TreatShortReadsAsError = FALSE;
    SetStatus(STATUS_SUCCESS);

    NTSTATUS Res = _Buffers.Status();
    if (Res != STATUS_SUCCESS)
    {
        SetStatus(Res);
        return Res;
    }
    return Status();
}

// KMemChannel Destructor
//
//
KMemChannel::~KMemChannel()
{
    if (_DeallocateOnDestruct)
    {
        Cleanup();
    }
}

// KMemChannel::Cleanup
//
//
void KMemChannel::Cleanup()
{
    for (unsigned i = 0; i < _Buffers.Count(); i++)
    {
        _delete(_Buffers[i]._Address);
    }
    _Buffers.Clear();
    Initialize();
}


//  KMemChannel::Grow
//
//  Allocates a new block and appends it to the block array. The
//  block must be at least _MinGrowBy, but can't be more than
//  _MaxGrowBy.
//
//  If the first block is being allocated, then the _FirstBlockSize
//  is used.
//
//  Parameters:
//      Hint            The number of bytes that are needed.
//
//  Return value:
//      STATUS_SUCCESS
//      STATUS_INSUFFICIENT_RESOURCES
//
NTSTATUS KMemChannel::Grow(
    __in ULONG Hint
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    ULONG BlockSize = 0;

    if (_Buffers.Count() == 0)
    {
        BlockSize = _FirstBlockSize;
    }
    else if (Hint < _MinGrowBy)
    {
        BlockSize = _MinGrowBy;
    }
    else if (Hint > _MaxGrowBy)
    {
        BlockSize = _MaxGrowBy;
    }
    else
    {
        // Round up using _MinGrowBy as the unit of granularity.
        //
		if ((Hint % _MinGrowBy) == 0)
		{
			BlockSize = Hint;
		} else {
			HRESULT hr;
			ULONG result = (Hint / _MinGrowBy) + 1;
			hr = ULongMult(result, _MinGrowBy, &BlockSize);
			KInvariant(SUCCEEDED(hr));
		}
	}

    if (BlockSize == 0)
    {
        // No extension allowed - indicated out of space on the current instance
        SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UCHAR *NewBuf = _new(KTL_TAG_NET_BLOCK, _Allocator) UCHAR[BlockSize];
    if (NewBuf == nullptr)
    {
        SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(NewBuf, BlockSize);

    // Now we have the memory.  Let's try to add a block to our buffer array.
    //
    KMemRef   NewBlock;
    NewBlock._Size = BlockSize;
    NewBlock._Address = NewBuf;
    NewBlock._Param = 0;              // How much has been used in this block

    NTSTATUS Result = _Buffers.Append(NewBlock);

    if (!NT_SUCCESS(Result))
    {
        // Ran out of memory just the wrong time. We had the block
        // but can't add it to the KArray.
        //
        _delete(NewBuf);
        SetStatus(Result);
        return Result;
    }

    return STATUS_SUCCESS;
}

//
// KMemChannel::SplitWrite
//
// This is a branch of Write() called when the write would exceed
// the bounds of the current block.  There are two cases, (1) where the write
// is withing the current set of allocated blocks (a rewrite), or
// (2) write past end, where new allocation must occur.
//
NTSTATUS KMemChannel::SplitWrite(
    __in ULONG ToCopy,
    __in const VOID* Buffer
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    NTSTATUS Result;

    PUCHAR Src = PUCHAR(Buffer);

    // If the write is large, we may have to iterate several
    // times using Grow() to complete the write.
    //
    for (;;)
    {
        if (_RemainingInBlock)
        {
            if (_RemainingInBlock < ToCopy)
            {
                // If here, we will fill the rest of this block
                // and will have to get more space afterwards.
                //
                KMemCpySafe(_CurrentPtr, _RemainingInBlock, Src, _RemainingInBlock);
                _CurrentPtr += _RemainingInBlock;
                ToCopy -= _RemainingInBlock;
                Src += _RemainingInBlock;
                _LogicalCursor += _RemainingInBlock;
                _RemainingInBlock = 0;
            }
            else
            {
                // If here, we were able to fit the write in the current
                // block.
                //
                KMemCpySafe(_CurrentPtr, _RemainingInBlock, Src, ToCopy);
                _CurrentPtr += ToCopy;
                _LogicalCursor += ToCopy;
                _RemainingInBlock -= ToCopy;
                break;
            }
        }

        // If here, we either have to move to the next block,
        // or grow the channel.
        //
        if ((ULONG)(++_CurrentBlockIndex) == _Buffers.Count())
        {
            // First, see if we can grow by the required amount.
            Result = Grow(ToCopy);
            if (!NT_SUCCESS(Result))
            {
                return Result;
            }
        }
        Result = SelectBlock(_CurrentBlockIndex);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
    }

    if (_LogicalCursor >= _TotalSize)
    {
       _TotalSize = _LogicalCursor;
    }
    return STATUS_SUCCESS;
}


// KMemChannel::SplitRead
//
// This is called when a read must occur across block boundaries.
//
NTSTATUS KMemChannel::SplitRead(
    __in      ULONG Count,
    __in      VOID *Destination,
    __out_opt ULONG* ActuallyRead
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    NTSTATUS Result;
    PUCHAR Target = PUCHAR(Destination);

    if (ActuallyRead)
    {
        *ActuallyRead = 0;
    }

    for (;;)
    {
        // Read as much as we can to satisfy the count.
        ULONG ToTransfer = Count;

        if (_RemainingInBlock)
        {
            if (ToTransfer > _RemainingInBlock)
            {
                // Transfer whatever is left in this block
                ToTransfer = _RemainingInBlock;
            }

            // Check for reading past EOF
            if (ToTransfer > Remaining())
            {
                ToTransfer = Remaining();
            }

            // Transfer the current chunk
            KMemCpySafe(Target, _RemainingInBlock, _CurrentPtr, ToTransfer);
            _CurrentPtr += ToTransfer;
            _LogicalCursor += ToTransfer;
            Target += ToTransfer;
            Count -= ToTransfer;
            _RemainingInBlock -= ToTransfer;

            if (ActuallyRead)
            {
                *ActuallyRead += ToTransfer;
            }

            if (Count == 0)
            {
                // If done looping
                break;
            }
        }

        // If here, we still have more to do. So,
        // move to the next bock.
        Result = SelectBlock(++_CurrentBlockIndex);
        if (!NT_SUCCESS(Result))
        {
            // If here, we tried to read past the end of the channel.
            // By default: this is okay, we tried to read as much as we could, 
            //             and *ActuallyRead is accurate.
            //
            break;
        }
    }

    if ((Count > 0) && _TreatShortReadsAsError)
    {
        SetStatus(K_STATUS_COULD_NOT_READ_STREAM);
        return K_STATUS_COULD_NOT_READ_STREAM;
    }

    return STATUS_SUCCESS;
}




//
//  KMemChannel::SelectBlock
//
//  Makes the specified block the active block.
//
//  It is possible in seeking the end of the channel
//  that STATUS_END_OF_MEDIA could be returned.
//  This needs to be checked by callers in order
//  to properly allocate a new block if writing
//  is occurring or to fail a read if a read
//  is attempted at this location.
//
NTSTATUS KMemChannel::SelectBlock(
    __in ULONG Block
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    ULONG Count = _Buffers.Count();
    if (Block > Count)
    {
        SetStatus(STATUS_INTERNAL_ERROR);
        return STATUS_INTERNAL_ERROR;
    }
    else if (Block == Count)
    {
        return STATUS_END_OF_MEDIA;
    }

    _CurrentBlockIndex = Block;
    _CurrentPtr = PUCHAR(_Buffers[Block]._Address);
    _RemainingInBlock = _Buffers[Block]._Size;

    return STATUS_SUCCESS;
}

//  KMemChannel::Reset
//
//
void KMemChannel::Reset()
{
    Cleanup();
    Initialize();
}

//  KMemChannel::Reset
//
//  This overload allows changing the sizes during the reset.
//
void KMemChannel::Reset(
    __in ULONG   FirstBlockSize,
    __in ULONG   MinGrowBy,
    __in ULONG   MaxGrowBy
    )
{
    _FirstBlockSize = FirstBlockSize;
    _MinGrowBy = MinGrowBy;
    _MaxGrowBy = MaxGrowBy;

    Cleanup();
    Initialize();
}


//  KMemChannel::StartCapture
//
//
NTSTATUS KMemChannel::StartCapture()
{
    // Any pre-existing stuff is deallocated
    //
    Cleanup();
    return STATUS_SUCCESS;
}

//  KMemChannel::StartCapture
//
//  Ensure that the last block strictly observes
//  that _Size is the allocated size and
//  _Param is how much is actually used.
//
NTSTATUS KMemChannel::Capture(
    __in KMemRef const & Memory
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    if (Memory._Size == 0)
    {
        SetStatus(STATUS_INVALID_PARAMETER_1);
        return STATUS_INVALID_PARAMETER_1;
    }

    NTSTATUS Result = _Buffers.Append(Memory);

    if (!NT_SUCCESS(Result))
    {
        SetStatus(Result);
        return Result;
    }

    return Result;
}

//  KMemChannel::EndCapture
//
//
NTSTATUS KMemChannel::EndCapture()
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    ULONG Count = _Buffers.Count();
    if (Count == 0)
    {
        // Somehow, we have no blocks.
        // The capture was not valid.
        //
        Initialize();
        SetStatus(STATUS_INTERNAL_ERROR);
        return STATUS_INTERNAL_ERROR;
    }

    // Now loop through all the blocks and
    // compute the total size.  All the incoming
    // blocks are assumed to be full except the
    // last, which has the _Param set to the
    // amount used in that block.

    _TotalSize = 0;

    for (ULONG i = 0; i < _Buffers.Count(); i++)
    {
        if (i == _Buffers.Count() - 1)
        {
            _TotalSize += _Buffers[Count-1]._Param;
        }
        else
        {
            _TotalSize += _Buffers[i]._Size;

            // Ensure that _Param is set to the _Size, indicating
            // that the block is fully 'used'.  Since we were capturing
            // these from the outside, we can't be sure the caller
            // has done this.
            //
            _Buffers[i]._Param = _Buffers[i]._Size;
        }
    }

    _LogicalCursor = 0;

    NTSTATUS Result = SelectBlock(0);
    return Result;
}


//  KMemChannel::Log
//
//  TBD: Internally log failures so that post-serialization
//  we can double check that nothing bad happened deep in this layer.
//
VOID
KMemChannel::Log(
    __in NTSTATUS Code
    ) const
{
    // TBD
    UNREFERENCED_PARAMETER(Code);
}


// KMemChannel::SetCursor
//
// This is called to move the logical cursor to a new position.
//
// TBD:This has to be rewritten for the new random block sizes.
//
NTSTATUS KMemChannel::SetCursor(
    __in ULONG MoveValue,
    __in ULONG MoveMethod
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    NTSTATUS Result;

    // Compute the new cursor position.
    //
    ULONG NewCursor = 0;

    if (MoveMethod == eFromBegin)
    {
        NewCursor = MoveValue;
    }
    else if (MoveMethod == eBeforeEnd)
    {
        NewCursor = _TotalSize - MoveValue;
    }
    else if (MoveMethod == eBeforeCurrent)
    {
        NewCursor = _LogicalCursor - MoveValue;
    }
    else if (MoveMethod == eAfterCurrent)
    {
        NewCursor = _LogicalCursor + MoveValue;
    }
    else
    {
        SetStatus(STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    if (NewCursor > _TotalSize)
    {
        SetStatus(STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    // Now we know the new logical cursor position.
    // But since blocks have variable size, we have to loop
    // through the buffers to figure out which block and which
    // offset into that block.

    _LogicalCursor = NewCursor;

    for (ULONG i = 0; i < _Buffers.Count(); i++)
    {
        if (NewCursor < _Buffers[i]._Size)
        {
             Result = SelectBlock(i);
             if (!NT_SUCCESS(Result))
             {
                return Result;
             }
             _CurrentPtr += NewCursor;
             _RemainingInBlock -= NewCursor;
             return STATUS_SUCCESS;
        }
        else
        {
            NewCursor -= _Buffers[i]._Size;
        }
    }

    // Special case of seeking to end-of-channel
    //
    if (_LogicalCursor == _TotalSize)
    {
        // If here, we are essentially positioned
        // to read/write past end, causing a new block alloc.
        //
        _RemainingInBlock = 0;
        _CurrentBlockIndex = _Buffers.Count() - 1;

        return STATUS_SUCCESS;
    }

    SetStatus(STATUS_INTERNAL_ERROR);
    return STATUS_INTERNAL_ERROR;
}


//  KMemChannel::GetBlockCount
//
//  Returns the number of internal blocks, and normalizes
//  so that the last block has a correct _Param value to indicate
//  it is only partially full.
//
ULONG
KMemChannel::GetBlockCount() const
{
    ULONG SizeTracer = _TotalSize;
    ULONG Count = _Buffers.Count();

    // Adjust the _Param values of the blocks.
    //
    for (ULONG i = 0; i < Count; i++)
    {
        if (i == Count - 1)
        {
            _Buffers[i]._Param = SizeTracer;
        }
        else
        {
            SizeTracer -= _Buffers[i]._Size;
            _Buffers[i]._Param = _Buffers[i]._Size;
        }
    }

    return _Buffers.Count();
}

//  KMemChannel::GetBlock
//
//
NTSTATUS
KMemChannel::GetBlock(
    __in  ULONG BlockIndex,
    __out KMemRef& Block
    )
{
    if (!NT_SUCCESS(Status()))
    {
        return Status();
    }

    ULONG Count = _Buffers.Count();

    if (BlockIndex >= Count)
    {
        return STATUS_NO_MORE_ENTRIES;
    }
    Block = _Buffers[BlockIndex];
    return STATUS_SUCCESS;
}

