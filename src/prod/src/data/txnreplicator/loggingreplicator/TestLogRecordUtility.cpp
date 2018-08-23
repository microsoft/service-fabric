// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Common;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

TestLogRecordUtility::TestLogRecordUtility()
    : LogRecord()
    , binaryWriter_(GetThisAllocator())
    , invalidRecords_(InvalidLogRecords::Create(GetThisAllocator()))
{
}

TestLogRecordUtility::~TestLogRecordUtility()
{
}

TestLogRecordUtility::SPtr TestLogRecordUtility::Create(KAllocator & allocator)
{
    TestLogRecordUtility * pointer = _new(TEST_LOGRECORDS, allocator) TestLogRecordUtility();

    return TestLogRecordUtility::SPtr(pointer);
}

void TestLogRecordUtility::Reset()
{
    binaryWriter_.Reset();
}

OperationData::CSPtr TestLogRecordUtility::WriteRecord(
    __in LogRecord & base, 
    __in bool isPhysicalWrite)
{
    CODING_ERROR_ASSERT(binaryWriter_.Position == 0);

    OperationData::CSPtr result = LogRecord::WriteRecord(base, binaryWriter_, GetThisAllocator(), isPhysicalWrite);

    return result;
}

LogRecord::SPtr TestLogRecordUtility::ReadRecordFromOperationData(
    __in OperationData::CSPtr operationData,
    __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords)
{
    // Create operation data to be read by removing first and last entries
    OperationData::SPtr toReadOperationData = OperationData::Create(GetThisAllocator());

    for(ULONG i = 1; i < operationData-> BufferCount - 1; i++)
    {
        toReadOperationData->Append(*(*operationData)[i]);
    }

    return LogRecord::ReadFromOperationData(*toReadOperationData, invalidLogRecords, GetThisAllocator());
}

LogRecord::SPtr TestLogRecordUtility::ReadRecord(
    __in OperationData::CSPtr writtenData, 
    __in bool isPhysicalRead)
{
    // Create KBuffer from OperationData returned by LogRecord::WriteRecord
    KBuffer::SPtr createdBuffer = CreateBufferFromRecordOperationData(writtenData);

    // Create BinaryReader from KBuffer
    BinaryReader binaryReader(*createdBuffer, GetThisAllocator());

    return LogRecord::ReadRecord(binaryReader, binaryReader.Position, *invalidRecords_, isPhysicalRead, GetThisAllocator());
}

int TestLogRecordUtility::PopulateRandomOperationData(
    __inout OperationData::SPtr operationData,
    __in int seed,
    __in KAllocator & allocator,
    __in DOUBLE probabilityOfZeroLengthBuffer,
    __in ULONG maxBufferSize,
    __in ULONG minBufferSize,
    __in ULONG maxBuffers,
    __in ULONG minBuffers)
{
    // Probability can never be greater than one
    CODING_ERROR_ASSERT(probabilityOfZeroLengthBuffer < 1.0);
    Random rand(seed);

    int bufferSize = (rand.Next() % maxBufferSize) + minBufferSize;
    int numBuffers = (rand.Next() % maxBuffers) + minBuffers;

    unsigned char * bufferCharPointer;
    KBuffer::SPtr buffer;
    NTSTATUS bufferStatus;

    int numZeroBuffers = 0;

    for (auto i = 0; i < numBuffers; i++)
    {
        if (probabilityOfZeroLengthBuffer != -1 && rand.Next(0, 100) * 0.01 > probabilityOfZeroLengthBuffer)
        {
            bufferSize = 0;
            numZeroBuffers++;
        }

        bufferStatus = KBuffer::Create(
            bufferSize,
            buffer,
            allocator);

        CODING_ERROR_ASSERT(bufferStatus == STATUS_SUCCESS);

        // Populate buffer
        bufferCharPointer = static_cast<unsigned char *>(buffer->GetBuffer());
        for (auto j = 0; j < bufferSize; j++)
        {
            bufferCharPointer[j] = rand.NextByte();
        }

        operationData->Append(*buffer);
        bufferSize = (rand.Next() % maxBufferSize) + minBufferSize;;
    }

    return numZeroBuffers;
}

KArray<ProgressVectorEntry> TestLogRecordUtility::ProgressVectorEntryArray(
    __in KAllocator & allocator,
    __in ULONG numEntries, 
    __in bool useInvalidEpoch, 
    __in ULONG startingLsn)
{
    KArray<ProgressVectorEntry> progressVectorEntries(allocator);
    Epoch epoch;

    for (LONG64 i = 0; i < numEntries; i++)
    {
        if (useInvalidEpoch)
        {
            epoch = Epoch::InvalidEpoch();
        }
        else
        {
            epoch = Epoch(i, i);
        }

        progressVectorEntries.Append(ProgressVectorEntry(epoch, startingLsn + i, startingLsn + i, DateTime::Now()));
    }

    return progressVectorEntries;
}

void TestLogRecordUtility::PopulateProgressVectorFromEntries(
    __in ProgressVector & newVector, 
    __in KArray<ProgressVectorEntry> entries)
{
    int numEntries = entries.Count();

    for (int i = 0; i < numEntries; i++)
    {
        newVector.Append(entries[i]);
    }
}

KBuffer::SPtr TestLogRecordUtility::CreateBufferFromOperationData(
    __in OperationData::CSPtr operationData,
    __in KAllocator & allocator,
    __in ULONG startIndex,
    __in ULONG lastIndex)
{
    if(lastIndex == 0)
    {
        lastIndex = operationData->BufferCount;
    }

    CODING_ERROR_ASSERT(lastIndex <= operationData->BufferCount);

    KBuffer::SPtr targetBuffer;
    ULONG offset = 0;
    ULONG currentSize;
    ULONG totalSerializedBufferSize = 0;
    NTSTATUS bufferCreateStatus;

    for (ULONG i = 0; i < operationData->BufferCount; i++)
    {
        totalSerializedBufferSize += (*operationData)[i]->QuerySize();
    }

    bufferCreateStatus = KBuffer::Create(totalSerializedBufferSize, targetBuffer, allocator);
    CODING_ERROR_ASSERT(bufferCreateStatus == STATUS_SUCCESS);

    for (ULONG i = startIndex; i < lastIndex; i++)
    {
        currentSize = (*operationData)[i]->QuerySize();

        (*targetBuffer).CopyFrom(offset, *(*operationData)[i], 0, currentSize);

        offset += currentSize;
    }

    return targetBuffer;
}

BeginCheckpointLogRecord::SPtr LoggingReplicatorTests::TestLogRecordUtility::CreateBeginCheckpointLogRecord(
    __in InvalidLogRecords::SPtr invalidLogRecords,
    __in KAllocator & allocator,
    __in bool nullPreviousPhysicalRecord,
    __in bool nullLinkedPhysicalRecord,
    __in bool invalidRecordPosition)
{
    // Create earliest pending tx record
    BeginTransactionOperationLogRecord::SPtr beginTxRecord = BeginTransactionOperationLogRecord::Create(
        LogRecordType::Enum::BeginTransaction,
        100,
        1,
        *invalidLogRecords->Inv_PhysicalLogRecord,
        *invalidLogRecords->Inv_TransactionLogRecord,
        allocator);

    // Data loss version 1, configuration version 0
    beginTxRecord->RecordEpoch = Epoch(2, 0);

    // Create last backup record
    KGuid guid;
    guid.CreateNew();

    LONG64 highestBackedUpLsn = 50;
    UINT backupLogRecordCount = 4;
    UINT backupLogSize = 2000;

    BackupLogRecord::SPtr backupLogRecord = BackupLogRecord::Create(
        guid,
        Epoch(3, 0),
        highestBackedUpLsn,
        backupLogRecordCount,
        backupLogSize,
        *invalidLogRecords->Inv_PhysicalLogRecord,
        allocator);

    KArray<ProgressVectorEntry> beginCheckpointProgressVectorEntries = TestLogRecordUtility::ProgressVectorEntryArray(allocator, 10, false);
    ProgressVector::SPtr recordVector = ProgressVector::Create(allocator);
    TestLogRecordUtility::PopulateProgressVectorFromEntries(*recordVector, beginCheckpointProgressVectorEntries);

    // Create begin checkpoint record
    BeginCheckpointLogRecord::SPtr baseRecord = BeginCheckpointLogRecord::Create(
    false,
        *recordVector,
        beginTxRecord.RawPtr(),
        Epoch::ZeroEpoch(),
        Epoch(4, 0),
        230,
        nullptr,
        *invalidLogRecords->Inv_PhysicalLogRecord,
        *backupLogRecord,
        0,
        0,
        0,
        allocator);

    // Set record position
    if (!invalidRecordPosition)
    {
        baseRecord->RecordPosition = 10;
    }

    // Assign previous and linked records
    if (!nullPreviousPhysicalRecord) 
    {
        baseRecord->PreviousPhysicalRecord = CreatePhysicalLogRecord(*invalidLogRecords->Inv_PhysicalLogRecord, allocator).RawPtr();
    }

    if (!nullLinkedPhysicalRecord) 
    {
        baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(
            *invalidLogRecords->Inv_PhysicalLogRecord,
            allocator,
            LoggingReplicatorTests::LinkedPhysicalRecordType::CompleteCheckpoint);
    }

    return baseRecord;
}

PhysicalLogRecord::SPtr LoggingReplicatorTests::TestLogRecordUtility::CreateLinkedPhysicalLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator,
    __in LoggingReplicatorTests::LinkedPhysicalRecordType::Enum recordType)
{
    PhysicalLogRecord::SPtr linkedPhysicalRecord;
    Epoch const & indexingEpoch = Epoch(1, 1);

    IndexingLogRecord::SPtr logHeadRecord = IndexingLogRecord::Create(
        indexingEpoch,
        23,
        nullptr,
        invalidPhysicalLogRecord,
        allocator);

    logHeadRecord->RecordPosition = 10;

    switch (recordType)
    {
        case LoggingReplicatorTests::LinkedPhysicalRecordType::TruncateHead:
        {
            TruncateHeadLogRecord::SPtr recordToLink = TruncateHeadLogRecord::Create(
                *logHeadRecord,
                25,
                nullptr,
                invalidPhysicalLogRecord,
                true,
                0, // timeStamp set to 0
                allocator);

            recordToLink->RecordPosition = 1;
            linkedPhysicalRecord = recordToLink.RawPtr();
            break;
        }
        case LoggingReplicatorTests::LinkedPhysicalRecordType::CompleteCheckpoint:
        {
            CompleteCheckPointLogRecord::SPtr recordToLink = CompleteCheckPointLogRecord::Create(
                652,
                *logHeadRecord,
                nullptr,
                invalidPhysicalLogRecord,
                allocator);

            recordToLink->RecordPosition = 15;
            linkedPhysicalRecord = recordToLink.RawPtr();
            break;
        }
        case LoggingReplicatorTests::LinkedPhysicalRecordType::EndCheckpoint:   // Implement as needed
        default:
        {
            CODING_ERROR_ASSERT(false);
        }
    };

    return linkedPhysicalRecord.RawPtr();
}

// TODO: Move all usages in PhysicalLogRecord.Test to this static method
PhysicalLogRecord::SPtr LoggingReplicatorTests::TestLogRecordUtility::CreatePhysicalLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    TruncateTailLogRecord::SPtr baseRecord = TruncateTailLogRecord::Create(
        LogRecordType::Enum::TruncateTail,
        0,
        1,
        invalidPhysicalLogRecord,
        allocator);

    return PhysicalLogRecord::SPtr(baseRecord.RawPtr());
}

KBuffer::SPtr TestLogRecordUtility::CreateBufferFromRecordOperationData(
    __in OperationData::CSPtr operationData)
{
    KBuffer::SPtr targetBuffer;
    ULONG totalSerializedBufferSize = 0;
    ULONG32 operationDataBufferCount = operationData->BufferCount;

    for (ULONG i = 0; i < operationDataBufferCount - 1; i++)
    {
        totalSerializedBufferSize += (*operationData)[i]->QuerySize();
    }

    // Read the record length prepended and appended to the returned operation data
    ULONG32 prependedRecordLength = 0;
    ULONG32 appendedRecordLength = 0;

    KBuffer::CSPtr prependedBuffer = (*operationData)[0];
    KBuffer::CSPtr appendedBuffer = (*operationData)[operationDataBufferCount - 1];

    // Create BinaryReader from KBuffer
    BinaryReader prependedBufferReader(*prependedBuffer, GetThisAllocator());
    prependedBufferReader.Read(prependedRecordLength);

    BinaryReader appendedBufferReader(*prependedBuffer, GetThisAllocator());
    appendedBufferReader.Read(appendedRecordLength);
    
    CODING_ERROR_ASSERT(prependedRecordLength == appendedRecordLength);

    // Create the buffer, do not include the first and last entries
    targetBuffer = CreateBufferFromOperationData(operationData, GetThisAllocator(), 1, operationDataBufferCount - 1);

    return targetBuffer;
}

KArray<OperationData::SPtr> TestLogRecordUtility::CreateOperationDataObjects(
    __in int numOperationDataObjects,
    __in KAllocator & allocator)
{
    KArray<OperationData::SPtr> operationDataArray(allocator);

    for (int i = 0; i < numOperationDataObjects; i++)
    {
        operationDataArray.Append(OperationData::Create(allocator));
    }

    return operationDataArray;
}

BeginTransactionOperationLogRecord::SPtr TestLogRecordUtility::CreateBeginTransactionLogRecord(
    __in int seed,
    __in Data::LogRecordLib::InvalidLogRecords & invalidRecords,
    __in KAllocator & allocator,
    __in bool nullMetadata,
    __in bool nullUndoData,
    __in bool nullRedoData,
    __in double probabilityOfZeroOperationData)
{
    KArray<OperationData::SPtr> operationDataObjects = TestLogRecordUtility::CreateOperationDataObjects(3, allocator);
    PopulateOperationDataObjects(operationDataObjects, seed, allocator, probabilityOfZeroOperationData);

    // Set redodata to nullptr
    TestTransaction::SPtr transaction =
        TestTransaction::Create(
            invalidRecords,
            nullMetadata ? nullptr : operationDataObjects[0].RawPtr(),
            nullUndoData ? nullptr : operationDataObjects[1].RawPtr(),
            nullRedoData ? nullptr : operationDataObjects[2].RawPtr(),
            false,
            STATUS_SUCCESS,
            allocator);

    BeginTransactionOperationLogRecord::SPtr baseRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(transaction->LogRecords[0].RawPtr());
    return baseRecord;
}

OperationLogRecord::SPtr TestLogRecordUtility::CreateOperationLogRecord(
    __in int seed,
    __in Data::LogRecordLib::InvalidLogRecords & invalidRecords,
    __in KAllocator & allocator,
    __in bool nullMetadata,
    __in bool nullUndoData,
    __in bool nullRedoData)
{
    KArray<OperationData::SPtr> operationDataObjects = TestLogRecordUtility::CreateOperationDataObjects(3, allocator);
    TestLogRecordUtility::PopulateOperationDataObjects(operationDataObjects, seed, allocator);

    TestTransaction::SPtr transaction = TestTransaction::Create(invalidRecords, 1, 1, false, STATUS_SUCCESS, allocator);

    transaction->AddOperation(
        nullMetadata ? nullptr : operationDataObjects[0].RawPtr(),
        nullUndoData ? nullptr : operationDataObjects[1].RawPtr(),
        nullRedoData ? nullptr : operationDataObjects[2].RawPtr());

    // Randomly populate redo/undo/metadata
    OperationLogRecord::SPtr baseRecord = dynamic_cast<OperationLogRecord *>(transaction->LogRecords[1].RawPtr());
    return baseRecord;
}

KArray<int> TestLogRecordUtility::PopulateOperationDataObjects(
    __in KArray<OperationData::SPtr> operationDataArray,
    __in int seed,
    __in KAllocator & allocator,
    __in DOUBLE probabilityOfZeroLengthBuffer)
{
    KArray<int> numberOfZeroBuffersArray(allocator);
    int numZeroBuffers;

    for (ULONG i = 0; i < operationDataArray.Count(); i++)
    {
        numZeroBuffers = TestLogRecordUtility::PopulateRandomOperationData(operationDataArray[i], seed, allocator, probabilityOfZeroLengthBuffer);
        numberOfZeroBuffersArray.Append(numZeroBuffers);
    }

    return numberOfZeroBuffersArray;
}
