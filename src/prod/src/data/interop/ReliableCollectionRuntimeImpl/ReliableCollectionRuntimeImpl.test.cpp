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

        StringLiteral const TraceComponent("ReliableCollectionRuntimeImplTests");
        std::wstring const TestLogFileName(L"ReliableCollectionRuntimeImplTests.log");

        class ReliableCollectionRuntimeImplTests
        {
        public:
            ReliableCollectionRuntimeImplTests()
            {
                Setup();
            }
            ~ReliableCollectionRuntimeImplTests()
            {
                Cleanup();
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

            void Setup();
            void Cleanup();

            Awaitable<Replica::SPtr> CreateReplica();
            void CloseReplica();

            void AddStore(
                __in KUri::CSPtr &stateProviderName);
            void AddRCQ(
                __in KUri::CSPtr &stateProviderName);
             LONG64 AddKeyValuePair(
                __in IStateProvider2* stateProvider,
                __in wstring const &key,
                __in size_t objHandle,
                __in wstring const &value);
   
        protected:
            CommonConfig config; // load the config object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;

            KGuid pId_;
            FABRIC_REPLICA_ID rId_;
            PartitionedReplicaId::SPtr prId_;

            ////
            Data::Log::LogManager::SPtr logManager_;
            Replica::SPtr replica_;
            wstring testFolderPath_;
        };

        void ReliableCollectionRuntimeImplTests::Setup()
        {
            NTSTATUS status;

            testFolderPath_ = CreateFileName(L"ReliableCollectionRuntimeImplTests");

            // Pre-clean up
            Directory::Delete_WithRetry(testFolderPath_, true, true);

            status = KtlSystem::Initialize(FALSE, &underlyingSystem_); 
            ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); 

            underlyingSystem_->SetStrictAllocationChecks(TRUE); 
            KAllocator & allocator = underlyingSystem_->PagedAllocator(); 
            int seed = GetTickCount(); 
            Common::Random r(seed); 
            rId_ = r.Next(); 
            pId_.CreateNew(); 
            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator); 

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            InitializeKtlConfig(testFolderPath_, TestLogFileName, underlyingSystem_->PagedAllocator(), sharedLogSettings);

            status = Data::Log::LogManager::Create(underlyingSystem_->PagedAllocator(), logManager_);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(logManager_->OpenAsync(CancellationToken::None, sharedLogSettings));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            replica_ = SyncAwait(CreateReplica());

            Test_UseEnv(true);
        }

        Awaitable<Replica::SPtr> ReliableCollectionRuntimeImplTests::CreateReplica()
        {
            wstring testFolder = Path::Combine(testFolderPath_, L"work");
            StateProviderFactory::SPtr factory;
            StateProviderFactory::Create(underlyingSystem_->PagedAllocator(), factory);

            Replica::SPtr replica = Replica::Create(
                pId_,
                rId_,
                testFolder,
                *logManager_,
                underlyingSystem_->PagedAllocator(),
                factory.RawPtr());

            co_await replica->OpenAsync();

            FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
            co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

            replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
            replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

            co_return replica;
        }

        void ReliableCollectionRuntimeImplTests::CloseReplica()
        {
            replica_->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
            replica_->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

            SyncAwait(replica_->CloseAsync());
        }

        void ReliableCollectionRuntimeImplTests::Cleanup()
        {
            NTSTATUS status;

            CloseReplica();

            replica_.Reset();

            status = SyncAwait(logManager_->CloseAsync(CancellationToken::None));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logManager_ = nullptr;

            prId_.Reset();
            
            KtlSystem::Shutdown();

            // Post-clean up
            Directory::Delete_WithRetry(testFolderPath_, true, true);
        }

        wstring ReliableCollectionRuntimeImplTests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void ReliableCollectionRuntimeImplTests::InitializeKtlConfig(
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

        KUri::CSPtr ReliableCollectionRuntimeImplTests::GetStateProviderName(
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

        void ReliableCollectionRuntimeImplTests::AddStore(
            __in KUri::CSPtr &stateProviderName)
        {
            NTSTATUS status = STATUS_SUCCESS;
            Transaction::SPtr txn;

            status = replica_->TxnReplicator->CreateTransaction(txn);
            THROW_ON_FAILURE(status);
            KFinally([&] {txn->Dispose(); });

            status = SyncAwait(replica_->TxnReplicator->AddAsync(*txn, *stateProviderName, L"0\1\1\n"));
            THROW_ON_FAILURE(status);

            status = SyncAwait(txn->CommitAsync());
            THROW_ON_FAILURE(status);
        }

        void ReliableCollectionRuntimeImplTests::AddRCQ(
            __in KUri::CSPtr &stateProviderName)
        {
            NTSTATUS status = S_OK;
            Transaction::SPtr txn;

            status = replica_->TxnReplicator->CreateTransaction(txn);
            THROW_ON_FAILURE(status);
            KFinally([&] {txn->Dispose(); });

            // Version = 1, StateProvider_Kind = ConcurrentQueue
            status = SyncAwait(replica_->TxnReplicator->AddAsync(*txn, *stateProviderName, L"0\1\2\n"));
            THROW_ON_FAILURE(status);

            status = SyncAwait(txn->CommitAsync());
            THROW_ON_FAILURE(status);
        }

        LONG64 ReliableCollectionRuntimeImplTests::AddKeyValuePair(
            __in IStateProvider2 * stateProvider,
            __in wstring const &key,
            __in size_t objHandle,
            __in wstring const &value)
        {
            NTSTATUS status = STATUS_SUCCESS;
            Transaction::SPtr txn;

            auto store = dynamic_cast<IStore<KString::SPtr, KBuffer::SPtr>*>(stateProvider);

            status = replica_->TxnReplicator->CreateTransaction(txn);
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
            
            SyncAwait(store->AddAsync(*storetxn, kstringkey, kBuffer, TimeSpan::MaxValue, ktl::CancellationToken::None));
            status = SyncAwait(txn->CommitAsync());
            THROW_ON_FAILURE(status);
            return txn->CommitSequenceNumber;
        }

        BOOST_FIXTURE_TEST_SUITE(ReliableCollectionRuntimeImplTestsSuite, ReliableCollectionRuntimeImplTests);

        BOOST_AUTO_TEST_CASE(TxnReplicator_CreateTransaction_SUCCESS)
        {
            wstring testName(L"TxnReplicator_CreateTransaction_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                TxnReplicator::Transaction* txn;
                HRESULT hresult = TxnReplicator_CreateTransaction(replica_->TxnReplicator.RawPtr(), (TransactionHandle*)&txn);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                KFinally([&] {Transaction_Dispose(txn); Transaction_Release(txn); });
            }
        }
        
        BOOST_AUTO_TEST_CASE(Transaction_CommitAsync_SUCCESS)
        {
            wstring testName(L"Transaction_CommitAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                HRESULT hresult;
                BOOL synchronousComplete;
                AwaitableCompletionSource<void>::SPtr acs = nullptr;

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<void>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                hresult = Transaction_CommitAsync(txn.RawPtr(), 
                            [](void* acsHandle, HRESULT _hresult) {
                                AwaitableCompletionSource<void>::SPtr acs = *(AwaitableCompletionSource<void>::SPtr*)acsHandle;
                                if (SUCCEEDED(_hresult))
                                    acs->Set();
                                else
                                    acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                            }, &acs, &synchronousComplete);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                if (!synchronousComplete)
                    SyncAwait(acs->GetAwaitable());
            }
        }

        BOOST_AUTO_TEST_CASE(Transaction_Abort_SUCCESS)
        {
            wstring testName(L"Transaction_Abort_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KFinally([&] {txn->Dispose(); });

                HRESULT hresult = Transaction_Abort(txn.RawPtr());
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
            }
        }
        
        BOOST_AUTO_TEST_CASE(TxnReplicator_AddDictThenGetOrAddAsync_SUCCESS)
        {
            wstring testName(L"TxnReplicator_AddDictThenGetOrAddAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                BOOL synchronouscomplete;
                BOOL alreadyExist = false;
                ktl::CancellationTokenSource* cts;
                wstring stateProviderName(L"fabric:/store/dict1");
                wstring lang = L"CSHARP";
                wstring langTypeInfo = L"Microsoft.ServiceFabric.ReliableCollections.ReliableDictionaryImpl";
                StateProvider_Info stateProviderInfo = {
                    StateProvider_Info_V1_Size,
                    StateProvider_Kind_Store,
                    langTypeInfo.c_str()
                };
                KUri::CSPtr spName;
                NTSTATUS status = KUri::Create(KStringView(stateProviderName.c_str()), underlyingSystem_->PagedAllocator(), spName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                AddStore(spName);
                AwaitableCompletionSource<IStateProvider2*>::SPtr acs = nullptr;
                IStateProvider2* stateProvider = nullptr;
                AwaitableCompletionSource<IStateProvider2*>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                HRESULT hresult = TxnReplicator_GetOrAddStateProviderAsync(
                    replica_->TxnReplicator.RawPtr(),
                    nullptr,
                    stateProviderName.c_str(),
                    lang.c_str(),
                    &stateProviderInfo,
                    std::numeric_limits<int64>::max(),
                    (CancellationTokenSourceHandle*)&cts,
                    (StoreHandle*)&stateProvider,
                    &alreadyExist,
                    [](void* acsHandle, HRESULT _hresult, StateProviderHandle stateProviderHandle, BOOL exist) {
                        auto acs = *(ktl::AwaitableCompletionSource<IStateProvider2*>::SPtr*)acsHandle;
                        if (SUCCEEDED(_hresult))
                        {
                            VERIFY_IS_TRUE(exist);
                            VERIFY_IS_NOT_NULL(stateProviderHandle);
                            acs->SetResult((IStateProvider2*)stateProviderHandle);
                        }
                        else
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    },
                    &acs,
                    &synchronouscomplete);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (!synchronouscomplete)
                {
                    //Ignore cts
                    CancellationTokenSource_Release(cts);
                    stateProvider = SyncAwait(acs->GetAwaitable());
                }

                Store_Release(stateProvider);
            }
        }
        
        BOOST_AUTO_TEST_CASE(TxnReplicator_AddDictThenGet_SUCCESS)
        {
            wstring testName(L"TxnReplicator_AddDictThenGet_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                KUri::CSPtr stateProviderName = GetStateProviderName(3);
                AddStore(stateProviderName);
                IStateProvider2* stateProvider = nullptr;
                HRESULT hresult = TxnReplicator_GetStateProvider(
                    replica_->TxnReplicator.RawPtr(),
                    (*stateProviderName).Get(KUriView::eRaw),
                    (StateProviderHandle*)&stateProvider);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                VERIFY_IS_NOT_NULL(stateProvider);
                Store_Release(stateProvider);
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_Get_FAIL)
        {
            wstring testName(L"TxnReplicator_Get_FAIL");

            TEST_TRACE_BEGIN(testName)
            {
                KUri::CSPtr stateProviderName = GetStateProviderName(3);
                IStateProvider2* stateProvider = nullptr;
                HRESULT hresult = TxnReplicator_GetStateProvider(
                    replica_->TxnReplicator.RawPtr(),
                    (*stateProviderName).Get(KUriView::eRaw),
                    (StateProviderHandle*)&stateProvider);
                VERIFY_IS_FALSE(SUCCEEDED(hresult));
                VERIFY_IS_NULL(stateProvider);
                Store_Release(stateProvider);
            }
        }

        BOOST_AUTO_TEST_CASE(Store_AddAsync_SUCCESS)
        {
            wstring testName(L"Store_AddAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                LONG64 count = 0;
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                IStateProvider2::SPtr stateProvider;
                AwaitableCompletionSource<void>::SPtr acs = nullptr;

                KUri::CSPtr stateProviderName = GetStateProviderName(3);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                //VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                wstring value(L"value1");
                AwaitableCompletionSource<void>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                HRESULT hresult = Store_AddAsync(
                    stateProvider.RawPtr(), 
                    txn.RawPtr(), 
                    L"key1", 
                    0, 
                    (void*)value.c_str(), 
                    (uint32_t)((value.size() + 1) * sizeof(value[0])), 
                    std::numeric_limits<int64>::max(), 
                    (CancellationTokenSourceHandle*)&cts,
                    [](void* acsHandle, HRESULT _hresult) {
                        AwaitableCompletionSource<void>* acs = (AwaitableCompletionSource<void>*)acsHandle;
                        if (!SUCCEEDED(_hresult))
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                        else
                            acs->Set();
                    }, 
                    acs.RawPtr(), 
                    &synchronouscomplete); // TODO cts should be passed as SPtr ref

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                if (!synchronouscomplete)
                {
                    CancellationTokenSource_Release(cts);
                    SyncAwait(acs->GetAwaitable());
                }

                SyncAwait(txn->CommitAsync());

                Store_GetCount(stateProvider.RawPtr(), &count);
                VERIFY_IS_TRUE(count == 1);
            }
        }

        BOOST_AUTO_TEST_CASE(Store_RemoveAsync_SUCCESS)
        {
            wstring testName(L"Store_RemoveAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                LONG64 count = 0;
                BOOL removed;
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts = nullptr;
                IStateProvider2::SPtr stateProvider;
                AwaitableCompletionSource<bool>::SPtr acs = nullptr;

                KUri::CSPtr stateProviderName = GetStateProviderName(4);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);

                AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<bool>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                HRESULT hresult = Store_ConditionalRemoveAsync(stateProvider.RawPtr(), txn.RawPtr(), L"key1", std::numeric_limits<int64>::max(), (CancellationTokenSourceHandle*)&cts, -1, &removed,
                    [](void* acsHandle, HRESULT _hresult, BOOL isRemoved) {
                    AwaitableCompletionSource<bool>* acs = (AwaitableCompletionSource<bool>*)acsHandle;
                    if (!SUCCEEDED(_hresult))
                        acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    else
                        acs->SetResult(isRemoved);
                }, acs.RawPtr(), &synchronouscomplete);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                if (!synchronouscomplete)
                {
                    CancellationTokenSource_Release(cts);
                    removed = SyncAwait(acs->GetAwaitable());
                }

                SyncAwait(txn->CommitAsync());

                VERIFY_IS_TRUE(removed);

                Store_GetCount(stateProvider.RawPtr(), &count);
                VERIFY_IS_TRUE(count == 0);
            }
        }

	BOOST_AUTO_TEST_CASE(Store_ContainsKeyAsync_SUCCESS)
        {
            wstring testName(L"Store_ContainsKeyAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                BOOL found;
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts = nullptr;
                IStateProvider2::SPtr stateProvider;
                AwaitableCompletionSource<bool>::SPtr acs = nullptr;

                KUri::CSPtr stateProviderName = GetStateProviderName(4);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);

                AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<bool>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                HRESULT hresult = Store_ContainsKeyAsync(stateProvider.RawPtr(), txn.RawPtr(), L"key1", std::numeric_limits<int64>::max(), Store_LockMode::Store_LockMode_Free,
                    (CancellationTokenSourceHandle*)&cts, &found, [](void* acsHandle, HRESULT _hresult, BOOL isFound) {
                    AwaitableCompletionSource<bool>* acs = (AwaitableCompletionSource<bool>*)acsHandle;
                    if (!SUCCEEDED(_hresult))
                        acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    else
                        acs->SetResult(isFound);
                }, acs.RawPtr(), &synchronouscomplete);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                if (!synchronouscomplete)
                {
                    CancellationTokenSource_Release(cts);
                    found = SyncAwait(acs->GetAwaitable());
                }

                SyncAwait(txn->CommitAsync());

                VERIFY_IS_TRUE(found);
            }
        }

        BOOST_AUTO_TEST_CASE(Store_ConditionalGetAsync_SUCCESS)
        {
            wstring testName(L"Store_ConditionalGetAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                Buffer value;
                BOOL found = false;
                size_t objectHandle;
                byte* bytesValue;
                uint32_t bytesValueLength;
                LONG64 versionSequenceNumber;
                ktl::CancellationTokenSource* cts;
                BOOL synchronouscomplete;
                IStateProvider2::SPtr stateProvider;
                AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t, LONG64>>::SPtr acs = nullptr;

                KUri::CSPtr stateProviderName = GetStateProviderName(0);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                //VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

                AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t, LONG64>>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                HRESULT hresult = Store_ConditionalGetAsync(
                    stateProvider.RawPtr(),
                    txn.RawPtr(), 
                    L"key1", 
                    std::numeric_limits<int64>::max(), 
                    Store_LockMode::Store_LockMode_Free,
                    &objectHandle,
                    &value, 
                    &versionSequenceNumber,
                    (CancellationTokenSourceHandle*)&cts,
                    &found,
                    [](void* acsHandle, HRESULT _hresult, BOOL r, size_t handle, void* bytes, uint32_t bytesLength, LONG64 lsn) {
                        auto acs = (AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t, LONG64>>*)acsHandle;
                        if (!SUCCEEDED(_hresult))
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                        else
                            acs->SetResult(make_tuple(r, handle, (byte*)bytes, bytesLength, lsn));
                    }, acs.RawPtr(), &synchronouscomplete);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (synchronouscomplete)
                {
                    bytesValue = (byte*)value.Bytes;
                    bytesValueLength = value.Length;
                }
                else
                {
                    CancellationTokenSource_Release(cts);
                    tie(found, objectHandle, bytesValue, bytesValueLength, versionSequenceNumber) = SyncAwait(acs->GetAwaitable());
                }

                VERIFY_IS_TRUE(found);
#ifdef FEATURE_CACHE_OBJHANDLE             
                VERIFY_IS_TRUE(objectHandle == 1);
#endif

                wstring str((LPCWSTR)bytesValue);
                VERIFY_IS_TRUE(str.compare(L"value1") == 0);
                VERIFY_IS_TRUE(bytesValueLength == (str.length() + 1) * (sizeof(unsigned short)));

                SyncAwait(txn->CommitAsync());
                if (synchronouscomplete)
                Buffer_Release(value.Handle);
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_SetNotifyStateManagerChangeCallback_SingleEntityChanged_SUCCESS)
        {
            wstring testName(L"TxnReplicator_SetNotifyStateManagerChangeCallback_SingleEntityChanged_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                IStateProvider2::SPtr stateProvider;

                struct StateManagerChangeCallbackContext
                {
                    bool added;
                    const KUri *uri;
                };
                
                KUri::CSPtr stateProviderName = GetStateProviderName(0);
                StateManagerChangeCallbackContext *context = new StateManagerChangeCallbackContext();
                context->added = false;
                context->uri = KUri::CSPtr(stateProviderName.RawPtr()).Detach();

                auto replicator = reinterpret_cast<TxnReplicatorHandle>(replica_->TxnReplicator.RawPtr());
                TxnReplicator_SetNotifyStateManagerChangeCallback(
                    replicator,
                    [](void* ctx, TxnReplicatorHandle txnReplicator, StateManagerChangeKind changeKind, void *pData) 
                    {
                        UNREFERENCED_PARAMETER(txnReplicator);

                        auto context = reinterpret_cast<StateManagerChangeCallbackContext *>(ctx);

                        if (changeKind == StateManagerChangeKind_Add)
                        {
                            auto addData = reinterpret_cast<StateManagerChangeData_SingleEntityChanged *>(pData);
                            VERIFY_IS_TRUE(wcscmp((LPCWSTR)(const_cast<KUri &>(*context->uri)), addData->StateProviderName) == 0);
                            context->added = true;
                        }
                    },
                    [](void *ctx)
                    {
                        auto context = reinterpret_cast<StateManagerChangeCallbackContext *>(ctx);
                        KUri::CSPtr ptr;
                        ptr.Attach(reinterpret_cast<const KUri *>(context->uri));
                        delete context;
                    },
                    context);                                    

                AddStore(stateProviderName);
                VERIFY_IS_TRUE(context->added);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);

                TxnReplicator_SetNotifyStateManagerChangeCallback(replicator, nullptr, nullptr, nullptr);
            }
        }

        BOOST_AUTO_TEST_CASE(Store_SetNotifyStoreChangeCallback_TransactionalNotification_SUCCESS)
        {
            wstring testName(L"Store_SetNotifyStoreChangeCallback_TransactionalNotification_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                IStateProvider2::SPtr stateProvider;

                KUri::CSPtr stateProviderName = GetStateProviderName(0);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);

                struct Context
                {
                    StateProviderHandle stateProvider;
                    LONG64 commitSequenceNumber;
                } context = { (StateProviderHandle)stateProvider.RawPtr(), 0 };

                Store_SetNotifyStoreChangeCallback((StateProviderHandle)stateProvider.RawPtr(),
                    [](void* ctx, StateProviderHandle stateProvider, StoreChangeKind storeChangeKind, void *pData) {
                        auto context = reinterpret_cast<Context *>(ctx);

                        VERIFY_IS_TRUE(context->stateProvider == stateProvider);

                        if (storeChangeKind == StoreChangeKind_Add)
                        {
                            auto pAddData = (StoreChangeData_Add *)pData;
                            VERIFY_IS_TRUE(wcscmp(pAddData->Key, L"key1") == 0);
                            VERIFY_IS_TRUE(wcscmp((LPCWSTR)(pAddData)->Value, L"value1") == 0);
                            auto txn = (TxnReplicator::Transaction *) pAddData->Transaction;
                            context->commitSequenceNumber = txn->CommitSequenceNumber;
                        }
                    },
                    nullptr,
                    &context);

                LONG64 commitSequenceNumber = AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");

                VERIFY_IS_TRUE(commitSequenceNumber == context.commitSequenceNumber);

                Store_SetNotifyStoreChangeCallback((StateProviderHandle)stateProvider.RawPtr(), nullptr, nullptr, nullptr);
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_SetNotifyTransactionChangeCallback_CommitNotification_SUCCESS)
        {
            wstring testName(L"TxnReplicator_SetNotifyTransactionChangeCallback_CommitNotification_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                HRESULT hresult;

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                IStateProvider2::SPtr stateProvider;

                KUri::CSPtr stateProviderName = GetStateProviderName(0);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);

                struct TransactionChangeCallbackContext
                {
                    bool transactionCommited;
                };

                TransactionChangeCallbackContext *context = new TransactionChangeCallbackContext();
                context->transactionCommited = false;

                // set the TransactionChanged Notification
                auto replicator = reinterpret_cast<TxnReplicatorHandle>(replica_->TxnReplicator.RawPtr());
                hresult = TxnReplicator_SetNotifyTransactionChangeCallback(
                    replicator,
                    [](void* ctx, TxnReplicatorHandle txnReplicator, TransactionChangeKind changeKind, void *pData)
                {
                    UNREFERENCED_PARAMETER(txnReplicator);
                    UNREFERENCED_PARAMETER(pData);

                    auto context = reinterpret_cast<TransactionChangeCallbackContext *>(ctx);

                    if (changeKind == TransactionChangeKind_Commit)
                    {
                        context->transactionCommited = true;
                    }
                },
                    [](void *ctx)
                {
                    auto context = reinterpret_cast<TransactionChangeCallbackContext *>(ctx);
                    delete context;
                },
                    context);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");

                VERIFY_IS_TRUE(context->transactionCommited);

                hresult = TxnReplicator_SetNotifyTransactionChangeCallback(replicator, nullptr, nullptr, nullptr);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));
            }
        }

        BOOST_AUTO_TEST_CASE(Store_EnumerateKeys_SUCCESS)
        {
            wstring testName(L"Store_EnumerateKeys_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                ULONG count = 0;
                BOOL synchronouscomplete;
                IStateProvider2::SPtr stateProvider;
                Data::IEnumerator<KString::SPtr>* enumerator;
                AwaitableCompletionSource<Data::IEnumerator<KString::SPtr>*>::SPtr acs = nullptr;

                KUri::CSPtr stateProviderName = GetStateProviderName(1);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                //VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

                AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");
                AddKeyValuePair(stateProvider.RawPtr(), L"key2", 2, L"value2");
                AddKeyValuePair(stateProvider.RawPtr(), L"key3", 3, L"value3");

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<Data::IEnumerator<KString::SPtr>*>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                HRESULT hresult = Store_CreateKeyEnumeratorAsync(stateProvider.RawPtr(), txn.RawPtr(), nullptr, nullptr, (void**)&enumerator,
                            [] (void* ctx, HRESULT _hresult, StoreKeyEnumeratorHandle enumerator) {
                                auto acs = *(AwaitableCompletionSource<IEnumerator<KString::SPtr>*>::SPtr*)ctx;
                                if (!SUCCEEDED(_hresult))
                                    acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                                else
                                    acs->SetResult((IEnumerator<KString::SPtr>*)enumerator);
                            }, &acs, &synchronouscomplete);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                if (!synchronouscomplete)
                    enumerator = SyncAwait(acs->GetAwaitable());

                KFinally([&] {StoreKeyEnumerator_Release(enumerator); });

                LPCWSTR key;
                auto funcMoveNext = [&key](Data::IEnumerator<KString::SPtr>* enumerator) -> bool
                {
                    BOOL advanced = false;
                    HRESULT hresult = StoreKeyEnumerator_MoveNext(enumerator, &advanced, &key);
                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    return advanced;
                };

                while (funcMoveNext(enumerator))
                {
                    count++;
                    wstring expectedKey(L"key");
                    expectedKey.append(to_wstring(count));
                    VERIFY_IS_TRUE(expectedKey.compare(key) == 0);
                }

                VERIFY_IS_TRUE(count == 3);
            }
        }

        BOOST_AUTO_TEST_CASE(Store_EnumerateKeyValue_SUCCESS)
        {
            wstring testName(L"Store_EnumerateKeyValue_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                ULONG count = 0;
                LPCWSTR key;
                size_t objHandle;
                byte* bytes;
                uint32_t bytesLength;
                LONG64 lsn;
                BOOL synchronouscomplete;
                Buffer byteValue;
                IStateProvider2::SPtr stateProvider;
                Utilities::IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>* enumerator;
                AwaitableCompletionSource<Utilities::IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>*>::SPtr acs = nullptr;

                KUri::CSPtr stateProviderName = GetStateProviderName(2);
                AddStore(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                //VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

                AddKeyValuePair(stateProvider.RawPtr(), L"key1", 1, L"value1");
                AddKeyValuePair(stateProvider.RawPtr(), L"key2", 2, L"value2");
                AddKeyValuePair(stateProvider.RawPtr(), L"key3", 3, L"value3");

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<Utilities::IAsyncEnumerator<Data::KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>*>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                HRESULT hresult = Store_CreateEnumeratorAsync(stateProvider.RawPtr(), txn.RawPtr(),
                            (void**)&enumerator,
                            [](void* ctx, HRESULT _hresult, StoreKeyValueAsyncEnumeratorHandle result) {
                                auto acs = *(AwaitableCompletionSource<IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>*>::SPtr*)ctx;
                                if (!SUCCEEDED(_hresult))
                                    acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                                else
                                    acs->SetResult((IAsyncEnumerator<KeyValuePair<KString::SPtr, Data::KeyValuePair<LONG64, KBuffer::SPtr>>>*)result);
                            }, &acs, &synchronouscomplete);

                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                if (!synchronouscomplete)
                    enumerator = SyncAwait(acs->GetAwaitable());

                KFinally([&] {StoreKeyValueEnumerator_Release(enumerator); });

                auto funcMoveNextSync = [&](Utilities::IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>* enumerator) -> bool {
                    BOOL advanced;
                    ktl::CancellationTokenSource* cts;
                    AwaitableCompletionSource<tuple<bool, LPCWSTR, size_t, byte*, uint32_t, LONG64>>::SPtr acs = nullptr;
                    AwaitableCompletionSource<tuple<bool, LPCWSTR, size_t, byte*, uint32_t, LONG64>>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                    hresult = StoreKeyValueEnumerator_MoveNextAsync(
                        enumerator, 
                        (CancellationTokenSourceHandle*)&cts,
                        &advanced,
                        &key,
                        &objHandle,
                        &byteValue, 
                        &lsn,
                        [](void* acsHandle, HRESULT _hresult, BOOL advanced, LPCWSTR key, size_t objectHandle, void* byteBuffer, uint32_t bufferLength, LONG64 versionSequenceNumber) {
                            auto acs = (AwaitableCompletionSource<tuple<bool, LPCWSTR, size_t, void*, uint32_t, LONG64>>*)acsHandle;
                            if (SUCCEEDED(_hresult))
                                acs->SetResult(make_tuple(advanced, key, objectHandle, byteBuffer, bufferLength, versionSequenceNumber));
                            else
                                acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                        }, 
                        acs.RawPtr(), 
                        &synchronouscomplete);

                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    if (synchronouscomplete)
                    {
                        bytes = (byte*)byteValue.Bytes;
                        bytesLength = byteValue.Length;
                    }
                    else
                    {
                        CancellationTokenSource_Release(cts);
                        tie(advanced, key, objHandle, bytes, bytesLength, lsn) = SyncAwait(acs->GetAwaitable());
                    }
                    return advanced;
                };

                while (funcMoveNextSync(enumerator))
                {
                    count++;
                    wstring expectedKey(L"key");
                    expectedKey.append(to_wstring(count));
                    wstring expectedValue(L"value");
                    expectedValue.append(to_wstring(count));
                    wstring value((wchar_t*)bytes);
                    VERIFY_IS_TRUE(expectedKey.compare(key) == 0);
#ifdef FEATURE_CACHE_OBJHANDLE             
                    VERIFY_IS_TRUE(objHandle == count);
#endif
                    VERIFY_IS_TRUE(expectedValue.compare(value) == 0);
                    if (synchronouscomplete)
                        Buffer_Release(byteValue.Handle);
                }

                VERIFY_IS_TRUE(count == 3);
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_AddStateProvider_SUCCESS)
        {
            wstring testName(L"TxnReplicator_AddStateProvider_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                wstring stateProviderName(L"fabric:/store/dict1");
                wstring lang = L"CSHARP";
                wstring langTypeInfo = L"Microsoft.ServiceFabric.ReliableCollections.ReliableConcurrentQueue";
                StateProvider_Info stateProviderInfo = {
                    StateProvider_Info_V1_Size,
                    StateProvider_Kind_Store,
                    langTypeInfo.c_str()
                };
                
                Transaction::SPtr txn;
                NTSTATUS status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<void>::SPtr acs = nullptr;
                AwaitableCompletionSource<void>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                HRESULT hresult = TxnReplicator_AddStateProviderAsync(
                    replica_->TxnReplicator.RawPtr(),
                    txn.RawPtr(),
                    stateProviderName.c_str(),
                    lang.c_str(),
                    &stateProviderInfo,
                    std::numeric_limits<int64>::max(),
                    (CancellationTokenSourceHandle*)&cts,
                    [](void* acsHandle, HRESULT _hresult) {
                        AwaitableCompletionSource<void>* acs = (AwaitableCompletionSource<void>*)acsHandle;
                        if (SUCCEEDED(_hresult))
                            acs->Set();
                        else
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    },
                    acs.RawPtr(),
                    &synchronouscomplete);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (!synchronouscomplete)
                {
                    //Ignore cts
                    CancellationTokenSource_Release(cts);
                    SyncAwait(acs->GetAwaitable());
                }

                status = SyncAwait(txn->CommitAsync());
                THROW_ON_FAILURE(status);

                IStateProvider2::SPtr stateProvider;
                StateProvider_Info stateProvider_Info;
                KUri::CSPtr spName;

                status = KUri::Create(KStringView(stateProviderName.c_str()), underlyingSystem_->PagedAllocator(), spName);
                THROW_ON_FAILURE(status);

                status = replica_->TxnReplicator->Get(*spName, stateProvider);
                THROW_ON_FAILURE(status);

                stateProvider_Info.Size = sizeof(StateProvider_Info);
                hresult = StateProvider_GetInfo(stateProvider.RawPtr(), lang.c_str(), &stateProvider_Info);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                VERIFY_IS_TRUE(stateProvider_Info.Kind == StateProvider_Kind_Store);
                VERIFY_IS_TRUE(langTypeInfo.compare(stateProvider_Info.LangMetadata) == 0);

                stateProvider_Info.Size = sizeof(StateProvider_Info);
                hresult = StateProvider_GetInfo(stateProvider.RawPtr(), L"JAVA", &stateProvider_Info);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                VERIFY_IS_TRUE(stateProvider_Info.Kind == StateProvider_Kind_Store);
                VERIFY_IS_TRUE(stateProvider_Info.LangMetadata == nullptr);
            }
        }
       
        BOOST_AUTO_TEST_CASE(TxnReplicator_GetOrAddStateProvider_SUCCESS)
        {
            wstring testName(L"TxnReplicator_GetOrAddStateProvider_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                StateProviderHandle stateProviderHandle;
                BOOL alreadyExists;
                wstring stateProviderName(L"fabric:/store/dict1");
                wstring lang = L"CSHARP";
                wstring langTypeInfo = L"Microsoft.ServiceFabric.ReliableCollections.ReliableDictionaryImpl";
                StateProvider_Info stateProviderInfo = {
                    StateProvider_Info_V1_Size,
                    StateProvider_Kind_Store,
                    langTypeInfo.c_str()
                };

                Transaction::SPtr txn;
                NTSTATUS status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<IStateProvider2*>::SPtr acs = nullptr;
                AwaitableCompletionSource<IStateProvider2*>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                HRESULT hresult = TxnReplicator_GetOrAddStateProviderAsync(
                    replica_->TxnReplicator.RawPtr(),
                    txn.RawPtr(),
                    stateProviderName.c_str(),
                    lang.c_str(),
                    &stateProviderInfo,
                    std::numeric_limits<int64>::max(),
                    (CancellationTokenSourceHandle*)&cts,
                    &stateProviderHandle,
                    &alreadyExists,
                    [](void* acsHandle, HRESULT _hresult, StateProviderHandle stateProviderHandle, BOOL exist) {
                        auto acs = *(ktl::AwaitableCompletionSource<IStateProvider2*>::SPtr*)acsHandle;
                        if (SUCCEEDED(_hresult))
                        {
                            VERIFY_IS_TRUE(exist);
                            VERIFY_IS_NOT_NULL(stateProviderHandle);
                            acs->SetResult((IStateProvider2*)stateProviderHandle);
                        }
                        else
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    },
                    acs.RawPtr(),
                    &synchronouscomplete);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (!synchronouscomplete)
                {
                    //Ignore cts
                    CancellationTokenSource_Release(cts);
                    stateProviderHandle = SyncAwait(acs->GetAwaitable());
                }

                VERIFY_IS_TRUE(stateProviderHandle != nullptr);
                VERIFY_IS_TRUE(alreadyExists == false);

                status = SyncAwait(txn->CommitAsync());
                THROW_ON_FAILURE(status);

                StateProvider_Info stateProvider_Info;
                stateProvider_Info.Size = sizeof(StateProvider_Info);
                hresult = StateProvider_GetInfo(stateProviderHandle, lang.c_str(), &stateProvider_Info);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                VERIFY_IS_TRUE(stateProvider_Info.Kind == StateProvider_Kind_Store);
                VERIFY_IS_TRUE(langTypeInfo.compare(stateProvider_Info.LangMetadata) == 0);

                StateProvider_Release(stateProviderHandle);
            }
        }
        
        BOOST_AUTO_TEST_CASE(TxnReplicator_GetInfo_SUCCESS)
        {
            wstring testName(L"TxnReplicator_GetInfo_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                TxnReplicator_Info info1, info2;
                info1.Size = sizeof(TxnReplicator_Info);
                HRESULT hr = TxnReplicator_GetInfo(replica_->TxnReplicator.RawPtr(), &info1);
                VERIFY_IS_TRUE(hr == S_OK);

                wstring stateProviderName(L"fabric:/store/dict1");
                KUri::CSPtr spName;
                NTSTATUS status = KUri::Create(KStringView(stateProviderName.c_str()), underlyingSystem_->PagedAllocator(), spName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                AddStore(spName);


                info2.Size = sizeof(TxnReplicator_Info);
                hr = TxnReplicator_GetInfo(replica_->TxnReplicator.RawPtr(), &info2);
                VERIFY_IS_TRUE(hr == S_OK);

                VERIFY_IS_TRUE(info2.LastStableSequenceNumber > info1.LastStableSequenceNumber);
                VERIFY_IS_TRUE(info2.LastCommittedSequenceNumber > info1.LastCommittedSequenceNumber);
                VERIFY_IS_TRUE(info2.CurrentEpoch.DataLossNumber == info1.CurrentEpoch.DataLossNumber);
                VERIFY_IS_TRUE(info2.CurrentEpoch.ConfigurationNumber == info1.CurrentEpoch.ConfigurationNumber);
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_GetOrAddConcurrentQueueAsync_SUCCESS)
        {
            wstring testName(L"TxnReplicator_GetOrAddConcurrentQueueAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                StateProviderHandle stateProviderHandle;
                BOOL alreadyExists;
                wstring stateProviderName(L"fabric:/rcq/rcq1");
                wstring lang = L"CSHARP";
                wstring langTypeInfo = L"Microsoft.ServiceFabric.ReliableCollections.ReliableConcurrentQueueImpl";
                StateProvider_Info stateProviderInfo = {
                    StateProvider_Info_V1_Size,
                    StateProvider_Kind_ConcurrentQueue,
                    langTypeInfo.c_str()
                };

                Transaction::SPtr txn;
                NTSTATUS status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<IStateProvider2*>::SPtr acs = nullptr;
                AwaitableCompletionSource<IStateProvider2*>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                HRESULT hresult = TxnReplicator_GetOrAddStateProviderAsync(
                    replica_->TxnReplicator.RawPtr(),
                    txn.RawPtr(),
                    stateProviderName.c_str(),
                    lang.c_str(),
                    &stateProviderInfo,
                    std::numeric_limits<int64>::max(),
                    (CancellationTokenSourceHandle*)&cts,
                    &stateProviderHandle,
                    &alreadyExists,
                    [](void* acsHandle, HRESULT _hresult, StateProviderHandle stateProviderHandle, BOOL exist) {
                        auto acs = *(ktl::AwaitableCompletionSource<IStateProvider2*>::SPtr*)acsHandle;
                        if (SUCCEEDED(_hresult))
                        {
                            VERIFY_IS_TRUE(exist);
                            VERIFY_IS_NOT_NULL(stateProviderHandle);
                            acs->SetResult((IStateProvider2*)stateProviderHandle);
                        }
                        else
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    },
                    acs.RawPtr(),
                    &synchronouscomplete);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (!synchronouscomplete)
                {
                    //Ignore cts
                    CancellationTokenSource_Release(cts);
                    stateProviderHandle = SyncAwait(acs->GetAwaitable());
                }

                VERIFY_IS_TRUE(stateProviderHandle != nullptr);
                VERIFY_IS_TRUE(alreadyExists == false);

                status = SyncAwait(txn->CommitAsync());
                THROW_ON_FAILURE(status);

                StateProvider_Info stateProvider_Info;
                stateProvider_Info.Size = sizeof(StateProvider_Info);
                hresult = StateProvider_GetInfo(stateProviderHandle, lang.c_str(), &stateProvider_Info);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                VERIFY_IS_TRUE(stateProvider_Info.Kind == StateProvider_Kind_ConcurrentQueue);
                VERIFY_IS_TRUE(langTypeInfo.compare(stateProvider_Info.LangMetadata) == 0);

                StateProvider_Release(stateProviderHandle);
            }
        }

        BOOST_AUTO_TEST_CASE(ConcurrentQueue_EnqueueDequeueAsync_SUCCESS)
        {
            wstring testName(L"ConcurrentQueue_EnqueueDequeueAsync_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                IStateProvider2::SPtr stateProvider;

                //
                // Create RCQ
                //
                KUri::CSPtr stateProviderName = GetStateProviderName(3);
                AddRCQ(stateProviderName);

                status = replica_->TxnReplicator->Get(*stateProviderName, stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);

                //
                // EnqueueAsync
                //
                wstring value(L"value1");
                {
                    Transaction::SPtr txn;
                    status = replica_->TxnReplicator->CreateTransaction(txn);
                    THROW_ON_FAILURE(status);
                    KFinally([&] {txn->Dispose(); });

                    AwaitableCompletionSource<void>::SPtr acs = nullptr;
                    AwaitableCompletionSource<void>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                    HRESULT hresult = ConcurrentQueue_EnqueueAsync(
                        stateProvider.RawPtr(), 
                        txn.RawPtr(), 
                        0,              // object handle
                        (void*)value.c_str(), 
                        (uint32_t)((value.size() + 1) * sizeof(value[0])), 
                        std::numeric_limits<int64>::max(), 
                        (CancellationTokenSourceHandle*)&cts,
                        [](void* acsHandle, HRESULT _hresult) {
                            AwaitableCompletionSource<void>* acs = (AwaitableCompletionSource<void>*)acsHandle;
                            if (!SUCCEEDED(_hresult))
                                acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                            else
                                acs->Set();
                        }, 
                        acs.RawPtr(), 
                        &synchronouscomplete);

                    VERIFY_IS_TRUE(SUCCEEDED(hresult));

                    if (!synchronouscomplete)
                    {
                        CancellationTokenSource_Release(cts);
                        SyncAwait(acs->GetAwaitable());
                    }

                    SyncAwait(txn->CommitAsync());

                    LONG64 count;
                    hresult = ConcurrentQueue_GetCount(stateProvider.RawPtr(), &count);
                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    VERIFY_IS_TRUE(count == 1);
                }

                //
                // TryDequeueAsync
                //
                {
                    Transaction::SPtr txn;
                    status = replica_->TxnReplicator->CreateTransaction(txn);
                    THROW_ON_FAILURE(status);
                    KFinally([&] {txn->Dispose(); });

                    AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t>>::SPtr acs = nullptr;
                    AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t>>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                    Buffer buffer;
                    BOOL succeeded;
                    byte *bytesValue;
                    uint32_t bytesValueLength;
                    size_t objectHandle;

                    HRESULT hresult = ConcurrentQueue_TryDequeueAsync(
                        stateProvider.RawPtr(),
                        txn.RawPtr(), 
                        std::numeric_limits<int64>::max(), 
                        &objectHandle,
                        &buffer, 
                        (CancellationTokenSourceHandle*)&cts,
                        &succeeded,
                        [](void* acsHandle, HRESULT _hresult, BOOL _succeeded, size_t handle, void* bytes, uint32_t bytesLength) {
                            auto acs = (AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t>>*)acsHandle;
                            if (!SUCCEEDED(_hresult))
                                acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                            else
                                acs->SetResult(make_tuple(_succeeded, handle, (byte*)bytes, bytesLength));
                        }, 
                        acs.RawPtr(), 
                        &synchronouscomplete);

                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    if (synchronouscomplete)
                    {
                        bytesValue = (byte*)buffer.Bytes;
                        bytesValueLength = buffer.Length;
                    }
                    else
                    {
                        CancellationTokenSource_Release(cts);
                        tie(succeeded, objectHandle, bytesValue, bytesValueLength) = SyncAwait(acs->GetAwaitable());
                    }

                    VERIFY_IS_TRUE(succeeded);

#ifdef FEATURE_CACHE_OBJHANDLE             
                    VERIFY_IS_TRUE(objectHandle == 1);
#endif

                    wstring str((LPCWSTR)bytesValue);
                    VERIFY_IS_TRUE(str.compare(L"value1") == 0);
                    VERIFY_IS_TRUE(bytesValueLength == (str.length() + 1) * (sizeof(unsigned short)));

                    SyncAwait(txn->CommitAsync());
                    if (synchronouscomplete)
                        Buffer_Release(buffer.Handle);

                    LONG64 count;
                    hresult = ConcurrentQueue_GetCount(stateProvider.RawPtr(), &count);
                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    VERIFY_IS_TRUE(count == 0);
                }

                //
                // TryDequeueAsync on empty queue
                //
                {
                    Transaction::SPtr txn;
                    status = replica_->TxnReplicator->CreateTransaction(txn);
                    THROW_ON_FAILURE(status);
                    KFinally([&] {txn->Dispose(); });

                    AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t>>::SPtr acs = nullptr;
                    AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t>>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);

                    Buffer buffer;
                    BOOL succeeded;
                    byte *bytesValue;
                    uint32_t bytesValueLength;
                    size_t objectHandle;

                    HRESULT hresult = ConcurrentQueue_TryDequeueAsync(
                        stateProvider.RawPtr(),
                        txn.RawPtr(), 
                        std::numeric_limits<int64>::max(), 
                        &objectHandle,
                        &buffer, 
                        (CancellationTokenSourceHandle*)&cts,
                        &succeeded,
                        [](void* acsHandle, HRESULT _hresult, BOOL _succeeded, size_t handle, void* bytes, uint32_t bytesLength) {
                            auto acs = (AwaitableCompletionSource<tuple<BOOL, size_t, byte*, uint32_t>>*)acsHandle;
                            if (!SUCCEEDED(_hresult))
                                acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                            else
                                acs->SetResult(make_tuple(_succeeded, handle, (byte*)bytes, bytesLength));
                        }, 
                        acs.RawPtr(), 
                        &synchronouscomplete);

                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    if (synchronouscomplete)
                    {
                        bytesValue = (byte*)buffer.Bytes;
                        bytesValueLength = buffer.Length;
                    }
                    else
                    {
                        CancellationTokenSource_Release(cts);
                        tie(succeeded, objectHandle, bytesValue, bytesValueLength) = SyncAwait(acs->GetAwaitable());
                    }

                    VERIFY_IS_TRUE(!succeeded);

                    SyncAwait(txn->CommitAsync());

                    LONG64 count;
                    hresult = ConcurrentQueue_GetCount(stateProvider.RawPtr(), &count);
                    VERIFY_IS_TRUE(SUCCEEDED(hresult));
                    VERIFY_IS_TRUE(count == 0);
                }
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_AddStateProvider_Compat_SUCCESS)
        {
            wstring testName(L"TxnReplicator_AddStateProvider_Compat_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                HRESULT hresult;
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                wstring stateProviderName(L"fabric:/store/dict1");
                wstring lang = L"CSHARP";
                wstring langTypeInfo = L"Microsoft.ServiceFabric.Data.Collections.DistributedDictionary";
                StateProvider_Info stateProviderInfo = {
                    StateProvider_Info_V1_Size,
                    StateProvider_Kind_ReliableDictionary_Compat,
                    langTypeInfo.c_str()
                };

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<void>::SPtr acs = nullptr;
                AwaitableCompletionSource<void>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                hresult = TxnReplicator_AddStateProviderAsync(
                    replica_->TxnReplicator.RawPtr(),
                    txn.RawPtr(),
                    stateProviderName.c_str(),
                    lang.c_str(),
                    &stateProviderInfo,
                    std::numeric_limits<int64>::max(),
                    (CancellationTokenSourceHandle*)&cts,
                    [](void* acsHandle, HRESULT _hresult) {
                        AwaitableCompletionSource<void>* acs = (AwaitableCompletionSource<void>*)acsHandle;
                        if (SUCCEEDED(_hresult))
                            acs->Set();
                        else
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    },
                    acs.RawPtr(),
                    &synchronouscomplete);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (!synchronouscomplete)
                {
                    //Ignore cts
                    CancellationTokenSource_Release(cts);
                    SyncAwait(acs->GetAwaitable());
                }

                status = SyncAwait(txn->CommitAsync());
                THROW_ON_FAILURE(status);

                IStateProvider2* stateProvider;
                StateProvider_Info stateProvider_Info;

                hresult = TxnReplicator_GetStateProvider(replica_->TxnReplicator.RawPtr(), stateProviderName.c_str(), (StateProviderHandle*)&stateProvider);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                     
                stateProvider_Info.Size = sizeof(StateProvider_Info);
                hresult = StateProvider_GetInfo(stateProvider, lang.c_str(), &stateProvider_Info);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                VERIFY_IS_TRUE(stateProvider_Info.Kind == StateProvider_Kind_Store);
                VERIFY_IS_TRUE(langTypeInfo.compare(stateProvider_Info.LangMetadata) == 0);

                // Verify that you can add to state provider
                AddKeyValuePair(stateProvider, L"key1", 1, L"value1");

                StateProvider_Release(stateProvider);
            }
        }

        BOOST_AUTO_TEST_CASE(TxnReplicator_GetOrAddStateProvider_Compat_SUCCESS)
        {
            wstring testName(L"TxnReplicator_GetOrAddStateProvider_Compat_SUCCESS");

            TEST_TRACE_BEGIN(testName)
            {
                NTSTATUS status;
                HRESULT hresult;
                BOOL synchronouscomplete;
                ktl::CancellationTokenSource* cts;
                wstring stateProviderName(L"fabric:/TxnReplicator_GetOrAddStateProvider_Compat/dict1");
                wstring lang = L"CSHARP";
                wstring langTypeInfo = L"Microsoft.ServiceFabric.Data.Collections.DistributedDictionary";
                StateProviderHandle stateProviderHandle;
                BOOL alreadyExists;
                StateProvider_Info stateProviderInfo = {
                    StateProvider_Info_V1_Size,
                    StateProvider_Kind_ReliableDictionary_Compat,
                    langTypeInfo.c_str()
                };

                Transaction::SPtr txn;
                status = replica_->TxnReplicator->CreateTransaction(txn);
                THROW_ON_FAILURE(status);
                KFinally([&] {txn->Dispose(); });

                AwaitableCompletionSource<IStateProvider2*>::SPtr acs = nullptr;
                AwaitableCompletionSource<IStateProvider2*>::Create(underlyingSystem_->PagedAllocator(), TEST_CEXPORT_TAG, acs);
                hresult = TxnReplicator_GetOrAddStateProviderAsync(
                    replica_->TxnReplicator.RawPtr(),
                    nullptr,
                    stateProviderName.c_str(),
                    lang.c_str(),
                    &stateProviderInfo,
                    std::numeric_limits<int64>::max(),
                    (CancellationTokenSourceHandle*)&cts,
                    &stateProviderHandle,
                    &alreadyExists,
                    [](void* acsHandle, HRESULT _hresult, StateProviderHandle stateProviderHandle, BOOL exist) {
                        auto acs = *(ktl::AwaitableCompletionSource<IStateProvider2*>::SPtr*)acsHandle;
                        if (SUCCEEDED(_hresult))
                        {
                            VERIFY_IS_TRUE(exist == false);
                            VERIFY_IS_NOT_NULL(stateProviderHandle);
                            acs->SetResult((IStateProvider2*)stateProviderHandle);
                        }
                        else
                            acs->SetException(ktl::Exception(StatusConverter::Convert(_hresult)));
                    },
                    &acs,
                    &synchronouscomplete);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));
                if (!synchronouscomplete)
                {
                    //Ignore cts
                    CancellationTokenSource_Release(cts);
                    stateProviderHandle = SyncAwait(acs->GetAwaitable());
                }
                else
                {
                    VERIFY_IS_TRUE(alreadyExists == false);
                }

                VERIFY_IS_TRUE(stateProviderHandle != nullptr);
                

                StateProvider_Info stateProvider_Info;
                stateProvider_Info.Size = sizeof(StateProvider_Info);
                hresult = StateProvider_GetInfo(stateProviderHandle, lang.c_str(), &stateProvider_Info);
                VERIFY_IS_TRUE(SUCCEEDED(hresult));

                VERIFY_IS_TRUE(stateProvider_Info.Kind == StateProvider_Kind_Store);
                VERIFY_IS_TRUE(langTypeInfo.compare(stateProvider_Info.LangMetadata) == 0);

                // Verify that you can add to state provider
                AddKeyValuePair((IStateProvider2*)stateProviderHandle, L"key1", 1, L"value1");

                status = SyncAwait(txn->CommitAsync());
                THROW_ON_FAILURE(status);

                StateProvider_Release(stateProviderHandle);
            }
        } 

        BOOST_AUTO_TEST_SUITE_END();
    }
}

// Dummy methods required to satisfy link of ReliableCollectionRuntimeImpl.test.exe
// These methods are not used at runtime for ReliableCollectionRuntimeImpl.test.exe

HRESULT FabricGetActivationContext( 
    REFIID riid,
    void **activationContext)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(activationContext);
    return S_OK;
}

HRESULT FabricLoadReplicatorSettings(
    IFabricCodePackageActivationContext const * codePackageActivationContext,
    LPCWSTR configurationPackageName,
    LPCWSTR sectionName,
    IFabricReplicatorSettingsResult ** replicatorSettings)
{
    UNREFERENCED_PARAMETER(codePackageActivationContext);
    UNREFERENCED_PARAMETER(configurationPackageName);
    UNREFERENCED_PARAMETER(sectionName);
    UNREFERENCED_PARAMETER(replicatorSettings);
    return S_OK;
}

HRESULT FabricLoadSecurityCredentials(
    IFabricCodePackageActivationContext const * codePackageActivationContext,
    LPCWSTR configurationPackageName,
    LPCWSTR sectionName,
    IFabricSecurityCredentialsResult ** securitySettings)
{
    UNREFERENCED_PARAMETER(codePackageActivationContext);
    UNREFERENCED_PARAMETER(configurationPackageName);
    UNREFERENCED_PARAMETER(sectionName);
    UNREFERENCED_PARAMETER(securitySettings);
    return S_OK;
}


