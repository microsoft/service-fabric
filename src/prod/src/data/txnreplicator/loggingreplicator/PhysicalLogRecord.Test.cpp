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

    StringLiteral const TraceComponent = "PhysicalLogRecordTest";

    class PhysicalLogRecordTests
    {
    protected:
        void EndTest();

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void PhysicalLogRecordTests::EndTest()
    {
        prId_.Reset();
    }

    TruncateTailLogRecord::SPtr CreateTruncateTailLogRecord(
        __in PhysicalLogRecord & invalidPhysicalLogRecord,
        __in KAllocator & allocator)
    {
        TruncateTailLogRecord::SPtr baseRecord = TruncateTailLogRecord::Create(
            LogRecordType::Enum::TruncateTail,
            0,
            1,
            invalidPhysicalLogRecord,
            allocator);

        return baseRecord;
    }

    IndexingLogRecord::SPtr CreateIndexingLogRecord(
        __in PhysicalLogRecord & invalidPhysicalLogRecord,
        __in KAllocator & allocator)
    {
        Epoch const & indexingEpoch = Epoch(1, 1);

        IndexingLogRecord::SPtr baseRecord = IndexingLogRecord::Create(
            indexingEpoch,
            23,
            nullptr,
            invalidPhysicalLogRecord,
            allocator);

        baseRecord->RecordPosition = 10;

        return baseRecord;
    }

    BOOST_FIXTURE_TEST_SUITE(PhysicalLogRecordTestsSuite, PhysicalLogRecordTests)

    BOOST_AUTO_TEST_CASE(VerifyInvalidLogRecordCast)
    {
        TEST_TRACE_BEGIN("VerifyInvalidLogRecordCast")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            TruncateTailLogRecord::SPtr baseRecord = CreateTruncateTailLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            IndexingLogRecord::SPtr testIndexingRecord = CreateIndexingLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);

            bool badCastExceptionThrown = false;

            try
            {
                LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);
                testIndexingRecord->Test_Equals(*recordFromOperationData);
            }
            catch (std::bad_cast)
            {
                badCastExceptionThrown = true;
            }

            VERIFY_ARE_EQUAL(badCastExceptionThrown, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyInformationLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyInformationLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Use random generator for LSN and RecordPosition
            InformationLogRecord::SPtr baseRecord = InformationLogRecord::Create(LogRecordType::Information, 10, 23, *util->InvalidRecords->Inv_PhysicalLogRecord, allocator);

            baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            baseRecord->LinkedPhysicalRecordOffset = 40;
            baseRecord->LinkedPhysicalRecord->LinkedPhysicalRecordOffset = 41;

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyTruncateTailLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyTruncateTailLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Use random generator for LSN and RecordPosition
            TruncateTailLogRecord::SPtr baseRecord = CreateTruncateTailLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            baseRecord->LinkedPhysicalRecordOffset = 40;
            baseRecord->LinkedPhysicalRecord->LinkedPhysicalRecordOffset = 41;

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyTruncateHeadLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyTruncateHeadLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);
            IndexingLogRecord::SPtr validIndexingRecord = CreateIndexingLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);

            TruncateHeadLogRecord::SPtr baseRecord = TruncateHeadLogRecord::Create(
                *validIndexingRecord,
                25,
                nullptr,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                true,
                0,
                allocator);

            baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            baseRecord->RecordPosition = 40;

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyIndexingLogRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyIndexingLogRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);
            IndexingLogRecord::SPtr baseRecord = CreateIndexingLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBeginCheckpointRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyBeginCheckpointRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Create begin checkpoint record
            BeginCheckpointLogRecord::SPtr baseRecord = TestLogRecordUtility::CreateBeginCheckpointLogRecord(util->InvalidRecords, allocator);

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyCompleteCheckpointRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyCompleteCheckpointRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            IndexingLogRecord::SPtr headRecord = CreateIndexingLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);

            CompleteCheckPointLogRecord::SPtr baseRecord = CompleteCheckPointLogRecord::Create(
                652,
                *headRecord,
                nullptr,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                allocator);

            baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            baseRecord->RecordPosition = 34;

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyEndCheckpointRecordEquals)
    {
        TEST_TRACE_BEGIN("VerifyEndCheckpointRecordEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            // Create indexing record
            IndexingLogRecord::SPtr headRecord = CreateIndexingLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            
            // Create begin checkpoint record
            BeginCheckpointLogRecord::SPtr beginCheckpointRecord = TestLogRecordUtility::CreateBeginCheckpointLogRecord(util->InvalidRecords, allocator);

            // Create end checkpoint record
            EndCheckpointLogRecord::SPtr baseRecord = EndCheckpointLogRecord::Create(
                LogRecordType::Enum::EndCheckpoint,
                56,
                231,
                *headRecord,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                *beginCheckpointRecord,
                allocator);

            // Link previous physical record
            baseRecord->LinkedPhysicalRecord = TestLogRecordUtility::CreateLinkedPhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator);
            baseRecord->PreviousPhysicalRecord = TestLogRecordUtility::CreatePhysicalLogRecord(*util->InvalidRecords->Inv_PhysicalLogRecord, allocator).RawPtr();

            OperationData::CSPtr writtenOperationData = util->WriteRecord(*baseRecord, true);
            LogRecord::SPtr recordFromOperationData = util->ReadRecord(writtenOperationData, true);

            bool initialEqualsRecordFromOpData = baseRecord->Test_Equals(*recordFromOperationData);

            VERIFY_ARE_EQUAL(initialEqualsRecordFromOpData, true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyProgressVectorEquals)
    {
        TEST_TRACE_BEGIN("VerifyProgressVectorEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);
            int numProgressVectorEntries = r.Next(10, 1500);
            Trace.WriteInfo(TraceComponent, "VerifyProgressVectorEquals : numProgressVectorEntries = {0}", numProgressVectorEntries);

            // Generate 50 progress vector entries
            KArray<ProgressVectorEntry> entries = TestLogRecordUtility::ProgressVectorEntryArray(allocator, numProgressVectorEntries, false);
            ProgressVector::SPtr writeVector = ProgressVector::Create(allocator);

            TestLogRecordUtility::PopulateProgressVectorFromEntries(*writeVector, entries);

            writeVector->Write(util->Writer);

            KBuffer::SPtr readBuffer = util->Writer.GetBuffer(0, util->Writer.Position);
            BinaryReader binaryReader(*readBuffer, allocator);

            ProgressVector::SPtr readVector = ProgressVector::Create(allocator);
            readVector->Read(binaryReader, false);

            VERIFY_IS_TRUE(writeVector->Equals(*readVector));
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyProgressVectorNotEquals)
    {
        TEST_TRACE_BEGIN("VerifyProgressVectorNotEquals")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);
            int numProgressVectorEntries = r.Next(10, 1500);

            // Generate two sets of progress vector entries
            KArray<ProgressVectorEntry> firstEntries = TestLogRecordUtility::ProgressVectorEntryArray(allocator, numProgressVectorEntries, false);
            KArray<ProgressVectorEntry> secondEntries = TestLogRecordUtility::ProgressVectorEntryArray(allocator, numProgressVectorEntries, false, 200);

            // Populate both progress vectors
            ProgressVector::SPtr firstVector = ProgressVector::Create(allocator);
            ProgressVector::SPtr secondVector = ProgressVector::Create(allocator);

            TestLogRecordUtility::PopulateProgressVectorFromEntries(*firstVector, firstEntries);
            TestLogRecordUtility::PopulateProgressVectorFromEntries(*secondVector, secondEntries);

            // Verify they are not equal
            VERIFY_IS_FALSE(firstVector->Equals(*secondVector));
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
