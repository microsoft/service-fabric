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

    StringLiteral const TraceComponent = "ProgressVectorTests";

    class ProgressVectorTests
    {
    protected:
        UpdateEpochLogRecord::SPtr CreateUpdateEpochLogRecord(
            LONG64 dataLossVersion,
            LONG64 configurationVersion,
            LONG64 primaryReplicaId,
            LONG64 lsn);

        void EndTest();

        InvalidLogRecords::SPtr invalidRecords_;
        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    UpdateEpochLogRecord::SPtr ProgressVectorTests::CreateUpdateEpochLogRecord(
        LONG64 dataLossVersion,
        LONG64 configurationVersion,
        LONG64 primaryReplicaId,
        LONG64 lsn)
    {
        UpdateEpochLogRecord::SPtr epoch = UpdateEpochLogRecord::Create(
            Epoch(dataLossVersion, configurationVersion), primaryReplicaId, *invalidRecords_->Inv_PhysicalLogRecord, underlyingSystem_->NonPagedAllocator());

        CODING_ERROR_ASSERT(epoch != nullptr);
        epoch->Lsn = lsn;
        return epoch;
    }

    void ProgressVectorTests::EndTest()
    {
        invalidRecords_.Reset();
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(ProgressVectorTestsSuite, ProgressVectorTests)

    BOOST_AUTO_TEST_CASE(FindEpoch_EmptyProgressVector)
    {
        TEST_TRACE_BEGIN("FindEpoch_EmptyProgressVector")

        {
            Epoch expectedResult = Epoch::InvalidEpoch();
            LONG64 lsn = 17;

            ProgressVector::SPtr vector = ProgressVector::Create(allocator);
            
            Epoch result = vector->FindEpoch(lsn);
            
            VERIFY_ARE_EQUAL(expectedResult, result);
        }
    }

    BOOST_AUTO_TEST_CASE(FindEpoch_OneVector)
    {
        TEST_TRACE_BEGIN("FindEpoch_OneVector")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
            Epoch expectedResult = Epoch::ZeroEpoch();
            LONG64 lsn = 17;

            ProgressVector::SPtr vector = ProgressVector::Create(allocator);
            UpdateEpochLogRecord::SPtr epoch1 = CreateUpdateEpochLogRecord(0, 0, 0, 0);

            ProgressVectorEntry vector1(*epoch1);
            vector->Append(vector1);
            
            Epoch result = vector->FindEpoch(lsn);
            
            VERIFY_ARE_EQUAL(expectedResult, result);
        }
    }

    BOOST_AUTO_TEST_CASE(FindEpoch_DupeLsn)
    {
        TEST_TRACE_BEGIN("FindEpoch_DupeLsn")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
            Epoch expectedResult(1, 1);
            LONG64 lsn = 17;

            ProgressVector::SPtr vector = ProgressVector::Create(allocator);

            UpdateEpochLogRecord::SPtr epoch1 = CreateUpdateEpochLogRecord(0, 0, 0, 0);
            UpdateEpochLogRecord::SPtr epoch2 = CreateUpdateEpochLogRecord(1, 1, 0, 0);

            ProgressVectorEntry vector1(*epoch1);
            ProgressVectorEntry vector2(*epoch2);

            vector->Append(vector1);
            vector->Append(vector2);
            
            Epoch result = vector->FindEpoch(lsn);
            
            VERIFY_ARE_EQUAL(expectedResult, result);
        }
    }

    BOOST_AUTO_TEST_CASE(FindEpoch_UpdateEpochLogRecordLsn)
    {
        TEST_TRACE_BEGIN("FindEpoch_UpdateEpochLogRecordLsn")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
            Epoch expectedResult(1, 1);
            LONG64 lsn = 17;

            ProgressVector::SPtr vector = ProgressVector::Create(allocator);

            UpdateEpochLogRecord::SPtr epoch1 = CreateUpdateEpochLogRecord(0, 0, 0, 0);
            UpdateEpochLogRecord::SPtr epoch2 = CreateUpdateEpochLogRecord(1, 1, 0, 7);
            UpdateEpochLogRecord::SPtr epoch3 = CreateUpdateEpochLogRecord(2, 2, 0, 17);
            UpdateEpochLogRecord::SPtr epoch4 = CreateUpdateEpochLogRecord(3, 3, 0, 17);

            ProgressVectorEntry vector1(*epoch1);
            ProgressVectorEntry vector2(*epoch2);
            ProgressVectorEntry vector3(*epoch3);
            ProgressVectorEntry vector4(*epoch4);

            vector->Append(vector1);
            vector->Append(vector2);
            vector->Append(vector3);
            vector->Append(vector4);
            
            Epoch result = vector->FindEpoch(lsn);
            
            VERIFY_ARE_EQUAL(expectedResult, result);
        }
    }

    BOOST_AUTO_TEST_CASE(FindEpoch_Sanity)
    {
        TEST_TRACE_BEGIN("FindEpoch_Sanity")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
            Epoch expectedResult(3, 3);
            LONG64 lsn = 18;

            ProgressVector::SPtr vector = ProgressVector::Create(allocator);
            UpdateEpochLogRecord::SPtr epoch1 = CreateUpdateEpochLogRecord(0, 0, 0, 0);
            UpdateEpochLogRecord::SPtr epoch2 = CreateUpdateEpochLogRecord(1, 1, 0, 7);
            UpdateEpochLogRecord::SPtr epoch3 = CreateUpdateEpochLogRecord(2, 2, 0, 17);
            UpdateEpochLogRecord::SPtr epoch4 = CreateUpdateEpochLogRecord(3, 3, 0, 17);

            ProgressVectorEntry vector1(*epoch1);
            ProgressVectorEntry vector2(*epoch2);
            ProgressVectorEntry vector3(*epoch3);
            ProgressVectorEntry vector4(*epoch4);

            vector->Append(vector1);
            vector->Append(vector2);
            vector->Append(vector3);
            vector->Append(vector4);
            
            Epoch result = vector->FindEpoch(lsn);
            
            VERIFY_ARE_EQUAL(expectedResult, result);
        }
    }

    BOOST_AUTO_TEST_CASE(FindEpoch_TruncateProgressVector)
    {
        TEST_TRACE_BEGIN("FindEpoch_TruncateProgressVector")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
            Epoch expectedResult = Epoch::InvalidEpoch();
            LONG64 lsn = 17;

            ProgressVector::SPtr vector = ProgressVector::Create(allocator);
            UpdateEpochLogRecord::SPtr epoch3 = CreateUpdateEpochLogRecord(3, 3, 0, 27);
            UpdateEpochLogRecord::SPtr epoch4 = CreateUpdateEpochLogRecord(4, 4, 0, 37);

            ProgressVectorEntry vector3(*epoch3);
            ProgressVectorEntry vector4(*epoch4);

            vector->Append(vector3);
            vector->Append(vector4);
            
            Epoch result = vector->FindEpoch(lsn);
            
            VERIFY_ARE_EQUAL(expectedResult, result);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_EmptyVectors_Test0)
    {
        TEST_TRACE_BEGIN("FindSharedVector_EmptyVectors_Test0")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG expectedSourceIndex = 0;
            ULONG expectedTargetIndex = 0;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_SourceAheadOfTarget_Test0)
    {
        TEST_TRACE_BEGIN("FindSharedVector_SourceAheadOfTarget_Test0")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700));
            ULONG expectedSourceIndex = 1;
            ULONG expectedTargetIndex = 1;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561024, 130833787458698950, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_SourceAheadOfTarget_Test1)
    {
        TEST_TRACE_BEGIN("FindSharedVector_SourceAheadOfTarget_Test1")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700));
            ULONG expectedSourceIndex = 1;
            ULONG expectedTargetIndex = 1;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561024, 130833787458698950, 720)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561025, 130833787458698951, 721)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561026, 130833787458698952, 722)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561027, 130833787458698953, 723)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_TargetAheadOfSource_Test0)
    {
        TEST_TRACE_BEGIN("FindSharedVector_TargetAheadOfSource_Test0")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700));
            ULONG expectedSourceIndex = 1;
            ULONG expectedTargetIndex = 1;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 188978561024, 130833787458698950, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 700)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561024, 130833787458698950, 720)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561025, 130833787458698951, 721)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561026, 130833787458698952, 722)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 188978561027, 130833787458698953, 723)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_FromNewSourceToEmptyTarget_Test0)
    {
        TEST_TRACE_BEGIN("FindSharedVector_FromNewSourceToEmptyTarget_Test0")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG expectedSourceIndex = 0;
            ULONG expectedTargetIndex = 0;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787450372335, 8589934592, 130833787458698950, 1)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_FromRestoredSourceToEmptyTarget_Test0)
    {
        TEST_TRACE_BEGIN("FindSharedVector_FromRestoredSourceToEmptyTarget_Test0")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG expectedSourceIndex = 0;
            ULONG expectedTargetIndex = 0;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 17179869184, 130833787524140812, 1023)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_NoProgressReconfigurations_Test0)
    {
        TEST_TRACE_BEGIN("FindSharedVector_NoProgressReconfigurations_Test0")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(130833787759687526, 21474836480, 130833787524140812, 1023));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(130833787759687526, 21474836480, 130833787524140812, 1023));
            ULONG expectedSourceIndex = 3;
            ULONG expectedTargetIndex = 3;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 17179869184, 130833787524140812, 1023)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 21474836480, 130833787524140812, 1023)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 25769803776, 130833787524140812, 1023)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 17179869184, 130833787524140812, 1023)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787759687526, 21474836480, 130833787524140812, 1023)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_DropDownDataLossNumber_TargetIndexMovesDownADataLoss)
    {
        TEST_TRACE_BEGIN("FindSharedVector_DropDownDataLossNumber_TargetIndexMovesDownADataLoss")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ULONG expectedSourceIndex = 1;
            ULONG expectedTargetIndex = 1;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 188978561024, 130833789329128536, 282)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 184683593728, 130833789329128536, 755)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_DropDownDataLossNumber_Test1)
    {
        TEST_TRACE_BEGIN("FindSharedVector_DropDownDataLossNumber_Test1")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG expectedSourceIndex = 0;
            ULONG expectedTargetIndex = 0;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 50)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 188978561024, 130833789329128536, 282)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 70)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindSharedVector_DropDownDataLossNumber_SourceIndexMovesDownADataLoss)
    {
        TEST_TRACE_BEGIN("FindSharedVector_DropDownDataLossNumber_SourceIndexMovesDownADataLoss")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            ProgressVectorEntry expectedSourceVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ProgressVectorEntry expectedTargetVector(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ULONG expectedSourceIndex = 1;
            ULONG expectedTargetIndex = 1;

            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 188978561024, 130833789329128536, 1000)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 184683593728, 130833789329128536, 755)));

            SharedProgressVectorEntry sharedEntry = ProgressVector::FindSharedVector(*sourceProgressVector, *targetProgressVector);

            VERIFY_ARE_EQUAL(sharedEntry.SourceProgressVectorEntry, expectedSourceVector);
            VERIFY_ARE_EQUAL(sharedEntry.TargetProgressVectorEntry, expectedTargetVector);
            VERIFY_ARE_EQUAL(sharedEntry.SourceIndex, expectedSourceIndex);
            VERIFY_ARE_EQUAL(sharedEntry.TargetIndex, expectedTargetIndex);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_TargetIsBrandNew_SourceHeadTruncated)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_TargetIsBrandNew_SourceHeadTruncated")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::InsufficientLogs;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG const expectedSourceIndex = 0;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG const expectedTargetIndex = 0;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch(4, 2);
            LONG64 sourceLogHeadLsn = 5;
            LONG64 sourceCurrentTailLsn = 50;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(10, 10, 1, 50)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = Constants::OneLsn;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_TargetIsBrandNew)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_TargetIsBrandNew")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG const expectedSourceIndex = 0;
            LONG64 const expectedSourceStartingLsn = 50;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG const expectedTargetIndex = 0;
            LONG64 const expectedTargetStartingLsn = Constants::OneLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = Constants::OneLsn;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(10, 1, 1, 50)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = Constants::OneLsn;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    // Boot scenario is where the Primary comes up and needs to build a new replica.
    // In this case, the Copy Mode is Partial because the update Epoch needs to be copied.
    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_Boot_PartialCopyUpdateEpochOnly)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_Boot_PartialCopyUpdateEpochOnly")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG const expectedSourceIndex = 0;
            LONG64 const expectedSourceStartingLsn = Constants::ZeroLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0));
            ULONG const expectedTargetIndex = 0;
            LONG64 const expectedTargetStartingLsn = Constants::ZeroLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = Constants::ZeroLsn;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 0)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = Constants::ZeroLsn;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_SecondaryWithUnknownEpoch_ZeroLsn_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_SecondaryWithUnknownEpoch_ZeroLsn_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Other;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 0));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 0));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = Constants::ZeroLsn;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 3, 0)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = Constants::ZeroLsn;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 2, 0)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_SameProgressVectors_PrimaryLsnAhead_PartialCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_SameProgressVectors_PrimaryLsnAhead_PartialCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720));
            ULONG const expectedSourceIndex = 2;
            LONG64 const expectedSourceStartingLsn = 740;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720));
            ULONG const expectedTargetIndex = 2;
            LONG64 const expectedTargetStartingLsn = 720;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 740;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 720;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_SameProgressVector_SecondaryMadeProgressInLastVectorLessThanPrimary_PartialCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_SameProgressVector_SecondaryMadeProgressInLastVectorLessThanPrimary_PartialCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720));
            ULONG const expectedSourceIndex = 2;
            LONG64 const expectedSourceStartingLsn = 740;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720));
            ULONG const expectedTargetIndex = 2;
            LONG64 const expectedTargetStartingLsn = 730;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 740;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 730;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(PartialCopy_SourceAndTargetIndexDifferent)
    {
        // This is not a very interesting test case, but was the one that caught a very subtle copy paste bug in 
        // SharedProgressVectorEntry.h property getter
        TEST_TRACE_BEGIN("PartialCopy_SourceAndTargetIndexDifferent")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 8, 1, 3237));
            ULONG const expectedSourceIndex = 6;
            LONG64 const expectedSourceStartingLsn = 4382;

            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 8, 1, 3237));
            ULONG const expectedTargetIndex = 5;
            LONG64 const expectedTargetStartingLsn = 4382;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 4384;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 1159)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 4, 1, 3009)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 6, 1, 3010)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 7, 1, 3237)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 8, 1, 3237)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 9, 1, 4382)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 10, 1, 4383)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 4382;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 1)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 1159)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 4, 1, 3009)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 6, 1, 3010)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 8, 1, 3237)));

            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }


    BOOST_AUTO_TEST_CASE(FindCopyMode_FalseProgress_PrimaryProgressVectorAhead_PartialCopyWithFalseProgress)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FalseProgress_PrimaryProgressVectorAhead_PartialCopyWithFalseProgress")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial | CopyModeFlag::Enum::FalseProgress;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = 720;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = 730;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 740;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 730;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FalseProgress_PrimaryProgressVectorAheadWithDataloss_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FalseProgress_PrimaryProgressVectorAheadWithDataloss_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 740;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(2, 3, 3, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 730;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_MatchingDataLossNumbers_SecondaryLsnHigher_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_MatchingDataLossNumbers_SecondaryLsnHigher_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 282;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 188978561024, 130833789329128536, 282)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 755;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 184683593728, 130833789329128536, 755)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_MatchingDataLossNumbers_SecondaryLsnLower_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_MatchingDataLossNumbers_SecondaryLsnLower_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 282;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 188978561024, 130833789329128536, 282)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 200;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833787446594158, 8589934592, 130833787458698950, 1)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(130833789517437822, 184683593728, 130833789329128536, 200)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_FalseProgress_FalseProgressContainsAtomicRedo_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_FalseProgress_FalseProgressContainsAtomicRedo_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::AtomicRedoOperationFalseProgressed;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 740;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 730;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = 725;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_FalseProgress_TruncatedPrimaryLog_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_FalseProgress_TruncatedPrimaryLog_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::InsufficientLogs;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = 735;
            LONG64 sourceCurrentTailLsn = 740;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 730;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = 10;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_SecondaryAtUnknownEpoch_SecondaryLowLsn_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_SecondaryAtUnknownEpoch_SecondaryLowLsn_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Other;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 60;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 6, 6, 60)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 50;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 5, 50)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = 10;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_SecondaryAtUnknownEpoch_SameLsn_FullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_SecondaryAtUnknownEpoch_SameLsn_FullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Other;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 60;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 6, 6, 60)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 60;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 40)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 5, 60)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = 10;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    // This scenario is a new Primary building the old Primary with no progress.
    // In this case, the Copy Mode is Partial because the UpdateEpochLogRecord needs to be copied.
    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_SecondaryMissingLastVector_MatchingLsn_PartialCopyOnlyUpdateEpochLogRecord)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_SecondaryMissingLastVector_MatchingLsn_PartialCopyOnlyUpdateEpochLogRecord")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = 720;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = 720;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 720;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 720;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_None_SameProgressVector_SameLsn_None)
    {
        TEST_TRACE_BEGIN("FindCopyMode_None_SameProgressVector_SameLsn_None")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::None;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720));
            ULONG const expectedSourceIndex = 2;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720));
            ULONG const expectedTargetIndex = 2;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 720;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 720;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 700)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 2, 720)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FalseProgressDueToProgressInUnknownEpoch1)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FalseProgressDueToProgressInUnknownEpoch1")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial | CopyModeFlag::Enum::FalseProgress;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = 2711;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = 2711;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 2713;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 2712)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 2712;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 2711)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FalseProgressDueToProgressInUnknownEpoch2)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FalseProgressDueToProgressInUnknownEpoch2")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Partial | CopyModeFlag::Enum::FalseProgress;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = 2711;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = 2711;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 2712;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 2711)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 2712;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 2710)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 2711)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_DoubleFalseProgressDetected)
    {
        TEST_TRACE_BEGIN("FindCopyMode_DoubleFalseProgressDetected")
        
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Expected output
            int expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            expectedCopyMode |= CopyModeFlag::Enum::FalseProgress;

            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 677));
            const int expectedSourceIndex = 1;
            LONG64 expectedSourceStartingLsn = 687;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 677));
            const int expectedTargetIndex = 1;
            LONG64 expectedTargetStartingLsn = 687;

            // Setup / Input
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 677)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 687)));

            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 687;

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);

            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 677)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 687)));

            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 688;

            LONG64 lastRecoredAtomicOperationLsn = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters,
                targetCopyContextParameters,
                lastRecoredAtomicOperationLsn);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_DoubleFalseProgressDetected2)
    {
        TEST_TRACE_BEGIN("FindCopyMode_DoubleFalseProgressDetected2")
        
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Expected output
            int expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            expectedCopyMode |= CopyModeFlag::Enum::FalseProgress;

            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 62));
            const int expectedSourceIndex = 1;
            LONG64 expectedSourceStartingLsn = 68;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 62));
            const int expectedTargetIndex = 1;
            LONG64 expectedTargetStartingLsn = 68;

            // Setup / Input
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 62)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 68)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 1, 68)));

            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 68;

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);

            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 62)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 68)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 4, 1, 73)));

            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 73;

            LONG64 lastRecoredAtomicOperationLsn = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters,
                targetCopyContextParameters,
                lastRecoredAtomicOperationLsn);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_DoubleFalseProgressDetected3)
    {
        TEST_TRACE_BEGIN("FindCopyMode_DoubleFalseProgressDetected3")
        
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Expected output
            int expectedCopyMode = CopyModeFlag::Enum::Partial;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;
            expectedCopyMode |= CopyModeFlag::Enum::FalseProgress;

            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10));
            const int expectedSourceIndex = 1;
            LONG64 expectedSourceStartingLsn = 15;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10));
            const int expectedTargetIndex = 1;
            LONG64 expectedTargetStartingLsn = 15;

            // Setup / Input
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 1, 17)));

            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 18;

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);

            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 4, 1, 15)));

            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 17;

            LONG64 lastRecoredAtomicOperationLsn = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters,
                targetCopyContextParameters,
                lastRecoredAtomicOperationLsn);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopyWouldEndUpCasuingAssertInFutureCopyAndHenceDoFullCopy)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopyWouldEndUpCasuingAssertInFutureCopyAndHenceDoFullCopy")

        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
           
            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Other;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 0));
            ULONG const expectedSourceIndex = 1;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 0));
            ULONG const expectedTargetIndex = 1;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 11;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 5;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 2, 1, 5)));
            
            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters, 
                targetCopyContextParameters, 
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_PartialCopy_ProgressVectorsTrimmed)
    {
        TEST_TRACE_BEGIN("FindCopyMode_PartialCopy_ProgressVectorsTrimmed")
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Source Trimmed / Target NOT Trimmed Scenario
            {
                // Expected Values
                LONG64 expectedSourceStartingLsn = 10;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 7));
                LONG64 expectedSourceIndex = 0;

                LONG64 expectedTargetStartingLsn = 9;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7));
                LONG64 expectedTargetIndex = 2;

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 7)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 12)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 1, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 6)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = expectedTargetStartingLsn;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Partial;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }

            // Source NOT Trimmed / Target Trimmed Scenario
            {
                // Expected Values
                LONG64 expectedSourceStartingLsn = 15;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12));
                LONG64 expectedSourceIndex = 4;

                LONG64 expectedTargetStartingLsn = 9;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12));
                LONG64 expectedTargetIndex = 1;

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 0, 8)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = expectedTargetStartingLsn;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Partial;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }

            // Source Trimmed / Target Trimmed Scenario
            {
                // Expected Values
                LONG64 expectedSourceStartingLsn = 13;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12));
                LONG64 expectedSourceIndex = 2;

                LONG64 expectedTargetStartingLsn = 9;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12));
                LONG64 expectedTargetIndex = 1;

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = expectedTargetStartingLsn;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Partial;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopyWithProgressVectorTrimmed_ProgressVectorsTrimmed)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopyWithProgressVectorTrimmed_ProgressVectorsTrimmed")
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);
            
            // Source Trimmed / Target NOT Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 12)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 1, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 6)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 9;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::ProgressVectorTrimmed;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }

            // Source NOT Trimmed / Target Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 0, 8)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 1, 4)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 6, 1, 5)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 9;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::ProgressVectorTrimmed;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }

            // Source Trimmed / Target Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 6, 1, 10)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 7, 1, 11)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 9;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::ProgressVectorTrimmed;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopyWithInsufficientLogs_ProgressVectorsTrimmed)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopyWithInsufficientLogs_ProgressVectorsTrimmed")
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Source Trimmed / Target NOT Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 7)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 12)));
                

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 1, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 6)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 3;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::InsufficientLogs;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }

            // Source NOT Trimmed / Target Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 0, 8)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 15)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 20;
                LONG64 sourceCurrentTailLsn = 25;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 9;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::InsufficientLogs;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }

            // Source Trimmed / Target Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 20;
                LONG64 targetCurrentTailLsn = 25;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::InsufficientLogs;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopyWithDataLoss_ProgressVectorsTrimmed)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopyWithDataLoss_ProgressVectorsTrimmed")
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Source Trimmed / Target NOT Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 7)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 0, 12)));


                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 1, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 6)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 3;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }

            // Source NOT Trimmed / Target Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 0, 8)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 0, 15)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 20;
                LONG64 sourceCurrentTailLsn = 25;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 9;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }

            // Source Trimmed / Target Trimmed Scenario
            {
                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 6, 0, 14)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 20;
                LONG64 targetCurrentTailLsn = 25;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(Constants::InvalidLsn, result.TargetStartingLsn);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FalseProgress_ProgressVectorsTrimmed)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FalseProgress_ProgressVectorsTrimmed")
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Source Trimmed / Target NOT Trimmed Scenario
            {
                // Expected Values
                LONG64 expectedSourceStartingLsn = 10;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 7));
                LONG64 expectedSourceIndex = 0;

                LONG64 expectedTargetStartingLsn = 20;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7));
                LONG64 expectedTargetIndex = 2;

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 7)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 12)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 1, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 1, 6)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 1, 7)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = expectedTargetStartingLsn;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::FalseProgress | CopyModeFlag::Enum::Partial;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }

            // Source NOT Trimmed / Target Trimmed Scenario
            {
                // Expected Values
                LONG64 expectedSourceStartingLsn = 14;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12));
                LONG64 expectedSourceIndex = 4;

                LONG64 expectedTargetStartingLsn = 14;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12));
                LONG64 expectedTargetIndex = 1;

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 1, 0, 8)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 10)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 0, 11)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 7, 0, 15)));


                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 25;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 6, 1, 14)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 15;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::FalseProgress | CopyModeFlag::Enum::Partial;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }

            // Source Trimmed / Target Trimmed Scenario
            {
                // Expected Values
                LONG64 expectedSourceStartingLsn = 13;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12));
                LONG64 expectedSourceIndex = 0;

                LONG64 expectedTargetStartingLsn = 25;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12));
                LONG64 expectedTargetIndex = 1;

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                // Setup / Input
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 12)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 6, 0, 14)));

                Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 15;

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 3, 1, 11)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 1, 12)));

                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = 0;
                LONG64 targetCurrentTailLsn = 25;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                int const expectedCopyMode = CopyModeFlag::Enum::FalseProgress | CopyModeFlag::Enum::Partial;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::Invalid;

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_ValidationFailure)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_ValidationFailure")

        {
#ifndef DBG
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // FailureLsn incremented must be <= 1
            {
                // Expected output
                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::ValidationFailed;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13));
                ULONG const expectedSourceIndex = 2;
                LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13));
                ULONG const expectedTargetIndex = 2;
                LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

                // Setup/input 
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                Epoch sourceLogHeadEpoch(4, 2);
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 50;
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 4)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 6, 0, 14)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(10, 11, 0, 50)));

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = Constants::ZeroLsn;
                LONG64 targetCurrentTailLsn = Constants::OneLsn;
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 3)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 6, 0, 15)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 7, 0, 51)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 8, 0, 52)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(10, 11, 0, 53)));

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
            }

            // SourceEntry != TargetEntry Validation Failure
            {
                // Expected output
                int const expectedCopyMode = CopyModeFlag::Enum::Full;
                FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::ValidationFailed;
                ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13));
                ULONG const expectedSourceIndex = 2;
                LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
                ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13));
                ULONG const expectedTargetIndex = 2;
                LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;
                std::string expectedFailedValidationMsg = "sourceEntry == targetEntry";

                // Setup/input 
                ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
                Epoch sourceLogHeadEpoch(4, 2);
                LONG64 sourceLogHeadLsn = 5;
                LONG64 sourceCurrentTailLsn = 50;
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 2, 0, 4)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));
                sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(10, 10, 1, 50)));

                ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
                Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
                LONG64 targetLogHeadLsn = Constants::ZeroLsn;
                LONG64 targetCurrentTailLsn = Constants::OneLsn;
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 4, 0, 3)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 5, 0, 13)));
                targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(10, 11, 1, 50)));

                LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

                CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
                CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

                CopyModeResult result = ProgressVector::FindCopyMode(
                    sourceCopyContextParameters,
                    targetCopyContextParameters,
                    lastRecoveredAtomicRedoOperationAtTarget);

                VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
                VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
                VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
                VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
                VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
                VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
                VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);

                VERIFY_ARE_EQUAL(expectedFailedValidationMsg, result.SharedProgressVector.FailedValidationMsg);
            }
#endif
        }
    }

    BOOST_AUTO_TEST_CASE(FindCopyMode_FullCopy_TargetHavingDataLossAfterSharedPoint)
    {
        TEST_TRACE_BEGIN("FindCopyMode_FullCopy_TargetHavingDataLossAfterSharedPoint")
        {
            invalidRecords_ = InvalidLogRecords::Create(allocator);
            CODING_ERROR_ASSERT(invalidRecords_ != nullptr);

            // Expected output
            int const expectedCopyMode = CopyModeFlag::Enum::Full;
            FullCopyReason::Enum const expectedFullCopyReason = FullCopyReason::Enum::DataLoss;
            ProgressVectorEntry expectedSourceProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10));
            ULONG const expectedSourceIndex = 2;
            LONG64 const expectedSourceStartingLsn = Constants::InvalidLsn;
            ProgressVectorEntry expectedTargetProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10));
            ULONG const expectedTargetIndex = 2;
            LONG64 const expectedTargetStartingLsn = Constants::InvalidLsn;

            // Setup/input 
            ProgressVector::SPtr sourceProgressVector = ProgressVector::Create(allocator);
            Epoch sourceLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 sourceLogHeadLsn = Constants::ZeroLsn;
            LONG64 sourceCurrentTailLsn = 12;
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 0)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10)));
            sourceProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(2, 7, 1, 12)));

            ProgressVector::SPtr targetProgressVector = ProgressVector::Create(allocator);
            Epoch targetLogHeadEpoch = Epoch::ZeroEpoch();
            LONG64 targetLogHeadLsn = Constants::ZeroLsn;
            LONG64 targetCurrentTailLsn = 17;
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(0, 0, 0, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 1, 1, 0)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 3, 1, 10)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 4, 1, 12)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 5, 1, 15)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(1, 6, 1, 16)));
            targetProgressVector->Append(ProgressVectorEntry(*CreateUpdateEpochLogRecord(2, 7, 1, 17)));

            LONG64 lastRecoveredAtomicRedoOperationAtTarget = Constants::InvalidLsn;

            CopyContextParameters sourceCopyContextParameters(*sourceProgressVector, sourceLogHeadEpoch, sourceLogHeadLsn, sourceCurrentTailLsn);
            CopyContextParameters targetCopyContextParameters(*targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, targetCurrentTailLsn);

            CopyModeResult result = ProgressVector::FindCopyMode(
                sourceCopyContextParameters,
                targetCopyContextParameters,
                lastRecoveredAtomicRedoOperationAtTarget);

            VERIFY_ARE_EQUAL(expectedCopyMode, result.CopyModeEnum);
            VERIFY_ARE_EQUAL(expectedFullCopyReason, result.FullCopyReasonEnum);
            VERIFY_ARE_EQUAL(expectedSourceProgressVectorEntry, result.SharedProgressVector.SourceProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedSourceIndex, result.SharedProgressVector.SourceIndex);
            VERIFY_ARE_EQUAL(expectedSourceStartingLsn, result.SourceStartingLsn);
            VERIFY_ARE_EQUAL(expectedTargetProgressVectorEntry, result.SharedProgressVector.TargetProgressVectorEntry);
            VERIFY_ARE_EQUAL(expectedTargetIndex, result.SharedProgressVector.TargetIndex);
            VERIFY_ARE_EQUAL(expectedTargetStartingLsn, result.TargetStartingLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_HeadEpochGreaterThanLastBackedUpEpoch)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_HeadEpochGreaterThanLastBackedUpEpoch")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch(200, 200), Epoch(500, 500));

            VERIFY_ARE_EQUAL(pv->Length, 1501);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }
    
    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_HeadEpochSmallerThanLastBackedUpEpoch)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_HeadEpochSmallerThanLastBackedUpEpoch")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch(500, 500), Epoch(200, 200));

            VERIFY_ARE_EQUAL(pv->Length, 1501);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_InvalidLastBackedUpEpoch)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_InvalidLastBackedUpEpoch")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch::InvalidEpoch(), Epoch(500, 500));

            VERIFY_ARE_EQUAL(pv->Length, 1501);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_InvalidLastBackedUpEpochAndHeadEpoch)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_InvalidLastBackedUpEpochAndHeadEpoch")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch::InvalidEpoch(), Epoch::InvalidEpoch());

            VERIFY_ARE_EQUAL(pv->Length, 2000);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_LastBackedUpEpoch_SomewhereInMiddle)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_LastBackedUpEpoch_SomewhereInMiddle")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch(500, 500), Epoch::ZeroEpoch());

            VERIFY_ARE_EQUAL(pv->Length, 1501);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_LastBackedUpEpoch_AtEnd)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_LastBackedUpEpoch_AtEnd")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch(2000, 2000), Epoch::ZeroEpoch());

            VERIFY_ARE_EQUAL(pv->Length, 1);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_LastBackedUpEpoch_AtBegin)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_LastBackedUpEpoch_AtBegin")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);
            pv->Test_SetProgressVectorMaxEntries(1000);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch(2, 2), Epoch::ZeroEpoch());

            VERIFY_ARE_EQUAL(pv->Length, 1999);
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.CurrentEpoch, Epoch(2000, 2000));
            VERIFY_ARE_EQUAL(pv->LastProgressVectorEntry.Lsn, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVectorTrim_DisabledFlag)
    {
        TEST_TRACE_BEGIN("ProgressVectorTrim_DisabledFlag")
        {
            ProgressVector::SPtr pv = ProgressVector::Create(allocator);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;
            for (int i = 0; i < 2000; i++)
            {
                pv->Append(ProgressVectorEntry(Epoch(++dataLossNumber, ++configurationNumber), ++lsn, 0, Common::DateTime::Now()));
            }
            pv->TrimProgressVectorIfNeeded(Epoch(1000, 1000), Epoch::ZeroEpoch());

            VERIFY_ARE_EQUAL(pv->Length, 2000);
        }
    }

    BOOST_AUTO_TEST_CASE(ProgressVector_ToString_Size_Validation)
    {
        TEST_TRACE_BEGIN("ProgressVector_ToString_Size_Validation")
        {
            ProgressVector::SPtr pvLarge = ProgressVector::Create(allocator);
            ProgressVector::SPtr pvSmall = ProgressVector::Create(allocator);

            LONG64 dataLossNumber = 0;
            LONG64 configurationNumber = 0;
            LONG64 lsn = 0;

            int largeProgressVectorLength = 2000;
            int smallProgressVectorLength = 10;

            // Populate a progress vector with entries totaling to > ProgressVectorMaxStringSizeInKb
            for (int i = 0; i < largeProgressVectorLength; i++)
            {
                pvLarge->Append(
                    ProgressVectorEntry(
                        Epoch(
                            ++dataLossNumber,
                            ++configurationNumber),
                        ++lsn,
                        0,
                        Common::DateTime::Now()));
            }

            dataLossNumber = 0;
            configurationNumber = 0;
            lsn = 0;

            // Populate a progress vector with entries smaller than ProgressVectorMaxStringSizeInKb / 2
            for (int i = 0; i < smallProgressVectorLength; i++)
            {
                pvSmall->Append(
                    ProgressVectorEntry(
                        Epoch(
                            ++dataLossNumber,
                            ++configurationNumber),
                        ++lsn,
                        0,
                        Common::DateTime::Now()));
            }

            VERIFY_ARE_EQUAL(pvLarge->Length, (ULONG)largeProgressVectorLength);

            std::wstring failedValidationCopyStr = L"Validation failed, starting full copy. {0}. \r\nTargetIndex = {1}, TargetEntry = {2} \r\nSourceIndex = {3}, SourceEntry = {4}\r\nTarget : {5}\r\nSource : {6}";

            // 64KB max trace string size
            ULONG64 maxTraceStringSizeInBytes = 65536;

            // 90% of max trace size should be utilized if vector entries are greater than total
            ULONG64 minTargetTraceSize = 58982;

            std::wstring validationStr;

            // Default string, printing backwards 
            validationStr = pvLarge->ToString();

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Default string, printing backwards | {1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 1
            // Target entry index starting at 1/4 total vector entry list
            validationStr = pvLarge->ToString(
                largeProgressVectorLength / 4,
                Constants::ProgressVectorMaxStringSizeInKb);

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);
            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) > minTargetTraceSize);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Target entry index starting at 1/4 total vector entry list | {1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 2
            // Target entry index starting at 1/2 total vector entry list
            validationStr = pvLarge->ToString(
                largeProgressVectorLength / 2,
                Constants::ProgressVectorMaxStringSizeInKb);

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);
            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) > minTargetTraceSize);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Target entry index starting at 1/2 total vector entry list | {1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 3
            // Target entry index starting at 3/4 total vector entry list
            validationStr = pvLarge->ToString(
                3 * (largeProgressVectorLength / 4),
                Constants::ProgressVectorMaxStringSizeInKb);

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);
            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) > minTargetTraceSize);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Target entry index starting at 3/4 total vector entry list | {1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 4
            // Target entry index starting at random index in vector entry list
            validationStr = pvLarge->ToString(
                r.Next(0, largeProgressVectorLength - 1),
                Constants::ProgressVectorMaxStringSizeInKb);

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);
            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) > minTargetTraceSize);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Target entry index starting at random index in vector entry list | {1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 5
            // Verify length of failed validation string traced in CheckpointManager
            // Validation : sourceEntry == targetEntry
            int targetIndex = r.Next(1, largeProgressVectorLength);
            int sourceIndex = r.Next(1, largeProgressVectorLength);
            ProgressVectorEntry targetEntry = pvLarge->Find(Epoch(targetIndex, targetIndex));
            ProgressVectorEntry sourceEntry = pvLarge->Find(Epoch(sourceIndex, sourceIndex));

            validationStr = Common::wformatString(
                failedValidationCopyStr,
                L"sourceentry == targetentry",
                targetIndex,
                targetEntry.ToString(),
                sourceIndex,
                sourceEntry.ToString(),
                pvLarge->ToString(
                    targetIndex,
                    Constants::ProgressVectorMaxStringSizeInKb / 2),
                pvLarge->ToString(
                    sourceIndex,
                    Constants::ProgressVectorMaxStringSizeInKb / 2));

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);
            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) > minTargetTraceSize);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Failed validation string \r\n{1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 6
            // Verify length of failed validation string traced in CheckpointManager
            // Validation : FailureLsn incremented must be <= 1. It is {0}
            targetIndex = r.Next(1, largeProgressVectorLength);
            sourceIndex = r.Next(1, largeProgressVectorLength);
            targetEntry = pvLarge->Find(Epoch(targetIndex, targetIndex));
            sourceEntry = pvLarge->Find(Epoch(sourceIndex, sourceIndex));

            validationStr = Common::wformatString(
                failedValidationCopyStr,
                L"FailureLsn incremented must be <= 1. It is 42",
                targetIndex,
                targetEntry.ToString(),
                sourceIndex,
                sourceEntry.ToString(),
                pvLarge->ToString(
                    targetIndex,
                    Constants::ProgressVectorMaxStringSizeInKb / 2),
                pvLarge->ToString(
                    sourceIndex,
                    Constants::ProgressVectorMaxStringSizeInKb / 2));

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);
            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) > minTargetTraceSize);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Failed validation string \r\n{1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            // Case 7
            // Verify functionality when target + source vectors are smaller than 
            // allowed output
            targetIndex = r.Next(1, smallProgressVectorLength);
            sourceIndex = r.Next(1, smallProgressVectorLength);
            ProgressVectorEntry targetEntrySmall = pvSmall->Find(Epoch(targetIndex, targetIndex));
            ProgressVectorEntry sourceEntrySmall = pvSmall->Find(Epoch(sourceIndex, sourceIndex));

            validationStr = Common::wformatString(
                failedValidationCopyStr,
                L"FailureLsn incremented must be <= 1. It is 42",
                targetIndex,
                targetEntrySmall.ToString(),
                sourceIndex,
                sourceEntrySmall.ToString(),
                pvSmall->ToString(
                    targetIndex,
                    Constants::ProgressVectorMaxStringSizeInKb / 2),
                pvSmall->ToString(
                    sourceIndex,
                    Constants::ProgressVectorMaxStringSizeInKb / 2));

            VERIFY_IS_TRUE(validationStr.length() * sizeof(char) < maxTraceStringSizeInBytes);

            Trace.WriteInfo(
                TraceComponent,
                "{0} Failed validation string \r\n{1} length = {2}",
                prId_->TraceId,
                validationStr,
                validationStr.length());

            /*
                Sample output:
                Validation failed, starting full copy. FailureLsn incremented must be <= 1. It is 42.
                TargetIndex = 8, TargetEntry = [(8,8), 8, 0, 131462881345027451]
                SourceIndex = 6, SourceEntry = [(6,6), 6, 0, 131462881345027451]
                Target :
                [(10,10), 10, 0, 131462881345027451]
                        [(9,9), 9, 0, 131462881345027451]
                        [(8,8), 8, 0, 131462881345027451]
                        [(7,7), 7, 0, 131462881345027451]
                        [(6,6), 6, 0, 131462881345027451]
                        [(5,5), 5, 0, 131462881345027451]
                        [(4,4), 4, 0, 131462881345027451]
                        [(3,3), 3, 0, 131462881345027451]
                        [(2,2), 2, 0, 131462881345027451]
                        [(1,1), 1, 0, 131462881345027451]

                Source :
                [(10,10), 10, 0, 131462881345027451]
                        [(9,9), 9, 0, 131462881345027451]
                        [(8,8), 8, 0, 131462881345027451]
                        [(7,7), 7, 0, 131462881345027451]
                        [(6,6), 6, 0, 131462881345027451]
                        [(5,5), 5, 0, 131462881345027451]
                        [(4,4), 4, 0, 131462881345027451]
                        [(3,3), 3, 0, 131462881345027451]
                        [(2,2), 2, 0, 131462881345027451]
                        [(1,1), 1, 0, 131462881345027451]
            */
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
