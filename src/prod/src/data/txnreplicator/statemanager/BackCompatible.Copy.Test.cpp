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
using namespace Data::StateManager;
using namespace Data::Utilities;
using namespace TxnReplicator;

namespace StateManagerTests
{
    class BackCompatibleCopyTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        BackCompatibleCopyTest()
        {
        }

        ~BackCompatibleCopyTest()
        {
        }

    public:
        enum TestMode
        {
            Empty_SM,
            One_NotCopied,
            One_Copied,
            Add4_Copied,
            Add4_Remove2_Copied,
            One_Copied_SP,
            One_Copied_Large_SP,
            Two_Copied_Large_SP
        };

    public:
        ktl::Awaitable<void> Test_Copy_StateManager(
            __in KWString const & fileName,
            __in TestMode mode);

    public:
        ktl::Awaitable<void> ReadManagedFileAsync(__in KWString const & fileName);
        ktl::Awaitable<void> CloseAsync();

        KArray<Data::Utilities::OperationData::SPtr> ReadAndCreateOperationDataKarray(
            __in BinaryReader & reader);

        void VerifyStateProviderExist(
            __in KStringView const & name,
            __in TestTransactionManager const & replica,
            __in NTSTATUS expectedStatus);

        void VerifyStateProviderExist(
            __in KStringView const & name,
            __in TestTransactionManager const & replica,
            __in NTSTATUS expectedStatus,
            __in KArray<Data::Utilities::OperationData::CSPtr> const & expectedOperationDataArray);

        KBuffer::SPtr CreateBuffer(
            __in_opt ULONG32 bufferSize,
            __in bool allZero);

        bool VerifyUserOperationDataArray(
            __in KArray<Data::Utilities::OperationData::CSPtr> const & thisArray,
            __in KArray<Data::Utilities::OperationData::CSPtr> const & otherArray);

    private:
        KBlockFile::SPtr fileSPtr_;
        ktl::io::KFileStream::SPtr fileStreamSPtr_;
    };

    ktl::Awaitable<void> BackCompatibleCopyTest::ReadManagedFileAsync(
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

    ktl::Awaitable<void> BackCompatibleCopyTest::CloseAsync()
    {
        co_await fileStreamSPtr_->CloseAsync();
        fileSPtr_->Close();

        co_return;
    }

    ktl::Awaitable<void> BackCompatibleCopyTest::Test_Copy_StateManager(
        __in KWString const & fileName,
        __in TestMode mode)
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

        KArray<Data::Utilities::OperationData::SPtr> operationDataArray(this->ReadAndCreateOperationDataKarray(itemReader));
        co_await this->CloseAsync();

        LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId;
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());
        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

        co_await secondaryTransactionalReplicator->CopyAsync(operationDataArray, CancellationToken::None);

        co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

        // Verify
        switch (mode)
        {
        case TestMode::One_NotCopied:
        {
            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, SF_STATUS_NAME_DOES_NOT_EXIST);
            break;
        }
        case TestMode::One_Copied:
        {
            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            break;
        }
        case TestMode::Add4_Copied:
        {
            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            VerifyStateProviderExist(L"fabric:/StateProvider1", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            VerifyStateProviderExist(L"fabric:/StateProvider2", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            VerifyStateProviderExist(L"fabric:/StateProvider3", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            break;
        }
        case TestMode::Add4_Remove2_Copied:
        {
            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, SF_STATUS_NAME_DOES_NOT_EXIST);
            VerifyStateProviderExist(L"fabric:/StateProvider1", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            VerifyStateProviderExist(L"fabric:/StateProvider2", *secondaryTransactionalReplicator, SF_STATUS_NAME_DOES_NOT_EXIST);
            VerifyStateProviderExist(L"fabric:/StateProvider3", *secondaryTransactionalReplicator, STATUS_SUCCESS);
            break;
        }
        case TestMode::One_Copied_SP:
        {
            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS);

            KArray<Data::Utilities::OperationData::CSPtr> expectedOperationDatas(GetAllocator());

            OperationData::SPtr operationData = OperationData::Create(GetAllocator());
            operationData->Append(*this->CreateBuffer(0, true).RawPtr());
            expectedOperationDatas.Append(operationData.RawPtr());

            OperationData::SPtr operationData1 = OperationData::Create(GetAllocator());
            operationData1->Append(*this->CreateBuffer(1, true).RawPtr());
            expectedOperationDatas.Append(operationData1.RawPtr());

            OperationData::SPtr operationData2 = OperationData::Create(GetAllocator());
            operationData2->Append(*this->CreateBuffer(2, true).RawPtr());
            expectedOperationDatas.Append(operationData2.RawPtr());

            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS, expectedOperationDatas);
            break;
        }
        case TestMode::One_Copied_Large_SP:
        {
            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS);

            KArray<Data::Utilities::OperationData::CSPtr> expectedOperationDatas(GetAllocator());

            OperationData::SPtr operationData = OperationData::Create(GetAllocator());
            operationData->Append(*this->CreateBuffer(1024, false).RawPtr());
            expectedOperationDatas.Append(operationData.RawPtr());

            OperationData::SPtr operationData1 = OperationData::Create(GetAllocator());
            operationData1->Append(*this->CreateBuffer(512, false).RawPtr());
            operationData1->Append(*this->CreateBuffer(1024, false).RawPtr());
            expectedOperationDatas.Append(operationData1.RawPtr());

            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS, expectedOperationDatas);
            break;
        }
        case TestMode::Two_Copied_Large_SP:
        {
            KArray<Data::Utilities::OperationData::CSPtr> expectedOperationDatas(GetAllocator());

            OperationData::SPtr operationData = OperationData::Create(GetAllocator());
            operationData->Append(*this->CreateBuffer(1024, false).RawPtr());
            expectedOperationDatas.Append(operationData.RawPtr());

            OperationData::SPtr operationData1 = OperationData::Create(GetAllocator());
            operationData1->Append(*this->CreateBuffer(512, false).RawPtr());
            operationData1->Append(*this->CreateBuffer(1024, false).RawPtr());
            expectedOperationDatas.Append(operationData1.RawPtr());

            VerifyStateProviderExist(L"fabric:/StateProvider", *secondaryTransactionalReplicator, STATUS_SUCCESS, expectedOperationDatas);
            VerifyStateProviderExist(L"fabric:/StateProvider1", *secondaryTransactionalReplicator, STATUS_SUCCESS, expectedOperationDatas);
            break;
        }
        default:
            break;
        }

        // Clean up
        co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
    }

    void BackCompatibleCopyTest::VerifyStateProviderExist(
        __in KStringView const & name,
        __in TestTransactionManager const & replica,
        __in NTSTATUS expectedStatus)
    {
        IStateProvider2::SPtr outStateProvider = nullptr;
        KUri::CSPtr stateProviderName = nullptr;

        NTSTATUS status = KUri::Create(name, GetAllocator(), stateProviderName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = replica.StateManager->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, expectedStatus);
    }

    void BackCompatibleCopyTest::VerifyStateProviderExist(
        __in KStringView const & name,
        __in TestTransactionManager const & replica,
        __in NTSTATUS expectedStatus,
        __in KArray<Data::Utilities::OperationData::CSPtr> const & expectedOperationDataArray)
    {
        IStateProvider2::SPtr outStateProvider = nullptr;
        KUri::CSPtr stateProviderName = nullptr;

        NTSTATUS status = KUri::Create(name, GetAllocator(), stateProviderName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = replica.StateManager->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, expectedStatus);

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
        VERIFY_IS_TRUE(this->VerifyUserOperationDataArray(testStateProvider.UserOperationDataArray, expectedOperationDataArray));
    }

    KArray<Data::Utilities::OperationData::SPtr> BackCompatibleCopyTest::ReadAndCreateOperationDataKarray(
        __in BinaryReader & reader)
    {
        KArray<Data::Utilities::OperationData::SPtr> operationDataArray(GetAllocator());

        // 1. Read Count of OperationDatas
        LONG32 operationCount = 0;
        reader.Read(operationCount);

        for (LONG32 i = 0; i < operationCount; i++)
        {
            NTSTATUS status = operationDataArray.Append(TestHelper::Read(reader, GetAllocator()));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        return operationDataArray;
    }

    KBuffer::SPtr BackCompatibleCopyTest::CreateBuffer(
        __in_opt ULONG32 bufferSize,
        __in bool allZero)
    {
        KBuffer::SPtr expectedBuffer = nullptr;
        NTSTATUS status = KBuffer::Create(bufferSize, expectedBuffer, GetAllocator(), TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        byte temp[1024] = {};

        for (ULONG32 j = 0; j < bufferSize; j++)
        {
            ULONG32 value = allZero ? 0 : j;
            temp[j] = static_cast<byte>(value);
        }

        byte * point = static_cast<byte*>(expectedBuffer->GetBuffer());
        KMemCpySafe(point, bufferSize, temp, bufferSize);

        return expectedBuffer;
    }

    bool BackCompatibleCopyTest::VerifyUserOperationDataArray(
        __in KArray<Data::Utilities::OperationData::CSPtr> const & thisArray,
        __in KArray<Data::Utilities::OperationData::CSPtr> const & otherArray)
    {
        if (thisArray.Count() != otherArray.Count())
        {
            return false;
        }

        for (ULONG i = 0; i < thisArray.Count(); i++)
        {
            bool result = TestHelper::Equals(thisArray[i].RawPtr(), otherArray[i].RawPtr());
            if (result == false)
            {
                return false;
            }
        }

        return true;
    }

    BOOST_FIXTURE_TEST_SUITE(BackCompatibleCopyTestSuite, BackCompatibleCopyTest)

    BOOST_AUTO_TEST_CASE(Copy_Empty_SM)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_Empty_SM.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::Empty_SM));
    }

    BOOST_AUTO_TEST_CASE(Copy_One_NotCopied_SM)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_One_NotCopied.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::One_NotCopied));
    }

    BOOST_AUTO_TEST_CASE(Copy_One_Copied_SM)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_One_Copied.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::One_Copied));
    }

    BOOST_AUTO_TEST_CASE(Copy_Add4_Copied_SM)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_Add4_Copied.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::Add4_Copied));
    }

    BOOST_AUTO_TEST_CASE(Copy_Add4_Remove2_Copied_SM)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_Add4_Remove2_Copied.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::Add4_Remove2_Copied));
    }

    BOOST_AUTO_TEST_CASE(Copy_One_Copied_SMSP)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_One_Copied_WithSP.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::One_Copied_SP));
    }

    BOOST_AUTO_TEST_CASE(Copy_One_Copied_Large_SMSP)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_One_Copied_Large_WithSP.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::One_Copied_Large_SP));
    }

    BOOST_AUTO_TEST_CASE(Copy_Two_Copied_Large_SMSP)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_Two_Copied_Large_WithSP.sm", GetAllocator(), TestHelper::FileSource::Managed);
        SyncAwait(this->Test_Copy_StateManager(fileName, TestMode::Two_Copied_Large_SP));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
