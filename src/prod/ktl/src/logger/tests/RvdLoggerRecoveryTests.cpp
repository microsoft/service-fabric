/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerVerifyTests.cpp

Abstract:

--*/
#pragma once
#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>
#include "RvdLogStreamAsyncIOTests.h"
#include <KtlLogRecovery.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


// BUG, richhas, xxxxxx, Once fault injection tests are in place - factor out more common functionality across the existing tests.


#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif

// Local support functions and types
struct RecoverySupport
{
private:
    struct TestRecordMetadata
    {
        RvdLogAsn       Asn;
        ULONGLONG       Version;
        ULONG           ElementCount;
    };

    struct TestRecordIoBufferElement
    {
        RvdLogAsn       Asn;
        ULONGLONG       Version;
        ULONG           ElementIndex;
    };

public:
    static size_t const SizeOfTestRecordIoBufferElement = sizeof(TestRecordIoBufferElement);

    // Create a TestRecord
    static NTSTATUS
    CreateTestRecord(
        __in KAllocator& Allocator,
        __in RvdLogAsn Asn,
        __in ULONGLONG Version,
        __in ULONG IoBufferElementCount,
        __out KBuffer::SPtr& Metadata,
        __out KIoBuffer::SPtr& IoBuffer)
    {
        NTSTATUS        status = STATUS_SUCCESS;

        status = KBuffer::Create(sizeof(TestRecordMetadata), Metadata, Allocator, KTL_TAG_TEST);
        if (NT_SUCCESS(status))
        {
            TestRecordMetadata*     mdPtr = (TestRecordMetadata*)Metadata->GetBuffer();
            mdPtr->Asn = Asn;
            mdPtr->Version = Version;
            mdPtr->ElementCount = IoBufferElementCount;

            if (IoBufferElementCount == 0)
            {
                // status = KIoBuffer::CreateEmpty(IoBuffer, Allocator, KTL_TAG_TEST);
                IoBuffer.Reset();
            }
            else
            {
                TestRecordIoBufferElement*  elements;
                status = KIoBuffer::CreateSimple(
                    RvdDiskLogConstants::RoundUpToBlockBoundary(IoBufferElementCount * sizeof(TestRecordIoBufferElement)),
                    IoBuffer,
                    (void* &)elements,
                    Allocator,
                    KTL_TAG_TEST);

                if (NT_SUCCESS(status))
                {
                    for (ULONG ix = 0; ix < IoBufferElementCount; ix++)
                    {
                        elements->Asn = Asn;
                        elements->Version = Version;
                        elements->ElementIndex = ix;

                        elements++;
                    }
                }
            }
        }

        return status;
    }

    // Function to verify testStreamType-type records
    static NTSTATUS
    RecordVerifier(
        __in UCHAR const *const MetaDataBuffer,
        __in ULONG MetaDataBufferSize,
        __in const KIoBuffer::SPtr& IoBuffer)
    {
        if (MetaDataBufferSize == sizeof(TestRecordMetadata))
        {
            TestRecordMetadata* mdPtr = (TestRecordMetadata*)MetaDataBuffer;
            RvdLogAsn           asn = mdPtr->Asn;
            ULONGLONG           version = mdPtr->Version;
            ULONG               elementCount = mdPtr->ElementCount;

            if (elementCount > 0)
            {
                if (IoBuffer->QueryNumberOfIoBufferElements() == 1)
                {
                    TestRecordIoBufferElement*  elements = (TestRecordIoBufferElement*)IoBuffer->First()->GetBuffer();

                    if ((elementCount * sizeof(TestRecordIoBufferElement)) <= IoBuffer->QuerySize())
                    {
                        for (ULONG ix = 0; ix < elementCount; ix++)
                        {
                            if ((elements->Asn != asn) || (elements->Version != version) || (elements->ElementIndex != ix))
                            {
                                return K_STATUS_LOG_STRUCTURE_FAULT;
                            }
                            elements++;
                        }

                        return STATUS_SUCCESS;
                    }
                }
            }
            else
            {
                if (IoBuffer->QuerySize() == 0)
                {
                    return STATUS_SUCCESS;
                }

                return K_STATUS_LOG_STRUCTURE_FAULT;
            }
        }
        return K_STATUS_LOG_STRUCTURE_FAULT;
    }

    // Function to prove the Verification registry works
    static NTSTATUS
    RecordVerifierTestCallback(
        __in UCHAR const *const MetaDataBuffer,
        __in ULONG MetaDataBufferSize,
        __in const KIoBuffer::SPtr& IoBuffer)
    {
        KInvariant((MetaDataBuffer == nullptr) && (MetaDataBufferSize == 0) && (IoBuffer == nullptr));
        return STATUS_SUCCESS;
    }

    // Function to allocate a LogState and snap the state of an open log
    static NTSTATUS
    GetLogsState(__in RvdLog::SPtr Log, __in KAllocator& Allocator, __out LogState::SPtr& LogState)
    {
        LogState.Reset();

        RvdLogManagerImp::RvdOnDiskLog*     logImpPtr = (RvdLogManagerImp::RvdOnDiskLog*)Log.RawPtr();
        while (!logImpPtr->IsLogFlushed())
        {
            KNt::Sleep(10);
        }

        NTSTATUS status = logImpPtr->UnsafeSnapLogState(LogState, KTL_TAG_TEST, Allocator);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLoggerRecoveryTests: UnsafeSnapLogState() failed: 0x%08X: Line: %i\n", status, __LINE__);
			KInvariant(FALSE);
            return status;
        }
        logImpPtr = nullptr;

        return status;
    }

    // Function that tests (and reports) if a RecoveredState (LogState) represents a properly recovered physical log
    // relative to a RefState (LogState)
    static BOOLEAN
    IsPhysicalRecoveryCorrect(LogState::SPtr RefState, LogState::SPtr RecoveredState)
    {
        BOOLEAN     result = TRUE;

        if (RtlCompareMemory(
                &RefState->_LogSignature[0],
                &RecoveredState->_LogSignature[0],
                RvdLogRecordHeader::LogSignatureSize)
            != RvdLogRecordHeader::LogSignatureSize)
        {
            KDbgPrintf("RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: LogSignature mismatch: Line: %i\n", __LINE__);
            result = FALSE;
        }

        if ((RecoveredState->_LogFileSize != RefState->_LogFileSize) ||
            (RecoveredState->_LogFileLsnSpace != RefState->_LogFileLsnSpace) ||
            (RecoveredState->_MaxCheckPointLogRecordSize != RefState->_MaxCheckPointLogRecordSize))
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: LogFileSize, LogFileLsnSpace, "
                "or MaxCheckPointLogRecordSize mismatch: Line: %i\n",
                __LINE__);

            result = FALSE;
        }

        if (RecoveredState->_LogFileFreeSpace > RefState->_LogFileFreeSpace)
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Recovered LogFileFreeSpace > RefState: Line: %i\n",
                __LINE__);

            result = FALSE;
        }

        if (RecoveredState->_LowestLsn.Get() > RefState->_LowestLsn.Get())
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Recovered LowestLsn > RefState: Line: %i\n",
                __LINE__);

            result = FALSE;
        }

        if (RecoveredState->_HighestCheckpointLsn.Get() > RefState->_HighestCheckpointLsn.Get())
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Recovered HighestCheckpointLsn > RefState: Line: %i\n",
                __LINE__);

            result = FALSE;
        }

        if (RecoveredState->_NextLsnToWrite.Get() != RefState->_NextLsnToWrite.Get())
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Recovered NextLsnToWrite != RefState: Line: %i\n",
                __LINE__);

            result = FALSE;
        }

        if (RecoveredState->_HighestCompletedLsn.Get() != RefState->_HighestCompletedLsn.Get())
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Recovered HighestCompletedLsn != RefState: Line: %i\n",
                __LINE__);

            result = FALSE;
        }

        if (RecoveredState->_NumberOfStreams != RefState->_NumberOfStreams)
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Recovered NumberOfStreams != RefState: Line: %i\n",
                __LINE__);

            result = FALSE;
        }
        else
        {
            // Validate each stream's physical lsn properties
            for (ULONG ix = 0; ix < RecoveredState->_NumberOfStreams; ix++)
            {
                LogState::StreamDescriptor*     refPtr = nullptr;
                LogState::StreamDescriptor&     recDesc = RecoveredState->_StreamDescs[ix];

                for (ULONG refIx = 0; refIx < RefState->_NumberOfStreams; refIx++)
                {
                    if (RefState->_StreamDescs[refIx]._Info.LogStreamId == recDesc._Info.LogStreamId)
                    {
                        refPtr = &RefState->_StreamDescs[refIx];
                        break;
                    }
                }

                if (refPtr == nullptr)
                {
                    RvdLogUtility::AsciiGuid       streamIdStr(recDesc._Info.LogStreamId.Get());

                    KDbgPrintf(
                        "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect: Missing Stream in RefState: %s: Line: %i\n",
                        streamIdStr.Get(),
                        __LINE__);

                    result = FALSE;
                    continue;
                }

                LogState::StreamDescriptor&         refDesc = *refPtr;

                if (recDesc._Info.LowestLsn.Get() > refDesc._Info.LowestLsn.Get())
                {
                    RvdLogUtility::AsciiGuid       streamIdStr(recDesc._Info.LogStreamId.Get());

                    KDbgPrintf(
                        "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect:Recovered LowestLsn > RefState: %s: Line: %i\n",
                        streamIdStr.Get(),
                        __LINE__);

                    result = FALSE;
                }

                if (recDesc._Info.NextLsn.Get() > refDesc._Info.NextLsn.Get())
                {
                    RvdLogUtility::AsciiGuid       streamIdStr(recDesc._Info.LogStreamId.Get());

                    KDbgPrintf(
                        "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect:Recovered NextLsn > RefState: %s: Line: %i\n",
                        streamIdStr.Get(),
                        __LINE__);

                    result = FALSE;
                }

                if (recDesc._Info.HighestLsn.Get() > refDesc._Info.HighestLsn.Get())
                {
                    RvdLogUtility::AsciiGuid       streamIdStr(recDesc._Info.LogStreamId.Get());

                    KDbgPrintf(
                        "RvdLoggerRecoveryTests: IsPhysicalRecoveryCorrect:Recovered HighestLsn > RefState: %s: Line: %i\n",
                        streamIdStr.Get(),
                        __LINE__);

                    result = FALSE;
                }
            }
        }

        return result;
    }

    static NTSTATUS
    ValidateStreamCorrectness(
        RvdLog::SPtr Log,
        RvdLogStreamId& StreamId,
        LogState::SPtr RefLogState,
        KAllocator& Allocator)
    {
        NTSTATUS                                status = STATUS_SUCCESS;
        RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
        Synchronizer                            compEvent;

        status = Log->CreateAsyncOpenLogStreamContext(openStreamOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf(
                "RvdLoggerRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                status,
                __LINE__);

            return status;
        }

        RvdLogStream::SPtr          stream;

        openStreamOp->StartOpenLogStream(StreamId, stream, nullptr, compEvent.AsyncCompletionCallback());
        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLoggerRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        openStreamOp->Reuse();

        LogState::StreamDescriptor*     refDescPtr = nullptr;

        for (ULONG refIx = 0; refIx < RefLogState->_NumberOfStreams; refIx++)
        {
            if (StreamId == RefLogState->_StreamDescs[refIx]._Info.LogStreamId)
            {
                refDescPtr = &RefLogState->_StreamDescs[refIx];
                break;
            }
        }

        if (refDescPtr == nullptr)
        {
            RvdLogUtility::AsciiGuid    streamIdStr(StreamId.Get());

            KDbgPrintf("RvdLoggerRecoveryTests: Missing stream id: %s: Line: %i\n", streamIdStr.Get(), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // Build hashtbl for all LSNs in the ref log state
        struct LsnInfo
        {
            ULONG       HeaderAndMetadataSize;
            ULONG       IoBufferSize;
        };

        ULONG                                               numberOfRefLsns = refDescPtr->_LsnIndex->QueryNumberOfRecords();
        KHashTable<ULONGLONG, LsnInfo>::HashFunctionType    hashFunc(&K_DefaultHashFunction);
        KHashTable<ULONGLONG, LsnInfo>                      refLsns(((numberOfRefLsns * 2) + 13), hashFunc, Allocator);
        KInvariant(NT_SUCCESS(refLsns.Status()));

        // load the lsns hashtable
        for (ULONG lsnIx = 0; lsnIx < numberOfRefLsns; lsnIx++)
        {
            RvdLogLsn       lsn;
            LsnInfo         lsnInfo;

            status = refDescPtr->_LsnIndex->QueryRecord(
                lsnIx,
                lsn,
                &lsnInfo.HeaderAndMetadataSize,
                &lsnInfo.IoBufferSize);

            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("RvdLoggerRecoveryTests: QueryRecord failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            LsnInfo         prevValue;
            ULONGLONG       lsnValue = (ULONGLONG)(lsn.Get());

            status = refLsns.Put(lsnValue, lsnInfo, TRUE, &prevValue);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("RvdLoggerRecoveryTests: Put failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        KAssert(numberOfRefLsns == refLsns.Count());

        // Validate the recovered stream contents
        RvdLogAsn           lowestAsn;
        RvdLogAsn           highestAsn;
        RvdLogAsn           truncAsn;
        status = stream->QueryRecordRange(&lowestAsn, &highestAsn, &truncAsn);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLoggerRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        if (refDescPtr->_AsnIndex->GetNumberOfEntries() == 0)
        {
            if (!lowestAsn.IsNull() || !highestAsn.IsNull() || !truncAsn.IsNull())
            {
                KDbgPrintf("RvdLoggerRecoveryTests: lowestAsn, highestestAsn, or truncAsn invalid: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            if (lowestAsn.Get() > refDescPtr->_AsnIndex->UnsafeFirst()->GetAsn().Get())
            {
                KDbgPrintf("RvdLoggerRecoveryTests: lowestAsn invalid: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (highestAsn.Get() > refDescPtr->_AsnIndex->UnsafeLast()->GetAsn().Get())
            {
                KDbgPrintf("RvdLoggerRecoveryTests: highestAsn invalid: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (truncAsn.Get() > refDescPtr->_TruncationPoint.Get())
            {
                KDbgPrintf("RvdLoggerRecoveryTests: truncAsn invalid: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        KArray<RvdLogStream::RecordMetadata>  streamState(Allocator);

        status = stream->QueryRecords(RvdLogAsn::Min(), RvdLogAsn::Max(), streamState);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLoggerRecoveryTests: QueryRecords failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        if (streamState.Count() < refDescPtr->_AsnIndex->GetNumberOfEntries())
        {
            KDbgPrintf("RvdLoggerRecoveryTests: recovered stream asn count too low: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        ULONG       overRecoveredAsnCount = streamState.Count() - refDescPtr->_AsnIndex->GetNumberOfEntries();
        if (overRecoveredAsnCount > 0)
        {
            KDbgPrintf("RvdLoggerRecoveryTests: Warning: over recovered asn table: by: %u; Line: %i\n", overRecoveredAsnCount, __LINE__);
        }

        // Make sure each record (ASN) in the recovered stream state is also in the ref state (both ASN and LSN)
        for (ULONG recIx = 0; recIx < streamState.Count(); recIx++)
        {
            ULONGLONG                           ver;
            RvdLogStream::RecordDisposition     disp;
            RvdLogLsn                           lsn;
            ULONG                               ioBufferSize;

            refDescPtr->_AsnIndex->GetAsnInformation(streamState[recIx].Asn, ver, disp, lsn, ioBufferSize);
            if (disp == RvdLogStream::RecordDisposition::eDispositionNone)
            {
                if (overRecoveredAsnCount == 0)
                {
                    KDbgPrintf(
                        "RvdLoggerRecoveryTests: Missing recovered Asn from ref desc asn table: %I64u; Line: %i\n",
                        streamState[recIx].Asn.Get(),
                        __LINE__);

                    return STATUS_UNSUCCESSFUL;
                }

                KDbgPrintf(
                    "RvdLoggerRecoveryTests: Warning: over recovered Asn: %I64u; Line: %i\n",
                    streamState[recIx].Asn.Get(),
                    __LINE__);

                overRecoveredAsnCount--;
            }
            else
            {
                if (ver != streamState[recIx].Version)
                {
                    KDbgPrintf("RvdLoggerRecoveryTests: recovered stream record Version incorrect: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (disp != streamState[recIx].Disposition)
                {
                    KDbgPrintf("RvdLoggerRecoveryTests: recovered stream record Disposition incorrect: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (ioBufferSize != streamState[recIx].Size)
                {
                    KDbgPrintf("RvdLoggerRecoveryTests: recovered stream record Size incorrect: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (lsn.Get() != streamState[recIx].Debug_LsnValue)
                {
                    KDbgPrintf("RvdLoggerRecoveryTests: recovered stream record Lsn incorrect: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                // Validate the LSN info for the recovered record [recIx]
                LsnInfo         value;
                ULONGLONG       lsnKey = (ULONGLONG)streamState[recIx].Debug_LsnValue;

                status = refLsns.Get(lsnKey, value);
                if (status != STATUS_SUCCESS)
                {
                    KDbgPrintf("RvdLoggerRecoveryTests: recovered stream record ASN's LSN entry not found: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (streamState[recIx].Size != (value.IoBufferSize + value.HeaderAndMetadataSize))
                {
                    KDbgPrintf(
                        "RvdLoggerRecoveryTests: recovered stream record ASN's Size does not match Ref LSN entry: Line: %i\n",
                        __LINE__);

                    return STATUS_UNSUCCESSFUL;
                }
            }
        }

        // Validate all recovered LSN information - make sure have not over recovered above the highest ref lsn. Also make sure that
        // for every LSN at or above the lowest ref lsn, there is a ref lsn entry.
        RvdLogStreamImp*        streamImpPtr = (RvdLogStreamImp*)stream.RawPtr();
        LogStreamDescriptor*    streamDescPtr = &streamImpPtr->UnsafeGetLogStreamDescriptor();
        streamImpPtr = nullptr;
        RvdLogLsnEntryTracker*  recLsnsPtr = &streamDescPtr->LsnIndex;
        streamDescPtr = nullptr;

        for (ULONG recLsnIx = 0; recLsnIx < recLsnsPtr->QueryNumberOfRecords(); recLsnIx++)
        {
            RvdLogLsn       recLsn;

            status = recLsnsPtr->QueryRecord(recLsnIx, recLsn, nullptr, nullptr);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("RvdLoggerRecoveryTests: QueryRecord failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            if (recLsn.Get() >= RefLogState->_NextLsnToWrite.Get())
            {
                KDbgPrintf(
                    "LogFullRecoveryTests: recovered stream record LSN beyond Ref LogState NextLsnToWrite: Line: %i\n",
                    __LINE__);

                return STATUS_UNSUCCESSFUL;
            }

            if (recLsn.Get() >= RefLogState->_LowestLsn.Get())
            {
                // the recovered record's LSN is within the rang of the log - it must exist in the ref state's LSN table
                // for the stream.
                ULONGLONG   key = (ULONGLONG)recLsn.Get();
                LsnInfo     value;

                status = refLsns.Get(key, value);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("RvdLoggerRecoveryTests: Get failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
            }
        }
        recLsnsPtr = nullptr;

        return STATUS_SUCCESS;
    }

    static NTSTATUS
    ReverseTruncateLogState(
        LogState::SPtr LogState,
        RvdLogRecordHeader& RecordHdrToTruncateFrom)
    {
        NTSTATUS            status = STATUS_SUCCESS;
        RvdLogLsn           newHighestCompletedLsn;
        RvdLogLsn           newNextLsn;
        RvdLogLsn           newHighestCheckpointLsn(LogState->_HighestCheckpointLsn);

        for (ULONG sIx = 0; sIx < LogState->_NumberOfStreams; sIx++)
        {
            if (LogState->_StreamDescs[sIx]._Info.NextLsn >= RecordHdrToTruncateFrom.Lsn)
            {
                // Stream[sIx]'s is impacted by the reverse truncate
                RvdAsnIndexEntry::SPtr      currentAsnEntry = LogState->_StreamDescs[sIx]._AsnIndex->UnsafeFirst();
                while (currentAsnEntry != nullptr)
                {
                    RvdAsnIndexEntry::SPtr      nextAsnEntry = LogState->_StreamDescs[sIx]._AsnIndex->UnsafeNext(currentAsnEntry);

                    if (currentAsnEntry->GetLsn() >= RecordHdrToTruncateFrom.Lsn)
                    {
                        LogState->_StreamDescs[sIx]._AsnIndex->TryRemove(currentAsnEntry, currentAsnEntry->GetVersion());
                    }

                    currentAsnEntry = nextAsnEntry;
                }

                currentAsnEntry = LogState->_StreamDescs[sIx]._AsnIndex->UnsafeFirst();
                if (currentAsnEntry == nullptr)
                {
                    LogState->_StreamDescs[sIx]._TruncationPoint = RvdLogAsn::Null();
                }

                ULONG       lsnEntries = LogState->_StreamDescs[sIx]._LsnIndex->QueryNumberOfRecords();
                RvdLogLsn   recordLsn;
                ULONG       hdrAndMetadataSize = 0;   // Assumes lsnEntries > 0
                ULONG       ioBufferSize = 0;         // Assumes lsnEntries > 0

                while (lsnEntries > 0)
                {
                    status = LogState->_StreamDescs[sIx]._LsnIndex->QueryRecord(lsnEntries - 1, recordLsn, &hdrAndMetadataSize, &ioBufferSize);
                    KInvariant(NT_SUCCESS(status));

                    if ((recordLsn.Get() + hdrAndMetadataSize + ioBufferSize) > RecordHdrToTruncateFrom.Lsn.Get())
                    {
                        // Reverse Truncate the LSN space
                        KInvariant(LogState->_StreamDescs[sIx]._LsnIndex->RemoveHighestLsnRecord() == recordLsn);
                    }
                    else
                    {
                        // Lower than the reverse truncate point
                        break;
                    }

                    lsnEntries--;
                }

                // Capture new high markers for the stream[sIx]
                if (lsnEntries > 0)
                {
                    LogState->_StreamDescs[sIx]._Info.HighestLsn = recordLsn;
                    LogState->_StreamDescs[sIx]._Info.NextLsn = recordLsn.Get() + hdrAndMetadataSize + ioBufferSize;
                }
                else
                {
                    // Empty stream
                    if ((LogState->_StreamDescs[sIx]._Info.LogStreamId == RvdDiskLogConstants::CheckpointStreamId()) &&
                        (RecordHdrToTruncateFrom.LogStreamId == RvdDiskLogConstants::CheckpointStreamId()))
                    {
                        LogState->_StreamDescs[sIx]._Info.HighestLsn = RecordHdrToTruncateFrom.PrevLsnInLogStream;
                        LogState->_StreamDescs[sIx]._Info.LowestLsn = RecordHdrToTruncateFrom.PrevLsnInLogStream;
                        LogState->_StreamDescs[sIx]._Info.NextLsn = RecordHdrToTruncateFrom.Lsn;
                    }
                    else
                    {
                        LogState->_StreamDescs[sIx]._Info.HighestLsn = RvdLogLsn::Null();
                        LogState->_StreamDescs[sIx]._Info.LowestLsn = RvdLogLsn::Null();
                        LogState->_StreamDescs[sIx]._Info.NextLsn = RvdLogLsn::Null();
                    }
                }

                // Accum new high marks for the overall log
                newHighestCompletedLsn.SetIfLarger(LogState->_StreamDescs[sIx]._Info.HighestLsn);
                newNextLsn.SetIfLarger(LogState->_StreamDescs[sIx]._Info.NextLsn);
                if (LogState->_StreamDescs[sIx]._Info.LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
                {
                    newHighestCheckpointLsn = LogState->_StreamDescs[sIx]._Info.HighestLsn;
                }
            }
        }

        // Update the log state's log high marks and free space
        KInvariant(LogState->_NextLsnToWrite >= newNextLsn);
        ULONGLONG       spaceTrimmed = LogState->_NextLsnToWrite.Get() - newNextLsn.Get();

        LogState->_LogFileFreeSpace += spaceTrimmed;
        LogState->_NextLsnToWrite = newNextLsn;
        LogState->_HighestCompletedLsn = newHighestCompletedLsn;
        LogState->_HighestCheckpointLsn = newHighestCheckpointLsn;

        return STATUS_SUCCESS;
    }
};



//*** Recovery Basic Tests for the logger.
//
//      Test1: Prove empty log (no streams) recovery
//      Test2: Prove log just 1 empty stream recovers
//      Test3: Prove log with many empty streams recovers
//      Test4: Prove 1 empty stream recovers (can be opened)
//      Test5: Prove many empty streams recovers (can be opened)
//      Test6: Prove a log with all streams deleted recovers
//      Test7: Prove a log with several streams deleted and several empty existing streams recovers
//      Test8: Prove stream opened for a log with several streams deleted and several empty existing streams recovers
//      Test9: Prove a recovered (empty) stream can be written to, closed, and recovered
//      Test10: Prove recovered (empty) streams can be written to, closed, and recovered
//
NTSTATUS
BasicLogRecoveryTests(WCHAR OnDrive, KAllocator& Allocator)
{
    static GUID const   testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const   testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    KWString            fullyQualifiedDiskFileName(Allocator);
    KGuid               driveGuid;
    RvdLogId            logId(testLogIdGuid);
    RvdLogStreamType    testStreamType(testStreamTypeGuid);
    NTSTATUS            status;
    LONGLONG            logSize = 1024 * 1024 * 1024;

    status = CleanAndPrepLog(OnDrive);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: CleanAndPrepLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: GetDriveAndPathOfLogFile failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::Create failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->Activate(nullptr, nullptr);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::Activate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    // Verify VerificationCallback facility
    {
        RvdLogManager::VerificationCallback callback;

        status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifierTestCallback);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf(
                "BasicLogRecoveryTests: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
                status,
                __LINE__);

            return status;
        }

        callback = logManager->QueryVerificationCallback(testStreamType);
        if (!callback || (callback((UCHAR*)nullptr, 0, KIoBuffer::SPtr()) != STATUS_SUCCESS))
        {
            KDbgPrintf("BasicLogRecoveryTests: VerificationCallback returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!logManager->UnRegisterVerificationCallback(testStreamType))
        {
            KDbgPrintf("BasicLogRecoveryTests: UnRegisterVerificationCallback failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifier);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf(
                "BasicLogRecoveryTests: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
                status,
                __LINE__);

            return status;
        }
    }

    RvdLogManager::AsyncCreateLog::SPtr     createLogOp;
    RvdLogManager::AsyncOpenLog::SPtr       openLogOp;
    RvdLogManager::AsyncDeleteLog::SPtr     deleteLogOp;
    Synchronizer                            compEvent;

    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::CreateAsyncOpenLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::CreateAsyncDeleteLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //** Test 1: Prove empty log (no streams) recovery
    {
        RvdLog::SPtr            log;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));
    
        createLogOp->StartCreateLog(
            driveGuid,
            logId,
            logType,
            logSize,
            log,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        createLogOp->Reuse();

        // Establish empty log state after creation
        RvdLogManagerImp::RvdOnDiskLog*     logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        LONGLONG            lowestLsnValue;
        LONGLONG            highestLsnValue;

        logImp->GetPhysicalExtent(lowestLsnValue, highestLsnValue);
        if ((lowestLsnValue != RvdLogLsn::Min().Get()) || (highestLsnValue != RvdLogLsn::Min().Get()))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        ULONGLONG           totalLogSpace;
        ULONGLONG           totalLogFreeSpace;

        log->QuerySpaceInformation(&totalLogSpace, &totalLogFreeSpace);

        log.Reset();

        // Empty log created - prove it can be opened (recovered)
        openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        openLogOp->Reuse();

        // Validate recovered empty log state from recovery
        logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        LONGLONG            lowestLsnValue1;
        LONGLONG            highestLsnValue1;

        logImp->GetPhysicalExtent(lowestLsnValue1, highestLsnValue1);
        if ((lowestLsnValue1 != lowestLsnValue) || (highestLsnValue1 != highestLsnValue))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        ULONGLONG           totalLogSpace1;
        ULONGLONG           totalLogFreeSpace1;

        log->QuerySpaceInformation(&totalLogSpace1, &totalLogFreeSpace1);
        if ((totalLogSpace1 != totalLogSpace) || (totalLogFreeSpace1 != totalLogFreeSpace))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // Prove there are no user streams
        if (log->GetNumberOfStreams() != 1)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        RvdLogStreamId      ids[1];
        if (log->GetStreams(1, &ids[0]) != 1)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (ids[0] != RvdDiskLogConstants::CheckpointStreamId())
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed with wrong id: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        log.Reset();

        deleteLogOp->StartDeleteLog(
            driveGuid,
            logId,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartDeleteLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        deleteLogOp->Reuse();
    }

    //** Test2: Prove log just 1 empty stream recovers
    //** Test4: Prove 1 empty stream recovers (can be opened)
    //** Test9: Prove a recovered (empty) stream can be written to, closed, and recovered
    {
        RvdLog::SPtr                            log;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));
    
        createLogOp->StartCreateLog(
            driveGuid,
            logId,
            logType,
            logSize,
            log,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        createLogOp->Reuse();

        RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
        status = log->CreateAsyncCreateLogStreamContext(createStreamOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: CreateAsyncCreateLogStreamContext failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        RvdLogStream::SPtr          stream;
        GUID const                  testStreamIdGuid = {1, 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};
        RvdLogStreamId              testStreamId(testStreamIdGuid);

        createStreamOp->StartCreateLogStream(
            testStreamId,
            RvdLogStreamType(testStreamTypeGuid),
            stream,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: StartCreateLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        // Establish empty log state after creation of one stream in empty log - s/b one one record
        // in the file - the initial CP record
        RvdLogManagerImp::RvdOnDiskLog*     logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        LONGLONG            lowestLsnValue;
        LONGLONG            highestLsnValue;

        logImp->GetPhysicalExtent(lowestLsnValue, highestLsnValue);
        if ((lowestLsnValue != RvdLogLsn::Min().Get()) || (highestLsnValue != RvdLogLsn::Min().Get() + RvdDiskLogConstants::BlockSize))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        ULONGLONG           totalLogSpace;
        ULONGLONG           totalLogFreeSpace;

        log->QuerySpaceInformation(&totalLogSpace, &totalLogFreeSpace);

        createStreamOp.Reset();
        stream.Reset();
        KNt::Sleep(250);
        KInvariant(log.Reset());

        // Empty log with one user stream created - prove it can be opened (recovered)
        openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        openLogOp->Reuse();

        // Validate recovered log state from recovery
        logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        LONGLONG            lowestLsnValue1;
        LONGLONG            highestLsnValue1;

        logImp->GetPhysicalExtent(lowestLsnValue1, highestLsnValue1);
        if ((lowestLsnValue1 != lowestLsnValue) || (highestLsnValue1 != highestLsnValue))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        ULONGLONG           totalLogSpace1;
        ULONGLONG           totalLogFreeSpace1;

        log->QuerySpaceInformation(&totalLogSpace1, &totalLogFreeSpace1);
        if ((totalLogSpace1 != totalLogSpace) || (totalLogFreeSpace1 != totalLogFreeSpace))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // Prove there are no user streams
        if (log->GetNumberOfStreams() != 2)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        RvdLogStreamId      ids[2];
        if (log->GetStreams(2, &ids[0]) != 2)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if ((ids[0] != RvdDiskLogConstants::CheckpointStreamId()) && (ids[0] != testStreamId))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed with wrong id: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        if ((ids[1] != RvdDiskLogConstants::CheckpointStreamId()) && (ids[1] != testStreamId))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed with wrong id: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Test4: Prove empty stream recovers (can be opened)
        {
            RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
            RvdLogStream::SPtr                      stream2;

            status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "BasicLogRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            openStreamOp->StartOpenLogStream(testStreamId, stream2, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("BasicLogRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openStreamOp.Reset();

            RvdLogAsn           lowestAsn;
            RvdLogAsn           highestAsn;
            RvdLogAsn           truncationAsn;

            status = stream2->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            if (!lowestAsn.IsNull() || !highestAsn.IsNull() || !truncationAsn.IsNull())
            {
                KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            BOOLEAN     isOpen;
            if (!log->GetStreamState(testStreamId, &isOpen))
            {
                KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (!isOpen)
            {
                KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            //* Test9: Prove a recovered (empty) stream can be written to, closed, and recovered
            {
                RvdLogAsn                               testAsn(25);
                ULONGLONG                               testVersion = 15;
                RvdLogStream::AsyncWriteContext::SPtr   writeOp;

                status = stream2->CreateAsyncWriteContext(writeOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "BasicLogRecoveryTests: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                KBuffer::SPtr           testRecMetadata;
                KIoBuffer::SPtr         testRecIoBuffer;

                status = RecoverySupport::CreateTestRecord(
                    Allocator,
                    testAsn,
                    testVersion,
                    1024,
                    testRecMetadata,
                    testRecIoBuffer);

                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "BasicLogRecoveryTests: Local.CreateTestRecord failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                writeOp->StartWrite(
                    testAsn,
                    testVersion,
                    testRecMetadata,
                    testRecIoBuffer,
                    nullptr,
                    compEvent.AsyncCompletionCallback());

                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                // Establish log state after write of one record into empty log - s/b CP rec and the one
                // user record just written.
                RvdLogManagerImp::RvdOnDiskLog* logImp2 = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

                logImp2->GetPhysicalExtent(lowestLsnValue, highestLsnValue);
                if ((lowestLsnValue != RvdLogLsn::Min().Get()) ||
                    (highestLsnValue != RvdLogLsn::Min().Get() +
                     RvdDiskLogConstants::BlockSize +       // CP rec
                     RvdDiskLogConstants::BlockSize +       // User Rec & Metadata
                     testRecIoBuffer->QuerySize()           // User Rec IoBuffer
                     ))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
                logImp2 = nullptr;

                // Close log down
                writeOp.Reset();
                stream2.Reset();
                KNt::Sleep(250);
                KInvariant(log.Reset());

                // Prove log open recovers
                openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openLogOp->Reuse();

                // Validate recovered log state from recovery
                logImp2 = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

                logImp2->GetPhysicalExtent(lowestLsnValue1, highestLsnValue1);
                if ((lowestLsnValue1 != lowestLsnValue) || (highestLsnValue1 != highestLsnValue))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
                logImp2 = nullptr;

                status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "BasicLogRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                openStreamOp->StartOpenLogStream(testStreamId, stream2, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openStreamOp.Reset();

                status = stream2->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if ((lowestAsn != testAsn) || (highestAsn != testAsn) || !truncationAsn.IsNull())
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (!log->GetStreamState(testStreamId, &isOpen))
                {
                    KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (!isOpen)
                {
                    KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }
        KNt::Sleep(250);
        KInvariant(log.Reset());

        deleteLogOp->StartDeleteLog(
            driveGuid,
            logId,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartDeleteLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        deleteLogOp->Reuse();
    }

    //** Test3: Prove log with many empty streams recovers
    //** Test5: Prove many empty streams recover (can be opened)
    //** Test10: Prove recovered (empty) streams can be written to, closed, and recovered
    //** Test6: Prove a log with all streams deleted recovers
    {
        RvdLog::SPtr            log;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));
    
        createLogOp->StartCreateLog(
            driveGuid,
            logId,
            logType,
            logSize,
            log,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        createLogOp->Reuse();

        RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
        status = log->CreateAsyncCreateLogStreamContext(createStreamOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: CreateAsyncCreateLogStreamContext failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        ULONG const                 numberOfStreams = 50;
        RvdLogStreamId              testStreamIds[numberOfStreams];

        for (ULONG ix = 0; ix < numberOfStreams; ix++)
        {
            RvdLogStream::SPtr          stream;
            GUID const                  testStreamIdGuid = {1 + ix, 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};

            RvdLogStreamId              testStreamId(testStreamIdGuid);
            testStreamIds[ix] = testStreamId;

            createStreamOp->StartCreateLogStream(
                testStreamId,
                RvdLogStreamType(testStreamTypeGuid),
                stream,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("BasicLogRecoveryTests: StartCreateLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            createStreamOp->Reuse();
        }

        // Establish empty log state after creation of numberOfStreams streams in empty log - s/b numberOfStreams records
        // in the file - the CP record for each stream create
        RvdLogManagerImp::RvdOnDiskLog*     logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        LONGLONG            lowestLsnValue;
        LONGLONG            highestLsnValue;

        logImp->GetPhysicalExtent(lowestLsnValue, highestLsnValue);
        if ((lowestLsnValue != RvdLogLsn::Min().Get()) ||
            (highestLsnValue != RvdLogLsn::Min().Get() + (RvdDiskLogConstants::BlockSize * numberOfStreams)))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        ULONGLONG           totalLogSpace;
        ULONGLONG           totalLogFreeSpace;

        log->QuerySpaceInformation(&totalLogSpace, &totalLogFreeSpace);
        createStreamOp.Reset();
        KNt::Sleep(250);
        KInvariant(log.Reset());

        // Empty log with one user stream created - prove it can be opened (recovered)
        openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        openLogOp->Reuse();

        // Prove there are no user streams
        if (log->GetNumberOfStreams() != numberOfStreams + 1)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        RvdLogStreamId      ids[numberOfStreams + 1];
        if (log->GetStreams(numberOfStreams + 1, &ids[0]) != numberOfStreams + 1)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] != RvdDiskLogConstants::CheckpointStreamId())
            {
                BOOLEAN     found = FALSE;
                for (ULONG ix1 = 0; ix1 < numberOfStreams; ix1++)
                {
                    if (ids[ix] == testStreamIds[ix1])
                    {
                        found = TRUE;
                        break;
                    }
                }

                if (!found)
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams failed with wrong id: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }

        // Validate recovered log state from recovery
        logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        LONGLONG            lowestLsnValue1;
        LONGLONG            highestLsnValue1;

        logImp->GetPhysicalExtent(lowestLsnValue1, highestLsnValue1);
        if ((lowestLsnValue1 != lowestLsnValue) || (highestLsnValue1 != highestLsnValue))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        ULONGLONG           totalLogSpace1;
        ULONGLONG           totalLogFreeSpace1;

        log->QuerySpaceInformation(&totalLogSpace1, &totalLogFreeSpace1);
        if ((totalLogSpace1 != totalLogSpace) || (totalLogFreeSpace1 != totalLogFreeSpace))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //** Test5: Prove many empty streams recover (can be opened)
        //** Test10 - part1: Prove multiple empty streams can be written to - (then closed and recovered - in part 2)
        ULONGLONG           logSpaceUsed = (RvdDiskLogConstants::BlockSize * numberOfStreams);

        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] != RvdDiskLogConstants::CheckpointStreamId())
            {
                RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
                RvdLogStream::SPtr                      stream;

                status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "BasicLogRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                openStreamOp->StartOpenLogStream(ids[ix], stream, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openStreamOp.Reset();

                RvdLogAsn           lowestAsn;
                RvdLogAsn           highestAsn;
                RvdLogAsn           truncationAsn;

                status = stream->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if (!lowestAsn.IsNull() || !highestAsn.IsNull() || !truncationAsn.IsNull())
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                BOOLEAN     isOpen;
                if (!log->GetStreamState(ids[ix], &isOpen))
                {
                    KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (!isOpen)
                {
                    KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                //* Test10 - part1: Prove multiple empty streams can be written to then closed (and recovered again - in part 2)
                {
                    RvdLogAsn                               testAsn(1000 - ix);
                    ULONGLONG                               testVersion = 15 + ix;
                    RvdLogStream::AsyncWriteContext::SPtr   writeOp;

                    status = stream->CreateAsyncWriteContext(writeOp);
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf(
                            "BasicLogRecoveryTests: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                            status,
                            __LINE__);

                        return status;
                    }

                    KBuffer::SPtr           testRecMetadata;
                    KIoBuffer::SPtr         testRecIoBuffer;

                    status = RecoverySupport::CreateTestRecord(
                        Allocator,
                        testAsn,
                        testVersion,
                        1024 - ix,
                        testRecMetadata,
                        testRecIoBuffer);

                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf(
                            "BasicLogRecoveryTests: Local.CreateTestRecord failed: 0x%08X: Line: %i\n",
                            status,
                            __LINE__);

                        return status;
                    }

                    writeOp->StartWrite(
                        testAsn,
                        testVersion,
                        testRecMetadata,
                        testRecIoBuffer,
                        nullptr,
                        compEvent.AsyncCompletionCallback());

                    status = compEvent.WaitForCompletion();
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("BasicLogRecoveryTests: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }

                    logSpaceUsed += (RvdDiskLogConstants::BlockSize + testRecIoBuffer->QuerySize());
                }
            }
        }

        // Establish empty log state after creation of numberOfStreams streams in empty log; then writing one record to each
        logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

        logImp->GetPhysicalExtent(lowestLsnValue, highestLsnValue);
        if ((lowestLsnValue != RvdLogLsn::Min().Get()) ||
            (highestLsnValue != RvdLogLsn::Min().Get() + (LONGLONG)logSpaceUsed))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        logImp = nullptr;

        log->QuerySpaceInformation(&totalLogSpace, &totalLogFreeSpace);
        if ((totalLogSpace - logSpaceUsed) != totalLogFreeSpace)
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value(s): Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //** Test10 - part2: Prove multiple empty streams written to after recovery then closed can be recovered again
        {
            KNt::Sleep(250);
            KInvariant(log.Reset());        // Close log

            // Recover log file
            openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openLogOp->Reuse();

            // Validate phys log extent after recovery
            logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

            logImp->GetPhysicalExtent(lowestLsnValue1, highestLsnValue1);
            if ((lowestLsnValue1 != lowestLsnValue) || (highestLsnValue1 != highestLsnValue))
            {
                KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
            logImp = nullptr;

            log->QuerySpaceInformation(&totalLogSpace1, &totalLogFreeSpace1);
            if ((totalLogSpace != totalLogSpace1) || (totalLogFreeSpace != totalLogFreeSpace1))
            {
                KDbgPrintf("BasicLogRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value(s): Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            // Prove each stream can be recovered and the written record is present
            RvdLog::AsyncOpenLogStreamContext::SPtr     openStreamOp;

            status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "BasicLogRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
            {
                if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                {
                    continue;
                }

                RvdLogStream::SPtr      stream;

                // Recover stream
                openStreamOp->StartOpenLogStream(ids[ix], stream, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openStreamOp->Reuse();

                // Verify recovered stream contents
                RvdLogAsn               lowestAsn;
                RvdLogAsn               highestAsn;
                RvdLogAsn               truncationAsn;

                status = stream->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                RvdLogAsn                   testAsn(1000 - ix);
                ULONGLONG                   testVersion = 15 + ix;

                if ((lowestAsn != testAsn) || (highestAsn != testAsn) || !truncationAsn.IsNull())
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                BOOLEAN     isOpen;
                if (!log->GetStreamState(ids[ix], &isOpen))
                {
                    KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (!isOpen)
                {
                    KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                ULONGLONG                           ver;
                RvdLogStream::RecordDisposition     disposition;
                ULONG                               ioBufferSize;
                ULONGLONG                           lsn;

                status = stream->QueryRecord(
                    testAsn,
                    &ver,
                    &disposition,
                    &ioBufferSize,
                    &lsn);

                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecord failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                // Vaidate record info
                if (ver != testVersion)
                {
                    KDbgPrintf("BasicLogRecoveryTests: QueryRecord failed - invalid ver returned: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }
            openStreamOp.Reset();

            //** Test7: Prove a log with several streams deleted and several empty existing streams recovers
            //** Test8: Prove streams opened for a log with several streams deleted and several empty existing streams recovers
            {
                RvdLog::AsyncDeleteLogStreamContext::SPtr     delStreamOp;

                status = log->CreateAsyncDeleteLogStreamContext(delStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "BasicLogRecoveryTests: RvdLog::CreateAsyncDeleteLogStreamContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                for (ULONG ix = 1; ix < numberOfStreams + 1; ix += 2)           // Delete all odd log streams
                {
                    if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        continue;
                    }

                    delStreamOp->StartDeleteLogStream(ids[ix],  nullptr, compEvent.AsyncCompletionCallback());
                    status = compEvent.WaitForCompletion();
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("BasicLogRecoveryTests: StartDeleteLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }
                    delStreamOp->Reuse();
                }
                delStreamOp.Reset();
                KNt::Sleep(250);
                KInvariant(log.Reset());

                // Recover log file
                openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openLogOp->Reuse();

                if (log->GetNumberOfStreams() != ((numberOfStreams / 2) + 1))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams returned wrong value: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                // Test8: Prove streams opened for a log with several streams deleted and several empty existing streams recover
                {
                    status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf(
                            "BasicLogRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                            status,
                            __LINE__);

                        return status;
                    }

                    for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)           // attempt to open all even streams
                    {
                        if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                        {
                            continue;
                        }

                        RvdLogStream::SPtr      stream;

                        openStreamOp->StartOpenLogStream(ids[ix], stream, nullptr, compEvent.AsyncCompletionCallback());
                        status = compEvent.WaitForCompletion();
                        if (!NT_SUCCESS(status))
                        {
                            KDbgPrintf("BasicLogRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                            return status;
                        }
                        openStreamOp->Reuse();

                        // Prove their is one user record in each undeleted stream just opened.
                        // Verify recovered stream contents
                        RvdLogAsn               lowestAsn;
                        RvdLogAsn               highestAsn;
                        RvdLogAsn               truncationAsn;

                        status = stream->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
                        if (!NT_SUCCESS(status))
                        {
                            KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                            return status;
                        }

                        RvdLogAsn                   testAsn(1000 - ix);
                        ULONGLONG                   testVersion = 15 + ix;

                        if ((lowestAsn != testAsn) || (highestAsn != testAsn) || !truncationAsn.IsNull())
                        {
                            KDbgPrintf("BasicLogRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                            return STATUS_UNSUCCESSFUL;
                        }

                        BOOLEAN     isOpen;
                        if (!log->GetStreamState(ids[ix], &isOpen))
                        {
                            KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                            return STATUS_UNSUCCESSFUL;
                        }

                        if (!isOpen)
                        {
                            KDbgPrintf("BasicLogRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                            return STATUS_UNSUCCESSFUL;
                        }

                        ULONGLONG                           ver;
                        RvdLogStream::RecordDisposition     disposition;
                        ULONG                               ioBufferSize;
                        ULONGLONG                           lsn;

                        status = stream->QueryRecord(
                            testAsn,
                            &ver,
                            &disposition,
                            &ioBufferSize,
                            &lsn);

                        if (!NT_SUCCESS(status))
                        {
                            KDbgPrintf("BasicLogRecoveryTests: QueryRecord failed: 0x%08X: Line: %i\n", status, __LINE__);
                            return status;
                        }

                        // Vaidate record info
                        if (ver != testVersion)
                        {
                            KDbgPrintf("BasicLogRecoveryTests: QueryRecord failed - invalid ver returned: Line: %i\n", __LINE__);
                            return STATUS_UNSUCCESSFUL;
                        }
                    }
                    openStreamOp.Reset();
                    KNt::Sleep(250);
                    KInvariant(log.Reset());
                }
            }

            //** Test6: Prove a log with all streams deleted recovers
            {
                openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openLogOp->Reuse();

                if (log->GetNumberOfStreams() != ((numberOfStreams / 2) + 1))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetNumberOfStreams returned wrong value: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                RvdLog::AsyncDeleteLogStreamContext::SPtr     delStreamOp;

                status = log->CreateAsyncDeleteLogStreamContext(delStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "BasicLogRecoveryTests: RvdLog::CreateAsyncDeleteLogStreamContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)           // all even logs
                {
                    if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        continue;
                    }

                    delStreamOp->StartDeleteLogStream(ids[ix],  nullptr, compEvent.AsyncCompletionCallback());
                    status = compEvent.WaitForCompletion();
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("BasicLogRecoveryTests: StartDeleteLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }
                    delStreamOp->Reuse();
                }
                delStreamOp.Reset();
                KNt::Sleep(250);
                KInvariant(log.Reset());

                // Recover log file
                openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openLogOp->Reuse();

                // Validate phys log extent after recovery.
                //
                // NOTE: Each stream deletion causes a trucation of all the subject stream's space
                //       and then causes a CP record to be written recording the fact that the stream
                //       is no longer present. The truncation logic also truncates the CP stream itself
                //       such that only the last CP record in that stream is reflected in the new CP
                //       record being written. This means that during recovery up to two CP records can
                //       be found in the allocated LSN space - this must the case in this test.
                logImp = (RvdLogManagerImp::RvdOnDiskLog*)log.RawPtr();

                logImp->GetPhysicalExtent(lowestLsnValue1, highestLsnValue1);
                if ((highestLsnValue1 - lowestLsnValue1) != 2 * 4096)
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::GetPhysicalExtent returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
                logImp = nullptr;

                log->QuerySpaceInformation(&totalLogSpace1, &totalLogFreeSpace1);
                if ((totalLogSpace1 - totalLogFreeSpace1) != 2 * 4096)
                {
                    KDbgPrintf("BasicLogRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }

        KNt::Sleep(250);
        KInvariant(log.Reset());

        deleteLogOp->StartDeleteLog(
            driveGuid,
            logId,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartDeleteLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        deleteLogOp->Reuse();
    }

    logManager->Deactivate();
    return STATUS_SUCCESS;
}


//*** Test multistream log-full physical and stream recovery
//
//      Test1: Write multiple streams (random record sizes) until log is full; prove recovery of contents
//      Test2: Delete multiple streams from Test1; prove log and stream recovered
//      Test3: Prove after full truncation of all streams, the log is recovered down to max free space
//          Test 3a: Prove a fully truncated stream with a record written after the truncation CP can recover
//      Test4: Delete remaining streams from Test3; prove log recovered down to max free space
//
NTSTATUS
LogFullRecoveryTests(WCHAR OnDrive, KAllocator& Allocator)
{
    static GUID const   testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const   testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    KWString            fullyQualifiedDiskFileName(Allocator);
    KGuid               driveGuid;
    RvdLogId            logId(testLogIdGuid);
    RvdLogStreamType    testStreamType(testStreamTypeGuid);
    NTSTATUS            status;
    LONGLONG            logSize = 1024 * 1024 * 1024;
    Synchronizer        deactivateCompEvent;

    status = CleanAndPrepLog(OnDrive);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: CleanAndPrepLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: GetDriveAndPathOfLogFile failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::Create failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->Activate(nullptr, deactivateCompEvent.AsyncCompletionCallback());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::Activate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifier);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf(
            "LogFullRecoveryTests: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
            status,
            __LINE__);

        return status;
    }

    RvdLogManager::AsyncCreateLog::SPtr     createLogOp;
    RvdLogManager::AsyncOpenLog::SPtr       openLogOp;
    RvdLogManager::AsyncDeleteLog::SPtr     deleteLogOp;
    Synchronizer                            compEvent;

    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::CreateAsyncOpenLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::CreateAsyncDeleteLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //** Test1: Write multiple streams (random record sizes) until log is full; prove recovery of contents
    {
        RandomGenerator::SetSeed(5);

        RvdLog::SPtr            log;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));
    
        createLogOp->StartCreateLog(
            driveGuid,
            logId,
            logType,
            logSize,
            log,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("LogFullRecoveryTests: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        createLogOp->Reuse();

        RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
        status = log->CreateAsyncCreateLogStreamContext(createStreamOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("LogFullRecoveryTests: CreateAsyncCreateLogStreamContext failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        ULONG const                 numberOfStreams = 11;
        RvdLogStreamId              testStreamIds[numberOfStreams];

        for (ULONG ix = 0; ix < numberOfStreams; ix++)
        {
            RvdLogStream::SPtr          stream;
            GUID const                  testStreamIdGuid = {1 + ix, 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};

            RvdLogStreamId              testStreamId(testStreamIdGuid);
            testStreamIds[ix] = testStreamId;

            createStreamOp->StartCreateLogStream(
                testStreamId,
                RvdLogStreamType(testStreamTypeGuid),
                stream,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: StartCreateLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            createStreamOp->Reuse();
        }
        createStreamOp.Reset();

        // Prove there are no user streams
        if (log->GetNumberOfStreams() != numberOfStreams + 1)
        {
            KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        RvdLogStreamId      ids[numberOfStreams + 1];
        if (log->GetStreams(numberOfStreams + 1, &ids[0]) != numberOfStreams + 1)
        {
            KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] != RvdDiskLogConstants::CheckpointStreamId())
            {
                BOOLEAN     found = FALSE;
                for (ULONG ix1 = 0; ix1 < numberOfStreams; ix1++)
                {
                    if (ids[ix] == testStreamIds[ix1])
                    {
                        found = TRUE;
                        break;
                    }
                }

                if (!found)
                {
                    KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams failed with wrong id: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }

        ULONGLONG                               logSpaceUsed = (RvdDiskLogConstants::BlockSize * numberOfStreams);
        RvdLogAsn                               highStreamsAsns[numberOfStreams + 1];
        LogState::SPtr                          logState;
        {
            RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;

            status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogFullRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            RvdLogStream::SPtr                      streams[numberOfStreams + 1];
            RvdLogStream::AsyncWriteContext::SPtr   writeOps[numberOfStreams + 1];

            for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
            {
                if (ids[ix] != RvdDiskLogConstants::CheckpointStreamId())
                {
                    openStreamOp->StartOpenLogStream(ids[ix], streams[ix], nullptr, compEvent.AsyncCompletionCallback());
                    status = compEvent.WaitForCompletion();
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("LogFullRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }
                    openStreamOp->Reuse();

                    RvdLogAsn           lowestAsn;
                    RvdLogAsn           highestAsn;
                    RvdLogAsn           truncationAsn;

                    status = streams[ix]->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("LogFullRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }

                    if (!lowestAsn.IsNull() || !highestAsn.IsNull() || !truncationAsn.IsNull())
                    {
                        KDbgPrintf("LogFullRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                        return STATUS_UNSUCCESSFUL;
                    }

                    BOOLEAN     isOpen;
                    if (!log->GetStreamState(ids[ix], &isOpen))
                    {
                        KDbgPrintf("LogFullRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                        return STATUS_UNSUCCESSFUL;
                    }

                    if (!isOpen)
                    {
                        KDbgPrintf("LogFullRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                        return STATUS_UNSUCCESSFUL;
                    }

                    status = streams[ix]->CreateAsyncWriteContext(writeOps[ix]);
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf(
                            "LogFullRecoveryTests: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                            status,
                            __LINE__);

                        return status;
                    }
                }
            }

            // Next, fill the log with random record size (0 - 10 blks) for the various streams
            BOOLEAN         logIsFull = FALSE;
            ULONGLONG       asnOffset = 1000;

            while (!logIsFull)
            {
                ULONG           numberOfSuccesses = 0;

                for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
                {
                    if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        continue;
                    }

                    RvdLogAsn               testAsn(ix + asnOffset);
                    ULONGLONG               testVersion = 13 + ix;


                    KBuffer::SPtr           testRecMetadata;
                    KIoBuffer::SPtr         testRecIoBuffer;
                    ULONGLONG               ioBufferSize = RandomGenerator::Get(10) * RvdDiskLogConstants::BlockSize;

                    status = RecoverySupport::CreateTestRecord(
                        Allocator,
                        testAsn,
                        testVersion,
                        (ULONG)(ioBufferSize / RecoverySupport::SizeOfTestRecordIoBufferElement),
                        testRecMetadata,
                        testRecIoBuffer);

                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf(
                            "LogFullRecoveryTests: CreateTestRecord failed: 0x%08X: Line: %i\n",
                            status,
                            __LINE__);

                        return status;
                    }

                    writeOps[ix]->StartWrite(
                        testAsn,
                        testVersion,
                        testRecMetadata,
                        testRecIoBuffer,
                        nullptr,
                        compEvent.AsyncCompletionCallback());

                    status = compEvent.WaitForCompletion();
                    writeOps[ix]->Reuse();

                    BOOLEAN     logFullStatus = (status == STATUS_LOG_FULL);

                    if (!NT_SUCCESS(status) && !logFullStatus)
                    {
                        KDbgPrintf("LogFullRecoveryTests: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }

                    if (!logFullStatus)
                    {
                        logSpaceUsed += (RvdDiskLogConstants::BlockSize + ((testRecIoBuffer == nullptr) ? 0 : testRecIoBuffer->QuerySize()));        // does not allow for CP records
                        numberOfSuccesses++;
                        highStreamsAsns[ix] = testAsn;
                    }
                }

                asnOffset++;
                logIsFull = numberOfSuccesses == 0;
            }

            // Snap state of the reference (ref) log just created and written to across multiple streams
            status = RecoverySupport::GetLogsState(log, Allocator, logState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        // Prove all the high asns match
        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
            {
                continue;
            }

            LogState::StreamDescriptor*     sPtr = nullptr;
            for (ULONG ix1 = 0; ix1 < logState->_NumberOfStreams; ix1++)
            {
                if (logState->_StreamDescs[ix1]._Info.LogStreamId == ids[ix])
                {
                    sPtr = &logState->_StreamDescs[ix1];
                    break;
                }
            }

            if (sPtr == nullptr)
            {
                KDbgPrintf("LogFullRecoveryTests: Missing StreamDescriptor: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if ((sPtr->_AsnIndex->GetNumberOfEntries() == 0) || (sPtr->_AsnIndex->UnsafeLast()->GetAsn() != highStreamsAsns[ix]))
            {
                KDbgPrintf("LogFullRecoveryTests:Highest ASN mismatch: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        KNt::Sleep(250);
        KInvariant(log.Reset());            // Close the log

        // Prove proper recovery of the log and each stream after log full
        openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        openLogOp->Reuse();

        // Prove physical recovered log state is at least a proper super set of logState
        {
            LogState::SPtr              recLogState;

            status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
            {
                KDbgPrintf("LogFullRecoveryTests:IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Prove each stream is recovered correctly
        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
            {
                continue;
            }

            status = RecoverySupport::ValidateStreamCorrectness(log, ids[ix], logState, Allocator);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: ValidateStreamCorrectness() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        //** Test2: Delete multiple streams from Test1; prove log and stream recovered
        {
            RvdLog::AsyncDeleteLogStreamContext::SPtr       delStreamOp;

            status = log->CreateAsyncDeleteLogStreamContext(delStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogFullRecoveryTests: RvdLog::CreateAsyncDeleteLogStreamContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            for (ULONG ix = 1; ix < numberOfStreams + 1; ix += 2)           // Delete all odd log streams
            {
                if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                {
                    continue;
                }

                delStreamOp->StartDeleteLogStream(ids[ix],  nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: StartDeleteLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                delStreamOp->Reuse();
            }
            delStreamOp.Reset();

            // Snap state of the reference (ref) log and remaining streams just after odd stream deletes
            {
                RvdLogStream::SPtr                      streams[numberOfStreams + 1];
                RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;

                status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "LogFullRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)
                {
                    if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        continue;
                    }

                    RvdLogStream::SPtr      stream;

                    openStreamOp->StartOpenLogStream(ids[ix], streams[ix], nullptr, compEvent.AsyncCompletionCallback());
                    status = compEvent.WaitForCompletion();
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("LogFullRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }
                    openStreamOp->Reuse();
                }
                openStreamOp.Reset();

                status = RecoverySupport::GetLogsState(log, Allocator, logState);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
            }

            // Close log
            KNt::Sleep(250);
            KInvariant(log.Reset());

            // Recover log file
            openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openLogOp->Reuse();

            if (log->GetNumberOfStreams() != ((numberOfStreams / 2) + 1))
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams returned wrong value: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            // Prove physical recovered log state is at least a proper super set of logState
            {
                LogState::SPtr              recLogState;

                status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
                {
                    KDbgPrintf("LogFullRecoveryTests:IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Prove each even stream is recovered correctly
            for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)
            {
                if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                {
                    continue;
                }

                status = RecoverySupport::ValidateStreamCorrectness(log, ids[ix], logState, Allocator);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: ValidateStreamCorrectness() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
            }
        }

        //* Test3: Prove after full truncation of all streams, the log is recovered down to max free space
        {
            RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;

            status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogFullRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            // Snap state of the reference (ref) log just after all streams fully truncated
            {
                RvdLogStream::SPtr          streams[numberOfStreams + 1];
                for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)
                {
                    if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        continue;
                    }

                    openStreamOp->StartOpenLogStream(ids[ix], streams[ix], nullptr, compEvent.AsyncCompletionCallback());
                    status = compEvent.WaitForCompletion();
                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf("LogFullRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }
                    openStreamOp->Reuse();

                    // Trigger truncation of the entire stream
                    streams[ix]->Truncate(RvdLogAsn::Max(), RvdLogAsn::Max());
                }
                openStreamOp.Reset();

                status = RecoverySupport::GetLogsState(log, Allocator, logState);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
            }

            {
                ULONGLONG       totalLogSpace;
                ULONGLONG       freeLogSpace;

                log->QuerySpaceInformation(&totalLogSpace, &freeLogSpace);

                if ((totalLogSpace - freeLogSpace) != (1 * RvdDiskLogConstants::BlockSize))
                {
                    KDbgPrintf("LogFullRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Close log
            KNt::Sleep(250);
            KInvariant(log.Reset());

            // Recover log file
            openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openLogOp->Reuse();

            if (log->GetNumberOfStreams() != ((numberOfStreams / 2) + 1))
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams returned wrong value: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            {
                ULONGLONG       totalLogSpace;
                ULONGLONG       freeLogSpace;

                log->QuerySpaceInformation(&totalLogSpace, &freeLogSpace);

                if ((totalLogSpace - freeLogSpace) != (2 * RvdDiskLogConstants::BlockSize))
                {
                    KDbgPrintf("LogFullRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Prove physical recovered log state after full truncation
            {
                LogState::SPtr              recLogState;

                status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
                {
                    KDbgPrintf("LogFullRecoveryTests:IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Prove each even stream is recovered correctly - all should be fully truncated
            for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)
            {
                if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                {
                    continue;
                }

                status = RecoverySupport::ValidateStreamCorrectness(log, ids[ix], logState, Allocator);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: ValidateStreamCorrectness() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
            }

            // Test 3a: Prove that a fully truncated stream this written to recovers
            {
                RvdLogStream::SPtr      stream;

                status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "LogFullRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                openStreamOp->StartOpenLogStream(ids[2], stream, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openStreamOp->Reuse();

                RvdLogStream::AsyncWriteContext::SPtr   writeOp;
                status = stream->CreateAsyncWriteContext(writeOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                RvdLogAsn               testAsn(1000);
                ULONGLONG               testVersion = 13;
                KBuffer::SPtr           testRecMetadata;
                KIoBuffer::SPtr         testRecIoBuffer;
                ULONGLONG               ioBufferSize = RandomGenerator::Get(10) * RvdDiskLogConstants::BlockSize;

                status = RecoverySupport::CreateTestRecord(
                    Allocator,
                    testAsn,
                    testVersion,
                    (ULONG)(ioBufferSize / RecoverySupport::SizeOfTestRecordIoBufferElement),
                    testRecMetadata,
                    testRecIoBuffer);

                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "LogFullRecoveryTests: CreateTestRecord failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                writeOp->StartWrite(
                    testAsn,
                    testVersion,
                    testRecMetadata,
                    testRecIoBuffer,
                    nullptr,
                    compEvent.AsyncCompletionCallback());

                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                stream->Truncate(testAsn, testAsn);
                while (!log->IsLogFlushed())
                {
                    KNt::Sleep(10);
                }

                testAsn = testAsn.Get() + 1;
                writeOp->Reuse();
                writeOp->StartWrite(
                    testAsn,
                    testVersion,
                    testRecMetadata,
                    testRecIoBuffer,
                    nullptr,
                    compEvent.AsyncCompletionCallback());

                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                writeOp.Reset();
                stream.Reset();
                openStreamOp.Reset();

                status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "LogFullRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                openStreamOp->StartOpenLogStream(ids[2], stream, nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openStreamOp->Reuse();
            }
        }

        //** Test4: Delete remaining streams from Test3; prove log recovered down to max free space
        {
            RvdLog::AsyncDeleteLogStreamContext::SPtr       delStreamOp;

            status = log->CreateAsyncDeleteLogStreamContext(delStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogFullRecoveryTests: RvdLog::CreateAsyncDeleteLogStreamContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            for (ULONG ix = 0; ix < numberOfStreams + 1; ix += 2)           // Delete all even log streams
            {
                if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                {
                    continue;
                }

                delStreamOp->StartDeleteLogStream(ids[ix],  nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: StartDeleteLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                delStreamOp->Reuse();
            }
            delStreamOp.Reset();

            if (log->GetNumberOfStreams() != 1)
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams returned wrong value: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            {
                ULONGLONG       totalLogSpace;
                ULONGLONG       freeLogSpace;

                log->QuerySpaceInformation(&totalLogSpace, &freeLogSpace);

                if ((totalLogSpace - freeLogSpace) != (1 * RvdDiskLogConstants::BlockSize))
                {
                    KDbgPrintf("LogFullRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Snap ref log state after all streams deleted
            status = RecoverySupport::GetLogsState(log, Allocator, logState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        {
            // Close log
            KNt::Sleep(250);
            KInvariant(log.Reset());

            // Recover log file
            openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openLogOp->Reuse();

            if (log->GetNumberOfStreams() != 1)
            {
                KDbgPrintf("LogFullRecoveryTests: RvdLog::GetNumberOfStreams returned wrong value: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            {
                ULONGLONG       totalLogSpace;
                ULONGLONG       freeLogSpace;

                log->QuerySpaceInformation(&totalLogSpace, &freeLogSpace);

                if ((totalLogSpace - freeLogSpace) != (2 * RvdDiskLogConstants::BlockSize))
                {
                    KDbgPrintf("LogFullRecoveryTests: RvdLog::QuerySpaceInformation returned wrong value: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Prove physical recovered log state after full truncation
            {
                LogState::SPtr              recLogState;

                status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("LogFullRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
                {
                    KDbgPrintf("LogFullRecoveryTests: IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }

        // Close log
        KNt::Sleep(250);
        KInvariant(log.Reset());

        deleteLogOp->StartDeleteLog(
            driveGuid,
            logId,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("LogFullRecoveryTests: RvdLogManager::StartDeleteLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        deleteLogOp->Reuse();
    }

    logManager->Deactivate();
    status = deactivateCompEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::Deactivate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}


//*** Test long duration (random) multistream physical and stream recovery
//
//      Test1: Write multiple streams (random record sizes) until desired duration; as log-full
//             is detected truncate lowest records across all streams; prove recovery of contents
//
NTSTATUS
ExtendedRecoveryTests(
    WCHAR OnDrive,
    KAllocator& Allocator,
    ULONG DurationInSecs,
    ULONG RandomSeed = 0)
{
    ULONG const         MaxRecordIoBlks = 50;
    LONGLONG const      LogSize = 1024 * 1024 * 1024;

    static GUID const   testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const   testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    KWString            fullyQualifiedDiskFileName(Allocator);
    KGuid               driveGuid;
    RvdLogId            logId(testLogIdGuid);
    RvdLogStreamType    testStreamType(testStreamTypeGuid);
    NTSTATUS            status;
    Synchronizer        deactivateCompEvent;

    status = CleanAndPrepLog(OnDrive);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: CleanAndPrepLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: GetDriveAndPathOfLogFile failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::Create failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->Activate(nullptr, deactivateCompEvent.AsyncCompletionCallback());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::Activate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifier);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf(
            "ExtendedRecoveryTests: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
            status,
            __LINE__);

        return status;
    }

    RvdLogManager::AsyncCreateLog::SPtr     createLogOp;
    RvdLogManager::AsyncOpenLog::SPtr       openLogOp;
    RvdLogManager::AsyncDeleteLog::SPtr     deleteLogOp;
    Synchronizer                            compEvent;

    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::CreateAsyncOpenLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::CreateAsyncDeleteLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //**    Test1: Write multiple streams (random record sizes) until desired duration; as log-full
    //             is detected truncate lowest records across all streams; prove recovery of contents
    {
        RandomGenerator::SetSeed(RandomSeed);

        RvdLog::SPtr            log;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));
    
        createLogOp->StartCreateLog(
            driveGuid,
            logId,
            logType,
            LogSize,
            log,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        createLogOp->Reuse();

        RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
        status = log->CreateAsyncCreateLogStreamContext(createStreamOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ExtendedRecoveryTests: CreateAsyncCreateLogStreamContext failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        ULONG const                 numberOfStreams = 11;
        RvdLogStreamId              testStreamIds[numberOfStreams];

        for (ULONG ix = 0; ix < numberOfStreams; ix++)
        {
            RvdLogStream::SPtr          stream;
            GUID const                  testStreamIdGuid = {1 + ix, 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};

            RvdLogStreamId              testStreamId(testStreamIdGuid);
            testStreamIds[ix] = testStreamId;

            createStreamOp->StartCreateLogStream(
                testStreamId,
                RvdLogStreamType(testStreamTypeGuid),
                stream,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("ExtendedRecoveryTests: StartCreateLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            createStreamOp->Reuse();
        }
        createStreamOp.Reset();

        // Prove there are no user streams
        if (log->GetNumberOfStreams() != numberOfStreams + 1)
        {
            KDbgPrintf("ExtendedRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        RvdLogStreamId      ids[numberOfStreams + 1];
        if (log->GetStreams(numberOfStreams + 1, &ids[0]) != numberOfStreams + 1)
        {
            KDbgPrintf("ExtendedRecoveryTests: RvdLog::GetNumberOfStreams failed: Line: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
            {
                continue;
            }

            BOOLEAN     found = FALSE;
            for (ULONG ix1 = 0; ix1 < numberOfStreams; ix1++)
            {
                if (ids[ix] == testStreamIds[ix1])
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                KDbgPrintf("ExtendedRecoveryTests: RvdLog::GetNumberOfStreams failed with wrong id: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        RvdLogAsn                               highStreamsAsns[numberOfStreams + 1];
        RvdLogAsn                               streamsTruncationAsns[numberOfStreams + 1];
        LogState::SPtr                          logState;
        {
            RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;

            status = log->CreateAsyncOpenLogStreamContext(openStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "ExtendedRecoveryTests: RvdLog::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            RvdLogStream::SPtr                      streams[numberOfStreams + 1];
            RvdLogStream::AsyncWriteContext::SPtr   writeOps[numberOfStreams + 1];

            for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
            {
                if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                {
                    continue;
                }

                openStreamOp->StartOpenLogStream(ids[ix], streams[ix], nullptr, compEvent.AsyncCompletionCallback());
                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("ExtendedRecoveryTests: StartOpenLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }
                openStreamOp->Reuse();

                RvdLogAsn           lowestAsn;
                RvdLogAsn           highestAsn;
                RvdLogAsn           truncationAsn;

                status = streams[ix]->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("ExtendedRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if (!lowestAsn.IsNull() || !highestAsn.IsNull() || !truncationAsn.IsNull())
                {
                    KDbgPrintf("ExtendedRecoveryTests: QueryRecordRange returned wrong value(s): Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                BOOLEAN     isOpen;
                if (!log->GetStreamState(ids[ix], &isOpen))
                {
                    KDbgPrintf("ExtendedRecoveryTests: GetStreamState failed - stream missing: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (!isOpen)
                {
                    KDbgPrintf("ExtendedRecoveryTests: GetStreamState failed - stream not open: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                status = streams[ix]->CreateAsyncWriteContext(writeOps[ix]);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "ExtendedRecoveryTests: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }
            }

            // Next, fill the log with random record size (0 - MaxRecordIoBlks blks) for the various streams
            ULONGLONG       asnOffset = 1000;
            ULONGLONG       totalRecords = 0;
            ULONGLONG       totalBytes = 0;
            ULONG           minSize = MAXULONG;
            ULONG           maxSize = 0;
            ULONGLONG       totalTruncations = 0;
            ULONGLONG       startTime = KNt::GetTickCount64();
            ULONGLONG       stopTime = startTime + (1000 * DurationInSecs);

            while (stopTime > KNt::GetTickCount64())
            {
                for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
                {
                    if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
                    {
                        continue;
                    }

                    RvdLogAsn               testAsn(ix + asnOffset);
                    ULONGLONG               testVersion = 13 + ix;
                    KBuffer::SPtr           testRecMetadata;
                    KIoBuffer::SPtr         testRecIoBuffer;
                    ULONGLONG               ioBufferSize = RandomGenerator::Get(MaxRecordIoBlks) * RvdDiskLogConstants::BlockSize;

                    status = RecoverySupport::CreateTestRecord(
                        Allocator,
                        testAsn,
                        testVersion,
                        (ULONG)(ioBufferSize / RecoverySupport::SizeOfTestRecordIoBufferElement),
                        testRecMetadata,
                        testRecIoBuffer);

                    if (!NT_SUCCESS(status))
                    {
                        KDbgPrintf(
                            "ExtendedRecoveryTests: CreateTestRecord failed: 0x%08X: Line: %i\n",
                            status,
                            __LINE__);

                        return status;
                    }

#pragma warning(disable:4127)   // C4127: conditional expression is constant                    
                    while (TRUE)
                    {
                        writeOps[ix]->StartWrite(
                            testAsn,
                            testVersion,
                            testRecMetadata,
                            testRecIoBuffer,
                            nullptr,
                            compEvent.AsyncCompletionCallback());

                        status = compEvent.WaitForCompletion();
                        writeOps[ix]->Reuse();

                        BOOLEAN     logFullStatus = (status == STATUS_LOG_FULL);

                        if (!NT_SUCCESS(status) && !logFullStatus)
                        {
                            KDbgPrintf("ExtendedRecoveryTests: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                            return status;
                        }

                        if (!logFullStatus)
                        {
                            highStreamsAsns[ix] = testAsn;
                            totalRecords++;

                            ULONG       testRecIoBufferSize = (testRecIoBuffer == nullptr) ? 0 : testRecIoBuffer->QuerySize();

                            ULONGLONG   sizeWritten = RvdDiskLogConstants::RoundUpToBlockBoundary(testRecMetadata->QuerySize()) + (ULONGLONG)testRecIoBufferSize;
                            KInvariant(sizeWritten <= MAXULONG);

                            totalBytes += sizeWritten;
                            if (sizeWritten < minSize)
                            {
                                minSize = (ULONG)sizeWritten;
                            }
                            if (sizeWritten > maxSize)
                            {
                                maxSize = (ULONG)sizeWritten;
                            }

                            break;
                        }
                        else
                        {
                            // Truncate the lowest record from each stream
                            for (ULONG ix1 = 0; ix1 < numberOfStreams + 1; ix1++)
                            {
                                if (ids[ix1] == RvdDiskLogConstants::CheckpointStreamId())
                                {
                                    continue;
                                }

                                RvdLogAsn       lowestAsn;
                                RvdLogAsn       highestAsn;
                                RvdLogAsn       truncAsn;

                                status = streams[ix1]->QueryRecordRange(&lowestAsn, &highestAsn, &truncAsn);
                                if (!NT_SUCCESS(status))
                                {
                                    KDbgPrintf("ExtendedRecoveryTests: QueryRecordRange failed: 0x%08X: Line: %i\n", status, __LINE__);
                                    return status;
                                }

                                // Attempt to truncate 5% of the stream
                                ULONGLONG       deltaAsnValue = (highestAsn.Get() - lowestAsn.Get()) / 20;
                                RvdLogAsn       newTruncAsn(lowestAsn.Get() + deltaAsnValue);

                                if (truncAsn.Get() < newTruncAsn.Get())
                                {
                                    totalTruncations++;
                                    streams[ix1]->Truncate(newTruncAsn, newTruncAsn);
                                    streamsTruncationAsns[ix1] = newTruncAsn;
                                }
                            }
                        }
                    }
                }

                asnOffset++;
            }

            stopTime = KNt::GetTickCount64();

            // Report stats:
            KDbgPrintf(
                "ExtendedRecoveryTests: Stats: %I64u MB/sec; #Recs: %I64u; #Bytes: %I64u; MinSize: %u; MaxSize %u; #Truncs: %I64u\n",
                (totalBytes / ((stopTime - startTime) / 1000)) / (1024 * 1024),
                totalRecords,
                totalBytes,
                minSize,
                maxSize,
                totalTruncations);

            // Snap state of the reference (ref) log just created and written to across multiple streams
            status = RecoverySupport::GetLogsState(log, Allocator, logState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("ExtendedRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        // Prove all the high asns match
        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
            {
                continue;
            }

            LogState::StreamDescriptor*     sPtr = nullptr;
            for (ULONG ix1 = 0; ix1 < logState->_NumberOfStreams; ix1++)
            {
                if (logState->_StreamDescs[ix1]._Info.LogStreamId == ids[ix])
                {
                    sPtr = &logState->_StreamDescs[ix1];
                    break;
                }
            }

            if (sPtr == nullptr)
            {
                KDbgPrintf("ExtendedRecoveryTests: Missing StreamDescriptor: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if ((sPtr->_AsnIndex->GetNumberOfEntries() == 0) || (sPtr->_AsnIndex->UnsafeLast()->GetAsn() != highStreamsAsns[ix]))
            {
                KDbgPrintf("ExtendedRecoveryTests:Highest ASN mismatch: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (sPtr->_TruncationPoint.Get() != streamsTruncationAsns[ix].Get())
            {
                KDbgPrintf("ExtendedRecoveryTests: Truncation ASN mismatch: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        KNt::Sleep(250);
        KInvariant(log.Reset());            // Close the log

        // Prove proper recovery of the log and each stream after log full
        openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }
        openLogOp->Reuse();

        // Prove physical recovered log state is at least a proper super set of logState
        {
            LogState::SPtr              recLogState;

            status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("ExtendedRecoveryTests: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
            {
                KDbgPrintf("ExtendedRecoveryTests: IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Prove each stream is recovered correctly
        for (ULONG ix = 0; ix < numberOfStreams + 1; ix++)
        {
            if (ids[ix] == RvdDiskLogConstants::CheckpointStreamId())
            {
                continue;
            }

            status = RecoverySupport::ValidateStreamCorrectness(log, ids[ix], logState, Allocator);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("ExtendedRecoveryTests: ValidateStreamCorrectness() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        // Close log
        KNt::Sleep(250);
        KInvariant(log.Reset());

        deleteLogOp->StartDeleteLog(
            driveGuid,
            logId,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::StartDeleteLog failed: 0x%08X: Line: %i\n", status, __LINE__);
            return status;
        }

        deleteLogOp->Reuse();
    }

    logManager->Deactivate();
    status = deactivateCompEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::Deactivate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}


//* Test that generates multi-segment stream checkpoint records and proves their recovery
NTSTATUS
LogLargeCheckpointRecoveryTest(
    WCHAR OnDrive,
    KAllocator& Allocator)
{
    static GUID const   testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const   testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    KWString            fullyQualifiedDiskFileName(Allocator);
    KGuid               driveGuid;
    RvdLogId            logId(testLogIdGuid);
    RvdLogStreamType    testStreamType(testStreamTypeGuid);
    NTSTATUS            status;
    LONGLONG            logSize = 1024 * 1024 * 1024;
    Synchronizer        deactivateCompEvent;

    status = CleanAndPrepLog(OnDrive);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: CleanAndPrepLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: GetDriveAndPathOfLogFile failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::Create failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->Activate(nullptr, deactivateCompEvent.AsyncCompletionCallback());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::Activate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifier);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf(
            "LogLargeCheckpointRecoveryTest: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
            status,
            __LINE__);

        return status;
    }

    RvdLogManager::AsyncCreateLog::SPtr     createLogOp;
    RvdLogManager::AsyncOpenLog::SPtr       openLogOp;
    RvdLogManager::AsyncDeleteLog::SPtr     deleteLogOp;
    Synchronizer                            compEvent;

    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::CreateAsyncOpenLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::CreateAsyncDeleteLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    {   //** Prove multi-segment CP write and recovery
        RvdLog::SPtr            log;
        LogState::SPtr          logState;
        RvdLogAsn               lowestAsn;
        RvdLogAsn               highestAsn;
        GUID const              testStreamIdGuid = {1, 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};
        RvdLogStreamId          testStreamId(testStreamIdGuid);

        {
            KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
            KInvariant(NT_SUCCESS(logType.Status()));

            createLogOp->StartCreateLog(
                driveGuid,
                logId,
                logType,
                logSize,
                log,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            createLogOp->Reuse();

            RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
            status = log->CreateAsyncCreateLogStreamContext(createStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: CreateAsyncCreateLogStreamContext failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            RvdLogStream::SPtr          stream;

            createStreamOp->StartCreateLogStream(
                testStreamId,
                RvdLogStreamType(testStreamTypeGuid),
                stream,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: StartCreateLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            createStreamOp->Reuse();

            //* Write user records until we detect a multi-segment CP record has been written
            RvdLogAsn                               testAsn(1000);
            ULONGLONG                               testVersion = 13;
            KBuffer::SPtr                           testRecMetadata;
            KIoBuffer::SPtr                         testRecIoBuffer;
            RvdLogStream::AsyncWriteContext::SPtr   writeOp;

            status = RecoverySupport::CreateTestRecord(
                Allocator,
                testAsn,
                testVersion,
                0,
                testRecMetadata,
                testRecIoBuffer);

            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogLargeCheckpointRecoveryTest: CreateTestRecord failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            status = stream->CreateAsyncWriteContext(writeOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogLargeCheckpointRecoveryTest: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            RvdLogStreamImp::AsyncWriteStream::SPtr writeOpImp = (RvdLogStreamImp::AsyncWriteStream*)writeOp.RawPtr();
#pragma warning(disable:4127)   // C4127: conditional expression is constant
            while (TRUE)
            {
                writeOp->StartWrite(
                    testAsn,
                    testVersion,
                    testRecMetadata,
                    testRecIoBuffer,
                    nullptr,
                    compEvent.AsyncCompletionCallback());

                status = compEvent.WaitForCompletion();
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "LogLargeCheckpointRecoveryTest: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                if (writeOpImp->_WroteMultiSegmentStreamCheckpoint)
                {
                    break;
                }

                writeOp->Reuse();

                testAsn = testAsn.Get() + 1;
            }

            // Prove we can still write record past the multi-seg CP
            testAsn = testAsn.Get() + 1;
            writeOp->Reuse();

            writeOp->StartWrite(
                testAsn,
                testVersion,
                testRecMetadata,
                testRecIoBuffer,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "LogLargeCheckpointRecoveryTest: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            status = stream->QueryRecordRange(&lowestAsn, &highestAsn, nullptr);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: QueryRecordRange() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            // Snap state of the reference (ref) log just created w/multi-seg CP record
            status = RecoverySupport::GetLogsState(log, Allocator, logState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        KNt::Sleep(250);
        KInvariant(log.Reset());

        {
            // Prove proper recovery of the log and the stream
            openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openLogOp->Reuse();

            LogState::SPtr              recLogState;

            status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            status = RecoverySupport::ValidateStreamCorrectness(log, testStreamId, logState, Allocator);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogLargeCheckpointRecoveryTest: ValidateStreamCorrectness() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
        }

        KNt::Sleep(250);
        KInvariant(log.Reset());
    }

    deleteLogOp->StartDeleteLog(
        driveGuid,
        logId,
        nullptr,
        compEvent.AsyncCompletionCallback());

    status = compEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::StartDeleteLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    deleteLogOp->Reuse();

    logManager->Deactivate();
    status = deactivateCompEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogLargeCheckpointRecoveryTest: RvdLogManager::Deactivate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}



//* Log chaos region power/hardware fault simulation tests. This test-set validates basic conditions
//  where the RvdDiskLogConstants::MaxQueuedWriteDepth space at the tail-end of the log can have had
//  partial and out-of-order state laid down on the disk due to hardware or power fault sorts of
//  conditions. The log runtime should never allow more than RvdDiskLogConstants::MaxQueuedWriteDepth
//  write to be outstanding and thus the recovery can assume no more than this amount of space at
//  the tail of the log can be checker-boarded (i.e. in chaos).
//
// Test 1: Prove partial user records in the middle of the chaos region are not recovered
//      Test 1a: Partial or missing Header/Metadata with at least one valid record after faulted record
//      Test 1b: Partial or missing IoBuffer region with at least one valid record after faulted record
//      Test 1c: Partial or missing Header/Metadata as last record in log
//      Test 1d: Partial or missing IoBuffer region as last record in log
// Test 2: Prove that a "lost" physical check point in the middle of the chaos region is not recovered.
//      Test 2a: Partial or missing CP with at least one valid record after faulted record
//      Test 2b: Partial or missing CP as last record in the log
//
NTSTATUS
DoFailureInjectedRecoveryTest(
    WCHAR OnDrive,
    KAllocator& Allocator)
{
    static GUID const           testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const           testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    static GUID const           testStreamIdGuid = {1 , 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};
    RvdLogId                    logId(testLogIdGuid);
    RvdLogStreamType            testStreamType(testStreamTypeGuid);
    RvdLogStreamId              testStreamId(testStreamIdGuid);

    KWString                    fullyQualifiedDiskFileName(Allocator);
    KGuid                       driveGuid;
    NTSTATUS                    status;
    LONGLONG const              logSize = 1024 * 1024 * 1024;
    Synchronizer                deactivateCompEvent;

    status = CleanAndPrepLog(OnDrive);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DoFailureInjectedRecoveryTest: CleanAndPrepLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DoFailureInjectedRecoveryTest: GetDriveAndPathOfLogFile failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::Create failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->Activate(nullptr, deactivateCompEvent.AsyncCompletionCallback());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::Activate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifier);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf(
            "DoFailureInjectedRecoveryTest: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
            status,
            __LINE__);

        return status;
    }

    //* Local support functions
    struct
    {
        static NTSTATUS
        CreateLogAndStream(
            __in RvdLogManager::SPtr LogManager,
            __in KGuid DriveGuid,
            __in RvdLogId LogId,
            __in ULONGLONG LogSize,
            __out RvdLog::SPtr& Log,
            __out RvdLogStream::SPtr& Stream,
            __out ULONGLONG& AvailableLogSpace)
        {
            Synchronizer                            compEvent;
            RvdLogManager::AsyncCreateLog::SPtr     createLogOp;

            NTSTATUS status = LogManager->CreateAsyncCreateLogContext(createLogOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::CreateAsyncCreateLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
            KInvariant(NT_SUCCESS(logType.Status()));
    
            createLogOp->StartCreateLog(
                DriveGuid,
                LogId,
                logType,
                LogSize,
                Log,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::StartCreateLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
            status = Log->CreateAsyncCreateLogStreamContext(createStreamOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: CreateAsyncCreateLogStreamContext failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;

            AvailableLogSpace = ((RvdLogManagerImp::RvdOnDiskLog*)(Log.RawPtr()))->RemainingLsnSpaceToEOF();

            createStreamOp->StartCreateLogStream(
                RvdLogStreamId(testStreamIdGuid),
                RvdLogStreamType(testStreamTypeGuid),
                Stream,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: StartCreateLogStream failed: 0x%08X: Line: %i\n", status, __LINE__);
            }

            return status;
        }

        static NTSTATUS
        WriteTestRecord(
            RvdLogStream::SPtr Stream,
            RvdLog::SPtr Log,
            KAllocator& Allocator,
            RvdLogAsn Asn)
        {
            RvdLogStream::AsyncWriteContext::SPtr   writeOp;
            Synchronizer                            compEvent;
//            RvdLogManagerImp::RvdOnDiskLog*         logImpPtr =
//            (RvdLogManagerImp::RvdOnDiskLog*)(Log.RawPtr()); // delme

            NTSTATUS status = Stream->CreateAsyncWriteContext(writeOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "DoFailureInjectedRecoveryTest: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            ULONGLONG       testVersion = 13;
            KBuffer::SPtr   testRecMetadata;
            KIoBuffer::SPtr testRecIoBuffer;
            ULONGLONG       ioBufferSize = RandomGenerator::Get(10) * RvdDiskLogConstants::BlockSize;

            status = RecoverySupport::CreateTestRecord(
                Allocator,
                Asn,
                testVersion,
                (ULONG)(ioBufferSize / RecoverySupport::SizeOfTestRecordIoBufferElement),
                testRecMetadata,
                testRecIoBuffer);

            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "DoFailureInjectedRecoveryTest: CreateTestRecord failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            writeOp->StartWrite(
                Asn,
                testVersion,
                testRecMetadata,
                testRecIoBuffer,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
            }

            return status;
        }

        static NTSTATUS
        FillAndWrapLog(
            RvdLogStream::SPtr Stream,
            RvdLog::SPtr Log,
            ULONGLONG MaxLogFreeSpace,
            KAllocator& Allocator)
        {
            RvdLogStream::AsyncWriteContext::SPtr   writeOp;
            Synchronizer                            compEvent;
            RvdLogManagerImp::RvdOnDiskLog*         logImpPtr = (RvdLogManagerImp::RvdOnDiskLog*)(Log.RawPtr());

            NTSTATUS status = Stream->CreateAsyncWriteContext(writeOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "DoFailureInjectedRecoveryTest: CreateAsyncWriteContext failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            // Next, fill the log with random record size (0 - 10 blks) for the various streams
            ULONGLONG       asnOffset = 1000;
            LONGLONG        writeUntilLsn = (LONGLONG)MaxLogFreeSpace + (MaxLogFreeSpace / 2);

#pragma warning(disable:4127)   // C4127: conditional expression is constant
            while (TRUE)
            {
                LONGLONG        lowLsn;
                LONGLONG        highLsn;
                logImpPtr->GetPhysicalExtent(lowLsn, highLsn);

                if (highLsn >= writeUntilLsn)
                {
                    break;
                }

                RvdLogAsn       testAsn(asnOffset);
                ULONGLONG       testVersion = 13;
                KBuffer::SPtr   testRecMetadata;
                KIoBuffer::SPtr testRecIoBuffer;
                ULONGLONG       ioBufferSize = RandomGenerator::Get(10) * RvdDiskLogConstants::BlockSize;

                status = RecoverySupport::CreateTestRecord(
                    Allocator,
                    testAsn,
                    testVersion,
                    (ULONG)(ioBufferSize / RecoverySupport::SizeOfTestRecordIoBufferElement),
                    testRecMetadata,
                    testRecIoBuffer);

                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "DoFailureInjectedRecoveryTest: CreateTestRecord failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

#pragma warning(disable:4127)   // C4127: conditional expression is constant
                while (TRUE)
                {
                    writeOp->StartWrite(
                        testAsn,
                        testVersion,
                        testRecMetadata,
                        testRecIoBuffer,
                        nullptr,
                        compEvent.AsyncCompletionCallback());

                    status = compEvent.WaitForCompletion();
                    writeOp->Reuse();

                    BOOLEAN logIsFull = (status == STATUS_LOG_FULL);
                    if (!NT_SUCCESS(status) && !logIsFull)
                    {
                        KDbgPrintf("DoFailureInjectedRecoveryTest: StartWrite failed: 0x%08X: Line: %i\n", status, __LINE__);
                        return status;
                    }

                    if (logIsFull)
                    {
                        // Truncate 10% of the reconds written so far
                        RvdLogAsn       lowestAsn;
                        RvdLogAsn       highestAsn;
                        KInvariant(NT_SUCCESS(Stream->QueryRecordRange(&lowestAsn, &highestAsn, nullptr)));

                        RvdLogAsn       newTruncPoint = ((highestAsn.Get() - lowestAsn.Get()) / 10) + lowestAsn.Get();
                        Stream->Truncate(newTruncPoint, newTruncPoint);

                        while (!Log->IsLogFlushed())
                        {
                            KNt::Sleep(10);
                        }

                        continue;
                    }

                    break;
                }

                asnOffset++;
            }

            return STATUS_SUCCESS;
        }

        static NTSTATUS
        DoInjectorTest(
            KAllocator& Allocator,
            RvdLogManager::SPtr LogManager,
            KGuid& DriveGuid,
            RvdLogId& LogId,
            RvdLogStreamId& StreamId,
            ULONGLONG LogSize,
            RvdLogAsn& TestAsn,
            KInjectorBlockDevice::InjectionConstraint InjectionConstraint,
            BOOLEAN DoFinalWrite)
        {
            RandomGenerator::SetSeed(5);

            RvdLog::SPtr                            log;
            RvdLogManagerImp::RvdOnDiskLog::SPtr    logImp;
            LogState::SPtr                          logState;
            ULONGLONG                               maxLogFreeSpace;
            RvdLogStream::SPtr                      stream;
            RvdLogManager::AsyncOpenLog::SPtr       openLogOp;
            RvdLogManager::AsyncDeleteLog::SPtr     deleteLogOp;
            Synchronizer                            compEvent;

            NTSTATUS status = LogManager->CreateAsyncDeleteLogContext(deleteLogOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::CreateAsyncDeleteLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            status = LogManager->CreateAsyncOpenLogContext(openLogOp);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::CreateAsyncOpenLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            status = CreateLogAndStream(LogManager, DriveGuid, LogId, LogSize, log, stream, maxLogFreeSpace);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: CreateLogAndStream failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            logImp = log.DownCast<RvdLogManagerImp::RvdOnDiskLog>();

            // Next, fill the log with random record size (0 - 10 blks)
            status = FillAndWrapLog(stream, log, maxLogFreeSpace, Allocator);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf(
                    "DoFailureInjectedRecoveryTest: FillAndWrapLog failed: 0x%08X: Line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            // Truncate log until there's at least RvdDiskLogConstants::CheckpointInterval space available
            ULONGLONG       totalSpace;
            ULONGLONG       freeSpace;
            log->QuerySpaceInformation(&totalSpace, &freeSpace);
            while (freeSpace < (logImp->GetConfig().GetCheckpointInterval() * 2))
            {
                RvdLogAsn       lowestAsn;

                stream->QueryRecordRange(&lowestAsn, nullptr, nullptr);

                do
                {
                    ULONGLONG                       ver;
                    RvdLogStream::RecordDisposition disp;
                    ULONG                           size;

                    KInvariant(NT_SUCCESS(stream->QueryRecord(lowestAsn, &ver, &disp, &size)));

                    freeSpace += size;
                    lowestAsn = lowestAsn.Get() + 1;
                } while(freeSpace < (logImp->GetConfig().GetCheckpointInterval() * 2));

                stream->Truncate(lowestAsn, lowestAsn);
                while (!log->IsLogFlushed())
                {
                    KNt::Sleep(10);
                }

                log->QuerySpaceInformation(&totalSpace, &freeSpace);
            }

            // Inject FaultInjector
            KInjectorBlockDevice::SPtr  faultInjector = _new(KTL_TAG_TEST, Allocator) KInjectorBlockDevice(
               ((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->GetBlockDevice(),
                log);

            ((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->SetBlockDevice(*faultInjector);

            faultInjector->SetInjectionConstraint(InjectionConstraint);
            faultInjector->SetWriteIntercept(TRUE);

            // Cause all further writes to appear to be in parallel
            ((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->_DebugDisableHighestCompletedLsnUpdates = TRUE;

            //((RvdLogManagerImp::RvdOnDiskLog*)(log.RawPtr()))->_DebugDisableHighestCheckpointLsnUpdates =
            //    (InjectionConstraint == KInjectorBlockDevice::InjectionConstraint::CheckpointOnly);

            // Write records until interception is done
            while (faultInjector->GetWriteIntercept())
            {
                status = WriteTestRecord(stream, log, Allocator, TestAsn);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "DoFailureInjectedRecoveryTest: WriteTestRecord failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }

                TestAsn = TestAsn.Get() + 1;
            }

            if (DoFinalWrite)
            {
                // Write another record past the one that's had a fault injected.
                status = WriteTestRecord(stream, log, Allocator, TestAsn);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf(
                        "DoFailureInjectedRecoveryTest: WriteTestRecord failed: 0x%08X: Line: %i\n",
                        status,
                        __LINE__);

                    return status;
                }
            }

            // Snap state of the reference (ref) log just created
            status = RecoverySupport::GetLogsState(log, Allocator, logState);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            logImp.Reset();
            stream.Reset();
            KNt::Sleep(250);
            KInvariant(log.Reset());            // Close the log

            // Fixup logState with the results in faultInjector
            KArray<KInjectorBlockDevice::FaultedRecordHeader::SPtr>&    faultedRecords = faultInjector->GetFaultedRecords();
            KArray<KInjectorBlockDevice::FaultedRegionDescriptor>&      faultedRegions = faultInjector->GetFaultedRegions();

            KInvariant(faultedRecords.Count() == 1);
            KInvariant(faultedRegions.Count() == 1);

            // Reverse truncate the log to NOT include the record and lsn space at and beyond the injected fault location
            KInvariant(NT_SUCCESS(RecoverySupport::ReverseTruncateLogState(logState, faultedRecords[0]->GetLogRecordHeader())));

            faultInjector.Reset();

            // Prove proper recovery of the log and each stream after log full
            openLogOp->StartOpenLog(DriveGuid, LogId, log, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            openLogOp->Reuse();

            // Prove physical recovered log state is at least a proper super set of logState
            {
                LogState::SPtr              recLogState;

                status = RecoverySupport::GetLogsState(log, Allocator, recLogState);
                if (!NT_SUCCESS(status))
                {
                    KDbgPrintf("DoFailureInjectedRecoveryTest: GetLogsState() failed: 0x%08X: Line: %i\n", status, __LINE__);
                    return status;
                }

                if (!RecoverySupport::IsPhysicalRecoveryCorrect(logState, recLogState))
                {
                    KDbgPrintf("DoFailureInjectedRecoveryTest:IsPhysicalRecoveryCorrect failed: Line: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Prove stream is recovered correctly
            status = RecoverySupport::ValidateStreamCorrectness(log, StreamId, logState, Allocator);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("DoFailureInjectedRecoveryTest: ValidateStreamCorrectness() failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }

            KNt::Sleep(250);
            KInvariant(log.Reset());            // Close the log

            deleteLogOp->StartDeleteLog(DriveGuid, LogId, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
                return status;
            }
            deleteLogOp->Reuse();

            return STATUS_SUCCESS;
        }

    } local;

    // Make /W4 happy:
    local;
    
    //** Test1: Prove partial user records in the middle of the chaos region are not recovered
    RvdLogAsn       testAsn(1000000);

    //* Test 1a: Partial or missing Header/Metadata with at least one valid record after faulted record
    //           Cause the record next to the last record (which is good) to have a bad header
    status = local.DoInjectorTest(
        Allocator,
        logManager,
        driveGuid,
        logId,
        testStreamId,
        logSize,
        testAsn,
        KInjectorBlockDevice::InjectionConstraint::UserRecordHeaderOnly,
        TRUE);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //* Test 1c: Partial or missing Header/Metadata as last record in log
    //           Cause the the last record to have a bad header
    status = local.DoInjectorTest(
        Allocator,
        logManager,
        driveGuid,
        logId,
        testStreamId,
        logSize,
        testAsn,
        KInjectorBlockDevice::InjectionConstraint::UserRecordHeaderOnly,
        FALSE);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //* Test 1b: Partial or missing IoBuffer region with at least one valid record after faulted record
    //           Cause the record next to the last record (which is good) to have a fault in its page-aligned region
    status = local.DoInjectorTest(
        Allocator,
        logManager,
        driveGuid,
        logId,
        testStreamId,
        logSize,
        testAsn,
        KInjectorBlockDevice::InjectionConstraint::UserRecordIoBufferDataOnly,
        TRUE);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //* Test 1d: Partial or missing IoBuffer region as last record in log
    //           Cause the last record to have a fault in its page-aligned region
    status = local.DoInjectorTest(
        Allocator,
        logManager,
        driveGuid,
        logId,
        testStreamId,
        logSize,
        testAsn,
        KInjectorBlockDevice::InjectionConstraint::UserRecordIoBufferDataOnly,
        FALSE);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //** Test2: Prove that a "lost" physical check point in the chaos region is not recovered

    //* Test 2a: Partial or missing CP with at least one valid record after faulted record
    //           Cause the last checkpoint (physical) to have a fault but with a valid record written after it
    status = local.DoInjectorTest(
        Allocator,
        logManager,
        driveGuid,
        logId,
        testStreamId,
        logSize,
        testAsn,
        KInjectorBlockDevice::InjectionConstraint::CheckpointOnly,
        TRUE);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    //* Test 2b: Partial or missing CP as last record in the log
    //           Cause the last checkpoint (physical) to have a fault
    status = local.DoInjectorTest(
        Allocator,
        logManager,
        driveGuid,
        logId,
        testStreamId,
        logSize,
        testAsn,
        KInjectorBlockDevice::InjectionConstraint::CheckpointOnly,
        FALSE);

    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: DoInjectorTest(UserRecordHeaderOnly) failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    logManager->Deactivate();
    status = deactivateCompEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogFullRecoveryTests: RvdLogManager::Deactivate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    return STATUS_SUCCESS;
}


//* Temp recovery
NTSTATUS
DoRecovery(
    WCHAR OnDrive,
    KAllocator& Allocator)
{
//    ULONG const         MaxRecordIoBlks = 50;   // Delme
//    LONGLONG const      LogSize = 1024 * 1024 * 1024; // delme

    static GUID const   testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const   testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    KWString            fullyQualifiedDiskFileName(Allocator);
    KGuid               driveGuid;
    RvdLogId            logId(testLogIdGuid);
    RvdLogStreamType    testStreamType(testStreamTypeGuid);
    NTSTATUS            status;
    Synchronizer        deactivateCompEvent;

    status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: GetDriveAndPathOfLogFile failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::Create failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->Activate(nullptr, deactivateCompEvent.AsyncCompletionCallback());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::Activate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    status = logManager->RegisterVerificationCallback(testStreamType, RecoverySupport::RecordVerifier);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf(
            "ExtendedRecoveryTests: RvdLogManager::RegisterVerificationCallback failed: 0x%08X: Line: %i\n",
            status,
            __LINE__);

        return status;
    }

    RvdLogManager::AsyncOpenLog::SPtr       openLogOp;
    Synchronizer                            compEvent;

    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::CreateAsyncOpenLogContext failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    RvdLog::SPtr            log;

    openLogOp->StartOpenLog(driveGuid, logId, log, nullptr, compEvent.AsyncCompletionCallback());
    status = compEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::StartOpenLog failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }
    openLogOp.Reset();

    KNt::Sleep(250);
    KInvariant(log.Reset());

    logManager->Deactivate();
    status = deactivateCompEvent.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ExtendedRecoveryTests: RvdLogManager::Deactivate failed: 0x%08X: Line: %i\n", status, __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}


//** Main Test Entry Point: RvdLoggerRecoveryTests
NTSTATUS
RvdLoggerRecoveryTests(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    KDbgPrintf("RvdLoggerRecoveryTests: Starting\n");

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([]()
    {
        EventUnregisterMicrosoft_Windows_KTL();
    });

    KtlSystem* underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(FALSE,
                                           &underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerRecoveryTests: KtlSystem::Initialize Failed: 0x%08X: Line %i\n", status, __LINE__);
        return status;
    }

    KFinally([]()
    {
        KtlSystem::Shutdown();
    });

    // Syntax: <drive> [OverallDurationInSecs [MaxRunDurationInSecs [MinRunDuration]]]
    if (argc < 1)
    {
        KDbgPrintf("RvdLoggerRecoveryTests: Drive Letter and/or Duration Test Parameter(s) Missing: %i\n", __LINE__);
        return STATUS_INVALID_PARAMETER_2;
    }

    //** Temp
    //return DoRecovery(*args[0], KtlSystem::GlobalNonPagedAllocator());

    ULONG           overallDurationInSecs = 60;
    if (argc > 1)
    {
        KWString    t(KtlSystem::GlobalNonPagedAllocator(), args[1]);
        if (!ToULong(t, overallDurationInSecs))
        {
            KDbgPrintf("LogStreamAsyncIoTests: OverallDurationInSecs Test Parameter Invalid: %i\n", __LINE__);
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    ULONG           maxRunDurationInSecs = (overallDurationInSecs * 2) + 1;       // Default to one pass
    if (argc > 2)
    {
        KWString    t(KtlSystem::GlobalNonPagedAllocator(), args[2]);
        if (!ToULong(t, maxRunDurationInSecs))
        {
            KDbgPrintf("LogStreamAsyncIoTests: MaxRunDurationInSecs Test Parameter Invalid: %i\n", __LINE__);
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    ULONG           minRunDurationInSecs = MAXULONG;
    if (argc > 3)
    {
        KWString    t(KtlSystem::GlobalNonPagedAllocator(), args[3]);
        if (!ToULong(t, minRunDurationInSecs))
        {
            KDbgPrintf("LogStreamAsyncIoTests: MinRunDurationInSecs Test Parameter Invalid: %i\n", __LINE__);
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    if (minRunDurationInSecs > maxRunDurationInSecs)
    {
        minRunDurationInSecs = maxRunDurationInSecs / 2;
    }

    LONGLONG baselineAllocations = KAllocatorSupport::gs_AllocsRemaining;

    KDbgPrintf("RvdLoggerRecoveryTests: DoFailureInjectedRecoveryTest Starting\n");
    status = DoFailureInjectedRecoveryTest(*args[0], KtlSystem::GlobalNonPagedAllocator());
    KDbgPrintf("RvdLoggerRecoveryTests: DoFailureInjectedRecoveryTest Completed\n");
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerRecoveryTests: DoFailureInjectedRecoveryTest failed: 0x%08X: Line %i\n", status, __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KNt::Sleep(250);       // Allow async object releases
    if (KAllocatorSupport::gs_AllocsRemaining != baselineAllocations)
    {
        KDbgPrintf("RvdLoggerRecoveryTests: DoFailureInjectedRecoveryTest Memory Leak Detected: %i\n", __LINE__);
        KtlSystemBase::DebugOutputKtlSystemBases();
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    KDbgPrintf("RvdLoggerRecoveryTests: BasicLogRecoveryTests Starting\n");
    status = BasicLogRecoveryTests(*args[0], KtlSystem::GlobalNonPagedAllocator());
    KDbgPrintf("RvdLoggerRecoveryTests: BasicLogRecoveryTests Completed\n");
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerRecoveryTests: BasicLogRecoveryTests failed: 0x%08X: Line %i\n", status, __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KNt::Sleep(250);       // Allow async object releases
    if (KAllocatorSupport::gs_AllocsRemaining != baselineAllocations)
    {
        KDbgPrintf("RvdLoggerRecoveryTests: BasicLogRecoveryTests Memory Leak Detected: %i\n", __LINE__);
        KtlSystemBase::DebugOutputKtlSystemBases();
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    KDbgPrintf("RvdLoggerRecoveryTests: LogFullRecoveryTests Starting\n");
    status = LogFullRecoveryTests(*args[0], KtlSystem::GlobalNonPagedAllocator());
    KDbgPrintf("RvdLoggerRecoveryTests: LogFullRecoveryTests Completed\n");
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerRecoveryTests: LogFullLogRecoveryTests failed: 0x%08X: Line %i\n", status, __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KNt::Sleep(250);       // Allow async object releases
    if (KAllocatorSupport::gs_AllocsRemaining != baselineAllocations)
    {
        KDbgPrintf("RvdLoggerRecoveryTests: LogFullLogRecoveryTests Memory Leak Detected: %i\n", __LINE__);
        KtlSystemBase::DebugOutputKtlSystemBases();
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    KDbgPrintf("RvdLoggerRecoveryTests: LogLargeCheckpointRecoveryTest Starting\n");
    status = LogLargeCheckpointRecoveryTest(*args[0], KtlSystem::GlobalNonPagedAllocator());
    KDbgPrintf("RvdLoggerRecoveryTests: LogLargeCheckpointRecoveryTest Completed\n");
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerRecoveryTests: LogLargeCheckpointRecoveryTest failed: 0x%08X: Line %i\n", status, __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KNt::Sleep(250);       // Allow async object releases
    if (KAllocatorSupport::gs_AllocsRemaining != baselineAllocations)
    {
        KDbgPrintf("RvdLoggerRecoveryTests: LogFullLogRecoveryTests Memory Leak Detected: %i\n", __LINE__);
        KtlSystemBase::DebugOutputKtlSystemBases();
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    ULONG           passSeed = 0;
    ULONGLONG       stopTime = KNt::GetTickCount64() + (overallDurationInSecs * 1000);
    ULONGLONG       currentTime;

    while ((currentTime = KNt::GetTickCount64()) < stopTime)
    {
        ULONG       runDurationInSecs = RandomGenerator::Get(maxRunDurationInSecs, minRunDurationInSecs);
        ULONGLONG   timeRemainingInSecs = (stopTime - currentTime) / 1000;

        if (runDurationInSecs > timeRemainingInSecs)
        {
            // Limit last run to something close to the remaining test time
            runDurationInSecs = (ULONG)(timeRemainingInSecs + 1);
        }

        KDbgPrintf(
            "RvdLoggerRecoveryTests: ExtendedRecoveryTests Starting: Pass %u; Approx pass duration: %u secs; %I64u secs left in test.\n",
            passSeed + 1,
            runDurationInSecs,
            timeRemainingInSecs);

        status = ExtendedRecoveryTests(*args[0], KtlSystem::GlobalNonPagedAllocator(), runDurationInSecs, passSeed);
        KDbgPrintf("RvdLoggerRecoveryTests: ExtendedRecoveryTests Completed: Pass %u\n", passSeed + 1);

        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLoggerRecoveryTests: ExtendedRecoveryTests failed: 0x%08X: Line %i\n", status, __LINE__);
			KInvariant(FALSE);
            return status;
        }

        KNt::Sleep(250);       // Allow async object releases
        if (KAllocatorSupport::gs_AllocsRemaining != baselineAllocations)
        {
            KDbgPrintf("RvdLoggerRecoveryTests: ExtendedRecoveryTests Memory Leak Detected: %i\n", __LINE__);
            KtlSystemBase::DebugOutputKtlSystemBases();
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        passSeed++;
    }

    KDbgPrintf("RvdLoggerRecoveryTests: Completed");
    return status;
}
