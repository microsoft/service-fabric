/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.AsyncOpenLogStream.cpp

    Description:
      RvdLogManagerImp::RvdOnDiskLog::AsyncOpenLogStreamContext implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"
#include "KtlLogRecovery.h"
#include <ktrace.h>


//*** AsyncOpenLogStreamContext
class AsyncOpenLogStreamContextImp : public RvdLog::AsyncOpenLogStreamContext
{
    K_FORCE_SHARED(AsyncOpenLogStreamContextImp);

public:
    
    VOID
    StartOpenLogStream(
        __in const RvdLogStreamId& LogStreamId, 
        __out RvdLogStream::SPtr& LogStream,
        __in_opt KAsyncContextBase* const ParentAsyncContext, 
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    AsyncOpenLogStreamContextImp(RvdLogManagerImp::RvdOnDiskLog* const Log);

private:
    VOID
    OnStart();

    VOID
    DoComplete(NTSTATUS Status);

    VOID
    GateAcquireComplete(BOOLEAN IsAcquired, KAsyncLock& Lock);

    VOID
    RecoveryComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletingSubOp);

private:
    class Recovery;

private:
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _Log;

    // Per operation state:
    RvdLogStreamId                          _LogStreamId;
    RvdLogStream::SPtr*                     _ResultLogStream;
    RvdLogStreamImp::SPtr                   _LogStream;
    LogStreamDescriptor*                    _StreamInfoPtr;
    KSharedPtr<Recovery>                    _RecoveryState;
};


//*** Concreate RvdLogStreamRecovery imp supporting stream open
class AsyncOpenLogStreamContextImp::Recovery : public RvdLogStreamRecovery
{
    K_FORCE_SHARED(Recovery);

public:
    Recovery(RvdLogManagerImp::RvdOnDiskLog::SPtr Parent)
        :	_Parent(Parent)
    {
    }

private:
    //*** Derivation interface overrides
    NTSTATUS
    ValidateUserRecord(
        RvdLogAsn UserRecordAsn,
        ULONGLONG UserRecordAsnVersion,
        void* UserMetadata,
        ULONG UserMetadataSize,
        RvdLogStreamType& LogStreamType,
        KIoBuffer::SPtr& UserIoBuffer)
    {
       UNREFERENCED_PARAMETER(UserRecordAsnVersion);
       UNREFERENCED_PARAMETER(UserRecordAsn);

        if (!_LastVerificationCallback)
        {
            // Resolve a VerificationCallback for the passed RvdLogStreamType - caching the results
            _LastVerificationCallback = _Parent->_LogManager->QueryVerificationCallback(LogStreamType);
            _LastLogStreamType = LogStreamType;
        }

        if (_LastVerificationCallback && (_LastLogStreamType == LogStreamType))
        {
            return _LastVerificationCallback((UCHAR*)UserMetadata, UserMetadataSize, UserIoBuffer);
        }

        KDbgErrorWStatus(GetActivityId(), "Registered RvdLogManager::VerificationCallback Missing", STATUS_NOT_FOUND);
        return STATUS_NOT_FOUND;
    }

    //** Recovery/Validate failure notification interface
    VOID
    MemoryAllocationFailed(ULONG LineNumber)
    {
        KTraceOutOfMemory(GetActivityId(), STATUS_INSUFFICIENT_RESOURCES, this, LineNumber, 0);
    }

    VOID
    AllocateTransferContextFailed(NTSTATUS Status, KBlockDevice::SPtr& BlockDevice, ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), Status, LineNumber, 0, 0, 0);
    }

    VOID
    PhysicalStartReadFailed(
        NTSTATUS Status, 
        KBlockDevice::SPtr& BlockDevice, 
        ULONGLONG FileOffset, 
        ULONG ReadSize, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), Status, LineNumber, FileOffset, ReadSize, 0);
    }

    VOID
    PhysicalReadFailed(
        NTSTATUS Status, 
        KBlockDevice::SPtr& BlockDevice, 
        ULONGLONG FileOffset, 
        ULONG ReadSize, 
        ULONG LineNumber)
    {
        UNREFERENCED_PARAMETER(BlockDevice);

        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), Status, LineNumber, FileOffset, ReadSize, 0);
    }

    VOID
    LogRecordReaderCreationFailed(NTSTATUS Status, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), Status, LineNumber, 0, 0, 0);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, RvdLogLsn ReadAt, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), Status, LineNumber, ReadAt.Get(), 0, 0);
    }

    VOID
    RecordReaderFailed(NTSTATUS Status, ULONGLONG ReadAtOffset, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(GetActivityId(), logAndStreamId.Get(), Status, LineNumber, ReadAtOffset, 0, 0);
    }

    VOID
    UserRecordHeaderFault(RvdLogUserStreamRecordHeader& UserStreamRecordHeader, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), UserStreamRecordHeader.LogStreamId.Get());

        KDbgErrorWData(
            GetActivityId(), 
            logAndStreamId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            LineNumber, 
            UserStreamRecordHeader.Lsn.Get(), 
            0, 
            0);
    }

    VOID
    InvalidMultiSegmentStreamCheckpointRecord(RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(
            GetActivityId(), 
            logAndStreamId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            LineNumber, 
            RecordHeader.Lsn.Get(), 
            RecordHeader.RecordType, 
            RecordHeader.TruncationPoint.Get());
    }

    VOID
    InvalidStreamIdInValidRecord(RvdLogUserStreamRecordHeader& RecordHeader)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), RecordHeader.LogStreamId.Get());

        KDbgErrorWStatus(GetActivityId(), logAndStreamId.Get(), K_STATUS_LOG_STRUCTURE_FAULT);
    }

    VOID
    AddLowerLsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogUserStreamRecordHeader& RecordHeader, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), RecordHeader.LogStreamId.Get());

        KDbgErrorWData(
            GetActivityId(), 
            logAndStreamId.Get(), 
            Status, 
            LineNumber, 
            RecordHeader.Lsn.Get(), 
            0, 
            0);
    }

    VOID
    AddAsnRecordIndexEntryFailed(NTSTATUS Status, RvdLogAsn RecordAsn, ULONGLONG RecordVersion, ULONG LineNumber)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(
            GetActivityId(), 
            logAndStreamId.Get(), 
            Status, 
            LineNumber, 
            RecordAsn.Get(), 
            RecordVersion, 
            0);
    }

    VOID
    LsnIndexQueryRecordFailed(NTSTATUS Status)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWStatus(GetActivityId(), logAndStreamId.Get(), Status);
    }

    VOID
    LsnIndexRecordMissing(RvdLogLsn Lsn)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), GetStreamId().Get());

        KDbgErrorWData(
            GetActivityId(), 
            logAndStreamId.Get(), 
            K_STATUS_LOG_STRUCTURE_FAULT, 
            0, 
            Lsn.Get(), 
            0, 
            0);
    }

    VOID
    ReportLogStreamRecovered(
        RvdLogStreamId& StreamId, 
        RvdLogStreamType& StreamType, 
        ULONG LsnCount, 
        RvdLogLsn LowestLsn, 
        RvdLogLsn HighestLsn, 
        RvdLogLsn NextLsn, 
        RvdLogLsn HighestStreamCheckPoint, 
        ULONG AsnCount, 
        RvdLogAsn LowestAsn, 
        RvdLogAsn HighestAsn, 
        RvdLogAsn TruncationAsn, 
        RvdLogLsn TruncationLsn)
    {
        RvdLogUtility::AsciiTwinGuid	logAndStreamId(_LogId.Get(), StreamId.Get());
        RvdLogUtility::AsciiGuid        streamTypeId(StreamType.Get());

        KDbgPrintf(
            "AsyncOpenLogStream::ReportLogStreamRecovered: %s: Type: %s\n"
                "\tLSNs: Count: %u; Low: %I64i; High: %I64i; Next: %I64i; HighStrCP: %I64i; Trunc: %I64i;\n"
                "\tASNs: Count: %u; Low: %I64u; High: %I64u; Trunc: %I64u",
            logAndStreamId.Get(),
            streamTypeId.Get(),
            LsnCount,
            LowestLsn.Get(),
            HighestLsn.Get(),
            NextLsn.Get(),
            HighestStreamCheckPoint.Get(),
            TruncationLsn.Get(),
            AsnCount,
            LowestAsn.Get(),
            HighestAsn.Get(),
            TruncationAsn.Get());
    }

private:
    RvdLogManagerImp::RvdOnDiskLog::SPtr	_Parent;
    RvdLogManager::VerificationCallback		_LastVerificationCallback;
    RvdLogStreamType						_LastLogStreamType;
};

AsyncOpenLogStreamContextImp::Recovery::~Recovery()
{
}


//** AsyncOpenLogStreamContext Implementation
NTSTATUS 
RvdLogManagerImp::RvdOnDiskLog::CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Result)
{
    Result = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncOpenLogStreamContextImp(this);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;    
}

RvdLog::AsyncOpenLogStreamContext::AsyncOpenLogStreamContext()
{
}

RvdLog::AsyncOpenLogStreamContext::~AsyncOpenLogStreamContext()
{
}

AsyncOpenLogStreamContextImp::AsyncOpenLogStreamContextImp(RvdLogManagerImp::RvdOnDiskLog* const Log)
    : _Log(Log)
{
}

AsyncOpenLogStreamContextImp::~AsyncOpenLogStreamContextImp()
{
}

VOID
AsyncOpenLogStreamContextImp::StartOpenLogStream(
    __in const RvdLogStreamId& LogStreamId, 
    __out RvdLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;
    _ResultLogStream = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncOpenLogStreamContextImp::DoComplete(NTSTATUS Status)
{
    ReleaseLock(_StreamInfoPtr->OpenCloseDeleteLock);
    _StreamInfoPtr = nullptr;
    _LogStream.Reset();

    Complete(Status);
}

VOID
AsyncOpenLogStreamContextImp::OnStart()
//
// Continued from StartOpenLogStream
{
    if (_LogStreamId == RvdDiskLogConstants::CheckpointStreamId()) 
    {
        // Disallow use of reserved streams
        Complete(STATUS_NOT_FOUND);
        return;
    }

    NTSTATUS status = _Log->FindOrCreateStream(FALSE, _LogStreamId, RvdLogStreamType(), _LogStream, _StreamInfoPtr);
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    // NOTE: From this point on: becasue we hold a ref in _LogStream, _StreamInfoPtr will stay valid as long as
    //       we hold that ref

    // At this point access to _LogStream access is guaranteed - take a 
    // lock and check for State == Opened/Deleted on the other side of the acquire
    AcquireLock(
        _StreamInfoPtr->OpenCloseDeleteLock, 
        KAsyncContextBase::LockAcquiredCallback(this, &AsyncOpenLogStreamContextImp::GateAcquireComplete));

    // Continued @GateAcquireComplete
}

VOID
AsyncOpenLogStreamContextImp::GateAcquireComplete(BOOLEAN IsAcquired, KAsyncLock& Lock)
{
    UNREFERENCED_PARAMETER(Lock);

    if (!IsAcquired)
    {
        AcquireLock(
            _StreamInfoPtr->OpenCloseDeleteLock, 
            KAsyncContextBase::LockAcquiredCallback(this, &AsyncOpenLogStreamContextImp::GateAcquireComplete));

        return;
    }

    // CreateOpenDeleteGate has been acquired - we may have lost the race to do the open however
    if (_StreamInfoPtr->State == LogStreamDescriptor::OpenState::Opened)
    {
        // Lost the race to open - we are done (successfully) as we are just a secondary opener
        (*_ResultLogStream) = Ktl::Move(reinterpret_cast<RvdLogStream::SPtr&>(_LogStream));
        DoComplete(STATUS_SUCCESS);
        return;
    }

    if (_StreamInfoPtr->State == LogStreamDescriptor::OpenState::Deleting)
    {
        // Lost the race - deleted from under us
        DoComplete(STATUS_DELETE_PENDING);
        return;
    }

    // We can continue with the opening of the stream
    KAssert(_StreamInfoPtr->State == LogStreamDescriptor::OpenState::Closed);

    _RecoveryState = _new(KTL_TAG_LOGGER, GetThisAllocator()) Recovery(_Log.RawPtr());
    if (_RecoveryState == nullptr)
    {
        DoComplete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    NTSTATUS status = _RecoveryState->Status();
    if (!NT_SUCCESS(status))
    {
        _RecoveryState.Reset();
        DoComplete(status);
        return;
    }

    _RecoveryState->StartRecovery(
        _Log->_BlockDevice,
        _Log->_Config,
        _Log->_LogId,
        _Log->_LogFileLsnSpace,
        _Log->_LowestLsn,
        _Log->_NextLsnToWrite,
        _Log->_LogSignature,
        _StreamInfoPtr->Info,
        this,
        CompletionCallback(this, &AsyncOpenLogStreamContextImp::RecoveryComplete));

    // Continued @ RecoveryComplete()
}

VOID
AsyncOpenLogStreamContextImp::RecoveryComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
//
// Continues from GateAcquireComplete()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // RecoveryComplete() failed
        DoComplete(status);
        return;
    }

    RvdLogStreamInformation	    recoveredInfo;
    RvdAsnIndex*                asnIndex;
    RvdLogLsnEntryTracker*      lsnIndex;
    RvdLogAsn                   truncationPoint;
    RvdLogLsn                   highestCheckPointLsn;

    _RecoveryState->GetRecoveredStreamInformation(recoveredInfo, asnIndex, lsnIndex, truncationPoint, highestCheckPointLsn);
    KFinally([asnIndex, lsnIndex]()
    {
        _delete(asnIndex);
        _delete(lsnIndex);
    });

    KAssert(_StreamInfoPtr->Info.LogStreamId == recoveredInfo.LogStreamId);
    KAssert(_StreamInfoPtr->Info.LogStreamType == recoveredInfo.LogStreamType);

    _StreamInfoPtr->Info.LowestLsn = recoveredInfo.LowestLsn;
    _StreamInfoPtr->Info.HighestLsn = recoveredInfo.HighestLsn;
    _StreamInfoPtr->Info.NextLsn = recoveredInfo.NextLsn;

    _StreamInfoPtr->TruncationPoint = truncationPoint;
    _StreamInfoPtr->LastCheckPointLsn = highestCheckPointLsn;

    KAssert(_StreamInfoPtr->LsnTruncationPointForStream.IsNull());
    if (recoveredInfo.LowestLsn >= RvdLogLsn::Min())
    {
        _StreamInfoPtr->LsnTruncationPointForStream = recoveredInfo.LowestLsn.Get() - 1;
    }

    status = _StreamInfoPtr->LsnIndex.Load(*lsnIndex, nullptr, &_StreamInfoPtr->HighestLsn);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }
    KAssert(((_StreamInfoPtr->LsnIndex.QueryNumberOfRecords() == 0) && _StreamInfoPtr->HighestLsn.IsNull()) || 
            (_StreamInfoPtr->HighestLsn == recoveredInfo.HighestLsn));

    status = _StreamInfoPtr->AsnIndex.Load(*asnIndex);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    KAssert(_StreamInfoPtr->MaxAsnWritten.IsNull());
    if (_StreamInfoPtr->AsnIndex.GetNumberOfEntries() > 0)
    {
        _StreamInfoPtr->MaxAsnWritten = _StreamInfoPtr->AsnIndex.UnsafeLast()->GetAsn();
    }

    _StreamInfoPtr->State = LogStreamDescriptor::OpenState::Opened;
    (*_ResultLogStream) = Ktl::Move(reinterpret_cast<RvdLogStream::SPtr&>(_LogStream));
    DoComplete(STATUS_SUCCESS);
}
