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
    class ManagedOperationDataTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        ManagedOperationDataTest()
        {
        }

        ~ManagedOperationDataTest()
        {
        }

    public:
        // Test cases used for set different kinds of verifications.
        enum TestMode
        {
            InitParametersNull,
            InitParametersEmpty,
            InitParametersSmall,
            InitParametersLarge,
            InitParametersNull_NoChild,
            InitParametersNull_Name,
            InitParametersLarge_NoChild,
            InitParametersNull_MultiSPs,
            Remove_Single,
            Remove_Multi,
            Replication_Single,
            Replication_Multi,
            FalseProgress,
            FalseProgress_NoChild
        }; 

        // Becase for Parent and Children, they have different TypeString, used
        // this enum to differential them and do the verification.
        enum StateProviderRole
        {
            Parent,
            Child1,
            Child2
        };

    public:
        ktl::Awaitable<void> Test_Deserialize_Managed_MetadataOperationData();
        ktl::Awaitable<void> Test_Deserialize_Managed_RedoOperationData();
        ktl::Awaitable<void> Test_Managed_ApplyMetadataOperationDataAndRedoOperationData(
            __in ApplyContext::Enum mode);

        ktl::Awaitable<void> Test_Managed_SM_ApplyAddOperationDatas_Recovery(
            __in KWString const & fileName,
            __in TestMode mode,
            __in_opt OperationData const * const initializationParameters);

        ktl::Awaitable<void> Test_Managed_SM_ApplyAddAndRemoveOperationDatas_Recovery(
            __in KWString const & addOperationsFile,
            __in KWString const & removeOperationsFile,
            __in TestMode mode,
            __in_opt OperationData const * const initializationParameters);

        ktl::Awaitable<void> Test_Managed_SM_ApplyAddOperationDatas_Replication(
            __in KWString const & addOperationsFile,
            __in TestMode mode,
            __in_opt OperationData const * const initializationParameters);

        ktl::Awaitable<void> Test_Managed_SM_ApplyAddOperationDatas_Replication(
            __in KWString const & addOperationsFile,
            __in KWString const & removeOperationsFile,
            __in TestMode mode,
            __in_opt OperationData const * const initializationParameters);

        ktl::Awaitable<void> Test_Managed_SM_ApplyAddOperationDatas_FalseProgress(
            __in KWString const & addOperationsFile,
            __in TestMode mode,
            __in_opt OperationData const * const initializationParameters);

    public: //Helper functions
        ktl::Awaitable<void> ReadManagedFileAsync(__in KWString const & fileName);
        ktl::Awaitable<void> CloseAsync();

        ktl::Awaitable<void> ApplyFileOperations(
            __in KWString const & fileName,
            __in LONG64 txnId,
            __in ApplyContext::Enum applyContext,
            __in bool reverseOrder = false);

        void VerifyStateProviderInfo(
            __in KUriView const & name,
            __in FABRIC_STATE_PROVIDER_ID expectedId,
            __in StateProviderRole stateProviderRole,
            __in_opt OperationData const * const expectedInitParameters);

        void VerifyStateProviderNotExist(
            __in KStringView const & name);

        OperationData::SPtr CreateInitParameters(
            __in_opt ULONG32 bufferSize);

    private:
        KBlockFile::SPtr fileSPtr_;
        ktl::io::KFileStream::SPtr fileStreamSPtr_;
    };

    ktl::Awaitable<void> ManagedOperationDataTest::ReadManagedFileAsync(
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

    ktl::Awaitable<void> ManagedOperationDataTest::CloseAsync()
    {
        co_await fileStreamSPtr_->CloseAsync();
        fileSPtr_->Close();

        co_return;
    }

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Deserialize_Managed_MetadataOperationData()
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_MetaOperationData.sm", GetAllocator(), TestHelper::FileSource::Managed);
        co_await this->ReadManagedFileAsync(fileName);

        ULONG fileLength = static_cast<ULONG>(fileStreamSPtr_->GetLength());
        fileStreamSPtr_->SetPosition(0);

        KBuffer::SPtr itemStream = nullptr;
        NTSTATUS status = KBuffer::Create(fileLength, itemStream, GetAllocator(), TEST_TAG);

        ULONG bytesRead = 0;
        status = co_await fileStreamSPtr_->ReadAsync(*itemStream, bytesRead, 0, fileLength);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(bytesRead == fileLength);

        BinaryReader itemReader(*itemStream, GetAllocator());
        CODING_ERROR_ASSERT(NT_SUCCESS(itemReader.Status()));

        // Here We take the fact that every metadataOperation has exactly one buffer, then the
        // count we read is the count of metadataOperations.
        LONG32 count = 0;
        itemReader.Read(count);

        for (LONG32 i = 1; i <= count; i++)
        {
            LONG32 bufferSize = 0;
            itemReader.Read(bufferSize);

            KArray<KBuffer::CSPtr> items(GetAllocator());
            KBuffer::SPtr buffer = nullptr;
            status = KBuffer::Create(
                bufferSize,
                buffer,
                GetAllocator(),
                TEST_TAG);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            itemReader.Read(bufferSize, buffer);
            status = items.Append(buffer.RawPtr());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            OperationData::CSPtr operationData = OperationData::Create(items, GetAllocator());

            MetadataOperationData::CSPtr metadataOperationData = nullptr;
            status = MetadataOperationData::Create(GetAllocator(), operationData.RawPtr(), metadataOperationData);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            FABRIC_STATE_PROVIDER_ID stateProviderId = metadataOperationData->StateProviderId;
            KUri::CSPtr stateProviderName = metadataOperationData->StateProviderName;
            ApplyType::Enum applyType = metadataOperationData->ApplyType;

            switch (i)
            {
            case 1:
            {
                KUriView expectedName = L"fabric:/test1";
                VERIFY_ARE_EQUAL(*stateProviderName, expectedName);
                VERIFY_ARE_EQUAL(stateProviderId, 1234567);
                VERIFY_ARE_EQUAL(applyType, ApplyType::Enum::Insert);
                break;
            }
            case 2:
            {
                KUriView expectedName = L"fabric:/名字";
                VERIFY_ARE_EQUAL(*stateProviderName, expectedName);
                VERIFY_ARE_EQUAL(stateProviderId, 2345678);
                VERIFY_ARE_EQUAL(applyType, ApplyType::Enum::Delete);
                break;
            }
            case 3:
            {
                KUriView expectedName = L"urn:/᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈ";
                VERIFY_ARE_EQUAL(*stateProviderName, expectedName);
                VERIFY_ARE_EQUAL(stateProviderId, 2345678);
                VERIFY_ARE_EQUAL(applyType, ApplyType::Enum::Delete);
                break;
            }
            case 4:
            {
                KUriView expectedName = L"fabric:/新字典";
                VERIFY_ARE_EQUAL(*stateProviderName, expectedName);
                VERIFY_ARE_EQUAL(stateProviderId, 3456789);
                VERIFY_ARE_EQUAL(applyType, ApplyType::Enum::Invalid);
                break;
            }
            case 5:
            {
                KUriView expectedName = L"urn:/ورم بڭا ضررى طوق";
                VERIFY_ARE_EQUAL(*stateProviderName, expectedName);
                VERIFY_ARE_EQUAL(stateProviderId, 3456789);
                VERIFY_ARE_EQUAL(applyType, ApplyType::Enum::Invalid);
                break;
            }
            default:
                CODING_ERROR_ASSERT(false);
            }
        }

        co_await this->CloseAsync();
        co_return;
    }

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Deserialize_Managed_RedoOperationData()
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_RedoOperationData.sm", GetAllocator(), TestHelper::FileSource::Managed);
        co_await this->ReadManagedFileAsync(fileName);

        ULONG fileLength = static_cast<ULONG>(fileStreamSPtr_->GetLength());
        fileStreamSPtr_->SetPosition(0);

        KBuffer::SPtr itemStream = nullptr;
        NTSTATUS status = KBuffer::Create(fileLength, itemStream, GetAllocator(), TEST_TAG);

        ULONG bytesRead = 0;
        status = co_await fileStreamSPtr_->ReadAsync(*itemStream, bytesRead, 0, fileLength);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(bytesRead == fileLength);

        BinaryReader itemReader(*itemStream, GetAllocator());
        CODING_ERROR_ASSERT(NT_SUCCESS(itemReader.Status()));

        // We take the fact that every redodataOperation has exactly one buffer, then the
        // count we read is the count of redodataOperation.
        LONG32 count = 0;
        itemReader.Read(count);

        for (LONG32 i = 1; i <= count; i++)
        {
            LONG32 bufferSize = 0;
            itemReader.Read(bufferSize);

            KArray<KBuffer::CSPtr> items(GetAllocator());
            KBuffer::SPtr buffer = nullptr;
            status = KBuffer::Create(
                bufferSize,
                buffer,
                GetAllocator(),
                TEST_TAG);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            itemReader.Read(bufferSize, buffer);
            status = items.Append(buffer.RawPtr());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            OperationData::CSPtr operationData = OperationData::Create(items, GetAllocator());
            RedoOperationData::CSPtr redoOperationData = nullptr;
            status = RedoOperationData::Create(*TestHelper::CreatePartitionedReplicaId(GetAllocator()), GetAllocator(), operationData.RawPtr(), redoOperationData);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KString::CSPtr typeName = redoOperationData->TypeName;
            OperationData::CSPtr initializationParameters = redoOperationData->InitializationParameters;
            FABRIC_STATE_PROVIDER_ID parentId = redoOperationData->ParentId;

            KStringView expectedName = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            VERIFY_ARE_EQUAL(expectedName, *typeName);
            VERIFY_ARE_EQUAL(parentId, 0);

            switch (i)
            {
            case 1:
            {
                VERIFY_IS_NULL(initializationParameters);
                break;
            }
            case 2:
            {
                VERIFY_ARE_EQUAL(initializationParameters->BufferCount, 0);
                break;
            }
            case 3:
            {
                byte managedInitializationParameters[2] = { 0x20, 0x20 };

                KBuffer::SPtr readBuffer = const_cast<KBuffer *>((*initializationParameters)[0].RawPtr());

                byte * point = static_cast<byte*>(readBuffer->GetBuffer());

                for (ULONG32 j = 0; j < 2; j++)
                {
                    CODING_ERROR_ASSERT(point[j] == managedInitializationParameters[j]);
                }

                break;
            }
            case 4:
            {
                byte managedInitializationParameters[3] = { 0x10, 0x20, 0x30 };

                KBuffer::SPtr readBuffer = const_cast<KBuffer *>((*initializationParameters)[0].RawPtr());

                byte * point = static_cast<byte*>(readBuffer->GetBuffer());

                for (ULONG32 j = 0; j < 3; j++)
                {
                    CODING_ERROR_ASSERT(point[j] == managedInitializationParameters[j]);
                }

                break;
            }
            case 5:
            {
                byte managedInitializationParameters[4] = { 0x10, 0x20, 0x30, 0x38 };

                KBuffer::SPtr readBuffer = const_cast<KBuffer *>((*initializationParameters)[0].RawPtr());

                byte * point = static_cast<byte*>(readBuffer->GetBuffer());

                for (ULONG32 j = 0; j < 4; j++)
                {
                    CODING_ERROR_ASSERT(point[j] == managedInitializationParameters[j]);
                }

                break;
            }
            default:
                CODING_ERROR_ASSERT(false);
            }
        }

        co_await this->CloseAsync();
        co_return;
    }

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Managed_ApplyMetadataOperationDataAndRedoOperationData(__in ApplyContext::Enum mode)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_RedoAndMetadataOperationData.sm", GetAllocator(), TestHelper::FileSource::Managed);
        co_await this->ReadManagedFileAsync(fileName);

        ULONG fileLength = static_cast<ULONG>(fileStreamSPtr_->GetLength());
        fileStreamSPtr_->SetPosition(0);

        KBuffer::SPtr itemStream = nullptr;
        NTSTATUS status = KBuffer::Create(fileLength, itemStream, GetAllocator(), TEST_TAG);

        ULONG bytesRead = 0;
        status = co_await fileStreamSPtr_->ReadAsync(*itemStream, bytesRead, 0, fileLength);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(bytesRead == fileLength);

        BinaryReader itemReader(*itemStream, GetAllocator());
        LONG32 count = 0;
        itemReader.Read(count);

        KArray<KUri::CSPtr> expectedNameList(GetAllocator(), count);
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        for (LONG32 i = 1; i <= count; i++)
        {
            const LONG64 transactionId = i;
            auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());

            LONG32 bufferSize = 0;
            itemReader.Read(bufferSize);

            KArray<KBuffer::CSPtr> items(GetAllocator());
            KBuffer::SPtr metadataBuffer = nullptr;
            status = KBuffer::Create(
                bufferSize,
                metadataBuffer,
                GetAllocator(),
                TEST_TAG);
            itemReader.Read(bufferSize, metadataBuffer);
            status = items.Append(metadataBuffer.RawPtr());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Get the MetadataOperationData buffer
            OperationData::CSPtr operationData = OperationData::Create(items, GetAllocator());

            // Deserialize the buffer and create the MetadataOperationData.
            MetadataOperationData::CSPtr readMetadataOperationData = nullptr;
            status = MetadataOperationData::Create(GetAllocator(), operationData.RawPtr(), readMetadataOperationData);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // For the previous MetadataOperationData, it has not serialized to the operation data.
            // So need to create a serialied one to use in the test.
            MetadataOperationData::CSPtr metadataOperationData;
            status = MetadataOperationData::Create(
                readMetadataOperationData->StateProviderId,
                *readMetadataOperationData->StateProviderName,
                readMetadataOperationData->ApplyType, GetAllocator(), metadataOperationData);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            expectedNameList.Append(readMetadataOperationData->StateProviderName);

            // Since we use the managed info to create a native MetadataOperationData and will
            // use it later, we specified SerializationMode::Native here.
            NamedOperationData::CSPtr namedMetadataOperationData;
            status = NamedOperationData::Create(
                LONG64_MIN,
                SerializationMode::Managed,
                GetAllocator(),
                metadataOperationData.RawPtr(),
                namedMetadataOperationData);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            itemReader.Read(bufferSize);

            KArray<KBuffer::CSPtr> items1(GetAllocator());
            KBuffer::SPtr redoBuffer = nullptr;
            status = KBuffer::Create(
                bufferSize,
                redoBuffer,
                GetAllocator(),
                TEST_TAG);
            itemReader.Read(bufferSize, redoBuffer);
            items1.Append(redoBuffer.RawPtr());

            operationData = OperationData::Create(items1, GetAllocator());

            // Deserialize the RedoOperationData from the buffer.
            RedoOperationData::CSPtr readRedoOperationData = nullptr;
            status = RedoOperationData::Create(*TestHelper::CreatePartitionedReplicaId(GetAllocator()), GetAllocator(), operationData.RawPtr(), readRedoOperationData);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Create the RedoOperationData in native.
            RedoOperationData::CSPtr redoOperationData = GenerateRedoOperationData(
                TestStateProvider::TypeName,
                readRedoOperationData->InitializationParameters.RawPtr(),
                readRedoOperationData->ParentId);

            TxnReplicator::OperationContext::CSPtr operationContext;
            status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                i,
                *txn,
                mode,
                namedMetadataOperationData.RawPtr(),
                redoOperationData.RawPtr(),
                operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            VERIFY_IS_TRUE(operationContext != nullptr);

            status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        co_await this->CloseAsync();

        if (mode == ApplyContext::RecoveryRedo)
        {
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        }

        this->VerifyExist(expectedNameList, true, nullptr, true);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    // Test the Apply on Recovery path.
    ktl::Awaitable<void> ManagedOperationDataTest::Test_Managed_SM_ApplyAddOperationDatas_Recovery(
        __in KWString const & fileName,
        __in TestMode mode,
        __in_opt OperationData const * const initializationParameters)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        LONG64 txnId = 1;
        co_await this->ApplyFileOperations(fileName, txnId, ApplyContext::RecoveryRedo);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        // Verification for different cases.
        switch (mode)
        {
        case TestMode::InitParametersNull:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499617854961137, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child", 131499617855051009, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child/child", 131499618097371936, Child2, initializationParameters);
            break;
        }
        case TestMode::InitParametersEmpty:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499107378785268, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child", 131499107378875544, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child/child", 131499107378990634, Child2, initializationParameters);
            break;
        }
        case TestMode::InitParametersSmall:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499109588892124, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child", 131499109589022220, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child/child", 131499109589137456, Child2, initializationParameters);
            break;
        }
        case TestMode::InitParametersLarge:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499317170901017, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child", 131499317171011114, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child/child", 131499317664436962, Child2, initializationParameters);
            break;
        }
        case TestMode::InitParametersNull_Name:
        {
            // Get state provider KUri valid check will return false. Disable the verify until the KUri fix.
            // this->VerifyStateProviderInfo(L"fabric:/字典", 131499660228332438, Parent, initializationParameters);
            break;
        }
        case TestMode::InitParametersNull_NoChild:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499395066746154, Parent, initializationParameters);
            break;
        }
        case TestMode::InitParametersLarge_NoChild:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499390465791631, Parent, initializationParameters);
            break;
        }
        case TestMode::InitParametersNull_MultiSPs:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499417075987746, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child", 131499417076097821, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child/child", 131499417409564224, Child2, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1", 131499417734475658, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1/child", 131499417734475659, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1/child/child", 131499418042281979, Child2, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider2", 131499418265381733, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider2/child", 131499418265381734, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider2/child/child", 131499418405837070, Child2, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3", 131499418647462020, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3/child", 131499418647462021, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3/child/child", 131499418790297897, Child2, initializationParameters);
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

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Managed_SM_ApplyAddAndRemoveOperationDatas_Recovery(
        __in KWString const & addOperationsFile,
        __in KWString const & removeOperationsFile,
        __in TestMode mode,
        __in_opt OperationData const * const initializationParameters)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        LONG64 txnId = 1;
        co_await this->ApplyFileOperations(addOperationsFile, txnId, ApplyContext::RecoveryRedo);
        co_await this->ApplyFileOperations(removeOperationsFile, ++txnId, ApplyContext::RecoveryRedo);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        switch (mode)
        {
        case TestMode::Remove_Single:
        {
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child/child");
            break;
        }
        case TestMode::Remove_Multi:
        {
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child/child");
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1", 131499655116272698, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1/child", 131499655116272699, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1/child/child", 131499655286968448, Child2, initializationParameters);
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider2");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider2/child");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider2/child/child");
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3", 131499655516290056, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3/child", 131499655516290057, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3/child/child", 131499655609188627, Child2, initializationParameters);
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

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Managed_SM_ApplyAddOperationDatas_Replication(
        __in KWString const & addOperationsFile,
        __in TestMode mode,
        __in_opt OperationData const * const initializationParameters)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

        LONG64 txnId = 1;
        co_await this->ApplyFileOperations(addOperationsFile, txnId, ApplyContext::SecondaryRedo);

        switch (mode)
        {
        case TestMode::Replication_Single:
        {
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499395066746154, Parent, initializationParameters);
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

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Managed_SM_ApplyAddOperationDatas_Replication(
        __in KWString const & addOperationsFile,
        __in KWString const & removeOperationsFile,
        __in TestMode mode,
        __in_opt OperationData const * const initializationParameters)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

        LONG64 txnId = 1;
        co_await this->ApplyFileOperations(addOperationsFile, txnId, ApplyContext::SecondaryRedo);
        co_await this->ApplyFileOperations(removeOperationsFile, ++txnId, ApplyContext::SecondaryRedo);

        switch (mode)
        {
        case TestMode::Replication_Multi:
        {
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child/child");
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1", 131499655116272698, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1/child", 131499655116272699, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider1/child/child", 131499655286968448, Child2, initializationParameters);
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider2");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider2/child");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider2/child/child");
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3", 131499655516290056, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3/child", 131499655516290057, Child1, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider3/child/child", 131499655609188627, Child2, initializationParameters);
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

    ktl::Awaitable<void> ManagedOperationDataTest::Test_Managed_SM_ApplyAddOperationDatas_FalseProgress(
        __in KWString const & addOperationsFile,
        __in TestMode mode,
        __in_opt OperationData const * const initializationParameters)
    {
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

        switch (mode)
        {
        case TestMode::FalseProgress_NoChild:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(addOperationsFile, txnId, ApplyContext::SecondaryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499395066746154, Parent, initializationParameters);

            co_await this->ApplyFileOperations(addOperationsFile, ++txnId, ApplyContext::SecondaryFalseProgress);
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider");
            break;
        }
        case TestMode::FalseProgress:
        {
            LONG64 txnId = 1;
            co_await this->ApplyFileOperations(addOperationsFile, txnId, ApplyContext::RecoveryRedo);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider", 131499819512510689, Parent, initializationParameters);
            this->VerifyStateProviderInfo(L"fabric:/StateProvider/child", 131499819512620756, Child1, initializationParameters);

            co_await this->ApplyFileOperations(addOperationsFile, ++txnId, ApplyContext::SecondaryFalseProgress, true);
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider");
            this->VerifyStateProviderNotExist(L"fabric:/StateProvider/child");
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

    ktl::Awaitable<void> ManagedOperationDataTest::ApplyFileOperations(
        __in KWString const & fileName,
        __in LONG64 txnId,
        __in ApplyContext::Enum applyContext,
        __in bool reverseOrder)
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

            // We write undo as null, so verify null here.
            VERIFY_IS_NULL(undo);

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

            // In the FlaseProgress case, redo should always be null.
            status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                txnId,
                *txn,
                applyContext,
                operationSet[reverseOrder? ((setCount - i - 1) *3): i * 3].RawPtr(),
                type != ApplyContext::FALSE_PROGRESS ? operationSet[reverseOrder ? ((setCount - i - 1) * 3 + 1) : i * 3 + 1].RawPtr() : nullptr,
                operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            if (operationContext != nullptr)
            {
                contextVector.push_back(operationContext);
            }
        }

        for (TxnReplicator::OperationContext::CSPtr operationContext : contextVector)
        {
            if (type != FalseProgress)
            {
                status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
        }

        co_await this->CloseAsync();
    }

    void ManagedOperationDataTest::VerifyStateProviderInfo(
        __in KUriView const & name,
        __in FABRIC_STATE_PROVIDER_ID expectedId,
        __in StateProviderRole stateProviderRole,
        __in_opt OperationData const * const expectedInitParameters)
    {
        IStateProvider2::SPtr outStateProvider = nullptr;

        NTSTATUS status = testTransactionalReplicatorSPtr_->StateManager->Get(name, outStateProvider);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
        VERIFY_ARE_EQUAL(testStateProvider.StateProviderId, expectedId);
        VERIFY_IS_TRUE(TestHelper::Equals(testStateProvider.GetInitializationParameters().RawPtr(), stateProviderRole == Parent ? expectedInitParameters : nullptr));

        switch (stateProviderRole)
        {
        case Parent:
        {
            KStringView expectedType = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            VERIFY_ARE_EQUAL(testStateProvider.GetInputTypeName(), expectedType);
            break;
        }
        case Child1:
        {
            KStringView expectedType = L"System.Fabric.Replication.Test.TestStateProvider1, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            VERIFY_ARE_EQUAL(testStateProvider.GetInputTypeName(), expectedType);
            break;
        }
        case Child2:
        {
            KStringView expectedType = L"System.Fabric.Replication.Test.TestStateProvider2, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            VERIFY_ARE_EQUAL(testStateProvider.GetInputTypeName(), expectedType);
            break;
        }
        default:
            CODING_ERROR_ASSERT(false);
        }
    }

    void ManagedOperationDataTest::VerifyStateProviderNotExist(
        __in KStringView const & name)
    {
        KUri::CSPtr stateProviderName = nullptr;
        IStateProvider2::SPtr outStateProvider = nullptr;
        NTSTATUS status = KUri::Create(name, GetAllocator(), stateProviderName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Specified state provider must not be exist.
        status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, outStateProvider);
        CODING_ERROR_ASSERT(status == SF_STATUS_NAME_DOES_NOT_EXIST);
    }

    OperationData::SPtr ManagedOperationDataTest::CreateInitParameters(
        __in_opt ULONG32 bufferSize)
    {
        OperationData::SPtr initParameters = OperationData::Create(GetAllocator());

        if (bufferSize == 0) 
        {
            return initParameters;
        }

        KBuffer::SPtr expectedBuffer = nullptr;
        NTSTATUS status = KBuffer::Create(bufferSize, expectedBuffer, GetAllocator(), TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        byte temp[1024] = {};

        for (ULONG32 j = 0; j < bufferSize; j++)
        {
            temp[j] = static_cast<byte>(j + 1);
        }

        byte * point = static_cast<byte*>(expectedBuffer->GetBuffer());
        KMemCpySafe(point, bufferSize, temp, bufferSize);

        initParameters->Append(*expectedBuffer);

        return initParameters;
    }

    BOOST_FIXTURE_TEST_SUITE(ManagedOperationDataTestSuite, ManagedOperationDataTest)

    //
    // Scenario:        MetadataOperationData generated by managed, deserialize the OperationData
    //                  through native deserialization.
    // Expected Result: Native should sucessfully deserialize the managed MetadataOperationData
    // 
    BOOST_AUTO_TEST_CASE(Deserialize_Managed_MetadataOperationData)
    {
        SyncAwait(this->Test_Deserialize_Managed_MetadataOperationData());
    }

    //
    // Scenario:        RedoOperationData generated by managed, deserialize the OperationData
    //                  through native deserialization.
    // Expected Result: Native should sucessfully deserialize the managed RedoOperationData
    // Note:            RedoOperationData includes the state provider initialization parameters.
    //                  the test includes null initialization parameters, empty initialization parameters
    //                  and initialization parameters with items.
    // 
    BOOST_AUTO_TEST_CASE(Deserialize_Managed_RedoOperationData)
    {
        SyncAwait(this->Test_Deserialize_Managed_RedoOperationData());
    }

    //
    // Scenario:        Managed MetadataOperationData and RedoOperationData, read by native.
    //                  Create corresponding OperationtData to recover.
    // Expected Result: The state providers specified in the OperationData should be recovered.
    // Note:            Although it is recovery redo test, but more about MetadataOperationData and RedoOperationData
    //                  Deserialization in this case. When we read the operationdata from the file
    //                  and create MetadataOperationData and RedoOperationData, they are not serialized.
    //                  So need to use the info deserialized from the operationdata to create native 
    //                  MetadataOperationData and RedoOperationData.
    // 
    BOOST_AUTO_TEST_CASE(Managed_ApplyMetadataOperationDataAndRedoOperationData_RecoveryRedo)
    {
        SyncAwait(this->Test_Managed_ApplyMetadataOperationDataAndRedoOperationData(ApplyContext::RecoveryRedo));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_NullIP_Children_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_NullIP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersNull, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_EmptyIP_Children_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_EmptyIP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = this->CreateInitParameters(0);
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersEmpty, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_4IP_Children_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_4IP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = this->CreateInitParameters(4);
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersSmall, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_1024IP_Children_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_1024IP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = this->CreateInitParameters(1024);
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersLarge, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_NullIP_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_NullIP.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersNull_NoChild, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_1024IP_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_1024IP.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = this->CreateInitParameters(1024);
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersLarge_NoChild, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_NullIP_Children_Name_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_NullIP_Children_Name.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersNull_Name, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSPs_SameTxn_NullIP_Recovery)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_SM_AddSPs_SameTxn_NullIP.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Recovery(fileName, TestMode::InitParametersNull_MultiSPs, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddRemoveSP_NullIP_Children_Recovery)
    {
        KWString addOperationsFile = TestHelper::CreateFileName(L"Managed_AddSP_NullIP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        KWString removeOperationsFile = TestHelper::CreateFileName(L"Managed_AddSP_NullIP_Children_Remove.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddAndRemoveOperationDatas_Recovery(addOperationsFile, removeOperationsFile, TestMode::Remove_Single, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddRemoveMultiSPs_NullIP_Children_Recovery)
    {
        KWString addOperationsFile = TestHelper::CreateFileName(L"Managed_AddMultiSPs_NullIP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        KWString removeOperationsFile = TestHelper::CreateFileName(L"Managed_RemoveMultiSPs_NullIP_Children_Remove.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddAndRemoveOperationDatas_Recovery(addOperationsFile, removeOperationsFile, TestMode::Remove_Multi, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_NullIP_Replication)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_NullIP.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Replication(fileName, TestMode::Replication_Single, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddRemoveMultiSPs_NullIP_Replication)
    {
        KWString addOperationsFile = TestHelper::CreateFileName(L"Managed_AddMultiSPs_NullIP_Children.txt", GetAllocator(), TestHelper::FileSource::Managed);
        KWString removeOperationsFile = TestHelper::CreateFileName(L"Managed_RemoveMultiSPs_NullIP_Children_Remove.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_Replication(addOperationsFile, removeOperationsFile, TestMode::Replication_Multi, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_NullIP_FalseProgress_NoChild)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_NullIP.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_FalseProgress(fileName, TestMode::FalseProgress_NoChild, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_CASE(Managed_SM_AddSP_NullIP_FalseProgress_Children)
    {
        KWString fileName = TestHelper::CreateFileName(L"Managed_AddSP_Children_FalseProgress.txt", GetAllocator(), TestHelper::FileSource::Managed);
        OperationData::SPtr expectedInitParameters = nullptr;
        SyncAwait(this->Test_Managed_SM_ApplyAddOperationDatas_FalseProgress(fileName, TestMode::FalseProgress, expectedInitParameters.RawPtr()));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
