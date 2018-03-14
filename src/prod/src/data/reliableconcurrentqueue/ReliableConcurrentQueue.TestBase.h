// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::Collections;
    using namespace TStoreTests;
    using namespace TxnReplicator;

    template<typename TValue>
    class ReliableConcurrentQueueTestBase
    {
    public:
        __declspec(property(get = get_RCQ)) KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> RCQ;
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> get_RCQ() const
        {
            return rcqSPtr_;
        }

        __declspec(property(get = get_Replicator)) KSharedPtr<MockTransactionalReplicator> Replicator;
        KSharedPtr<MockTransactionalReplicator> get_Replicator() const
        {
            return mockReplicatorSPtr_;
        }

        ReliableConcurrentQueueTestBase()
        {
            Setup();
        }

        ~ReliableConcurrentQueueTestBase()
        {
            Cleanup();
        }

        void Setup()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);

            mockReplicatorSPtr_ = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
            mockReplicatorSPtr_->SetReadable(true);

            // Set up the work folder.
            auto currentFolderPath = CreateTempDirectory();

            KWeakRef<ITransactionalReplicator>::SPtr tmpWRef;
            status = mockReplicatorSPtr_->ITransactionalReplicator::GetWeakIfRef(tmpWRef);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KUriView name(L"fabric:/RCQ");

            rcqSPtr_ = CreateRCQ(*tmpWRef, name, 5, *currentFolderPath, nullptr, nullptr);

            IStateProvider2::SPtr stateProviderSPtr(rcqSPtr_.RawPtr());
            mockReplicatorSPtr_->RegisterStateProvider(rcqSPtr_->Name, *stateProviderSPtr);
            SyncAwait(mockReplicatorSPtr_->OpenAsync());
        }

        void Cleanup()
        {
            SyncAwait(mockReplicatorSPtr_->RemoveStateAndCloseAsync());

            rcqSPtr_ = __nullptr;
            mockReplicatorSPtr_ = nullptr;
            ktlSystem_->Shutdown();
        }

        KSharedPtr<TxnReplicator::Transaction> CreateReplicatorTransaction()
        {
            return Transaction::CreateTransaction(*mockReplicatorSPtr_, GetAllocator());
        }

        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> CreateRCQ(
            __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef, 
            __in KUriView const & name,
            __in LONG64 stateProviderId,
            __in KStringView const & workFolder, 
            __in_opt OperationData const * const initializationContext,
            __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children)
        {
            UNREFERENCED_PARAMETER(initializationContext);

            KAllocator& allocator = GetAllocator();

            KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = nullptr;

            typename TestStateSerializer<TValue>::SPtr longSerializer = nullptr;
            NTSTATUS status = TestStateSerializer<TValue>::Create(allocator, longSerializer);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            hashFunction_ = K_DefaultHashFunction;

            KUriView nameCast = const_cast<KUriView &>(name);
            status = Data::Collections::ReliableConcurrentQueue<TValue>::Create(
                allocator,
                *GetPartitionedReplicaId(),
                hashFunction_,
                nameCast,
                stateProviderId,
                *longSerializer,
                rcq);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            rcq->Initialize(transactionalReplicatorWRef, workFolder, children);

            return rcq;
        }

        PartitionedReplicaId::SPtr GetPartitionedReplicaId()
        {
            KAllocator& allocator = GetAllocator();
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            return PartitionedReplicaId::Create(guid, replicaId, allocator);
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

        KSharedPtr<MockTransactionalReplicator> CreateMockReplicator(FABRIC_REPLICA_ROLE role) //TODO: IStatefulServicePartition& partition
        {
            KAllocator& allocator = GetAllocator();
            MockTransactionalReplicator::SPtr replicator = nullptr;
            NTSTATUS status = MockTransactionalReplicator::Create(allocator, replicator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            replicator->Initialize(*GetPartitionedReplicaId());
            replicator->Role = role;
            return replicator;
        }

        KString::SPtr CreateGuidString()
        {
            KGuid guid;
            guid.CreateNew();
            KString::SPtr stringSPtr = nullptr;
            NTSTATUS status = KString::Create(stringSPtr, GetAllocator(), KStringView::MaxGuidString);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            BOOLEAN result = stringSPtr->FromGUID(guid);
            CODING_ERROR_ASSERT(result == TRUE);

            return stringSPtr;
        }

        KString::CSPtr CreateFileString(__in KStringView const & name)
        {
            KString::SPtr fileName;
            KAllocator& allocator = GetAllocator();

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
            NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        KString::CSPtr CreateTempDirectory()
        {
            // TODO: Create directories based on partition and replica ID
            KString::SPtr fileName = CreateGuidString();
            KString::CSPtr currentFolderPath = CreateFileString(*fileName);
            Common::ErrorCode errorCode = Common::Directory::Create2(currentFolderPath->operator LPCWSTR());
            if (errorCode.IsSuccess() == false)
            {
                throw ktl::Exception(errorCode.ToHResult());
            }
            return currentFolderPath;
        }

    private:
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<TValue>> rcqSPtr_ = nullptr;
        KSharedPtr<TStoreTests::MockTransactionalReplicator> mockReplicatorSPtr_ = nullptr;
        KtlSystem* ktlSystem_;
        KDelegate<ULONG(const LONG64 & Key)> hashFunction_;
    };
}
