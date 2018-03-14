// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DICTIONARYENUM_TAG 'meID'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class SweepEnumerator : public IEnumerator<KSharedPtr<VersionedItem<TValue>>>
        {
            K_FORCE_SHARED(SweepEnumerator)

        public:
            static NTSTATUS Create(
                __in ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>> & deltaList,
                __in ULONG32 highestIndex,
                __in KAllocator & allocator,
                __in StoreTraceComponent & traceComponent,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(DICTIONARYENUM_TAG, allocator) SweepEnumerator(deltaList, highestIndex, traceComponent);

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            __declspec(property(get = get_CurrentComponentSPtr)) KSharedPtr<DifferentialStoreComponent<TKey, TValue>> CurrentComponentSPtr;
            KSharedPtr<DifferentialStoreComponent<TKey, TValue>> get_CurrentComponentSPtr() const
            {
               return currentComponentSPtr_;
            }

        public:
            KSharedPtr<VersionedItem<TValue>> Current()
            {
                auto currentItem = currentComponentEnumeratorSPtr_->Current();
                return currentItem.Value;
            }

            bool MoveNext()
            {
                while (currentComponentEnumeratorSPtr_ == nullptr || currentComponentEnumeratorSPtr_->MoveNext() == false)
                {
                    if (index_ <= 0)
                    {
                        return false;
                    }

                    KSharedPtr<DifferentialStoreComponent<TKey, TValue>> differentialStoreComponent = nullptr;
                    bool exists = componentSPtr_->TryGetValue(index_, differentialStoreComponent);
                    STORE_ASSERT(exists, "index={1} should exist", index_);
                    STORE_ASSERT(differentialStoreComponent != nullptr, "differential store component cannot be null");
                    currentComponentSPtr_ = differentialStoreComponent;
                    auto enumerator = differentialStoreComponent->GetKeyAndValues();
                    currentComponentEnumeratorSPtr_ = static_cast<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> *>(enumerator.RawPtr());
                    STORE_ASSERT(currentComponentEnumeratorSPtr_ != nullptr, "current enumerator cannot be null");
                    index_--;
                }

                return true;
            }

        private:
            // Note: TKey, TValue should have default constructor.
            NOFAIL SweepEnumerator(
                __in ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>> & deltaList, 
                __in ULONG32 highestIndex,
                __in StoreTraceComponent & traceComponent);

            ULONG32 index_;
            KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>> componentSPtr_;
            KSharedPtr<DifferentialStoreComponent<TKey, TValue>> currentComponentSPtr_;
            KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> currentComponentEnumeratorSPtr_ = nullptr;

            StoreTraceComponent::SPtr traceComponent_;
        };

        // Skip the last delta state since we do not want to sweep the items that comes from the differntial state yet.
        // Due to background consolidation, file metadata associated with items that just came from the differentrial state might not yet be set to Next Metadata table,
        // so last delta needs to be skipped to avoid reads from failing.

        template <typename TKey, typename TValue>
        SweepEnumerator<TKey, TValue>::SweepEnumerator(
            __in ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>> & deltaList,
            __in ULONG32 highestIndex,
            __in StoreTraceComponent & traceComponent)
            : index_(highestIndex > 0 ? highestIndex-1 : 0), // Skip the last delta state
           componentSPtr_(&deltaList),
           currentComponentSPtr_(nullptr),
           traceComponent_(&traceComponent)
        {
        }

        template <typename TKey, typename TValue>
        SweepEnumerator<TKey, TValue>::~SweepEnumerator()
        {
        }
    }
}
