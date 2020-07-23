/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogStream.cpp

    Description:
      RvdLogStream implementation.

    History:
      PengLi/Richhas        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

#include "ktrace.h"



//** RvdLogStream base class ctor and dtor.
//
RvdLogStream::RvdLogStream()
{
}

RvdLogStream::~RvdLogStream()
{
}

//** RvdLogStreamImp
RvdLogStreamImp::RvdLogStreamImp(
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& StreamDesc,
    __in ULONG UniqueInstanceId)
    : _Log(&Log),
      _LogStreamDescriptor(StreamDesc),
      _ThisInstanceId(UniqueInstanceId),
      _ReservationHeld(0),
      _TruncationCompletionEvent(nullptr)
{
      _TruncationCompletion = KAsyncContextBase::CompletionCallback(this, &RvdLogStreamImp::OnTruncationComplete);
      NTSTATUS      status = AsyncWriteStream::CreateUpdateReservationAsyncWriteContext(
          GetThisAllocator(), 
          Log, 
          StreamDesc, 
          _AutoCloseReserveOp);

      if (!NT_SUCCESS(status))
      {
          SetConstructorStatus(status);
          return;
      }
}

RvdLogStreamImp::~RvdLogStreamImp()
{
    if (_ReservationHeld > 0)
    {
        // Start Auto relieve of any outstanding reserve in the background
        _AutoCloseReserveOp->StartUpdateReservation( -((LONGLONG)_ReservationHeld), nullptr, nullptr);
    }

    if (_DestructionCallback)
    {
        // There is a pending delete (from either a delete or create op - continue it now that all interest in this 
        // stream instance has been dropped
        _DestructionCallback();
    }
    else
    {
        // Report into _Log that interest in this version of the stream is closing
        _Log->StreamClosed(_LogStreamDescriptor, _ThisInstanceId);
    }
    
    KInvariant(_TruncationCompletionEvent == nullptr);
}

LONGLONG
RvdLogStreamImp::QueryLogStreamUsage(
   )
{
    return(_LogStreamDescriptor.LsnIndex.QueryTotalSize());
}


VOID 
RvdLogStreamImp::QueryLogStreamType(__out RvdLogStreamType& LogStreamType)
{
    LogStreamType = _LogStreamDescriptor.Info.LogStreamType;
}
    
VOID 
RvdLogStreamImp::QueryLogStreamId(__out RvdLogStreamId& LogStreamId)
{
    LogStreamId = _LogStreamDescriptor.Info.LogStreamId;
}
    
NTSTATUS 
RvdLogStreamImp::QueryRecordRange(
    __out_opt RvdLogAsn* const LowestAsn, 
    __out_opt RvdLogAsn* const HighestAsn,
    __out_opt RvdLogAsn* const LogTruncationAsn)
{
    RvdLogAsn low;
    RvdLogAsn high;

    _LogStreamDescriptor.GetAsnRange(low, high);

    if (LowestAsn != nullptr)
    {
        *LowestAsn = low;
    }
    if (HighestAsn != nullptr)
    {
        *HighestAsn = high;
    }
    if (LogTruncationAsn != nullptr)
    {
        *LogTruncationAsn = _LogStreamDescriptor.TruncationPoint;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogStreamImp::QueryRecord(
    __inout RvdLogAsn& RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt ULONGLONG* const Version,
    __out_opt RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1)
{
    ULONGLONG v; 
    RecordDisposition rd;
    RvdLogLsn lsn;
    ULONG ioSize;

    switch(Type)
    {
        case RvdLogStream::AsyncReadContext::ReadExactRecord:
        {
            _LogStreamDescriptor.AsnIndex.GetAsnInformation(RecordAsn, v, rd, lsn, ioSize);
            break;
        }

        case RvdLogStream::AsyncReadContext::ReadNextRecord:
        {
            _LogStreamDescriptor.AsnIndex.GetNextAsnInformationFromNearest(RecordAsn, v, rd, lsn, ioSize);
            break;
        }

        case RvdLogStream::AsyncReadContext::ReadPreviousRecord:
        {
            _LogStreamDescriptor.AsnIndex.GetPreviousAsnInformationFromNearest(RecordAsn, v, rd, lsn, ioSize);
            break;
        }

        case RvdLogStream::AsyncReadContext::ReadContainingRecord:
        {
            _LogStreamDescriptor.AsnIndex.GetContainingAsnInformation(RecordAsn, v, rd, lsn, ioSize);
            break;
        }

        default:
        {
            return(STATUS_INVALID_PARAMETER);
        }
    }

    if (Version != nullptr)
    {
        *Version = v;
    }
    if (Disposition != nullptr)
    {
        *Disposition = rd;
    }
    if (IoBufferSize != nullptr)
    {
        *IoBufferSize = ioSize;
    }
    if (DebugInfo1 != nullptr)
    {
        *DebugInfo1 = lsn.Get();
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogStreamImp::QueryRecord(
    __in RvdLogAsn RecordAsn, 
    __out_opt ULONGLONG* const Version,
    __out_opt RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1)
{
    return(QueryRecord(RecordAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         Version,
                         Disposition,
                         IoBufferSize,
                         DebugInfo1));
}

NTSTATUS
RvdLogStreamImp::QueryRecords(
    __in RvdLogAsn LowestAsn,
    __in RvdLogAsn HighestAsn,
    __in KArray<RecordMetadata>& ResultsArray)
{
    ResultsArray.Clear();
    return _LogStreamDescriptor.AsnIndex.GetEntries(LowestAsn, HighestAsn, ResultsArray);
}

NTSTATUS
RvdLogStreamImp::DeleteRecord(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version)
{
    NTSTATUS status;
    RvdLogLsn minStreamLsnTruncationPoint = RvdLogLsn::Null();

    status = _LogStreamDescriptor.AsnIndex.TryRemoveForDelete(RecordAsn, Version, minStreamLsnTruncationPoint);
    
    if (NT_SUCCESS(status))
    {
        //
        // If the ASN was successfully removed and the LSN truncation point advances then there is LSN
        // space that can be cleaned up. In that case initiate a truncation write operation to cause that
        // space to be freed. Note that we can only advance if the ASN removed has an LSN that is lower 
        // than all other records otherwise there are records below the one deleted that can't be removed.
        //
        RvdLogLsn lowestLsnOfHigherASNs = _LogStreamDescriptor.AsnIndex.GetLowestLsnOfHigherASNs(_LogStreamDescriptor.Info.NextLsn);
        if (minStreamLsnTruncationPoint < lowestLsnOfHigherASNs.Get())
        {
            if (_LogStreamDescriptor.LsnTruncationPointForStream.SetIfLarger(minStreamLsnTruncationPoint))
            {
                InitiateTruncationWrite();
            }
        }
    }
    return(status);
}

NTSTATUS
RvdLogStreamImp::SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if ((_TruncationCompletionEvent != nullptr) && (EventToSignal != nullptr))
        {
            return STATUS_OBJECT_NAME_COLLISION;
        }

        _TruncationCompletionEvent = EventToSignal;
    }

    return STATUS_SUCCESS;  
}


VOID
RvdLogStreamImp::InitiateTruncationWrite(
)
{
    if (InterlockedIncrement(&_LogStreamDescriptor.StreamTruncateWriteOpsRequested) == 1)
    {
        // This thread has responsibility to start a real truncation process
        AddRef();               // #1 used to protect the truncation completion callback
        
#ifdef EXTRA_TRACING
        KDbgCheckpointWDataInformational(GetRvdLogStreamActivityId(), "TRUNC** start truncation op", STATUS_SUCCESS,
                            (ULONGLONG)this,
                            (ULONGLONG)_LogStreamDescriptor.LsnTruncationPointForStream.Get(),
                            (ULONGLONG)0,
                            (ULONGLONG)0);        
#endif
        
        InterlockedIncrement(&_Log->_CountOfOutstandingWrites);
        _LogStreamDescriptor.StreamTruncateWriteOp->StartTruncation(
            FALSE,                         // Truncation may be skipped if not freeing any space
            _LogStreamDescriptor.LsnTruncationPointForStream,
            _Log,
            nullptr,
            _TruncationCompletion);

        // Continues @ OnTruncationComplete
    } else {
#ifdef EXTRA_TRACING
        KDbgCheckpointWDataInformational(GetRvdLogStreamActivityId(), "TRUNC** truncation already in progress", STATUS_SUCCESS,
                            (ULONGLONG)this,
                            (ULONGLONG)_LogStreamDescriptor.LsnTruncationPointForStream.Get(),
                            (ULONGLONG)0,
                            (ULONGLONG)0);
#endif
    }
}

VOID
RvdLogStreamImp::TruncateBelowVersion(
    __in RvdLogAsn NewTruncationPoint,
    __in ULONGLONG Version)
{
    //
    // First check if the truncation is going to truncate any records at that LSN or
    // below that have a higher Version
    //
    BOOLEAN b;
    ULONGLONG higherVersion;

    b = _LogStreamDescriptor.AsnIndex.CheckForRecordsWithHigherVersion(NewTruncationPoint, Version, higherVersion);
    if (!b)
    {
        Truncate(NewTruncationPoint, NewTruncationPoint);
    } else {
#ifdef EXTRA_TRACING
        KDbgCheckpointWDataInformational(GetRvdLogStreamActivityId(), "TRUNC** skip truncation due to higher version", STATUS_SUCCESS,
                            (ULONGLONG)this,
                            (ULONGLONG)NewTruncationPoint.Get(),
                            (ULONGLONG)Version,
                            (ULONGLONG)higherVersion);
#endif
    }
}

VOID
RvdLogStreamImp::Truncate(
    __in RvdLogAsn NewTruncationPoint, 
    __in RvdLogAsn PreferredTruncationPoint)
{
    UNREFERENCED_PARAMETER(PreferredTruncationPoint);
    
    // BUG, richhas, xxxxx, add selector to choose a value from [PreferredTruncationPoint, NewTruncationPoint]
    //                      
    // The POR is that the physical log will maintain a "Truncation Criticality Meter" that will produce a 
    // value from -1..1 that can be used to produce a truncation point in the interval of 
    // [PreferredTruncationPoint, NewTruncationPoint]; with -1 producing PreferredTruncationPoint and a value of 1
    // producing NewTruncationPoint; and with 0 producing the value close to 
    // (NewTruncationPoint-PreferredTruncationPoint) / 2. 
    //
    // This Log "Meter" will be based fundementally on the difference between the rates of new LSN space allocation
    // and deallocation (LSN space truncation rate). In addition this Meter will be biased (amplified) by the by the
    // amount of free LSN space such that as the free space moves toward 0 the Meter is biased more 
    // and more (exponentially) toward producing a 1.
    // 
    KAssert(NewTruncationPoint >= PreferredTruncationPoint);

    //
    // Ensure this is not called after the RvdLogStreamImp has been destructed
    //
    KInvariant(this->_RefCount != 0);

    if (_LogStreamDescriptor.Truncate(NewTruncationPoint))
    {
        InitiateTruncationWrite();
    }
}

VOID
RvdLogStreamImp::OnTruncationComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase&       CompletingSubOp)
//
// Continued from Truncate
{
    UNREFERENCED_PARAMETER(Parent);

    K_LOCK_BLOCK(_ThisLock)
    {
        if (_TruncationCompletionEvent != nullptr)
        {
            _TruncationCompletionEvent->SetEvent();
        }
    }
    
    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
         KTraceFailedAsyncRequest(CompletingSubOp.Status(), &CompletingSubOp, 0, 0);
    }

    CompletingSubOp.Reuse();

    KInvariant(_LogStreamDescriptor.StreamTruncateWriteOpsRequested > 0);
    if (InterlockedDecrement(&_LogStreamDescriptor.StreamTruncateWriteOpsRequested) > 0)
    {
        // More work to to do - reduce StreamTruncateWriteOpsRequested to 1
        InterlockedExchangeAdd(
            &_LogStreamDescriptor.StreamTruncateWriteOpsRequested,
            -(_LogStreamDescriptor.StreamTruncateWriteOpsRequested - 1));

        // Start next truncation op on this stream
        _LogStreamDescriptor.StreamTruncateWriteOp->StartTruncation(
            FALSE,                         // Truncation may be skipped if not freeing any space
            _LogStreamDescriptor.LsnTruncationPointForStream,
            _Log, 
            nullptr,
            _TruncationCompletion);

        // Continued @ OnTruncationComplete
        return;     
    }

    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);
    Release();          // Reverse #1 in Truncate() above
}

//** Test support API
VOID
RvdLogStreamImp::GetPhysicalExtent(__out RvdLogLsn& Lowest, __out RvdLogLsn& Highest)
{
    Lowest = _LogStreamDescriptor.Info.LowestLsn;
    Highest = _LogStreamDescriptor.Info.NextLsn;
}

