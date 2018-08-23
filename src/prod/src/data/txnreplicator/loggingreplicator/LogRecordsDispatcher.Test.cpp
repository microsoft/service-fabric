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
    
    StringLiteral const TraceComponent = "LogDispatcherTest";

    class LogRecordsDispatcherTests 
    {
    protected:
        LogRecordsDispatcherTests() 
        {
            CommonConfig config; // load the config object as its needed for the tracing to work
        }

        void EndTest();

        TestOperationProcessor::SPtr logProcessor_;

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void LogRecordsDispatcherTests::EndTest()
    {
        prId_.Reset();
        logProcessor_->Close();
        logProcessor_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(LogRecordsDispatcherTestsSuite, LogRecordsDispatcherTests)

    BOOST_AUTO_TEST_CASE(VerifyBVT)
    {
        TEST_TRACE_BEGIN("VerifyBVT")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(1, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 0, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::Create(*prId_, 2, 20, seed, settings, allocator);
            ULONG currentBarrierCount = logProcessor_->BarrierCount;

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(currentBarrierCount + 1, TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifyBasicFunctionality)
    {
        TEST_TRACE_BEGIN("VerifyBasicFunctionality")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(4, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 2, 0, 1, 3, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::Create(*prId_, 2, 40, seed, settings, allocator);
            ULONG currentBarrierCount = logProcessor_->BarrierCount;

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(currentBarrierCount + 1, TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    BOOST_AUTO_TEST_CASE(Verify2ConsecutiveBarriers)
    {
        TEST_TRACE_BEGIN("Verify2ConsecutiveBarriers")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(4, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 2, 0, 2, 1, seed, allocator));
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 0, 0, 1, 1, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::Create(*prId_, 2, 40, seed, settings, allocator);
            ULONG currentBarrierCount = logProcessor_->BarrierCount;

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(currentBarrierCount + 2, TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 2);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    BOOST_AUTO_TEST_CASE(TestNoBarrierExpectsNoProcessing)
    {
        TEST_TRACE_BEGIN("TestNoBarrierExpectsNoProcessing")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::CreateWithParallelDispatcher(*prId_, 2, 40, seed, settings, allocator);

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = logProcessor_->WaitForProcessingToComplete(txnRecords.Count(), TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, false);
            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 0);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, 0);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 0);
        }
    }

    BOOST_AUTO_TEST_CASE(TestOneBarrierExpectsOneImmediateProcessing)
    {
        TEST_TRACE_BEGIN("TestOneBarrierExpectsOneImmediateProcessing")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 2, 0, 1, 1, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::Create(*prId_, 2, 30, seed, settings, allocator);

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = logProcessor_->WaitForProcessingToComplete(txnRecords.Count(), TimeSpan::FromSeconds(2));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(1, TimeSpan::FromSeconds(2));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 1);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    BOOST_AUTO_TEST_CASE(TestTwoBarrierExpectsTwoImmediateProcessing)
    {
        TEST_TRACE_BEGIN("TestTwoBarrierExpectsTwoImmediateProcessing")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 1, 1, seed, allocator));
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 1, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::CreateWithParallelDispatcher(*prId_, 1, 50, seed, settings, allocator);

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = logProcessor_->WaitForProcessingToComplete(txnRecords.Count(), TimeSpan::FromSeconds(4));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(2, TimeSpan::FromSeconds(2));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 2);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, txnRecords.Count() - 2);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);
        }
    }

    BOOST_AUTO_TEST_CASE(TestPauseContinue)
    {
        TEST_TRACE_BEGIN("TestPauseContinue")

        {
            KArray<TestTransaction::SPtr> testTxList1(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList1.Append(TestTransactionGenerator::Create(2, true, allocator));
            testTxList1.Append(TestTransactionGenerator::Create(2, true, allocator));
            KArray<LogRecord::SPtr> txnRecords1 = TestTransactionGenerator::InterleaveTransactions(testTxList1, lsn, seed, allocator, lsn);
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords1, 2, 0, 1, 1, seed, allocator));

            KArray<TestTransaction::SPtr> testTxList2(allocator);
            testTxList2.Append(TestTransactionGenerator::Create(2, true, allocator));
            testTxList2.Append(TestTransactionGenerator::Create(2, true, allocator));
            KArray<LogRecord::SPtr> txnRecords2 = TestTransactionGenerator::InterleaveTransactions(testTxList2, lsn, seed, allocator, lsn);
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords2, 2, 0, 1, 1, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::CreateWithParallelDispatcher(*prId_, 50, 100, seed, settings, allocator);

            LoggedRecords::SPtr loggedRecords1 = LoggedRecords::Create(txnRecords1, allocator);
            LoggedRecords::SPtr loggedRecords2 = LoggedRecords::Create(txnRecords2, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords1);
            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords2);

            // this will sleep until at least dispatching started
            Sleep(100);
            auto tcs = logProcessor_->RecordsDispatcher.DrainAndPauseDispatchAsync();
            SyncAwait(tcs);

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(1, TimeSpan::FromSeconds(3));
            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 1);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, txnRecords1.Count() - 1);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);

            Sleep(1000);

            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 1);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, txnRecords1.Count() - 1);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);

            logProcessor_->RecordsDispatcher.ContinueDispatch();

            isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(2, TimeSpan::FromSeconds(3));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 2);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, txnRecords1.Count() - 1 + txnRecords2.Count() - 1);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 2);

            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    BOOST_AUTO_TEST_CASE(TestPauseContinueWithoutAnyRecords)
    {
        TEST_TRACE_BEGIN("TestPauseContinueWithoutAnyRecords")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, true, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 3, 0, 1, 1, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::CreateWithParallelDispatcher(*prId_, 2, 3, seed, settings, allocator);
            auto tcs = logProcessor_->RecordsDispatcher.DrainAndPauseDispatchAsync();
            SyncAwait(tcs);

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            logProcessor_->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);

            Sleep(500);

            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 0);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, 0);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 0);

            logProcessor_->RecordsDispatcher.ContinueDispatch();

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(1, TimeSpan::FromSeconds(2));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            VERIFY_ARE_EQUAL(logProcessor_->ImmediateCalledCount, 1);
            VERIFY_ARE_EQUAL(logProcessor_->NormalCalledCount, txnRecords.Count() - 1);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    /* DISABLED As APPVERIFIER RUNS FAIL With high memory usage
    BOOST_AUTO_TEST_CASE(TenThousandRecords)
    {
        TEST_TRACE_BEGIN("TenThousandRecords")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;
            int totalTxCount = 10000;

            for (int count = 0; count < totalTxCount; count++)
            {
                testTxList.Append(TestTransactionGenerator::Create(3, true, allocator));
            }

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, totalTxCount, 0, 1000, 3, seed, allocator));

            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::Create(*prId_, 0, 0, seed, settings, allocator); // No delay as processing will take long time
            ULONG currentBarrierCount = logProcessor_->BarrierCount;

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            TestOperationProcessor::SPtr localProcessor = logProcessor_;

            Threadpool::Post([localProcessor, loggedRecords] {
                localProcessor->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);
            });

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(currentBarrierCount + 1, TimeSpan::FromSeconds(10));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(logProcessor_->UpdateDipsatchedBarrierTaskCount, 1);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }

    BOOST_AUTO_TEST_CASE(TwoThousandBarriersCausingStackOverFlow)
    {
        TEST_TRACE_BEGIN("TwoThousandBarriersCausingStackOverFlow")

        {
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;
            int txCountPerBarrier = 2;
            int barrierCount = 2000;
            TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
            logProcessor_ = TestOperationProcessor::Create(*prId_, 0, 0, seed, settings, allocator); // No delay as processing will take long time

            Stopwatch s;
            s.Start();

            for (int i = 0; i < barrierCount; i++)
            {
                for (int count = 0; count < txCountPerBarrier; count++)
                {
                    testTxList.Append(TestTransactionGenerator::Create(3, true, allocator));
                }

                KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

                expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, txCountPerBarrier, 0, 0, 3, seed, allocator));
                LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

                TestOperationProcessor::SPtr localProcessor = logProcessor_;

                localProcessor->RecordsDispatcher.DispatchLoggedRecords(*loggedRecords);
            }

            bool isProcessingComplete = logProcessor_->WaitForBarrierProcessingToComplete(barrierCount, TimeSpan::FromSeconds(500));
            s.Stop();

            Trace.WriteWarning(TraceComponent, "It took {0} milliseconds to finish dispatching all barriers", s.ElapsedMilliseconds);

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(TestGroupCommitValidationResult::Compare(expectedResults, logProcessor_->GroupCommits), true);
        }
    }*/

    BOOST_AUTO_TEST_SUITE_END()
}
