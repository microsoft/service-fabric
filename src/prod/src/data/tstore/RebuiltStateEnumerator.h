// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define REBUILTSTATEENUMERATOR_TAG 'eaRS'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class RebuiltStateAsyncEnumerator
            : public KObject<RebuiltStateAsyncEnumerator<TKey, TValue>>
            , public KShared<RebuiltStateAsyncEnumerator<TKey, TValue>>
            , public IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>
        {
            K_FORCE_SHARED(RebuiltStateAsyncEnumerator)
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator)
        public:
            static NTSTATUS Create(
                __in bool isValueAReferenceType,
                __in IEnumerator<TKey> & keyEnumerator,
                __in DifferentialStoreComponent<TKey, TValue> & differentialState,
                __in ConsolidationManager<TKey, TValue> & consolidatedState,
                __in MetadataTable & currentMetadataTable,
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                 
                result = _new(REBUILTSTATEENUMERATOR_TAG, allocator) RebuiltStateAsyncEnumerator(
                    isValueAReferenceType, 
                    keyEnumerator, 
                    differentialState, 
                    consolidatedState, 
                    currentMetadataTable, 
                    valueSerializer,
                    traceComponent);

                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> GetCurrent() override
            {
                if (isInvalidated_)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                return currentItem_;
            }

            ktl::Awaitable<bool> MoveNextAsync(__in ktl::CancellationToken const & cancellationToken) override
            {
                if (isInvalidated_)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                while (keyEnumeratorSPtr_->MoveNext())
                {
                    TKey currentKey = keyEnumeratorSPtr_->Current();

                    KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = differentialStateSPtr_->Read(currentKey);
                    if (versionedItemSPtr == nullptr)
                    {
                        versionedItemSPtr = consolidatedStateSPtr_->Read(currentKey);
                    }

                    if (versionedItemSPtr->GetRecordKind() == RecordKind::DeletedVersion)
                    {
                        continue;
                    }

                    // Current metadata table is sufficient since this is only called during restore, recovery, and copy
                    auto pagedValue = co_await versionedItemSPtr->GetValueAsync(*currentMetadataTableSPtr_, *valueSerializerSPtr_, ReadMode::ReadValue, *traceComponent_, ktl::CancellationToken::None);
                    auto versionValuePair = KeyValuePair<LONG64, TValue>(versionedItemSPtr->GetVersionSequenceNumber(), pagedValue);
                    currentItem_ = KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>(currentKey, versionValuePair);
                    co_return true;

                }

                co_return false;
            }

            void Reset() override
            {
                // TODO: IEnumerator also needs a Reset() method
                throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
            }

            void Dispose() override
            {
            }

            void InvalidateEnumerator()
            {
                STORE_ASSERT(isInvalidated_ == false, "Must only be invalidated once");
                isInvalidated_ = true;
            }


        private:
            RebuiltStateAsyncEnumerator(
                __in bool isValueAReferenceType,
                __in IEnumerator<TKey> & keyEnumerator,
                __in DifferentialStoreComponent<TKey, TValue> & differentialState,
                __in ConsolidationManager<TKey, TValue> & consolidatedState,
                __in MetadataTable & currentMetadataTable,
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
                __in StoreTraceComponent & traceComponent);

            bool isValueAReferenceType_;
            KSharedPtr<IEnumerator<TKey>> keyEnumeratorSPtr_;
            KSharedPtr<DifferentialStoreComponent<TKey, TValue>> differentialStateSPtr_;
            KSharedPtr<ConsolidationManager<TKey, TValue>> consolidatedStateSPtr_;
            KSharedPtr<MetadataTable> currentMetadataTableSPtr_;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr_;

            KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> currentItem_;
            bool isInvalidated_;

            StoreTraceComponent::SPtr traceComponent_;
        };

        template<typename TKey, typename TValue>
        RebuiltStateAsyncEnumerator<TKey, TValue>::RebuiltStateAsyncEnumerator(
            __in bool isValueAReferenceType,
            __in IEnumerator<TKey> & keyEnumerator,
            __in DifferentialStoreComponent<TKey, TValue> & differentialState,
            __in ConsolidationManager<TKey, TValue> & consolidatedState,
            __in MetadataTable & currentMetadataTable,
            __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
            __in StoreTraceComponent & traceComponent) :
            isValueAReferenceType_(isValueAReferenceType),
            keyEnumeratorSPtr_(&keyEnumerator),
            differentialStateSPtr_(&differentialState),
            consolidatedStateSPtr_(&consolidatedState),
            currentMetadataTableSPtr_(&currentMetadataTable),
            valueSerializerSPtr_(&valueSerializer),
            isInvalidated_(false),
            traceComponent_(&traceComponent)
        {
            currentItem_ = KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>();
        }

        template<typename TKey, typename TValue>
        RebuiltStateAsyncEnumerator<TKey, TValue>::~RebuiltStateAsyncEnumerator()
        {
        }
    }
}
