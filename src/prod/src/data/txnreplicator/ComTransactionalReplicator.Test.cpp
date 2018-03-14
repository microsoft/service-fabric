// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

extern void TR_Test_InitializeKtlConfig(
    __in KAllocator & allocator,
    __in std::wstring workDir,
    __in std::wstring fileName,
    __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings)
{
    auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();
    KString::SPtr sharedLogFileName;
    BOOLEAN res;
    NTSTATUS status;

#if !defined(PLATFORM_UNIX) 
    status = KString::Create(
        sharedLogFileName,
        allocator,
        L"\\??\\");
#else
    status = KString::Create(
        sharedLogFileName,
        allocator);
#endif

    KInvariant(NT_SUCCESS(status));

    res = sharedLogFileName->Concat(workDir.c_str());
    KInvariant(res);

    res = sharedLogFileName->Concat(KVolumeNamespace::PathSeparator);
    KInvariant(res);

    res = sharedLogFileName->Concat(fileName.c_str());
    KInvariant(res);

    if (!Common::Directory::Exists(workDir))
    {
        Common::Directory::Create(workDir);
    }

    KInvariant(sharedLogFileName->LengthInBytes() + sizeof(WCHAR) < 512 * sizeof(WCHAR)); // check to make sure there is space for the null terminator
    KMemCpySafe(&settings->Path[0], 512 * sizeof(WCHAR), sharedLogFileName->operator PVOID(), sharedLogFileName->LengthInBytes());
    settings->Path[sharedLogFileName->LengthInBytes() / sizeof(WCHAR)] = L'\0'; // set the null terminator
    settings->LogContainerId.GetReference().CreateNew();
    settings->LogSize = 1024 * 1024 * 1024;
    settings->MaximumNumberStreams = 0;
    settings->MaximumRecordSize = 0;
    settings->Flags = static_cast<ULONG>(Data::Log::LogCreationFlags::UseSparseFile);
    sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
};

namespace TransactionalReplicatorTests
{
    using namespace Common;
    using namespace Data;
    using namespace Utilities;
    using namespace ktl;
    using namespace TxnReplicator;
    using namespace TxnReplicator::TestCommon;

    std::wstring const DefaultSharedLogContainerGuid(L"3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62");
    std::wstring const DefaultTestSubDirectoryName(L"ComTransactionalReplicator_Test");
    std::wstring const TestLogFileName(L"ComTransactionalReplicatorTest.log");

    class ComTransactionalReplicatorTest 
        : ComponentRoot
    {
    private:
        CommonConfig config_; // load the config object as its needed for the tracing to work

        ComPointer<IFabricPrimaryReplicator> primaryReplicator_;
        ITransactionalReplicator::SPtr transactionalReplicator_;
        Reliability::ReplicationComponent::IReplicatorFactoryUPtr replicatorFactory_;
        ITransactionalReplicatorFactoryUPtr transactionalReplicatorFactory_;

        void Create(
            __in KtlSystem & ktlSystem, 
            __in Data::Log::LogManager & logManager, 
            __in_opt KGuid partitionId = KGuid(true), 
            __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler = nullptr)
        {
            KAllocator & allocator = ktlSystem.NonPagedAllocator();

            NTSTATUS status;
            HANDLE handle = nullptr;

            if(partitionId == KGuid(true))
            {
                partitionId.CreateNew();
            }

            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(partitionId, 17, allocator);

            Data::TestCommon::TestComStatefulServicePartition::SPtr partition = Data::TestCommon::TestComStatefulServicePartition::Create(partitionId, allocator);
            Data::TestCommon::TestComCodePackageActivationContext::SPtr codePackage = Data::TestCommon::TestComCodePackageActivationContext::Create(GetWorkingDirectory().c_str(), allocator);

            Reliability::ReplicationComponent::ReplicatorFactoryConstructorParameters replicatorFactoryConstructorParameters;
            replicatorFactoryConstructorParameters.Root = this;
            replicatorFactory_ = Reliability::ReplicationComponent::ReplicatorFactoryFactory(replicatorFactoryConstructorParameters);
            replicatorFactory_->Open(L"empty");

            TransactionalReplicatorFactoryConstructorParameters transactionalReplicatorFactoryConstructorParameters;
            transactionalReplicatorFactoryConstructorParameters.Root = this;
            transactionalReplicatorFactory_ = TransactionalReplicatorFactoryFactory(transactionalReplicatorFactoryConstructorParameters);
            transactionalReplicatorFactory_->Open(ktlSystem, logManager);

            TxnReplicator::TestCommon::TestStateProviderFactory::SPtr factory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(allocator);
            ComPointer<IFabricStateProvider2Factory> comFactory = TxnReplicator::TestCommon::TestComStateProvider2Factory::Create(
                *factory, allocator);

            TestHealthClientSPtr healthClient = TestHealthClient::Create(true);

            TxnReplicator::IDataLossHandler::SPtr localDataLossHandlerSPtr = dataLossHandler;
            if (localDataLossHandlerSPtr == nullptr)
            {
                localDataLossHandlerSPtr = TxnReplicator::TestCommon::TestDataLossHandler::Create(allocator).RawPtr();
            }

            ComPointer<IFabricDataLossHandler> comProxyDataLossHandler = TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(
                allocator,
                localDataLossHandlerSPtr.RawPtr());

            TRANSACTIONAL_REPLICATOR_SETTINGS txnReplicatorSettings = { 0 };
            txnReplicatorSettings.Flags = FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION;

            // Overwrite the SerializationVersion to native here because we test the initialization parameters with multiple buffers
            // in the test suite. In the managed version, the initialization parameters could only have at most 1 buffer. 
            txnReplicatorSettings.SerializationVersion = 1;

            status = transactionalReplicatorFactory_->CreateReplicator(
                10,
                *replicatorFactory_,
                partition.RawPtr(),
                nullptr,
                &txnReplicatorSettings,
                nullptr,
                *codePackage,
                TRUE,
                move(healthClient),
                comFactory.GetRawPointer(),
                comProxyDataLossHandler.GetRawPointer(),
                primaryReplicator_.InitializationAddress(),
                &handle);

            ITransactionalReplicator * txnReplicatorPtr = static_cast<ITransactionalReplicator *>(handle);
            CODING_ERROR_ASSERT(txnReplicatorPtr != nullptr);

            transactionalReplicator_.Attach(txnReplicatorPtr);

            CODING_ERROR_ASSERT(primaryReplicator_.GetRawPointer() != nullptr);
            CODING_ERROR_ASSERT(transactionalReplicator_ != nullptr);
        };

        void Destruct()
        {
            replicatorFactory_->Close();
            replicatorFactory_.reset();
            transactionalReplicatorFactory_->Close();
            transactionalReplicatorFactory_.reset();
            primaryReplicator_.Release();

            transactionalReplicator_.Reset();
        };

        void ResetLogDirectory()
        {
            Directory::Delete_WithRetry(DefaultTestSubDirectoryName, true, true);
        }

        void Open()
        {
            HRESULT hr = S_OK;
            shared_ptr<ManualResetEvent> resetEvent = make_shared<ManualResetEvent>(false);
            ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > openCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
                [&](IFabricAsyncOperationContext * context)
            {
                ComPointer<IFabricStringResult> endpoint;
                hr = primaryReplicator_->EndOpen(context, endpoint.InitializationAddress());
                CODING_ERROR_ASSERT(SUCCEEDED(hr));
                resetEvent->Set();
            });
            
            ComPointer<IFabricAsyncOperationContext> openContext;
            hr = primaryReplicator_->BeginOpen(openCallback.GetRawPointer(), openContext.InitializationAddress());
            bool waitResult = resetEvent->WaitOne(10000); // open must complete in 10 seconds
            CODING_ERROR_ASSERT(waitResult);
        };

        void Close()
        {
            HRESULT hr = S_OK;
            shared_ptr<ManualResetEvent> resetEvent = make_shared<ManualResetEvent>(false);
            // Close the Replicator
            ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > closeCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
                [&](IFabricAsyncOperationContext * context)
            {
                hr = primaryReplicator_->EndClose(context);
                CODING_ERROR_ASSERT(SUCCEEDED(hr));
                resetEvent->Set();
            });
            
            ComPointer<IFabricAsyncOperationContext> closeContext;
            hr = primaryReplicator_->BeginClose(closeCallback.GetRawPointer(), closeContext.InitializationAddress());
            bool waitResult = resetEvent->WaitOne(10000); // close must complete in 10 seconds
            CODING_ERROR_ASSERT(waitResult);
        };

        void ChangeRole(
            __in FABRIC_EPOCH epoch, 
            __in FABRIC_REPLICA_ROLE newRole,
            __in_opt ULONG timeoutInMs = 10000,
            __in_opt HRESULT expectedBeginCRResult = S_OK)
        {
            HRESULT hr = S_OK;
            shared_ptr<ManualResetEvent> resetEvent = make_shared<ManualResetEvent>(false);
            // Change role replicator to primary
            ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > changeRoleCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
                [&](IFabricAsyncOperationContext * context)
            {
                hr = primaryReplicator_->EndChangeRole(context);
                CODING_ERROR_ASSERT(SUCCEEDED(hr));
                resetEvent->Set();
            });
            
            ComPointer<IFabricAsyncOperationContext> changeRoleContext;
            hr = primaryReplicator_->BeginChangeRole(
                &epoch,
                newRole,
                changeRoleCallback.GetRawPointer(), 
                changeRoleContext.InitializationAddress());
            
            CODING_ERROR_ASSERT(hr == expectedBeginCRResult);
            
            if (SUCCEEDED(hr))
            {
                bool waitResult = resetEvent->WaitOne(timeoutInMs); // change role must complete in 10 seconds
                CODING_ERROR_ASSERT(waitResult);
            }
        };

        void OnDataLoss(__in BOOLEAN * isStateChanged)
        {
            HRESULT hr = S_OK;
            shared_ptr<ManualResetEvent> resetEvent = make_shared<ManualResetEvent>(false);
            ComPointer<Data::TestCommon::ComAsyncOperationCallbackTestHelper > openCallback = make_com<Data::TestCommon::ComAsyncOperationCallbackTestHelper>(
                [&](IFabricAsyncOperationContext * context)
            {
                hr = primaryReplicator_->EndOnDataLoss(context, isStateChanged);
                CODING_ERROR_ASSERT(SUCCEEDED(hr));
                resetEvent->Set();
            });

            ComPointer<IFabricAsyncOperationContext> openContext;
            hr = primaryReplicator_->BeginOnDataLoss(openCallback.GetRawPointer(), openContext.InitializationAddress());
            bool waitResult = resetEvent->WaitOne(10000); // OnDataLoss must complete in 10 seconds
            CODING_ERROR_ASSERT(waitResult);
        };

    public:
        std::wstring GetWorkingDirectory()
        {
            std::wstring workDir = Directory::GetCurrentDirectoryW();
            workDir = Path::Combine(workDir, DefaultTestSubDirectoryName);
            workDir = Path::Combine(workDir, DefaultSharedLogContainerGuid);
            return workDir;
        }

        void Test_Create(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            Create(ktlSystem, logManager);
            Destruct();
            ResetLogDirectory();
        };

        void Test_OpenClose(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            Create(ktlSystem, logManager);
            Open();
            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_ChangeRoleToPrimary(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            Create(ktlSystem, logManager);
            Open();
            
            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);
            
            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_CRFails_AllFurtherCRFails(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            Create(ktlSystem, logManager);
            Open();
            
            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            ComTransactionalReplicator * comTxnReplicator = static_cast<ComTransactionalReplicator*>(primaryReplicator_.GetRawPointer());
            comTxnReplicator->Test_SetFaulted();

            e.DataLossNumber = 20;
            e.ConfigurationNumber = 20;
            ChangeRole(e, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, 10000, E_UNEXPECTED);
            
            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_BasicOperations(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager,
            __in bool initParameterNull,
            __in_opt ULONG32 bufferCount = 2,
            __in_opt ULONG32 bufferSize = 16)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            Create(ktlSystem, logManager);
            Open();

            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            KUriView const stateProviderName(L"fabric:/stateProvider");
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());
            OperationData::SPtr initParameters = nullptr;

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });

                if (!initParameterNull)
                {
                    KAllocator & allocator = ktlSystem.NonPagedAllocator();
                    initParameters = CreateInitParameters(allocator, bufferCount, bufferSize);
                }

                status = SyncAwait(
                    transactionalReplicator_->AddAsync(
                        *txn,
                        stateProviderName,
                        typeName,
                        initParameters.RawPtr()));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                SyncAwait(txn->CommitAsync());
            }

            IStateProvider2::SPtr stateProvider;
            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            TxnReplicator::TestCommon::TestStateProvider & testStateProvider = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);

            if (initParameterNull)
            {
                CODING_ERROR_ASSERT(testStateProvider.GetInitializationParameters() == nullptr);
            }
            else
            {
                // Verify the buffer is as expected
                VerifyInitParameters(*testStateProvider.GetInitializationParameters(), bufferCount, bufferSize);
            }

            KString::SPtr value;
            FABRIC_SEQUENCE_NUMBER version = -1;
            SyncAwait(testStateProvider.GetAsync(version, value));
            CODING_ERROR_ASSERT(version == 1);
            CODING_ERROR_ASSERT(value->IsEmpty() == TRUE);

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                FABRIC_SEQUENCE_NUMBER temp;

                SyncAwait(testStateProvider.TryUpdateAsync(*txn, L"newValue", temp, 1));
                SyncAwait(txn->CommitAsync());
            }

            SyncAwait(testStateProvider.GetAsync(version, value));
            CODING_ERROR_ASSERT(version > 1);
            CODING_ERROR_ASSERT(value != nullptr);
            auto comparison = value->Compare(L"newValue");
            CODING_ERROR_ASSERT(comparison == 0);

            { 
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                status = SyncAwait(
                    transactionalReplicator_->RemoveAsync(
                        *txn,
                        stateProviderName));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                SyncAwait(txn->CommitAsync());
            }

            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);

            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_GetOrAdd(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            Create(ktlSystem, logManager);
            Open();

            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            KUriView const stateProviderName(L"fabric:/stateProvider");
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());
            OperationData::SPtr initParameters = nullptr;
            IStateProvider2::SPtr stateProvider;

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                
                status = SyncAwait(
                    transactionalReplicator_->AddAsync(
                        *txn,
                        stateProviderName,
                        typeName,
                        initParameters.RawPtr()));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                SyncAwait(txn->CommitAsync());
            }

            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                bool alreadyExist = false;
                status = SyncAwait(
                    transactionalReplicator_->GetOrAddAsync(
                        *txn,
                        stateProviderName,
                        typeName,
                        stateProvider,
                        alreadyExist));

                // The state provider exists, GetOrAddAsync sets alreadyExist as true
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(alreadyExist);
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());
                SyncAwait(txn->CommitAsync());
            }

            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            KUriView const stateProviderName1(L"fabric:/stateProvider1");

            status = transactionalReplicator_->Get(stateProviderName1, stateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                bool alreadyExist = false;
                status = SyncAwait(
                    transactionalReplicator_->GetOrAddAsync(
                        *txn,
                        stateProviderName1,
                        typeName,
                        stateProvider,
                        alreadyExist));

                // The state provider does not exist, GetOrAddAsync sets alreadyExist as false
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_FALSE(alreadyExist);
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(stateProviderName1, stateProvider->GetName());
                SyncAwait(txn->CommitAsync());
            }

            status = transactionalReplicator_->Get(stateProviderName1, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName1, stateProvider->GetName());

            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_TestStateProviderCheckpoint(
            __in bool openTwice,
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            KGuid partitionId;
            partitionId.CreateNew();

            Create(ktlSystem, logManager, partitionId);
            Open();

            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            KUriView const stateProviderName(L"fabric:/stateProvider");
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });

                status = SyncAwait(
                    transactionalReplicator_->AddAsync(
                        *txn,
                        stateProviderName,
                        typeName));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                SyncAwait(txn->CommitAsync());
            }

            IStateProvider2::SPtr stateProvider;
            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            TxnReplicator::TestCommon::TestStateProvider & testStateProvider = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);
            CODING_ERROR_ASSERT(testStateProvider.GetInitializationParameters() == nullptr);

            // Update the value and version for the first time to avoid writing empty string to the checkpoint file 
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                FABRIC_SEQUENCE_NUMBER temp;

                bool result = SyncAwait(testStateProvider.TryUpdateAsync(*txn, L"newValue", temp, 1));
                CODING_ERROR_ASSERT(result)

                SyncAwait(txn->CommitAsync());
            }

            KString::SPtr expectedValue1;
            FABRIC_SEQUENCE_NUMBER expectedVersion1 = FABRIC_INVALID_SEQUENCE_NUMBER;
            SyncAwait(testStateProvider.GetAsync(expectedVersion1, expectedValue1));
            CODING_ERROR_ASSERT(expectedVersion1 != FABRIC_INVALID_SEQUENCE_NUMBER)
            CODING_ERROR_ASSERT(expectedValue1->IsEmpty() == FALSE)

            // Take checkpoint when next transaction committed
            transactionalReplicator_->Test_RequestCheckpointAfterNextTransaction();

            // Update the value and version for the second time
            // So the value and version written to the checkpoint file must be one of the updates
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                FABRIC_SEQUENCE_NUMBER temp = 0;

                SyncAwait(testStateProvider.TryUpdateAsync(*txn, L"newValue", temp, 1));

                SyncAwait(txn->CommitAsync());
            }

            KString::SPtr expectedValue2;
            FABRIC_SEQUENCE_NUMBER expectedVersion2 = FABRIC_INVALID_SEQUENCE_NUMBER;

            SyncAwait(testStateProvider.GetAsync(expectedVersion2, expectedValue2));
            CODING_ERROR_ASSERT(expectedVersion2 != FABRIC_INVALID_SEQUENCE_NUMBER)
            CODING_ERROR_ASSERT(expectedValue2->IsEmpty() == FALSE)

            Close();
            Destruct();

            Create(ktlSystem, logManager, partitionId);
            Open();

            e.Reserved = nullptr;
            e.DataLossNumber = 20;
            e.ConfigurationNumber = 20;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            TxnReplicator::TestCommon::TestStateProvider & testStateProvider2 = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);

            KString::SPtr value;
            FABRIC_SEQUENCE_NUMBER version = FABRIC_INVALID_SEQUENCE_NUMBER;
            SyncAwait(testStateProvider2.GetAsync(version, value));

            ASSERT_IFNOT(
                version == expectedVersion1 || version == expectedVersion2, 
                "version read from the state provider is {0}, but the expected verion1 is {1} and version2 is {2}", 
                expectedVersion1,
                expectedVersion2)

            ASSERT_IFNOT(
                *value == *expectedValue1 || *value == *expectedValue2,
                "value read from the state provider is {0}, but the expected value1 is {1} and value2 is {2}",
                ToStringLiteral(*value),
                ToStringLiteral(*expectedValue1),
                ToStringLiteral(*expectedValue2))

            Close();
            Destruct();

            // Recover from the checkpoint file for the second time, make sure the second open will not change the file size and recover is fine
            if (openTwice)
            {
                Create(ktlSystem, logManager, partitionId);
                Open();
   

                e.Reserved = nullptr;
                e.DataLossNumber = 30;
                e.ConfigurationNumber = 30;
                ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

                status = transactionalReplicator_->Get(stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

                TxnReplicator::TestCommon::TestStateProvider & testStateProvider3 = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);

                SyncAwait(testStateProvider3.GetAsync(version, value));

                ASSERT_IFNOT(
                    version == expectedVersion1 || version == expectedVersion2,
                    "version read from the state provider is {0}, but the expected verion1 is {1} and version2 is {2}",
                    expectedVersion1,
                    expectedVersion2)

                ASSERT_IFNOT(
                    *value == *expectedValue1 || *value == *expectedValue2,
                    "value read from the state provider is {0}, but the expected value1 is {1} and value2 is {2}",
                    ToStringLiteral(*value),
                    ToStringLiteral(*expectedValue1),
                    ToStringLiteral(*expectedValue2))

                Close();
                Destruct();
            }

            ResetLogDirectory();
        };


        void Test_TestStateProvider_Copy(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            KGuid partitionId;
            partitionId.CreateNew();

            Create(ktlSystem, logManager, partitionId);
            Open();

            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            KUriView const stateProviderName(L"fabric:/stateProvider");
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });

                status = SyncAwait(
                    transactionalReplicator_->AddAsync(
                        *txn,
                        stateProviderName,
                        typeName));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                SyncAwait(txn->CommitAsync());
            }

            IStateProvider2::SPtr stateProvider;
            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            TxnReplicator::TestCommon::TestStateProvider & testStateProvider = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);
            CODING_ERROR_ASSERT(testStateProvider.GetInitializationParameters() == nullptr);

            // Update the value and version for the first time to avoid writing empty string to the checkpoint file 
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                FABRIC_SEQUENCE_NUMBER temp;

                bool result = SyncAwait(testStateProvider.TryUpdateAsync(*txn, L"newValue", temp, 1));
                CODING_ERROR_ASSERT(result);

                SyncAwait(txn->CommitAsync());
            }

            KString::SPtr expectedValue1;
            FABRIC_SEQUENCE_NUMBER expectedVersion1 = FABRIC_INVALID_SEQUENCE_NUMBER;
            SyncAwait(testStateProvider.GetAsync(expectedVersion1, expectedValue1));
            CODING_ERROR_ASSERT(expectedVersion1 != FABRIC_INVALID_SEQUENCE_NUMBER)
            CODING_ERROR_ASSERT(expectedValue1->IsEmpty() == FALSE);

            // Take checkpoint when next transaction committed
            transactionalReplicator_->Test_RequestCheckpointAfterNextTransaction();

            // Update the value and version for the second time
            // So the value and version written to the checkpoint file must be one of the updates
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                FABRIC_SEQUENCE_NUMBER temp = 0;

                SyncAwait(testStateProvider.TryUpdateAsync(*txn, L"newValue", temp, 1));

                SyncAwait(txn->CommitAsync());
            }

            KString::SPtr expectedValue2;
            FABRIC_SEQUENCE_NUMBER expectedVersion2 = FABRIC_INVALID_SEQUENCE_NUMBER;

            SyncAwait(testStateProvider.GetAsync(expectedVersion2, expectedValue2));
            CODING_ERROR_ASSERT(expectedVersion2 != FABRIC_INVALID_SEQUENCE_NUMBER);
            CODING_ERROR_ASSERT(expectedValue2->IsEmpty() == FALSE);

            Close();
            Destruct();

            Create(ktlSystem, logManager, partitionId);
            Open();

            e.Reserved = nullptr;
            e.DataLossNumber = 20;
            e.ConfigurationNumber = 20;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            TxnReplicator::TestCommon::TestStateProvider & testStateProvider2 = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);

            KString::SPtr value;
            FABRIC_SEQUENCE_NUMBER version = FABRIC_INVALID_SEQUENCE_NUMBER;
            SyncAwait(testStateProvider2.GetAsync(version, value));

            ASSERT_IFNOT(
                version == expectedVersion1 || version == expectedVersion2,
                "version read from the state provider is {0}, but the expected verion1 is {1} and version2 is {2}",
                expectedVersion1,
                expectedVersion2);

            ASSERT_IFNOT(
                *value == *expectedValue1 || *value == *expectedValue2,
                "value read from the state provider is {0}, but the expected value1 is {1} and value2 is {2}",
                ToStringLiteral(*value),
                ToStringLiteral(*expectedValue1),
                ToStringLiteral(*expectedValue2));

            // This is a simple test to verify that we do not assert.
            {
                OperationDataStream::SPtr copyStream = testStateProvider2.GetCurrentState();
                KFinally([&] {copyStream->Dispose(); });
                
                OperationData::CSPtr data;
                while (true)
                {
                    SyncAwait(copyStream->GetNextAsync(CancellationToken::None, data));
                    if (data == nullptr)
                    {
                        break;
                    }
                }
            }

            // Take checkpoint when next transaction committed
            transactionalReplicator_->Test_RequestCheckpointAfterNextTransaction();

            // Testing copy after the checkpoint.
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                FABRIC_SEQUENCE_NUMBER temp = 0;

                SyncAwait(testStateProvider2.TryUpdateAsync(*txn, L"newerValue", temp, version));

                SyncAwait(txn->CommitAsync());
            }

            // This is a simple test to verify that we do not assert.
            {
                OperationDataStream::SPtr copyStream = testStateProvider2.GetCurrentState();
                KFinally([&] {copyStream->Dispose(); });

                OperationData::CSPtr data;
                while (true)
                {
                    SyncAwait(copyStream->GetNextAsync(CancellationToken::None, data));
                    if (data == nullptr)
                    {
                        break;
                    }
                }
            }

            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_VersionManager(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            KGuid partitionId;
            partitionId.CreateNew();

            Create(ktlSystem, logManager, partitionId);
            Open();

            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            // Test 0: Test that before any operation, GetVisibilitySequenceNumberAsync
            FABRIC_SEQUENCE_NUMBER vsn0 = FABRIC_INVALID_SEQUENCE_NUMBER;
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                SyncAwait(txn->GetVisibilitySequenceNumberAsync(vsn0));

                // Visibility sequence number needs to be greater than zero.
                VERIFY_IS_TRUE(vsn0 > 0);

                FABRIC_SEQUENCE_NUMBER duplicateVsn = FABRIC_INVALID_SEQUENCE_NUMBER;
                SyncAwait(txn->GetVisibilitySequenceNumberAsync(duplicateVsn));

                VERIFY_ARE_EQUAL(vsn0, duplicateVsn);
            }

            KUriView const stateProviderName(L"fabric:/stateProvider");
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());

            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });
                status = SyncAwait(
                    transactionalReplicator_->AddAsync(
                        *txn,
                        stateProviderName,
                        typeName));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                SyncAwait(txn->CommitAsync());
            }

            // Test 1: Verify that when there are other operations, VSN increases.
            FABRIC_SEQUENCE_NUMBER vsn1 = FABRIC_INVALID_SEQUENCE_NUMBER;
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });

                SyncAwait(txn->GetVisibilitySequenceNumberAsync(vsn1));

                // Visibility sequence number needs to be greater than zero.
                VERIFY_IS_TRUE(vsn1 > vsn0);

                FABRIC_SEQUENCE_NUMBER duplicateVsn = FABRIC_INVALID_SEQUENCE_NUMBER;
                SyncAwait(txn->GetVisibilitySequenceNumberAsync(duplicateVsn));

                VERIFY_ARE_EQUAL(vsn1, duplicateVsn);
            }

            IStateProvider2::SPtr stateProvider;
            status = transactionalReplicator_->Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, stateProvider->GetName());

            TxnReplicator::TestCommon::TestStateProvider & testStateProvider = dynamic_cast<TxnReplicator::TestCommon::TestStateProvider &>(*stateProvider);

            // Test 2: Verify that TryRemoveVersion and TryRemoveCheckpoint for invisible versions return CanBeRemoved
            // Note by using "vsn1 + 1" as next, we ensure that the UnRegister is called on Test 0 & 1.
            {
                KSharedPtr<TryRemoveVersionResult> tryRemoveResult;
                transactionalReplicator_->TryRemoveVersion(testStateProvider.StateProviderId, 1, vsn1 + 1, tryRemoveResult);
                VERIFY_IS_TRUE(tryRemoveResult->CanBeRemoved);

                SyncAwait(transactionalReplicator_->TryRemoveCheckpointAsync(1, vsn1 + 1));
            }

            // Test 3: Verify that TryRemoveVersion and TryRemoveCheckpoint for visible versions return not removable.
            // And the unregister unblocks them.
            TryRemoveVersionResult::SPtr removeResult = nullptr;;
            Awaitable<NTSTATUS> checkpointResult;
            {
                Transaction::SPtr txn;
                transactionalReplicator_->CreateTransaction(txn);
                KFinally([&]() { txn->Dispose(); });

                LONG64 vsn2;
                SyncAwait(txn->GetVisibilitySequenceNumberAsync(vsn2));

                // Visibility sequence number needs to be greater than zero.
                // vsn2 may be equal to vsn1 + 1 due to 'idle' barrier insertion
                VERIFY_IS_TRUE(vsn1 == vsn2 || vsn1 + 1 == vsn2);

                transactionalReplicator_->TryRemoveVersion(testStateProvider.StateProviderId, 1, vsn2 + 1, removeResult);
                VERIFY_IS_FALSE(removeResult->CanBeRemoved);
                VERIFY_ARE_EQUAL(removeResult->EnumerationSet->Count(), 1);
                VERIFY_ARE_EQUAL(removeResult->EnumerationCompletionNotifications->Count(), 1);

                checkpointResult = Ktl::Move(transactionalReplicator_->TryRemoveCheckpointAsync(1, vsn2 + 1));
                VERIFY_IS_FALSE(checkpointResult.IsComplete());
            }

            auto enumerator = removeResult->EnumerationCompletionNotifications->GetEnumerator();
            VERIFY_IS_TRUE(enumerator->MoveNext());

            SyncAwait(enumerator->Current()->Notification);
            SyncAwait(checkpointResult);

            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_ChangeRoleToIdle(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            Create(ktlSystem, logManager);
            Open();
            
            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            
            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_PrimarySwap(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            Create(ktlSystem, logManager);
            Open();
            
            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);
            e.DataLossNumber = 20;
            e.ConfigurationNumber = 20;
            ChangeRole(e, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            
            Close();
            Destruct();
            ResetLogDirectory();
        };

        void Test_DataLossHandler(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager)
        {
            KGuid partitionId;
            partitionId.CreateNew();
            Create(ktlSystem, logManager, partitionId);
            Open();

            FABRIC_EPOCH e;
            e.Reserved = nullptr;
            e.DataLossNumber = 10;
            e.ConfigurationNumber = 10;
            ChangeRole(e, FABRIC_REPLICA_ROLE_PRIMARY);

            BOOLEAN isStateChanged = FALSE;
            OnDataLoss(&isStateChanged);

            CODING_ERROR_ASSERT(isStateChanged == FALSE);

            Close();
            Destruct();
            ResetLogDirectory();
        };

        OperationData::SPtr CreateInitParameters(
            __in KAllocator & allocator,
            __in_opt ULONG32 bufferCount,
            __in_opt ULONG32 bufferSize)
        {
            OperationData::SPtr initParameters = OperationData::Create(allocator);

            for (ULONG32 i = 0; i < bufferCount; i++)
            {
                KBuffer::SPtr expectedBuffer = nullptr;
                NTSTATUS status = KBuffer::Create(bufferSize, expectedBuffer, allocator, KTL_TAG_TEST);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                // Create a byte array needs the const array size
                // Set the size to 1024 because the largest buffer we created for test is 512.
                CODING_ERROR_ASSERT(bufferSize < 1024);
                byte temp[1024] = {};

                for (ULONG32 j = 0; j < bufferSize; j++)
                {
                    temp[j] = static_cast<byte>(i);
                }

                byte * point = static_cast<byte*>(expectedBuffer->GetBuffer());
                KMemCpySafe(point, bufferSize, temp, bufferSize);

                // If append failed, it will throw status, thus we failed the CreateInitParameters
                try
                {
                    initParameters->Append(*expectedBuffer);
                }
                catch (Exception & exception)
                {
                    CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_UNSUCCESSFUL);
                    CODING_ERROR_ASSERT(false);
                }
            }

            return initParameters;
        }

        void VerifyInitParameters(
            __in OperationData const & initParameters,
            __in_opt ULONG32 bufferCount,
            __in_opt ULONG32 bufferSize)
        {
            CODING_ERROR_ASSERT(bufferCount == initParameters.get_BufferCount());

            for (ULONG32 i = 0; i < bufferCount; i++)
            {
                KBuffer::SPtr buffer = const_cast<KBuffer *>(initParameters[i].RawPtr());

                CODING_ERROR_ASSERT(buffer->QuerySize() == bufferSize);

                byte * point = static_cast<byte*>(buffer->GetBuffer());

                for (ULONG32 j = 0; j < bufferSize; j++)
                {
                    CODING_ERROR_ASSERT(point[j] == static_cast<byte>(i));
                }
            }
        }
    };

    BOOST_FIXTURE_TEST_SUITE(ComTransactionalReplicatorTestSuite, ComTransactionalReplicatorTest)
            
    BOOST_AUTO_TEST_CASE(Life_CreateDelete_Success)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;

        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_Create(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_CASE(Life_OpenClose_Success)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;

        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_OpenClose(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_CASE(Life_ChangeRoleToP_Success)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;

        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_ChangeRoleToPrimary(underlyingSystem, *logManager);
        }
        
        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_CASE(Life_ChangeRoleToI_Success)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

    KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_ChangeRoleToIdle(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_CASE(Life_PrimarySwap_Success)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_PrimarySwap(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_CASE(Life_CRFails_EnsureAllFurtherCRFails)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;

        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_CRFails_AllFurtherCRFails(underlyingSystem, *logManager);
        }
        
        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test Basic Operation with state provider which has null initialization parameter
    BOOST_AUTO_TEST_CASE(Basic)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_BasicOperations(underlyingSystem, *logManager, true);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test Basic Operation GetOrAdd, test both get and add situation 
    BOOST_AUTO_TEST_CASE(Basic_GetOrAdd)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_GetOrAdd(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test Basic Operation with state provider which contains small initialization parameter info
    BOOST_AUTO_TEST_CASE(Basic_WithSmallInitParameters)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_BasicOperations(underlyingSystem, *logManager, false);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test Basic Operation with state provider which contains large initialization parameter info
    BOOST_AUTO_TEST_CASE(Basic_WithLargeInitParameters)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_BasicOperations(underlyingSystem, *logManager, false, 512, 16);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test State Provider Checkpoint and Recover:
    // Add state provider, update the version and value, take checkpoint
    // close replicator, create replicator with same patitionId, the recovery will be called when 
    // Open() and the state provider will be recovered from checkpoint file
    BOOST_AUTO_TEST_CASE(Basic_TestStateProviderCheckpoint)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_TestStateProviderCheckpoint(false, underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test State Provider Checkpoint and Recover:
    // Recover form the checkpoint file twice
    BOOST_AUTO_TEST_CASE(Basic_TestStateProviderCheckpoint_OpenTwice)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_TestStateProviderCheckpoint(true, underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_CASE(Basic_TestStateProvider_Copy)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_TestStateProvider_Copy(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Tests basic functionality of VersionManager: RegisterAsync, TryRemoveVersion, TryRemoveCheckpointAsync.
    BOOST_AUTO_TEST_CASE(Basic_VersionManager)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_VersionManager(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    // Test the basic functionality of DataLossHandler
    BOOST_AUTO_TEST_CASE(Basic_DataLossHandler)
    {
        KGuid nodeId;
        nodeId.CreateNew();
        KtlSystemBase underlyingSystem(nodeId);
        underlyingSystem.Activate(5000);
        NTSTATUS status = underlyingSystem.Status();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem.SetStrictAllocationChecks(TRUE);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
        TR_Test_InitializeKtlConfig(
            underlyingSystem.NonPagedAllocator(),
            GetWorkingDirectory(),
            TestLogFileName,
            sharedLogSettings);

        KInvariant(sharedLogSettings != nullptr);

        Data::Log::LogManager::SPtr logManager;
        status = Data::Log::LogManager::Create(underlyingSystem.NonPagedAllocator(), logManager);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        {
            Test_DataLossHandler(underlyingSystem, *logManager);
        }

        status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        logManager = nullptr;

        underlyingSystem.Deactivate(5000);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
