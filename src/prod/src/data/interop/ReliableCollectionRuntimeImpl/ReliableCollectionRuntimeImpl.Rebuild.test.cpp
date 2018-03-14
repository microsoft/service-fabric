// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Data::Integration;

namespace Data
{
    namespace Interop
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;
        using namespace TxnReplicator;
        using namespace Data::TStore;
        using namespace Data;

        StringLiteral const TraceComponent("ReliableCollectionRuntimeImplTests.BackupRebuild");
        std::wstring const TestLogFileName(L"ReliableCollectionRuntimeImplTests.BackupRebuild.log");

        typedef function<void(__in TxnReplicator::ITransactionalReplicator & source,
            __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>> & stateProviders)> RebuiltCallback;

        class ReliableCollectionRuntimeImpl_Rebuild_Tests
        {
        public:
            ReliableCollectionRuntimeImpl_Rebuild_Tests()
            {
            }
            ~ReliableCollectionRuntimeImpl_Rebuild_Tests()
            {
            }

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

            Awaitable<void> AddStateProviderAsync(
                __in Replica::SPtr &replica,
                __in KUri::CSPtr &stateProviderName);

            Awaitable<void> AddKeyValuePairAsync(
                __in Replica::SPtr &replica,
                __in IStore<KString::SPtr, KBuffer::SPtr>* store,
                __in wstring const &key,
                __in size_t objHandle,
                __in wstring const &value);

            bool IsSameReplica(
                __in KGuid const & sourcePartitionId,
                __in FABRIC_REPLICA_ID sourceReplicaID,
                __in KGuid const & targetPartitionId,
                __in FABRIC_REPLICA_ID targetReplicaID);

            Awaitable<void> Test_DictionaryRebuilt(
                __in function<Awaitable<void>(Replica::SPtr &)> createStateProviderCallback,
                __in function<Awaitable<void>(Replica::SPtr &)> createDataCallback,
                __in RebuiltCallback beforeRestoreCallback,
                __in wstring const & testFolder,
                __in wstring const & backupFolder,
                __in int numberOfStateProvidersInFullCheckpoint,
                __in int numberOfStateProvidersPerIteration,
                __in int numberOfIterations,
                __in Data::Log::LogManager & logManager,
                __in KGuid const & sourcePartitionId,
                __in FABRIC_REPLICA_ID sourceReplicaID,
                __in KGuid const & targetPartitionId,
                __in FABRIC_REPLICA_ID targetReplicaID);

        protected:
            KtlSystem * underlyingSystem_;
        };

        wstring ReliableCollectionRuntimeImpl_Rebuild_Tests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void ReliableCollectionRuntimeImpl_Rebuild_Tests::InitializeKtlConfig(
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
            settings->Flags = static_cast<ULONG>(Data::Log::LogCreationFlags::UseSparseFile);
            sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
        }

        KUri::CSPtr ReliableCollectionRuntimeImpl_Rebuild_Tests::GetStateProviderName(
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

        Awaitable<void> ReliableCollectionRuntimeImpl_Rebuild_Tests::AddStateProviderAsync(
            __in Replica::SPtr &replica,
            __in KUri::CSPtr &stateProviderName)
        {
            NTSTATUS status = STATUS_SUCCESS;
            Transaction::SPtr txn;

            status = replica->TxnReplicator->CreateTransaction(txn);
            THROW_ON_FAILURE(status);
            KFinally([&] {txn->Dispose(); });

            status = co_await replica->TxnReplicator->AddAsync(*txn, *stateProviderName, L"0\1\1\n");
            THROW_ON_FAILURE(status);

            status = co_await txn->CommitAsync();
            THROW_ON_FAILURE(status);
        }

        Awaitable<void> ReliableCollectionRuntimeImpl_Rebuild_Tests::AddKeyValuePairAsync(
            __in Replica::SPtr &replica,
            __in Data::TStore::IStore<KString::SPtr, KBuffer::SPtr>* store,
            __in wstring const &key,
            __in size_t objHandle,
            __in wstring const &value)
        {
            NTSTATUS status = STATUS_SUCCESS;
            Transaction::SPtr txn;

            status = replica->TxnReplicator->CreateTransaction(txn);
            THROW_ON_FAILURE(status);
            KFinally([&] {txn->Dispose(); });

            KSharedPtr<IStoreTransaction<KString::SPtr, KBuffer::SPtr>> storetxn;
            store->CreateOrFindTransaction(*txn, storetxn);

            KBuffer::SPtr kBuffer;
            KString::SPtr kstringkey;
            KString::Create(kstringkey, underlyingSystem_->PagedAllocator(), key.c_str());

            ULONG bufferLength = (ULONG)(value.size() + 1) * sizeof(value[0]);
#ifdef FEATURE_CACHE_OBJHANDLE
            bufferLength += sizeof(size_t);
#endif
            KBuffer::Create(bufferLength, kBuffer, underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG);

            byte* buffer = (byte*)kBuffer->GetBuffer();
#ifdef FEATURE_CACHE_OBJHANDLE
            *(size_t*)buffer = objHandle;
            buffer += sizeof(size_t);
#else
            UNREFERENCED_PARAMETER(objHandle);
#endif
            memcpy(buffer, value.c_str(), (value.size() + 1) * sizeof(value[0]));

            co_await store->AddAsync(*storetxn, kstringkey, kBuffer, TimeSpan::MaxValue, ktl::CancellationToken::None);
            THROW_ON_FAILURE(status);

            status = co_await txn->CommitAsync();
            THROW_ON_FAILURE(status);
        }

        bool ReliableCollectionRuntimeImpl_Rebuild_Tests::IsSameReplica(
            __in KGuid const & sourcePartitionId,
            __in FABRIC_REPLICA_ID sourceReplicaID,
            __in KGuid const & targetPartitionId,
            __in FABRIC_REPLICA_ID targetReplicaID)
        {
            return sourcePartitionId == targetPartitionId && sourceReplicaID == targetReplicaID;
        }    

        using ::_delete;

        class TestStateManagerChangeHandler final
            : public KObject<TestStateManagerChangeHandler>
            , public KShared<TestStateManagerChangeHandler>
            , public TxnReplicator::IStateManagerChangeHandler
        {
            K_FORCE_SHARED(TestStateManagerChangeHandler)
            K_SHARED_INTERFACE_IMP(IStateManagerChangeHandler)

        public:
            static SPtr Create(__in KAllocator & allocator, RebuiltCallback rebuiltCallback)
            {
                SPtr result = _new(TEST_CEXPORT_TAG, allocator) TestStateManagerChangeHandler(rebuiltCallback);
                CODING_ERROR_ASSERT(result != nullptr);
                CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

                return result;   
            }

        public: // IStateManagerChangeHandler APIs.
            void OnRebuilt(
                __in TxnReplicator::ITransactionalReplicator & source,
                __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>> & stateProviders) override
            {
                rebuiltCallback_(source, stateProviders);
            }

            void OnAdded(
                __in TxnReplicator::ITransactionalReplicator & source,
                __in TxnReplicator::ITransaction const & transaction,
                __in KUri const & stateProviderName,
                __in TxnReplicator::IStateProvider2 & stateProvider) override
            {
                UNREFERENCED_PARAMETER(source);
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(stateProviderName);
                UNREFERENCED_PARAMETER(stateProvider);
            }

            void OnRemoved(
                __in TxnReplicator::ITransactionalReplicator & source,
                __in TxnReplicator::ITransaction const & transaction,
                __in KUri const & stateProviderName,
                __in TxnReplicator::IStateProvider2 & stateProvider) override
            {
                UNREFERENCED_PARAMETER(source);
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(stateProviderName);
                UNREFERENCED_PARAMETER(stateProvider);
            }

        private:
            TestStateManagerChangeHandler(__in RebuiltCallback callback) 
                : rebuiltCallback_(callback) { }

        private:
            RebuiltCallback rebuiltCallback_;
        }; 

        TestStateManagerChangeHandler::~TestStateManagerChangeHandler() {}

        Awaitable<void> ReliableCollectionRuntimeImpl_Rebuild_Tests::Test_DictionaryRebuilt(
            __in function<Awaitable<void> (Replica::SPtr &)> createStateProviderCallback,
            __in function<Awaitable<void> (Replica::SPtr &)> createDataCallback,
            __in RebuiltCallback rebuiltCallback,
            __in wstring const & testFolder,
            __in wstring const & backupFolder,
            __in int numberOfStateProvidersInFullCheckpoint,
            __in int numberOfStateProvidersPerIteration,
            __in int numberOfIterations,
            __in Data::Log::LogManager & logManager,
            __in KGuid const & sourcePartitionId,
            __in FABRIC_REPLICA_ID sourceReplicaID,
            __in KGuid const & targetPartitionId,
            __in FABRIC_REPLICA_ID targetReplicaID)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            KAllocator & allocator = underlyingSystem_->PagedAllocator();

            StoreStateProviderFactory::SPtr stateProviderFactory = StoreStateProviderFactory::CreateStringUTF16BufferFactory(underlyingSystem_->PagedAllocator());

            KString::SPtr backupFolderPath = KPath::CreatePath(backupFolder.c_str(), allocator);
            Data::Utilities::OperationData::SPtr initParams = Data::Utilities::OperationData::Create(allocator);
            Data::Utilities::BinaryWriter bw(allocator);
            bw.Write(L"Initial Value", Data::Utilities::UTF16);
            initParams->Append(*bw.GetBuffer(0));

            {
                Replica::SPtr replica = Replica::Create(
                    sourcePartitionId,
                    sourceReplicaID,
                    testFolder,
                    logManager,
                    allocator,
                    stateProviderFactory.RawPtr());

                co_await replica->OpenAsync();

                // Become Primary after open:  
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                // 4. Update Status: Granted.
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                //
                // Create state providers as needed
                //
                co_await createStateProviderCallback(replica);
                
                status = replica->TxnReplicator->Test_RequestCheckpointAfterNextTransaction();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Put some data into the state provider
                //
                co_await createDataCallback(replica);

                // Close the replica:  
                // 1. Update Status: NOT PRIMARY
                // 2. Close
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                bool isSameReplica = IsSameReplica(sourcePartitionId, sourceReplicaID, targetPartitionId, targetReplicaID);
                if (isSameReplica == false)
                {
                    FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 1; epoch3.ConfigurationNumber = 2; epoch3.Reserved = nullptr;
                    co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_NONE);
                }

                co_await replica->CloseAsync();
            }

            {
                Replica::SPtr replica = Replica::Create(
                    targetPartitionId,
                    targetReplicaID,
                    testFolder,
                    logManager,
                    allocator,
                    stateProviderFactory.RawPtr());

                TestStateManagerChangeHandler::SPtr stateManagerChangedHandler = TestStateManagerChangeHandler::Create(allocator, rebuiltCallback);
                replica->TxnReplicator->RegisterStateManagerChangeHandler(*stateManagerChangedHandler);

                // Open the replica.
                co_await replica->OpenAsync();

                // Become Primary after open (For OnDataLossAsync):  
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 2; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_PRIMARY);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                // Verify that the replica is latest version number after recovery.
                {
                    FABRIC_EPOCH currentEpoch;
                    status = replica->TxnReplicator->GetCurrentEpoch(currentEpoch);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VERIFY_ARE_EQUAL(epoch2.DataLossNumber, 2);
                    VERIFY_ARE_EQUAL(epoch2.ConfigurationNumber, 2);
                }

                // Drop the replica:  
                // 1. Update Status: NOT PRIMARY
                // 2. Change Role to None
                // 3. Close
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 2; epoch3.ConfigurationNumber = 3; epoch3.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_NONE);

                co_await replica->CloseAsync();
            }

            co_return;
        }      

#define TEST_TRACE_BEGIN_BACKUP_REBUILD(testName) \
NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_); \
ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); \
underlyingSystem_->SetStrictAllocationChecks(TRUE); \
underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE); \
int seed = GetTickCount(); \
Common::Random random(seed); \
Trace.WriteWarning(TraceComponent, "Begin Test Case: {0} with seed {1}", testName, seed); \
KFinally([&]() { Trace.WriteWarning(TraceComponent, "Finishing Test Case {0}", testName); KtlSystem::Shutdown(); });

        BOOST_FIXTURE_TEST_SUITE(ReliableCollectionRuntimeImpl_BackupRebuild_TestSuite, ReliableCollectionRuntimeImpl_Rebuild_Tests);

        BOOST_AUTO_TEST_CASE(Store_SetNotifyStoreChangeCallback_RebuildNotification_SUCCESS)
        {
            //
            // @TODO - The whole backup restore part is completely unnecessary but it is useful for actual backup/restore testing later
            //

            // Setup
            wstring testName(L"Store_SetNotifyStoreChangeCallback_RebuildNotification_SUCCESS");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN_BACKUP_REBUILD(testName)
            {
                // KTL test input
                KGuid sourcePartitionId;
                sourcePartitionId.CreateNew();
                FABRIC_REPLICA_ID const sourceReplicaId = 777;

                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                IStateProvider2::SPtr stateProvider;
                KUri::CSPtr stateProviderName = GetStateProviderName(1);

                auto createStoreCallback = [&](Replica::SPtr &replica) -> Awaitable<void> {
                    co_await AddStateProviderAsync(replica, stateProviderName);

                    status = replica->TxnReplicator->Get(*stateProviderName, stateProvider);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider);
                };

                auto createDataCallback = [&](Replica::SPtr &replica) -> Awaitable<void> {
                    auto store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider.RawPtr());
                    co_await AddKeyValuePairAsync(replica, store, L"key1", 1, L"value1");
                    stateProvider = nullptr;
                };
                
                bool called;

                auto rebuiltCallback = [&](
                    __in ITransactionalReplicator& source, 
                    __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>> & stateProviders) {
                    UNREFERENCED_PARAMETER(source);

                    while (stateProviders.MoveNext())
                    {
                        Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr> current = stateProviders.Current();

                        KUri::CSPtr name = current.Key;
                        if (name->Compare(*stateProviderName))
                        {
                            stateProvider = current.Value;
                            break;
                        }
                    }           

                    VERIFY_IS_TRUE(stateProvider != nullptr);
                    
                    Store_SetNotifyStoreChangeCallback(
                        (StateProviderHandle)stateProvider.RawPtr(),
                        [](void* ctx, StateProviderHandle stateProvider, StoreChangeKind storeChangeKind, void *pData) {
                            UNREFERENCED_PARAMETER(stateProvider);
                            bool *called = reinterpret_cast<bool *>(ctx);
                            *called = true;

                            if (storeChangeKind != StoreChangeKind_Rebuild)
                                return;
                            
                            auto pRebuildData = reinterpret_cast<StoreChangeData_Rebuild *>(pData);
                            auto enumAsync = [](StoreChangeData_Rebuild *pRebuildData) -> Task {
                                auto enumerator = 
                                    reinterpret_cast<Utilities::IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>> *>(
                                        pRebuildData->Enumerator);

                                int i = 0;
                                while (co_await enumerator->MoveNextAsync(ktl::CancellationToken::None))
                                {
                                    auto pair = enumerator->GetCurrent();
                                    i++;
                                    if (i == 1)
                                    {
                                        VERIFY_IS_TRUE(wcscmp(*pair.Key, L"key1") == 0);
                                        VERIFY_IS_TRUE(wcscmp((LPCWSTR)pair.Value.Value->GetBuffer(), L"value1") == 0);                                        
                                    }
                                }
                                
                                // Signal completion
                                pRebuildData->Callback(pRebuildData->Context);

                                co_return;
                            };

                            // This calls the async function, and let it run by itself. When it finishes, it'll signal 
                            // the underlying RebuiltNotification to complete via the callback.
                            enumAsync(pRebuildData);
                        },
                        [](void *ctx) {
                            UNREFERENCED_PARAMETER(ctx);
                            // do nothing
                        },
                        &called);
                };

                // Backup and Restore - we should receive rebuild notifications in this process
                SyncAwait(Test_DictionaryRebuilt(
                    createStoreCallback,
                    createDataCallback,
                    rebuiltCallback,
                    workFolder, 
                    backupFolder, 
                    16, 
                    32, 
                    1, 
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId,
                    sourcePartitionId,
                    sourceReplicaId)); 
                VERIFY_IS_TRUE(called);

                Store_SetNotifyStoreChangeCallback((StateProviderHandle)stateProvider.RawPtr(), NULL, NULL, NULL);

                stateProvider = nullptr;

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }                             
        }

        BOOST_AUTO_TEST_SUITE_END();
    }
}
