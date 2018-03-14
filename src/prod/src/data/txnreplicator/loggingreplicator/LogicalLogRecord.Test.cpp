// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;

    StringLiteral const TraceComponent = "LogicalLogRecordTest";

    class LogicalLogRecordTests
    {
    protected:
        void EndTest();

        template<typename T>
        LogRecord::SPtr WriteAndVerifyLogicalRecord(__in TestLogRecordUtility & util, __in T & baseRecord);

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    //
    // Create array of 3 operation data objects
    // Used to create metadata/undo/redodata operation data for TransactionLogRecords
    //
    KArray<OperationData::SPtr> CreateTxRecordOperationDataArray(__in KAllocator & allocator)
    {
        return TestLogRecordUtility::CreateOperationDataObjects(3, allocator);
    }

    template <typename T>
    LogRecord::SPtr LogicalLogRecordTests::WriteAndVerifyLogicalRecord(
        __in TestLogRecordUtility & util,
        __in T & baseRecord)
    {
        // Reset BinaryWriter
        util.Reset();

        // Write record
        OperationData::CSPtr writtenData = util.WriteRecord(*baseRecord, false);

        // LogRecord::ReadFromOperationData path (ReadLogical)    
        // Read record from returned operation data object
        LogRecord::SPtr recordFromOperationData = util.ReadRecordFromOperationData(writtenData, *util.InvalidRecords);
        bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

        // LogRecord::ReadRecord path 
        // Converts the returned OperationData object into a contiguous KBuffer in memory
        // Record is read using a BinaryReader initialized with KBuffer
        LogRecord::SPtr recordFromBinaryReader = util.ReadRecord(writtenData, false);
        bool initialEqualsRecordFromBinaryReader = baseRecord->Test_Equals(*recordFromBinaryReader);

        VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        VERIFY_ARE_EQUAL(initialEqualsRecordFromBinaryReader, true);

        // Return the base record from operation data
        return recordFromOperationData;
    }

    void SerializeAndVerifyOperationDataEquality(
        __in BinaryWriter & writer,
        __in OperationData::SPtr operationDataToVerify,
        __in int numZeroBuffers,
        __in KAllocator & allocator)
    {
        // Reset BinaryWriter
        writer.Reset();

        OperationData::SPtr serializedOperationData = OperationData::Create(allocator);
        OperationData::Serialize(writer, operationDataToVerify.RawPtr(), *serializedOperationData);

        // The serialized operation data buffer count = 2x + 1 - y
        // x = initial operation data buffer count
        // y = number of zero length buffers
        // 1 represents the buffercount at the beginning of the serialized operation data
        // x is multiplied by 2 as each buffer in the initial operation data adds unique entries for size and content
        // y is subtracted from the sum of (2x + 1) since zero length buffers are not appended during serialization

        VERIFY_ARE_EQUAL(serializedOperationData->BufferCount, (2 * operationDataToVerify->BufferCount + 1 - numZeroBuffers));

        // Deserialize with the returned operation data
        int index = -1;
        OperationData::CSPtr constDeserializedOperationData = OperationData::DeSerialize(*serializedOperationData, index, allocator);

        // The deserialized operation data must match the initial operation data
        VERIFY_ARE_EQUAL(operationDataToVerify->Test_Equals(*constDeserializedOperationData), true);

        // Deserialize with BinaryReader initialized with a KBuffer of the returned operation data 
        KBuffer::SPtr bufferToDeserialize = TestLogRecordUtility::CreateBufferFromOperationData(serializedOperationData.RawPtr(), allocator, 0, serializedOperationData->BufferCount);
        BinaryReader r(*bufferToDeserialize, allocator);
        OperationData::CSPtr readerDeserializedOperationData = OperationData::DeSerialize(r, allocator);

        // The deserialized operation data from BinaryReader must match the initial operation data
        VERIFY_ARE_EQUAL(operationDataToVerify->Test_Equals(*readerDeserializedOperationData), true);
    }

    void SerializeAndVerifyConstOperationDataEquality(
        __in BinaryWriter & writer,
        __in OperationData::CSPtr operationDataToVerify,
        __in int numZeroBuffers,
        __in KAllocator & allocator)
    {
        // Remove const from input OperationData::CSPtr
        SerializeAndVerifyOperationDataEquality(writer, const_cast<OperationData *>(operationDataToVerify.RawPtr()), numZeroBuffers, allocator);
    }

    void LogicalLogRecordTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(LogicalLogRecordTestSuite, LogicalLogRecordTests)

        BOOST_AUTO_TEST_CASE(VerifyOperationDataEqualsEmptyBuffer)
    {
        TEST_TRACE_BEGIN("VerifyOperationDataEqualsEmptyBuffer")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            OperationData::SPtr operationData = OperationData::Create(allocator);
            OperationData::SPtr serializedOperationData = OperationData::Create(allocator);

            // Static number used to verify equals with empty operation data
            int bufSize = 230;
            KBuffer::SPtr buffer;
            NTSTATUS bufferStatus = KBuffer::Create(
                bufSize,
                buffer,
                allocator);

            VERIFY_ARE_EQUAL(bufferStatus, STATUS_SUCCESS);

            // Append buffer to operation data
            operationData->Append(*buffer);

            SerializeAndVerifyOperationDataEquality(util->Writer, operationData, 0, allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationDataEquals_RandomBufferContent)
    {
        TEST_TRACE_BEGIN("VerifyOperationDataEquals_RandomBufferContent")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            int numZeroBuffers;
            int numOperationDataObjects = r.Next(10, 30);
            KArray<OperationData::SPtr> operationDataArray = TestLogRecordUtility::CreateOperationDataObjects(numOperationDataObjects, allocator);

            Trace.WriteInfo(TraceComponent, "VerifyOperationDataEquals_RandomBufferContent : {0} OperationData objects", operationDataArray.Count());

            for(int i = 0; i < numOperationDataObjects; i++)
            {
                // Randomly populate operationData
                numZeroBuffers = TestLogRecordUtility::PopulateRandomOperationData(operationDataArray[i], seed, allocator);

                // No zero-length buffers expected
                CODING_ERROR_ASSERT(numZeroBuffers == 0);
                SerializeAndVerifyOperationDataEquality(util->Writer, operationDataArray[i], numZeroBuffers, allocator);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationDataEquals_RandomBufferContent_ZeroLengthBuffers)
    {
        TEST_TRACE_BEGIN("VerifyOperationDataEquals_RandomBufferContent_ZeroLengthBuffers")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            int numZeroBuffers;
            int numOperationDataObjects = r.Next(10, 30);
            KArray<OperationData::SPtr> operationDataArray = TestLogRecordUtility::CreateOperationDataObjects(numOperationDataObjects, allocator);

            Trace.WriteInfo(TraceComponent, "VerifyOperationDataEquals_RandomBufferContent_ZeroLengthBuffers : {0} OperationData objects", operationDataArray.Count());

            for (int i = 0; i < numOperationDataObjects; i++)
            {
                // Randomly populate operationData
                numZeroBuffers = TestLogRecordUtility::PopulateRandomOperationData(operationDataArray[i], seed, allocator, 0.3);
                Trace.WriteInfo(TraceComponent, "VerifyOperationDataEquals_RandomBufferContent_ZeroLengthBuffers : added {0} zero-length buffers to operation data {1}", numZeroBuffers, i);

                SerializeAndVerifyOperationDataEquality(util->Writer, operationDataArray[i], numZeroBuffers, allocator);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBarrierLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyBarrierLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            BarrierLogRecord::SPtr baseRecord = BarrierLogRecord::Create(
                LogRecordType::Enum::Barrier,
                25,
                232,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                allocator);

            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyUpdateEpochLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyUpdateEpochLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            Epoch const & updateEpoch = Epoch(1, 0);;

            UpdateEpochLogRecord::SPtr baseRecord = UpdateEpochLogRecord::Create(
                updateEpoch,
                34,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                allocator);

            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBackupLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyBackupLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            Epoch const & highestBackedUpEpoch = Epoch(1, 0);
            KGuid backupId;
            backupId.CreateNew();
            
            LONG64 highestBackedUpLsn = 340;
            UINT backupLogRecordCount = 12;
            UINT backupLogSize = 5000;

            BackupLogRecord::SPtr baseRecord = BackupLogRecord::Create(
                backupId,
                highestBackedUpEpoch,
                highestBackedUpLsn,
                backupLogRecordCount,
                backupLogSize,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                allocator);

            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyEndTxRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyEndTxRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            TestTransaction::SPtr transaction = TestTransaction::Create(*util->InvalidRecords, 1, 1, false, STATUS_SUCCESS, allocator);

            bool isThisReplicaTransition = !!(r.Next() % 2);
            bool isCommitted = !!(r.Next() % 2);

            EndTransactionLogRecord::SPtr baseRecord = EndTransactionLogRecord::Create(
                *transaction->Tx,
                isThisReplicaTransition,
                isCommitted,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                *util->InvalidRecords->Inv_TransactionLogRecord,
                allocator);

            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyOperationLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Randomly populate redo/undo/metadata
            OperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateOperationLogRecord(seed, *util->InvalidRecords, allocator);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationLogRecordEquals_NullMetaData)
    {
        TEST_TRACE_BEGIN("VerifyOperationLogRecordEquals_NullMetaData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Set meta data to nullptr
            OperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateOperationLogRecord(seed, *util->InvalidRecords, allocator, true);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationLogRecordEquals_NullUndoData)
    {
        TEST_TRACE_BEGIN("VerifyOperationLogRecordEquals_NullUndoData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Set undo data to nullptr
            OperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateOperationLogRecord(seed, *util->InvalidRecords, allocator, false, true);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationLogRecordEquals_NullRedoData)
    {
        TEST_TRACE_BEGIN("VerifyOperationLogRecordEquals_NullRedoData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Set redo data to nullptr
            OperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateOperationLogRecord(seed, *util->InvalidRecords, allocator, false, false, true);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBeginTxRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyBeginTxRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            BeginTransactionOperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *util->InvalidRecords, allocator);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBeginTxRecordEquals_NullMetaData)
    {
        TEST_TRACE_BEGIN("VerifyBeginTxRecordEquals_NullMetaData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Set meta data to nullptr
            BeginTransactionOperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *util->InvalidRecords, allocator, true);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBeginTxRecordEquals_NullUndoData)
    {
        TEST_TRACE_BEGIN("VerifyBeginTxRecordEquals_NullUndoData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Set undo data to nullptr
            BeginTransactionOperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *util->InvalidRecords, allocator, false, true);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBeginTxRecordEquals_NullRedoData)
    {
        TEST_TRACE_BEGIN("VerifyBeginTxRecordEquals_NullRedoData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Set redodata to nullptr
            BeginTransactionOperationLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateBeginTransactionLogRecord(seed, *util->InvalidRecords, allocator, false, false, true);
            WriteAndVerifyLogicalRecord(*util, baseRecord);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBeginTxRecordEquals_ZeroBuffers_OperationData)
    {
        TEST_TRACE_BEGIN("VerifyBeginTxRecordEquals_ZeroBuffers_OperationData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            KArray<OperationData::SPtr> operationDataObjects = CreateTxRecordOperationDataArray(allocator);
            KArray<int> numZeroBuffersArray = TestLogRecordUtility::PopulateOperationDataObjects(operationDataObjects, seed, allocator, 0.3);

            TestTransaction::SPtr transaction =
                TestTransaction::Create(
                    *util->InvalidRecords,
                    operationDataObjects[0].RawPtr(),
                    operationDataObjects[1].RawPtr(),
                    operationDataObjects[2].RawPtr(),
                    false,
                    STATUS_SUCCESS,
                    allocator);

            BeginTransactionOperationLogRecord::SPtr baseRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(transaction->LogRecords[0].RawPtr());

            SerializeAndVerifyConstOperationDataEquality(util->Writer, baseRecord->Metadata, numZeroBuffersArray[0], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, baseRecord->Undo, numZeroBuffersArray[1], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, baseRecord->Redo, numZeroBuffersArray[2], allocator);

            LogRecord::SPtr fromOperationData = WriteAndVerifyLogicalRecord(*util, baseRecord);
            BeginTransactionOperationLogRecord::SPtr outputRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(fromOperationData.RawPtr());

            SerializeAndVerifyConstOperationDataEquality(util->Writer, outputRecord->Metadata, numZeroBuffersArray[0], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, outputRecord->Undo, numZeroBuffersArray[1], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, outputRecord->Redo, numZeroBuffersArray[2], allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyOperationLogRecordEquals_ZeroBuffers_OperationData)
    {
        TEST_TRACE_BEGIN("VerifyOperationLogRecordEquals_ZeroBuffers_OperationData")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            KArray<OperationData::SPtr> operationDataObjects = CreateTxRecordOperationDataArray(allocator);
            KArray<int> numZeroBuffersArray = TestLogRecordUtility::PopulateOperationDataObjects(operationDataObjects, seed, allocator, 0.3);
            
            TestTransaction::SPtr transaction = TestTransaction::Create(*util->InvalidRecords, 1, 1, false, STATUS_SUCCESS, allocator);

            transaction->AddOperation(operationDataObjects[0].RawPtr(), operationDataObjects[1].RawPtr(), operationDataObjects[2].RawPtr());

            OperationLogRecord::SPtr baseRecord = dynamic_cast<OperationLogRecord *>(transaction->LogRecords[1].RawPtr());

            SerializeAndVerifyConstOperationDataEquality(util->Writer, baseRecord->Metadata, numZeroBuffersArray[0], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, baseRecord->Undo, numZeroBuffersArray[1], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, baseRecord->Redo, numZeroBuffersArray[2], allocator);

            LogRecord::SPtr fromOperationData = WriteAndVerifyLogicalRecord(*util, baseRecord);
            OperationLogRecord::SPtr outputRecord = dynamic_cast<OperationLogRecord *>(fromOperationData.RawPtr());

            SerializeAndVerifyConstOperationDataEquality(util->Writer, outputRecord->Metadata, numZeroBuffersArray[0], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, outputRecord->Undo, numZeroBuffersArray[1], allocator);
            SerializeAndVerifyConstOperationDataEquality(util->Writer, outputRecord->Redo, numZeroBuffersArray[2], allocator);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
