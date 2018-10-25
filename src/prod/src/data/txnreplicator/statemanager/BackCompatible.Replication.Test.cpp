// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace ktl;
using namespace Data;
using namespace Data::StateManager;
using namespace Data::Utilities;
using namespace TxnReplicator;

namespace StateManagerTests
{
    class BackCompatibleReplicationTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        BackCompatibleReplicationTest()
        {
        }

        ~BackCompatibleReplicationTest()
        {
        }

    public:
        // Test cases used for set different kinds of verifications.
        enum TestMode
        {
            Recovery_SP_EmptyOD,
            Recovery_SP_NullOD,
            Recovery_SP_SmallOD,
            Recovery_SP_LargeOD,
            Recovery_SP_DiffOD,
            Recovery_SP_MultiBuffers, 
            Recovery_SP_MultiSPs,
            Replication_SP_EmptyOD,
            Replication_SP_NullOD,
            Replication_SP_SmallOD,
            Replication_SP_LargeOD,
            Replication_SP_DiffOD,
            Replication_SP_MultiBuffers,
            Replication_SP_MultiSPs,
            FalseProgress_SP_NullOD,
            FalseProgress_SP_EmptyOD,
            FalseProgress_SP_LargeOD,
            FalseProgress_SP_DiffOD,
            FalseProgress_SP_MultiSPs,
        }; 

    public:
        ktl::Awaitable<void> Test_Managed_SP_ApplyAddOperation_Recovery(
            __in KWString const & smFile,
            __in KWString const & spFile,
            __in TestMode mode,
            __in_opt OperationData const * const expectedData);

        ktl::Awaitable<void> Test_Managed_SP_ApplyAddOperation_Replication(
            __in KWString const & smFile,
            __in KWString const & spFile,
            __in TestMode mode,
            __in_opt OperationData const * const expectedData);

        ktl::Awaitable<void> Test_Managed_SP_ApplyAddOperation_FalseProgress(
            __in KWString const & smFile,
            __in KWString const & spFile,
            __in TestMode mode,
            __in_opt OperationData const * const expectedData);

    public: //Helper functions
        ktl::Awaitable<void> ReadManagedFileAsync(__in KWString const & fileName);
        ktl::Awaitable<void> CloseAsync();

        ktl::Awaitable<void> ApplyFileOperations(
            __in KWString const & fileName,
            __in LONG64 txnId,
            __in ApplyContext::Enum applyContext,
            __in bool spApply = false);

        void VerifyStateProviderInfo(
            __in KUriView const & name,
            __in FABRIC_STATE_PROVIDER_ID expectedId,
            __in_opt OperationData const * const expectedData,
            __in_opt OperationData const * const expectedData1 = nullptr);

        void TestCaseVerification(
            __in TestMode mode,
            __in_opt OperationData const * const expectedData);

        OperationData::SPtr CreateOperationData(
            __in ULONG32 bufferCount,
            __in ULONG32 bufferSize);

    private:
        KBlockFile::SPtr fileSPtr_;
        ktl::io::KFileStream::SPtr fileStreamSPtr_;
    };

    ktl::Awaitable<void> BackCompatibleReplicationTest::ReadManagedFileAsync(
        __in KWString const & fileName)
    {
        NTSTATUS status = co_await KBlockFile::CreateSparseFileAsync(
            const_cast<KWString &>(fileName),
            FALSE, // IsWriteThrough.
            KBlockFile::eOpenExisting,
            fileSPtr_,
            nullptr,
            GetAllocator(),
            TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = ktl::io::KFileStream::Create(
            fileStreamSPtr_,
            GetAllocator(),
            TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = co_await fileStreamSPtr_->OpenAsync(*fileSPtr_);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        co_return;
    }

    ktl::Awaitable<void> BackCompatibleReplicationTest::CloseAsync()
    {
        co_await fileStreamSPtr_->CloseAsync();
        fileSPtr_->Close();

        co_return;
    }

    // Test the Apply on Recovery path.
    ktl::Awaitable<void> BackCompatibleReplicationTest::Test_Managed_SP_ApplyAddOperation_Recovery(
        __in KWString const & smFile,
        __in KWString const & spFile,
        __in TestMode mode,
        __in_opt OperationData const * const expectedData)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        LONG64 txnId = 1;
        co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::RecoveryRedo);
        co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::RecoveryRedo, true);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        this->TestCaseVerification(mode, expectedData);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    ktl::Awaitable<void> BackCompatibleReplicationTest::Test_Managed_SP_ApplyAddOperation_Replication(
        __in KWString const & smFile,
        __in KWString const & spFile,
        __in TestMode mode,
        __in_opt OperationData const * const expectedData)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

        LONG64 txnId = 1;
        co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::SecondaryRedo);
        co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::SecondaryRedo, true);

        this->TestCaseVerification(mode, expectedData);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    ktl::Awaitable<void> BackCompatibleReplicationTest::Test_Managed_SP_ApplyAddOperation_FalseProgress(
        __in KWString const & smFile,
        __in KWString const & spFile,
        __in TestMode mode,
        __in_opt OperationData const * const expectedData)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

        switch (mode)
        {
        case TestMode::FalseProgress_SP_NullOD:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::SecondaryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501852665045454, nullptr);

            co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::SecondaryFalseProgress, true);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501852665045454, expectedData);
            break;
        }
        case TestMode::FalseProgress_SP_EmptyOD:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::SecondaryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501834118853809, nullptr);

            co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::SecondaryFalseProgress, true);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501834118853809, expectedData);
            break;
        }
        case TestMode::FalseProgress_SP_LargeOD:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::SecondaryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501866868457003, nullptr);

            co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::SecondaryFalseProgress, true);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501866868457003, expectedData);
            break;
        }
        case TestMode::FalseProgress_SP_DiffOD:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::SecondaryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501876739385099, nullptr);

            co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::SecondaryFalseProgress, true);
            OperationData::SPtr expectedOD = CreateOperationData(1, 8);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501876739385099, expectedData, expectedOD.RawPtr());
            break;
        }
        case TestMode::FalseProgress_SP_MultiSPs:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(smFile, txnId, ApplyContext::SecondaryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501896672102998, nullptr);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1", 131501896742254246, nullptr);

            co_await this->ApplyFileOperations(spFile, ++txnId, ApplyContext::SecondaryFalseProgress, true);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501896672102998, expectedData);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1", 131501896742254246, nullptr);
            break;
        }
        default:
            CODING_ERROR_ASSERT(false);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    void BackCompatibleReplicationTest::TestCaseVerification(
        __in TestMode mode,
        __in_opt OperationData const * const expectedData)
    {
        // Verification for different cases.
        switch (mode)
        {
        case TestMode::Recovery_SP_NullOD:
        case TestMode::Replication_SP_NullOD:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501852665045454, expectedData);
            break;
        }
        case TestMode::Recovery_SP_EmptyOD:
        case TestMode::Replication_SP_EmptyOD:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501834118853809, expectedData);
            break;
        }
        case TestMode::Recovery_SP_SmallOD:
        case TestMode::Replication_SP_SmallOD:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501861504915475, expectedData);
            break;
        }
        case TestMode::Recovery_SP_LargeOD:
        case TestMode::Replication_SP_LargeOD:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501866868457003, expectedData);
            break;
        }
        case TestMode::Recovery_SP_DiffOD:
        case TestMode::Replication_SP_DiffOD:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501876739385099, expectedData);
            break;
        }
        case TestMode::Recovery_SP_MultiBuffers:
        case TestMode::Replication_SP_MultiBuffers:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501879846272877, expectedData);
            break;
        }
        case TestMode::Recovery_SP_MultiSPs:
        case TestMode::Replication_SP_MultiSPs:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131501896672102998, expectedData);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1", 131501896742254246, nullptr);
            break;
        }
        default:
            CODING_ERROR_ASSERT(false);
        }
    }

    ktl::Awaitable<void> BackCompatibleReplicationTest::ApplyFileOperations(
        __in KWString const & fileName,
        __in LONG64 txnId,
        __in ApplyContext::Enum applyContext,
        __in bool spApply)
    {
        co_await this->ReadManagedFileAsync(fileName);

        ULONG fileLength = static_cast<ULONG>(fileStreamSPtr_->GetLength());
        fileStreamSPtr_->SetPosition(0);

        KBuffer::SPtr itemStream = nullptr;
        NTSTATUS status = KBuffer::Create(fileLength, itemStream, GetAllocator(), TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ULONG bytesRead = 0;
        status = co_await fileStreamSPtr_->ReadAsync(*itemStream, bytesRead, 0, fileLength);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(bytesRead == fileLength);

        BinaryReader itemReader(*itemStream, GetAllocator());
        CODING_ERROR_ASSERT(NT_SUCCESS(itemReader.Status()));

        // 1. Read Sets Count (Metadata, Redo) of OperationDatas
        LONG32 setCount = 0;
        itemReader.Read(setCount);

        KArray<OperationData::CSPtr> operationSet(GetAllocator(), setCount * 3);
        CODING_ERROR_ASSERT(NT_SUCCESS(operationSet.Status()));

        for (LONG32 i = 0; i < setCount; i++)
        {
            // 2. Read all the OperationDatas, one set of OperationData includes metadata, redo and undo.
            OperationData::CSPtr metadata = nullptr;
            OperationData::CSPtr redo = nullptr;
            OperationData::CSPtr undo = nullptr;
            TestHelper::ReadOperationDatas(itemReader, GetAllocator(), metadata, redo, undo);

            status = operationSet.Append(metadata);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = operationSet.Append(redo);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = operationSet.Append(undo);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        TransactionBase::SPtr txn = TransactionBase::CreateTransaction(txnId, false, GetAllocator());

        auto type = applyContext & ApplyContext::FALSE_PROGRESS;
        vector<TxnReplicator::OperationContext::CSPtr> contextVector;
        for (LONG32 i = 0; i < setCount; i++)
        {
            TxnReplicator::OperationContext::CSPtr operationContext;
            OperationData::CSPtr inputData = nullptr;
            if (type != ApplyContext::FALSE_PROGRESS || !spApply)
            {
                inputData = operationSet[i * 3 + 1];
            }
            else
            {
                inputData = operationSet[i * 3 + 2];
            }

            // In the FlaseProgress case, redo should always be null.
            status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                txnId,
                *txn,
                applyContext,
                operationSet[i * 3].RawPtr(),
                inputData.RawPtr(),
                operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            if (operationContext != nullptr)
            {
                contextVector.push_back(operationContext);
            }
        }

        for (TxnReplicator::OperationContext::CSPtr operationContext : contextVector)
        {
            if (type != ApplyContext::FALSE_PROGRESS)
            {
                status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
        }

        co_await this->CloseAsync();
    }

    void BackCompatibleReplicationTest::VerifyStateProviderInfo(
        __in KUriView const & name,
        __in FABRIC_STATE_PROVIDER_ID expectedId,
        __in_opt OperationData const * const expectedData,
        __in_opt OperationData const * const expectedData1)
    {
        IStateProvider2::SPtr outStateProvider = nullptr;

        NTSTATUS status = testTransactionalReplicatorSPtr_->StateManager->Get(name, outStateProvider);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
        VERIFY_ARE_EQUAL(testStateProvider.StateProviderId, expectedId);
        VERIFY_IS_NULL(testStateProvider.GetInitializationParameters().RawPtr());
        KStringView expectedType = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
        VERIFY_ARE_EQUAL(testStateProvider.GetInputTypeName(), expectedType);

        VERIFY_IS_TRUE(TestHelper::Equals(testStateProvider.UserMetadataOperationData.RawPtr(), expectedData));
        OperationData::CSPtr temp = expectedData1 == nullptr ? expectedData : expectedData1;
        VERIFY_IS_TRUE(TestHelper::Equals(testStateProvider.UserRedoOrUndoOperationData.RawPtr(), temp.RawPtr()));
    }

    OperationData::SPtr BackCompatibleReplicationTest::CreateOperationData(
        __in ULONG32 bufferCount,
        __in ULONG32 bufferSize)
    {
        OperationData::SPtr initParameters = OperationData::Create(GetAllocator());

        if (bufferCount == 0)
        {
            return initParameters;
        }

        for (ULONG32 i = 0; i < bufferCount; i++)
        {
            KBuffer::SPtr expectedBuffer = nullptr;
            NTSTATUS status = KBuffer::Create(bufferSize, expectedBuffer, GetAllocator(), TEST_TAG);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            byte temp[1024] = {};

            for (ULONG32 j = 0; j < bufferSize; j++)
            {
                temp[j] = static_cast<byte>(0);
            }

            byte * point = static_cast<byte*>(expectedBuffer->GetBuffer());
            KMemCpySafe(point, bufferSize, temp, bufferSize);

            initParameters->Append(*expectedBuffer);
        }

        return initParameters;
    }

    BOOST_FIXTURE_TEST_SUITE(BackCompatibleReplicationTestSuite, BackCompatibleReplicationTest)

    BOOST_AUTO_TEST_CASE(Managed_SP_NullOD_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_NullOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_NullOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_NullOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_EmptyOD_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_EmptyOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_EmptyOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(0, 0);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_EmptyOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_SmallOD_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_SmallOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_SmallOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 4);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_SmallOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_LargeOD_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_LargeOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_LargeOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 1024);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_LargeOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_DiffOD_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_RedoAndUndoDiff.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_RedoAndUndoDiff.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 1 buffer, and the buffer size is 4.
        // UndoOperationData has 1 buffer, and the buffer size is 8.
        // Use this test to verify the redo/undo is correctly passed into state provider.
        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 4);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_DiffOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_MultiBuffers_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_MultiBuffers.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_MultiBuffers.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 3 buffers, and the buffer size is 8.
        OperationData::SPtr expectedInitParameters = CreateOperationData(3, 8);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_MultiBuffers, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_MultiSPs_Recovery)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_MultiSPs.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_MultiSPs.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 3 buffers, and the buffer size is 8.
        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 1024);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Recovery(SMFile, SPFile, TestMode::Recovery_SP_MultiSPs, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_NullOD_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_NullOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_NullOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_NullOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_EmptyOD_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_EmptyOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_EmptyOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(0, 0);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_EmptyOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_SmallOD_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_SmallOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_SmallOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 4);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_SmallOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_LargeOD_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_LargeOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_LargeOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 1024);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_LargeOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_DiffOD_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_RedoAndUndoDiff.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_RedoAndUndoDiff.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 1 buffer, and the buffer size is 4.
        // UndoOperationData has 1 buffer, and the buffer size is 8.
        // Use this test to verify the redo/undo is correctly passed into state provider.
        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 4);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_DiffOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_MultiBuffers_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_MultiBuffers.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_MultiBuffers.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 3 buffers, and the buffer size is 8.
        OperationData::SPtr expectedInitParameters = CreateOperationData(3, 8);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_MultiBuffers, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_MultiSPs_Replication)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_MultiSPs.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_MultiSPs.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 3 buffers, and the buffer size is 8.
        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 1024);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_Replication(SMFile, SPFile, TestMode::Replication_SP_MultiSPs, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_NullOD_FalseProgress)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_NullOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_NullOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_FalseProgress(SMFile, SPFile, TestMode::FalseProgress_SP_NullOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_EmptyOD_FalseProgress)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_EmptyOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_EmptyOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(0, 0);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_FalseProgress(SMFile, SPFile, TestMode::FalseProgress_SP_EmptyOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_LargeOD_FalseProgress)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_LargeOD.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_LargeOD.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 1024);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_FalseProgress(SMFile, SPFile, TestMode::FalseProgress_SP_LargeOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_MultiSPs_FalseProgress)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_MultiSPs.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_MultiSPs.sm", GetAllocator(), TestHelper::FileSource::Managed);

        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 1024);
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_FalseProgress(SMFile, SPFile, TestMode::FalseProgress_SP_MultiSPs, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SP_DiffOD_FalseProgress)
    {
        KWString SMFile = TestHelper::CreateFileName(L"Managed_Replication_SM_RedoAndUndoDiff.sm", GetAllocator(), TestHelper::FileSource::Managed);
        KWString SPFile = TestHelper::CreateFileName(L"Managed_Replication_SP_RedoAndUndoDiff.sm", GetAllocator(), TestHelper::FileSource::Managed);

        // In this case, RedoOperationData has 1 buffer, and the buffer size is 4.
        // UndoOperationData has 1 buffer, and the buffer size is 8.
        // Use this test to verify the redo/undo is correctly passed into state provider.
        OperationData::SPtr expectedInitParameters = CreateOperationData(1, 4);        
        SyncAwait(this->Test_Managed_SP_ApplyAddOperation_FalseProgress(SMFile, SPFile, TestMode::FalseProgress_SP_DiffOD, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
