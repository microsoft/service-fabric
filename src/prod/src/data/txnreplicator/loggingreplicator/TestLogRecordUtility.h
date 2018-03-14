// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    namespace LinkedPhysicalRecordType
    {
        enum Enum
        {
            TruncateHead = 0,

            CompleteCheckpoint = 1,

            EndCheckpoint = 2
        };
    }

    class TestLogRecordUtility final
        : public Data::LogRecordLib::LogRecord
    {
        K_FORCE_SHARED(TestLogRecordUtility)

    public:

        static TestLogRecordUtility::SPtr Create(KAllocator & allocator);

        static int PopulateRandomOperationData(
            __inout Data::Utilities::OperationData::SPtr operationData,
            __in int seed,
            __in KAllocator & allocator,
            __in DOUBLE probabilityOfZeroLengthBuffer = -1,
            __in ULONG maxBufferSize = 2000,
            __in ULONG minBufferSize = 500,
            __in ULONG maxBuffers = 300,
            __in ULONG minBuffers = 50);

        static KBuffer::SPtr CreateBufferFromOperationData(
            __in Data::Utilities::OperationData::CSPtr operationData,
            __in KAllocator & allocator,
            __in ULONG startIndex = 0,
            __in ULONG lastIndex = 0);

        static Data::LogRecordLib::BeginCheckpointLogRecord::SPtr CreateBeginCheckpointLogRecord(
            __in Data::LogRecordLib::InvalidLogRecords::SPtr invalidLogRecords,
            __in KAllocator & allocator, 
            __in bool nullPreviousPhysicalRecord = false,
            __in bool nullLinkedPhysicalRecord = false,
            __in bool invalidRecordPosition = false);

        static Data::LogRecordLib::PhysicalLogRecord::SPtr CreatePhysicalLogRecord(
            __in Data::LogRecordLib::PhysicalLogRecord & invalidPhysicalLogRecord,
            __in KAllocator & allocator);

        static KArray<Data::LogRecordLib::ProgressVectorEntry> ProgressVectorEntryArray(
            __in KAllocator & allocator,
            __in ULONG numEntries = 10,
            __in bool useInvalidEpoch = true,
            __in ULONG startingLsn = 0);

        static void PopulateProgressVectorFromEntries(
            __in Data::LogRecordLib::ProgressVector & newVector,
            __in KArray<Data::LogRecordLib::ProgressVectorEntry> entries);

        //
        // Create an array of 'n' operation data objects
        //
        static KArray<Data::Utilities::OperationData::SPtr> CreateOperationDataObjects(
            __in int numOperationDataObjects,
            __in KAllocator & allocator);

        static Data::LogRecordLib::BeginTransactionOperationLogRecord::SPtr CreateBeginTransactionLogRecord(
            __in int seed,
            __in Data::LogRecordLib::InvalidLogRecords & invalidRecords,
            __in KAllocator & allocator,
            __in bool nullMetadata = false,
            __in bool nullUndoData = false,
            __in bool nullRedoData = false,
            __in double probabilityOfZeroOperationData = 0.0);

        static Data::LogRecordLib::OperationLogRecord::SPtr CreateOperationLogRecord(
            __in int seed,
            __in Data::LogRecordLib::InvalidLogRecords & invalidRecords,
            __in KAllocator & allocator,
            __in bool nullMetadata = false,
            __in bool nullUndoData = false,
            __in bool nullRedoData = false);

        static Data::LogRecordLib::PhysicalLogRecord::SPtr CreateLinkedPhysicalLogRecord(
            __in Data::LogRecordLib::PhysicalLogRecord & invalidPhysicalLogRecord,
            __in KAllocator & allocator,
            __in LoggingReplicatorTests::LinkedPhysicalRecordType::Enum recordType = LoggingReplicatorTests::LinkedPhysicalRecordType::TruncateHead);

        //
        // Randomly populate operation data objects
        // Return an array with the number of zero length buffers in each operation data object
        //
        static KArray<int> PopulateOperationDataObjects(
            __in KArray<Data::Utilities::OperationData::SPtr> operationDataArray,
            __in int seed,
            __in KAllocator & allocator,
            __in DOUBLE probabilityOfZeroLengthBuffer = 0);

        __declspec(property(get = get_BinaryWriter)) Data::Utilities::BinaryWriter & Writer;
        Data::Utilities::BinaryWriter & get_BinaryWriter()
        {
            return binaryWriter_;
        }

        __declspec(property(get = get_InvalidRecords)) Data::LogRecordLib::InvalidLogRecords::SPtr InvalidRecords;
        Data::LogRecordLib::InvalidLogRecords::SPtr get_InvalidRecords() const
        {
            return invalidRecords_;
        }

        __declspec(property(get = get_Allocator)) KAllocator & Allocator;
        KAllocator & get_Allocator()
        {
            return GetThisAllocator();
        }

        Data::Utilities::OperationData::CSPtr WriteRecord(
            __in Data::LogRecordLib::LogRecord & base,
            __in bool isPhysicalWrite);

        Data::LogRecordLib::LogRecord::SPtr ReadRecord(
            __in Data::Utilities::OperationData::CSPtr writtenData,
            __in bool isPhysicalRead);

        Data::LogRecordLib::LogRecord::SPtr ReadRecordFromOperationData(
            __in Data::Utilities::OperationData::CSPtr writtenData,
            __in Data::LogRecordLib::InvalidLogRecords & invalidLogRecords);

        // Creates a KBuffer instance from operation data returned by LogRecord:WriteRecord
        // This method removes the 4 byte size bookending the operation data object
        KBuffer::SPtr CreateBufferFromRecordOperationData(
            __in Data::Utilities::OperationData::CSPtr operationData);

        void Reset();

    private:

        Data::Utilities::BinaryWriter binaryWriter_;
        Data::LogRecordLib::InvalidLogRecords::SPtr invalidRecords_;
    };
}
