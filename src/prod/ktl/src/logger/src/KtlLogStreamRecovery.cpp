/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    RvdLogStreamRecovery.cpp

    Description:
      RvdLog Stream recovery implementation.

    History:
      PengLi/richhas        4-Sep-2011         Initial version.

--*/

#include "InternalKtlLogger.h"
#include "KtlLogRecovery.h"

#include "ktrace.h"


//*** RvdLogStreamRecovery implementation
RvdLogStreamRecovery::RvdLogStreamRecovery()
    :   _AsnIndex(nullptr),
        _LsnIndex(nullptr),
        _Config(nullptr)
{
}

RvdLogStreamRecovery::~RvdLogStreamRecovery()
{
    _delete(_AsnIndex);
    _delete(_LsnIndex);
}

VOID
RvdLogStreamRecovery::StartRecovery(
    __in KBlockDevice::SPtr BlockDevice,
    __in RvdLogConfig& Config,
    __in RvdLogId& LogId,
    __in ULONGLONG LogFileLsnSpace,
    __in RvdLogLsn LogLowestLsn,
    __in RvdLogLsn LogNextLsnToWrite,
    __in_ecount(NUMBER_SIGNATURE_ULONGS) ULONG LogSignature[RvdDiskLogConstants::SignatureULongs],
    __in RvdLogStreamInformation& ForStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _BlockDevice = Ktl::Move(BlockDevice);
    _Config = &Config;
    _LogId = LogId;
    _StreamInfo = ForStream;
    _LogFileLsnSpace = LogFileLsnSpace;
    _LogLowestLsn = LogLowestLsn;
    _LogNextLsnToWrite = LogNextLsnToWrite;
    KMemCpySafe(&_LogSignature[0], sizeof(_LogSignature), &LogSignature[0], RvdLogRecordHeader::LogSignatureSize);
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogStreamRecovery::GetRecoveredStreamInformation(
        __out RvdLogStreamInformation&    StreamInfo,
        __out RvdAsnIndex*&    AsnIndex,
        __out RvdLogLsnEntryTracker*& LsnIndex,
        __out RvdLogAsn& TruncationPoint,
        __out RvdLogLsn& HighestCheckPointLsn)
{
    StreamInfo = _StreamInfo;
    AsnIndex = _AsnIndex;
    LsnIndex = _LsnIndex;
    TruncationPoint = _TruncationPoint;
    HighestCheckPointLsn = _StreamHighestCheckPointLsn;
    _AsnIndex = nullptr;
    _LsnIndex = nullptr;
}

VOID
RvdLogStreamRecovery::OnStart()
{
    NTSTATUS status = Init();
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    status = RecordReader::Create(
        _BlockDevice,
        *_Config,
        _LogFileLsnSpace,
        _LogId,
        _LogSignature,
        _RecordReader,
        KTL_TAG_LOGGER,
        GetThisAllocator());

    if (!NT_SUCCESS(status))
    {
        LogRecordReaderCreationFailed(status, __LINE__);
        DoComplete(status);
        return;
    }

    KAssert((_AsnIndex == nullptr) && (_LsnIndex == nullptr));
    _AsnIndex = _new(KTL_TAG_LOGGER, GetThisAllocator()) RvdAsnIndex(GetThisAllocator());
    _LsnIndex = _new(KTL_TAG_LOGGER, GetThisAllocator()) RvdLogLsnEntryTracker(GetThisAllocator());
    if ((_AsnIndex == nullptr) || (_LsnIndex == nullptr))
    {
        // NOTE: dtor will delete _AsnIndex and _LsnIndex if not null
        MemoryAllocationFailed(__LINE__);
        DoComplete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _NextStreamLsn = _StreamInfo.HighestLsn;
    _TruncationPoint = RvdLogAsn::Null();
    _StreamHighestCheckPointLsn = RvdLogLsn::Min();

    if (_StreamInfo.LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
    {
        DoComplete(STATUS_SUCCESS);
        return;
    }

    KInvariant(_StreamInfo.LogStreamId != RvdDiskLogConstants::FreeStreamInfoStreamId());
    KInvariant(_StreamInfo.LogStreamId != RvdDiskLogConstants::InvalidLogStreamId());
    KInvariant(_StreamInfo.LogStreamType != RvdDiskLogConstants::CheckpointStreamType());
    KInvariant(_StreamInfo.LogStreamType != RvdDiskLogConstants::DeletingStreamType());

    if (!_StreamInfo.IsEmptyStream() && (_NextStreamLsn >= _StreamInfo.LowestLsn))
    {
        KAssert(!_NextStreamLsn.IsNull());
        KInvariant(_StreamInfo.LowestLsn >= _LogLowestLsn);
        KInvariant(_StreamInfo.NextLsn <= _LogNextLsnToWrite);
        KInvariant(_StreamInfo.HighestLsn < _LogNextLsnToWrite);

        _NumberOfCheckpointSegments = MAXULONG;
        ContinueRecoveryOfStream();
        return;
    }

    // We have an empty stream or are done with the current stream's records beyond that
    // last checkpoint
    StreamRecoveryDone();
}

VOID
RvdLogStreamRecovery::ContinueRecoveryOfStream()
{
    // BUG, richhas, xxxxx, consider if the entire record (with IoBuffer) should be read here. At
    //                      some point we will be reading this thru a read cache during stream open
    //                      so leaving for now.
    _RecordReader->StartRead(
        _NextStreamLsn,
        _IoBuffer0,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &RvdLogStreamRecovery::ReadStreamRecordComplete));
    // Continued @ ReadStreamRecordComplete()
}

VOID
RvdLogStreamRecovery::ReadStreamRecordComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
// Continued from ContinueRecoveryOfStream()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletedContext.Status();
    CompletedContext.Reuse();

    if (!NT_SUCCESS(status))
    {
        RecordReaderFailed(status, _NextStreamLsn, __LINE__);
        DoComplete(status);
        return;
    }

    KAssert(_IoBuffer0->QueryNumberOfIoBufferElements() == 1);
    RvdLogUserStreamRecordHeader* recordHeader = (RvdLogUserStreamRecordHeader*)_IoBuffer0->First()->GetBuffer();

    if (recordHeader->LogStreamId != _StreamInfo.LogStreamId)
    {
        InvalidStreamIdInValidRecord(*recordHeader);
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    _TruncationPoint.SetIfLarger(recordHeader->TruncationPoint);
    KAssert(_NextStreamLsn == recordHeader->Lsn);   // TODO: Why assert and not a failure ?

    switch (recordHeader->RecordType)
    {
        case RvdLogUserStreamRecordHeader::UserRecord:
        {
            if (_NumberOfCheckpointSegments != MAXULONG)
            {
                // Should never see a user record once we are scanning a multi-segment CP record
                InvalidMultiSegmentStreamCheckpointRecord(*recordHeader, __LINE__);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }

            status = _LsnIndex->AddLowerLsnRecord(
                recordHeader->Lsn,
                recordHeader->ThisHeaderSize,
                recordHeader->IoBufferSize);

            if (!NT_SUCCESS(status))
            {
                AddLowerLsnRecordIndexEntryFailed(status, *recordHeader, __LINE__);
                DoComplete(status);
                return;
            }

            RvdLogUserRecordHeader* userRecordHeader = (RvdLogUserRecordHeader*)recordHeader;

            status = AddOrUpdateRvdAsnIndexEntry(
                userRecordHeader->Asn,
                userRecordHeader->AsnVersion,
                userRecordHeader->ThisHeaderSize + userRecordHeader->IoBufferSize,
                userRecordHeader->Lsn);

            if (!NT_SUCCESS(status))
            {
                AddAsnRecordIndexEntryFailed(status, userRecordHeader->Asn, userRecordHeader->AsnVersion, __LINE__);
                DoComplete(status);
                return;
            }

            // Read backwards in the stream if not at the start of the stream
            _NextStreamLsn = userRecordHeader->PrevLsnInLogStream;
            if (!_NextStreamLsn.IsNull() && (_NextStreamLsn >= _StreamInfo.LowestLsn))
            {
                ContinueRecoveryOfStream();
                return;
            }
            break;
        }

        case RvdLogUserStreamRecordHeader::CheckPointRecord:
        {
            //* Found the most recent user stream check point. This terminates the recovery for this stream.
            RvdLogUserStreamCheckPointRecordHeader* cpRecordHeader = (RvdLogUserStreamCheckPointRecordHeader*)recordHeader;

            if (_NumberOfCheckpointSegments == MAXULONG)
            {
                // First (highest - s/b) stream CP segment (maybe only 1 if not multi-seg)
                _NumberOfCheckpointSegments = cpRecordHeader->NumberOfSegments;
                KAssert(_NumberOfCheckpointSegments > 0);
                _NextExpectedCheckpointSegment = _NumberOfCheckpointSegments - 1;

                // Only record lowest multi-seg CP record's LSN - truncating that should have the effect of truncating in the seq
                KAssert(cpRecordHeader->Lsn.Get() >= ((_NumberOfCheckpointSegments - 1) * 
                                                      RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(*_Config)));
                KAssert(recordHeader->IoBufferSize == 0);

                //
                // Record the last segment LSN
                //
                RvdLogLsn       seg0Lsn = cpRecordHeader->Lsn;
                ULONG           seg0Size = cpRecordHeader->ThisHeaderSize;

                status = _LsnIndex->AddLowerLsnRecord(seg0Lsn, seg0Size, 0);
                if (!NT_SUCCESS(status))
                {
                    AddLowerLsnRecordIndexEntryFailed(status, *recordHeader, __LINE__);
                    DoComplete(status);
                    return;
                }
            }

            if (cpRecordHeader->SegmentNumber != _NextExpectedCheckpointSegment)
            {
                InvalidMultiSegmentStreamCheckpointRecord(*recordHeader, __LINE__);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }

            if ((_NumberOfCheckpointSegments > 1) && 
                (cpRecordHeader->SegmentNumber < (_NumberOfCheckpointSegments - 1)) &&
                (cpRecordHeader->ThisHeaderSize != RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(*_Config)))
            {
                // All segments in a multi segment CP record below the last segment must be fixed in size
                InvalidMultiSegmentStreamCheckpointRecord(*recordHeader, __LINE__);
                DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
                return;
            }

            // Capture all of the Asn and Lsn record information for this stream that are within physical stream bounds
            _StreamHighestCheckPointLsn             = recordHeader->Lsn;
            AsnLsnMappingEntry* const   asnMappings = cpRecordHeader->GetAsnLsnMappingArray();
            RvdLogLsnEntry* const       lsnMappings = cpRecordHeader->GetLsnEntryArray();

            for (AsnLsnMappingEntry* asnMapping = asnMappings;
                                     asnMapping < (asnMappings + cpRecordHeader->NumberOfAsnMappingEntries);
                                     asnMapping++)
            {
                KInvariant(asnMapping->RecordLsn < _LogNextLsnToWrite);

                if (asnMapping->RecordLsn >= _StreamInfo.LowestLsn)
                {
                    // Only records at or above the lowest stream bounds
                    status = AddOrUpdateRvdAsnIndexEntry(
                        asnMapping->RecordAsn,
                        asnMapping->RecordAsnVersion,
                        asnMapping->IoBufferSizeHint,
                        asnMapping->RecordLsn);

                    if (!NT_SUCCESS(status))
                    {
                        AddAsnRecordIndexEntryFailed(status, asnMapping->RecordAsn, asnMapping->RecordAsnVersion, __LINE__);
                        DoComplete(status);
                        return;
                    }
                }
            }

            if (cpRecordHeader->NumberOfLsnEntries > 0)
            {
                for (RvdLogLsnEntry* lsnMapping = lsnMappings + (cpRecordHeader->NumberOfLsnEntries - 1);
                                     lsnMapping >= lsnMappings;
                                     lsnMapping--)
                {
                    KInvariant(lsnMapping->Lsn < _LogNextLsnToWrite);

                    if (lsnMapping->Lsn < _StreamInfo.LowestLsn)
                    {
                        // The rest of the RvdLogLsnEntrys must be < streams lowest limit in LSN space - ignore them all
                        break;
                    }

                    status = _LsnIndex->AddLowerLsnRecord(
                        lsnMapping->Lsn,
                        lsnMapping->HeaderAndMetaDataSize,
                        lsnMapping->IoBufferSize);

                    if (!NT_SUCCESS(status))
                    {
                        AddLowerLsnRecordIndexEntryFailed(status, *cpRecordHeader, __LINE__);
                        DoComplete(status);
                        return;
                    }
                }
            }

            // Read backwards in the stream if not at the start of the stream or we are done with
            // a multi-segment CP
            if (_NextExpectedCheckpointSegment > 0)
            {
                _NextExpectedCheckpointSegment--;
                _NextStreamLsn = cpRecordHeader->PrevLsnInLogStream;
                if (!_NextStreamLsn.IsNull() && (_NextStreamLsn >= _StreamInfo.LowestLsn))
                {
                    ContinueRecoveryOfStream();
                    return;
                }

                // Start of stream encountered before entire multi-segment CP read complete. This
                // could occur due to partial truncation of a multi-segment CP record seq. This is ok
                // as no user record can exist before such a trim point.
            }
            break;
        }

        default:
        {
            UserRecordHeaderFault(*recordHeader, __LINE__);
            DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
            return;
        }
    }

    // Fix _LowestLsnOfHigherASN values in _AsnIndex
    RvdLogAsn               prevAsn;
    RvdLogLsn               lowestLsn;
    RvdAsnIndexEntry::SPtr  currentAsnEntry = _AsnIndex->UnsafeLast();
    RvdAsnIndexEntry*       currentAsnEntryPtr;
    RvdAsnIndexEntry*       lastEntryPtr = currentAsnEntry.RawPtr();

    while ((currentAsnEntryPtr = currentAsnEntry.RawPtr()) != nullptr)
    {
        if (currentAsnEntryPtr == lastEntryPtr)
        {
            // special case for last ASN
            prevAsn = currentAsnEntryPtr->GetAsn();
            lowestLsn = currentAsnEntryPtr->GetLsn();
            currentAsnEntryPtr->SetLowestLsnOfHigherASNs(lowestLsn);
        }
        else
        {
            KAssert(currentAsnEntryPtr->GetAsn() < prevAsn);
            if (currentAsnEntryPtr->GetLsn() < lowestLsn)
            {
                lowestLsn = currentAsnEntryPtr->GetLsn();
            }

            currentAsnEntryPtr->SetLowestLsnOfHigherASNs(lowestLsn);
        }

        currentAsnEntry = _AsnIndex->UnsafePrev(currentAsnEntry);
    }

    StreamRecoveryDone();
}

VOID
RvdLogStreamRecovery::StreamRecoveryDone()
{
    NTSTATUS status;
    
    // Validate the stream's LSN index against its ASN index. There should be an LSN entry for every ASN that's
    // marked as persisted.
    KAssert(_StreamInfo.LogStreamId != RvdDiskLogConstants::CheckpointStreamId());

    // Truncate any record markers that below the truncation point of this current stream
    RvdLogLsn           truncationLsn = _AsnIndex->Truncate(_TruncationPoint, _StreamInfo.HighestLsn);
    _LsnIndex->Truncate(truncationLsn);

    ULONG                           numberOfLsns = _LsnIndex->QueryNumberOfRecords();
    RvdAsnIndexEntry::SPtr          currentAsnEntry = _AsnIndex->UnsafeFirst();
    KHashTable<ULONGLONG, void*>::HashFunctionType  hashFunc(&K_DefaultHashFunction);
    KHashTable<ULONGLONG, void*>    lsns(((numberOfLsns * 2) + 13), hashFunc, GetThisAllocator());
    
    status = lsns.Status();
    if (! NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // load the lsns hashtable
    for (ULONG ix = 0; ix < numberOfLsns; ix++)
    {
        RvdLogLsn lsn;
        status = _LsnIndex->QueryRecord(ix, lsn, nullptr, nullptr);
        if (!NT_SUCCESS(status))
        {
            LsnIndexQueryRecordFailed(status);
            DoComplete(status);
            return;
        }

        void*       prevValue = nullptr;
        ULONGLONG   lsnValue = (ULONGLONG)(lsn.Get());

        status = lsns.Put(lsnValue, prevValue, TRUE, &prevValue);
        if (!NT_SUCCESS(status))
        {
            // Bug, richhas, xxxxx, Add: lsn hash table failure report
            DoComplete(status);
            return;
        }

        if (prevValue != nullptr)
        {
            // Bug, richhas, xxxxx, Add: lsn hash table failure report
            DoComplete(STATUS_OBJECT_NAME_COLLISION);
            return;
        }
    }

    KAssert(numberOfLsns == lsns.Count());

    while (currentAsnEntry != nullptr)
    {
        KAssert(currentAsnEntry->GetDisposition() == RvdLogStream::RecordDisposition::eDispositionPersisted);
        KAssert(currentAsnEntry->GetAsn() > _TruncationPoint);

        void*       value;
        ULONGLONG   lsnKey = (ULONGLONG)(currentAsnEntry->GetLsn().Get());
        status = lsns.Get(lsnKey, value);
        if (status != STATUS_SUCCESS)
        {
            // ASN's LSN entry not found
            LsnIndexRecordMissing(currentAsnEntry->GetLsn());
            DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
            return;
        }

        currentAsnEntry = _AsnIndex->UnsafeNext(currentAsnEntry);
    }

    RvdLogAsn       lowAsn;
    RvdLogAsn       highAsn;
    ULONG           asnCount = _AsnIndex->GetNumberOfEntries();
    if (asnCount > 0)
    {
        lowAsn = _AsnIndex->UnsafeFirst()->GetAsn();
        highAsn = _AsnIndex->UnsafeLast()->GetAsn();
    }

    ReportLogStreamRecovered(
        _StreamInfo.LogStreamId, 
        _StreamInfo.LogStreamType, 
        numberOfLsns, 
        _StreamInfo.LowestLsn, 
        _StreamInfo.HighestLsn, 
        _StreamInfo.NextLsn, 
        _StreamHighestCheckPointLsn, 
        asnCount, 
        lowAsn, 
        highAsn, 
        _TruncationPoint, 
        truncationLsn);

    DoComplete(STATUS_SUCCESS);
}

//* Support methods
NTSTATUS
RvdLogStreamRecovery::AddOrUpdateRvdAsnIndexEntry(
    RvdLogAsn Asn,
    ULONGLONG AsnVersion,
    ULONG IoBufferSize,
    RvdLogLsn Lsn)
{
    RvdAsnIndexEntry::SPtr asnEntry = _new(KTL_TAG_LOGGER, GetThisAllocator()) RvdAsnIndexEntry(
        Asn,
        AsnVersion,
        IoBufferSize,
        RvdLogStream::RecordDisposition::eDispositionPersisted, // NOTE: Assumes only presisted entries Checkpointed
        Lsn);

    if (asnEntry == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RvdAsnIndexEntry::SPtr          resultIndexEntry;
    RvdAsnIndexEntry::SavedState    previousIndexEntryValue;
    BOOLEAN                         previousIndexEntryWasStored;
    ULONGLONG                       duplicateRecordAsnVersion;
    
    NTSTATUS status = _AsnIndex->AddOrUpdate(
        asnEntry,
        resultIndexEntry,
        previousIndexEntryValue,
        previousIndexEntryWasStored,
        duplicateRecordAsnVersion);

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_OBJECT_NAME_COLLISION)             
        {
            if (AsnVersion != duplicateRecordAsnVersion)
            {
                // It's ok and very expected that there is an existing (newer version)
                // already in the index but not an identical version 
                status = STATUS_SUCCESS;
            }
            else 
            {
                status = K_STATUS_LOG_STRUCTURE_FAULT;
            }
        }
    }

    return status;
}

