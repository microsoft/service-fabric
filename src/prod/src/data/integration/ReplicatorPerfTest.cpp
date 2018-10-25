// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define PERF_TEST

namespace Data
{
    namespace Integration
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;
        using namespace Data::TStore;
        using namespace TxnReplicator;

        StringLiteral const TraceComponent("ReplicatorPerfTest");
        std::wstring const TestLogFileName(L"ReplicatorPerfTest.log");

        class ReplicatorPerfTest
        {
        public:
            Awaitable<void> Run(
                __in wstring const & workFolder,
                __in int concurrentTransactions,
                __in int totalTransactions,
                __in Data::Log::LogManager & logManager);

        protected:
            wstring CreateFileName(
                __in wstring const & folderName);

            void InitializeKtlConfig(
                __in std::wstring workDir,
                __in std::wstring fileName,
                __in KAllocator & allocator,
                __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings);

            KUri::CSPtr GetStateProviderName(
                __in int stateProviderIndex);

            void EndTest();

            Awaitable<void> DoSingleOutstandingTaskWork(
                __in TxnReplicator::TestCommon::NoopStateProvider::SPtr sp,
                __in Replica::SPtr replica,
                __in int numberofUpdates,
                __in int size);

            Awaitable<void> AddOperation(
                __in TxnReplicator::TestCommon::NoopStateProvider::SPtr sp,
                __in Replica::SPtr replica,
                __in Data::Utilities::OperationData const & data);

            CommonConfig config; // load the config object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;

            KGuid pId_;
            FABRIC_REPLICA_ID rId_;
            Common::atomic_long txCount_;
        };

        Awaitable<void> ReplicatorPerfTest::DoSingleOutstandingTaskWork(
            __in TxnReplicator::TestCommon::NoopStateProvider::SPtr sp,
            __in Replica::SPtr replica,
            __in int numberofUpdates,
            __in int size)
        {
            Data::Utilities::OperationData::SPtr data = Data::Utilities::OperationData::Create(sp->GetThisAllocator());
            KBuffer::SPtr b;
            KBuffer::Create(size, b, sp->GetThisAllocator());
            data->Append(*b);

            for (int i = 0; i < numberofUpdates; i++)
            {
                co_await AddOperation(sp, replica, *data);
                txCount_++;
            }

            co_return;
        }

        Awaitable<void> ReplicatorPerfTest::AddOperation(
            __in TxnReplicator::TestCommon::NoopStateProvider::SPtr sp,
            __in Replica::SPtr replica,
            __in Data::Utilities::OperationData const & data)
        {
            TxnReplicator::Transaction::SPtr innerTx;
            replica->TxnReplicator->CreateTransaction(innerTx);

            sp->AddOperation(
                *innerTx,
                &data,
                nullptr,
                nullptr,
                nullptr);

            co_await innerTx->CommitAsync();

            innerTx->Dispose();

            co_return;
        }

        Awaitable<void> ReplicatorPerfTest::Run(
            __in wstring const & testFolder,
            __in int concurrentTransactions,
            __in int totalTransactions,
            __in Data::Log::LogManager & logManager)
        {
#ifndef PERF_TEST
            UNREFERENCED_PARAMETER(testFolder);
            UNREFERENCED_PARAMETER(concurrentTransactions);
            UNREFERENCED_PARAMETER(totalTransactions);
            UNREFERENCED_PARAMETER(logManager);
#else
            Replica::SPtr replica = Replica::Create(
                pId_,
                rId_,
                testFolder,
                logManager,
                underlyingSystem_->PagedAllocator(),
                TxnReplicator::TestCommon::NoopStateProviderFactory::Create(underlyingSystem_->PagedAllocator()).RawPtr());

            underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE);

            co_await replica->OpenAsync();

            FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
            co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

            replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
            replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

            KUri::CSPtr stateProviderName = GetStateProviderName(0);
            {
                Transaction::SPtr txn;
                replica->TxnReplicator->CreateTransaction(txn);
                KFinally([&] {txn->Dispose(); });

                NTSTATUS status = co_await replica->TxnReplicator->AddAsync(*txn, *stateProviderName, TxnReplicator::TestCommon::NoopStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                co_await txn->CommitAsync();
            }

            {
                IStateProvider2::SPtr stateProvider2;
                NTSTATUS status = replica->TxnReplicator->Get(*stateProviderName, stateProvider2);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider2);
                VERIFY_ARE_EQUAL(*stateProviderName, stateProvider2->GetName());

                TxnReplicator::TestCommon::NoopStateProvider::SPtr sp = dynamic_cast<TxnReplicator::TestCommon::NoopStateProvider *>(stateProvider2.RawPtr());

                Sleep(5000);
                Stopwatch s;
                s.Start();

                double opsPerMillisecond = 0;
                double opsPerSecond = 0;

                KArray<Awaitable<void>> tasks(underlyingSystem_->PagedAllocator(), concurrentTransactions, 0);

                for (int i = 0; i < concurrentTransactions; i++)
                {
                    status = tasks.Append(DoSingleOutstandingTaskWork(sp, replica, totalTransactions / concurrentTransactions, i));
                    KInvariant(NT_SUCCESS(status));
                }

                ktl::AwaitableCompletionSource<void>::SPtr waiter;
                ktl::AwaitableCompletionSource<void>::Create(underlyingSystem_->PagedAllocator(), 'abcd', waiter);

                for (;;)
                {
                    auto opCount = txCount_ .load();
                    if (txCount_.load() >= totalTransactions)
                    {
                        break;
                    }

                    opsPerMillisecond = (double)opCount / (double)s.ElapsedMilliseconds;
                    opsPerSecond = 1000 * opsPerMillisecond;

                    Trace.WriteInfo(
                        TraceComponent,
                        "Ops Completed = {0}. OpsPerSecond = {1}",
                        opCount,
                        opsPerSecond);

                    KTimer::SPtr timer;
                    KTimer::Create(timer, underlyingSystem_->PagedAllocator(), 'abcd');
                    co_await timer->StartTimerAsync(1000, nullptr);
                }

                co_await TaskUtilities<Awaitable<void>>::WhenAll(tasks);

                s.Stop();

                int64 txPerSec = ((totalTransactions * 1000) / s.ElapsedMilliseconds);

                Trace.WriteInfo(
                    TraceComponent,
                    "Tx/Sec is {0}",
                    txPerSec);
            }

            replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
            replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

            co_await replica->CloseAsync();
#endif
            co_return;
        }

        wstring ReplicatorPerfTest::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void ReplicatorPerfTest::EndTest()
        {
        }

        void ReplicatorPerfTest::InitializeKtlConfig(
            __in std::wstring workDir,
            __in std::wstring fileName,
            __in KAllocator & allocator,
            __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings)
        {
            auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();

            KString::SPtr sharedLogFileName = KPath::CreatePath(workDir.c_str(), allocator);
            KPath::CombineInPlace(*sharedLogFileName, fileName.c_str());

            if (!Common::Directory::Exists(workDir))
            {
                Common::Directory::Create(workDir);
            }

            KInvariant(sharedLogFileName->LengthInBytes() + sizeof(WCHAR) < 512 * sizeof(WCHAR)); // check to make sure there is space for the null terminator
            KMemCpySafe(&settings->Path[0], 512 * sizeof(WCHAR), sharedLogFileName->operator PVOID(), sharedLogFileName->LengthInBytes());
            settings->Path[sharedLogFileName->LengthInBytes() / sizeof(WCHAR)] = L'\0'; // set the null terminator
            settings->LogContainerId.GetReference().CreateNew();
            settings->LogSize = 1024 * 1024 * 512; // 512 MB.
            settings->MaximumNumberStreams = 0;
            settings->MaximumRecordSize = 0;
            sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
        };

        KUri::CSPtr ReplicatorPerfTest::GetStateProviderName(
            __in int stateProviderIndex)
        {
            wstring stateProviderName = wformatString(L"fabric:/store/{0}", stateProviderIndex);

            KUri::CSPtr spName;
            NTSTATUS status = KUri::Create(KStringView(stateProviderName.c_str()), underlyingSystem_->PagedAllocator(), spName);
            if (NT_SUCCESS(status) == false)
            {
                throw Exception(status);
            }

            return spName;
        }

        BOOST_FIXTURE_TEST_SUITE(ReplicatorPerfTestSuite, ReplicatorPerfTest);

        BOOST_AUTO_TEST_CASE(ThousandTransactions)
        {
            // Setup
            wstring testName(L"ThousandTransactions");
            wstring testFolderPath = CreateFileName(testName);

            // Pre-clean up
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

            TEST_TRACE_BEGIN(testName)
            {
                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(Run(workFolder, 1000, 400000, *logManager));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            Directory::Delete_WithRetry(testFolderPath, true, true);
        }

        BOOST_AUTO_TEST_SUITE_END();
    }
}
