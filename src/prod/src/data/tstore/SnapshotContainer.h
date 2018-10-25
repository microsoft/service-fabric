// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SNAPSHOT_CONTAINER_TAG 'snCO'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class SnapshotContainer
            : public KObject<SnapshotContainer<TKey, TValue>>
            , public KShared<SnapshotContainer<TKey, TValue>>
        {
            K_FORCE_SHARED(SnapshotContainer)
        public:
            static NTSTATUS Create(
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(SNAPSHOT_CONTAINER_TAG, allocator) SnapshotContainer(valueSerializer, traceComponent);

                if (!output)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return status;
            }

            __declspec(property(get = get_FileMetadataTableLsnMap)) ConcurrentDictionary<LONG64, ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr>::SPtr FileMetadataTableLsnMap;
            ConcurrentDictionary<LONG64, ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr>::SPtr get_FileMetadataTableLsnMap()
            {
                return fileMetadataTableLsnMapSPtr_;
            }

            __declspec(property(get = get_ValueSerializer)) KSharedPtr<Data::StateManager::IStateSerializer<TValue>> ValueSerializer;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> get_ValueSerializer()
            {
                return valueSerializerSPtr_;
            }

            // For testability
            __declspec(property(get = get_DelayRemoveTcs, put = set_DelayRemoveTcs)) ktl::AwaitableCompletionSource<bool>::SPtr DelayRemoveTcsSPtr;
            ktl::AwaitableCompletionSource<bool>::SPtr get_DelayRemoveTcs()
            {
                return delayRemoveTcsSPtr_;
            }

            void put_DelayRemoveTcs(ktl::AwaitableCompletionSource<bool> & delayRemoveTcs)
            {
                delayRemoveTcsSPtr_ = &delayRemoveTcs;
            }

            bool ContainsKey(__in LONG64 key)
            {
                return containerSPtr_->ContainsKey(key);
            }

            KSharedPtr<SnapshotComponent<TKey, TValue>> Read(__in LONG64 key)
            {
                KSharedPtr<SnapshotComponent<TKey, TValue>> componentSPtr = nullptr;
                if (containerSPtr_->TryGetValue(key, componentSPtr))
                {
                    return componentSPtr;
                }

                return nullptr;
            }

            KSharedPtr<SnapshotComponent<TKey, TValue>> GetOrAdd(__in LONG64 key, __in SnapshotComponent<TKey, TValue> & value)
            {
                KSharedPtr<SnapshotComponent<TKey, TValue>> valueSPtr = &value;
                auto result = containerSPtr_->GetOrAdd(key, valueSPtr);
                return result;
            }

            bool TryAddFileMetadata(__in LONG64 lsn, __in FileMetadata & fileMetadata)
            {
                ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr defaultConcurrentDict = nullptr;

                NTSTATUS status = ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::Create(
                    ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::DefaultConcurrencyLevel, 
                    ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::DefaultInitialCapacity,
                    ULONG32Comparer::HashFunction,
                    fileIdComparerSPtr_,
                    this->GetThisAllocator(), 
                    defaultConcurrentDict);
                Diagnostics::Validate(status);

                ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr table = fileMetadataTableLsnMapSPtr_->GetOrAdd(lsn, defaultConcurrentDict);

                FileMetadata::SPtr fileMetadataSPtr = &fileMetadata;

                if (table->TryAdd(fileMetadataSPtr->FileId, fileMetadataSPtr))
                {
                    fileMetadataSPtr->AddReference();
                    return true;
                }

                return false;
            }

            ktl::Awaitable<void> RemoveAsync(__in LONG64 key)
            {
                KShared$ApiEntry();
                SharedException::CSPtr exceptionCSPtr = nullptr;

                ktl::AwaitableCompletionSource<bool>::SPtr removeTCS = nullptr;
                
                bool addedToInflightRemoves = false;

                try
                {
                    KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<SnapshotComponent<TKey, TValue>>>> snappedContainer;

                    NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(this->GetThisAllocator(), SNAPSHOT_CONTAINER_TAG, removeTCS);
                    Diagnostics::Validate(status);

                    K_LOCK_BLOCK(closeLock_)
                    {
                        if (containerSPtr_ == nullptr)
                        {
                            co_return;
                        }

                        snappedContainer = containerSPtr_;

                        inflightRemovesSPtr_->Add(key, removeTCS);
                        addedToInflightRemoves = true;
                    }

                    KSharedPtr<SnapshotComponent<TKey, TValue>> value = nullptr;
                    auto result = snappedContainer->TryRemove(key, value);
                    STORE_ASSERT(result, "SnapshotComponent for LSN {1} should be removed from snapshot container", key);

                    ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr fileMetadataTableSPtr = nullptr;
                    if (fileMetadataTableLsnMapSPtr_->TryRemove(key, fileMetadataTableSPtr))
                    {
                        auto enumeratorSPtr = fileMetadataTableSPtr->GetEnumerator();
                        while (enumeratorSPtr->MoveNext())
                        {
                            auto item = enumeratorSPtr->Current();
                            auto metadataSPtr = item.Value;
                            STORE_ASSERT(metadataSPtr != nullptr, "found null file metadata in table");
                            co_await metadataSPtr->ReleaseReferenceAsync();
                        }
                    }

                    if (delayRemoveTcsSPtr_ != nullptr)
                    {
                        co_await delayRemoveTcsSPtr_->GetAwaitable();
                    }
                }
                catch (ktl::Exception const & e)
                {
                    exceptionCSPtr = SharedException::Create(e, this->GetThisAllocator());
                }

                // Ensure that the TCS is removed from the inflight removes, even in case of exceptions
                // Using try-catch instead of KFinally because there's an issue on Linux where the key 
                // parameter is passed incorrectly to the KFinally
                if (removeTCS != nullptr)
                {
                    removeTCS->SetResult(true);
                }

                if (addedToInflightRemoves && !isClosed_)
                {
                    K_LOCK_BLOCK(closeLock_)
                    {
                        if (!isClosed_)
                        {
                            bool removed = inflightRemovesSPtr_->Remove(key);
                            STORE_ASSERT(removed, "Key {1} should have been removed from list of inflight remove tasks", key);
                        }
                    }
                }

                if (exceptionCSPtr != nullptr)
                {
                    auto exec = exceptionCSPtr->Info;
                    throw exec;
                }
            } 
 
            ULONG32 GetCount()
            {
                return containerSPtr_->Count;
            }
            
            LONG64 GetMemorySize(__in LONG64 keySize)
            {
                LONG64 size = 0;
                auto enumeratorSPtr = containerSPtr_->GetEnumerator();
                while (enumeratorSPtr->MoveNext())
                {
                    auto current = enumeratorSPtr->Current();
                    auto component = current.Value;
                    size += component->Size;
                    size += component->Count * (keySize + Constants::ValueMetadataSizeBytes);
                }
                return size;
            }

            ktl::Awaitable<void> CloseAsync()
            {
                KShared$ApiEntry();

                try
                {
                    if (isClosed_)
                    {
                        co_return;
                    }

                    K_LOCK_BLOCK(closeLock_)
                    {
                        valueSerializerSPtr_ = nullptr;
                        containerSPtr_ = nullptr;
                        isClosed_ = true;
                    }

                    // Wait for all pending removes to finish
                    auto inflightRemovesEnumerator = inflightRemovesSPtr_->GetEnumerator();
                    while (inflightRemovesEnumerator->MoveNext())
                    {
                        auto item = inflightRemovesEnumerator->Current();
                        ktl::AwaitableCompletionSource<bool>::SPtr completionSourceSPtr = item.Value;
                        co_await completionSourceSPtr->GetAwaitable();
                    }

                    if (fileMetadataTableLsnMapSPtr_ != nullptr)
                    {
                        auto lsnMapEnumeratorSPtr = fileMetadataTableLsnMapSPtr_->GetEnumerator();
                        while (lsnMapEnumeratorSPtr->MoveNext())
                        {
                            auto table = lsnMapEnumeratorSPtr->Current().Value;
                            auto tableEnumeratorSPtr = table->GetEnumerator();
                            while (tableEnumeratorSPtr->MoveNext())
                            {
                                auto currentItem = tableEnumeratorSPtr->Current();
                                auto fileMetadataSPtr = currentItem.Value;

                                STORE_ASSERT(fileMetadataSPtr != nullptr, "found null metadata in table");
                                co_await fileMetadataSPtr->ReleaseReferenceAsync();
                            }
                        }

                        fileMetadataTableLsnMapSPtr_->Clear();
                    }
                }
                catch (ktl::Exception const & e)
                {
                    AssertException(L"CloseAsync", e);
                }
            }

        private:
            void AssertException(__in KStringView const & methodName, __in ktl::Exception const & exception)
            {
                KDynStringA stackString(this->GetThisAllocator());
                Diagnostics::GetExceptionStackTrace(exception, stackString);
                STORE_ASSERT(
                    false,
                    "UnexpectedException: Message: {2} Code:{4}\nStack: {3}",
                    ToStringLiteral(methodName),
                    ToStringLiteral(stackString),
                    exception.GetStatus());
            }

            SnapshotContainer(__in Data::StateManager::IStateSerializer<TValue> & valueSerializer, __in StoreTraceComponent & traceComponent)
                : valueSerializerSPtr_(&valueSerializer),
                containerSPtr_(nullptr),
                fileMetadataTableLsnMapSPtr_(nullptr),
                fileIdComparerSPtr_(nullptr),
                closeLock_(),
                isClosed_(false),
                delayRemoveTcsSPtr_(nullptr),
                traceComponent_(&traceComponent)
            {
                NTSTATUS status = ConcurrentDictionary<LONG64, KSharedPtr<SnapshotComponent<TKey, TValue>>>::Create(this->GetThisAllocator(), containerSPtr_);
                Diagnostics::Validate(status);

                status = ConcurrentDictionary<LONG64, ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr>::Create(this->GetThisAllocator(), fileMetadataTableLsnMapSPtr_);
                Diagnostics::Validate(status);

                ULONG32Comparer::SPtr ulong32ComparerSPtr;
                status = ULONG32Comparer::Create(this->GetThisAllocator(), ulong32ComparerSPtr);
                Diagnostics::Validate(status);

                fileIdComparerSPtr_ = ulong32ComparerSPtr.DownCast<IComparer<ULONG32>>();

                status = ConcurrentDictionary<LONG64, ktl::AwaitableCompletionSource<bool>::SPtr>::Create(this->GetThisAllocator(), inflightRemovesSPtr_);
                Diagnostics::Validate(status);
            }
            
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr_;

            // Contains version that cannot be removed due to ongoing snapshot reads
            KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<SnapshotComponent<TKey, TValue>>>> containerSPtr_;

            // Contains the snapshot of the metadata table used to service snapshot reads
            ConcurrentDictionary<LONG64, ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr>::SPtr fileMetadataTableLsnMapSPtr_;

            IComparer<ULONG32>::SPtr fileIdComparerSPtr_;

            // This is to synchronize Close and Remove, otherwise Remove won't have a valid container
            KSpinLock closeLock_;

            // Dictionary for keeping track of pending removes
            KSharedPtr<ConcurrentDictionary<LONG64, ktl::AwaitableCompletionSource<bool>::SPtr>> inflightRemovesSPtr_;

            bool isClosed_;

            ktl::AwaitableCompletionSource<bool>::SPtr delayRemoveTcsSPtr_;

            StoreTraceComponent::SPtr traceComponent_;
        };
        
        template <typename TKey, typename TValue>
        SnapshotContainer<TKey, TValue>::~SnapshotContainer()
        {
        }
    }
}
