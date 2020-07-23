// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TEST_DICTIONARYCHANGEHANDLER_TAG 'dcHT'

namespace TStoreTests
{
    template<typename TKey, typename TValue>
    class TestDictionaryChangeHandler :
        public KObject<TestDictionaryChangeHandler<TKey, TValue>>,
        public KShared<TestDictionaryChangeHandler<TKey, TValue>>,
        public IDictionaryChangeHandler<TKey, TValue>
    {
        K_FORCE_SHARED(TestDictionaryChangeHandler)
        K_SHARED_INTERFACE_IMP(IDictionaryChangeHandler)
    public:

    public:
        static NTSTATUS Create(
            __in IComparer<TKey> & keyComparer,
            __in IComparer<TValue> & valueComparer,
            __in KAllocator & allocator, 
            __out SPtr & result)
        {
            NTSTATUS status;

            SPtr output = _new(TEST_DICTIONARYCHANGEHANDLER_TAG, allocator) TestDictionaryChangeHandler(keyComparer, valueComparer);

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

    public: // IDictionaryChangeHandler methods
        ktl::Awaitable<void> OnAddedAsync(
            __in TxnReplicator::TransactionBase const& replicatorTransaction,
            __in TKey key,
            __in TValue value,
            __in LONG64 sequenceNumber,
            __in bool isPrimary) noexcept override
        {
            UNREFERENCED_PARAMETER(isPrimary);

            Notification n;
            n.Operation = StoreModificationType::Add;
            n.Key = key;
            n.Value = value;
            n.SequenceNumber = sequenceNumber;

            actualNotifications_->Append(n);
            co_await suspend_never();
        }

        ktl::Awaitable<void> OnUpdatedAsync(
            __in TxnReplicator::TransactionBase const& replicatorTransaction,
            __in TKey key,
            __in TValue value,
            __in LONG64 sequenceNumber,
            __in bool isPrimary) noexcept override
        {
            UNREFERENCED_PARAMETER(isPrimary);

            Notification n;
            n.Operation = StoreModificationType::Update;
            n.Key = key;
            n.Value = value;
            n.SequenceNumber = sequenceNumber;

            actualNotifications_->Append(n);
            co_await suspend_never();
        }

        ktl::Awaitable<void> OnRemovedAsync(
            __in TxnReplicator::TransactionBase const& replicatorTransaction,
            __in TKey key,
            __in LONG64 sequenceNumber,
            __in bool isPrimary) noexcept override
        {
            UNREFERENCED_PARAMETER(sequenceNumber);
            UNREFERENCED_PARAMETER(isPrimary);

            Notification n;
            n.Operation = StoreModificationType::Remove;
            n.Key = key;
            n.Value = TValue();
            n.SequenceNumber = sequenceNumber;

            actualNotifications_->Append(n);
            co_await suspend_never();
        }

        ktl::Awaitable<void> OnRebuiltAsync(__in Data::IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>> & enumerableState) noexcept override
        {
            rebuildCallCount_++;

            typename Sort<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>::ComparisonFunctionType compareFunc;
            compareFunc.Bind(this, &TestDictionaryChangeHandler::KeyValueCompareFunc);
            Sort<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>::QuickSort(true, compareFunc, expectedItemsSPtr_);

            // Need to verify immediately because the enumerator is invalidated after this function ends
            ULONG count = 0;

            while (co_await enumerableState.MoveNextAsync(ktl::CancellationToken::None) != false)
            {

                KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> current = enumerableState.GetCurrent();
                KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> expectedItem = (*expectedItemsSPtr_)[count];
                
                LONG64 currentLSN = current.Value.Key;
                LONG64 expectedLSN = expectedItem.Value.Key;
                TValue currentValue = current.Value.Value;
                TValue expectedValue = expectedItem.Value.Value;
                
                CODING_ERROR_ASSERT(currentLSN == expectedLSN);
                CODING_ERROR_ASSERT(keyComparerSPtr_->Compare(expectedItem.Key, current.Key) == 0);
                CODING_ERROR_ASSERT(valueComparerSPtr_->Compare(expectedValue, currentValue) == 0);

                count++;
            }

            CODING_ERROR_ASSERT(expectedItemsSPtr_->Count() == count);
            co_return;
        }

    public:
        void AddToExpected(
            __in StoreModificationType::Enum operation,
            __in TKey key,
            __in TValue value,
            __in LONG64 sequenceNumber)
        {
            Notification n;
            n.Operation = operation;
            n.Key = key;
            n.Value = value;
            n.SequenceNumber = sequenceNumber;

            expectedNotifications_->Append(n);
        }

        void AddToExpectedRebuild(__in TKey key, __in TValue value, __in LONG64 sequenceNumber)
        {
            KeyValuePair<LONG64, TValue> item(sequenceNumber, value);
            KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> pair(key, item);
            expectedItemsSPtr_->Append(pair);
        }

        byte RebuildCallCount()
        {
            return rebuildCallCount_;
        }

        void ResetRebuildCallCount()
        {
            rebuildCallCount_ = 0;
        }

        void Validate(__in_opt bool testSequenceNumbers = true)
        {
            Validate(*expectedNotifications_, *actualNotifications_, testSequenceNumbers);
        }

    private:
        class Notification
        {
        public:
            StoreModificationType::Enum Operation;
            TKey Key;
            TValue Value;
            LONG64 SequenceNumber;
        };

    private:
        LONG32 KeyValueCompareFunc(
            __in KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> const & itemOne, 
            __in KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> const & itemTwo)
        {
            return keyComparerSPtr_->Compare(itemOne.Key, itemTwo.Key);
        }

        void Validate(
            __in KSharedArray<Notification> & expected,
            __in KSharedArray<Notification> & actual,
            __in_opt bool testSequenceNumbers = true)
        {
            CODING_ERROR_ASSERT(actual.Count() == expected.Count());

            for (ULONG32 i = 0; i < expected.Count(); i++)
            {
                auto expectedNotification = expected[i];
                auto actualNotification = actual[i];
                
                CODING_ERROR_ASSERT(expectedNotification.Operation == actualNotification.Operation);
                CODING_ERROR_ASSERT(!testSequenceNumbers || expectedNotification.SequenceNumber == actualNotification.SequenceNumber);
                CODING_ERROR_ASSERT(keyComparerSPtr_->Compare(expectedNotification.Key, actualNotification.Key) == 0);
                CODING_ERROR_ASSERT(valueComparerSPtr_->Compare(expectedNotification.Value, actualNotification.Value) == 0);
            }
        }

    private:
        TestDictionaryChangeHandler(__in IComparer<TKey> & keyComparer, __in IComparer<TValue> & valueComparer);
        KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
        KSharedPtr<IComparer<TValue>> valueComparerSPtr_;
        
        KSharedPtr<KSharedArray<Notification>> expectedNotifications_;
        KSharedPtr<KSharedArray<Notification>> actualNotifications_;

        KSharedPtr<KSharedArray<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>> expectedItemsSPtr_;
        byte rebuildCallCount_ = 0;
    };
    
    template<typename TKey, typename TValue>
    TestDictionaryChangeHandler<TKey, TValue>::TestDictionaryChangeHandler(__in IComparer<TKey> & keyComparer, __in IComparer<TValue> & valueComparer) :
        keyComparerSPtr_(&keyComparer),
        valueComparerSPtr_(&valueComparer)
    {
        expectedNotifications_ = _new(TEST_DICTIONARYCHANGEHANDLER_TAG, this->GetThisAllocator()) KSharedArray<Notification>();
        actualNotifications_ = _new(TEST_DICTIONARYCHANGEHANDLER_TAG, this->GetThisAllocator()) KSharedArray<Notification>();
        expectedItemsSPtr_ = _new(TEST_DICTIONARYCHANGEHANDLER_TAG, this->GetThisAllocator()) KSharedArray<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>();
    }

    template<typename TKey, typename TValue>
    TestDictionaryChangeHandler<TKey, TValue>::~TestDictionaryChangeHandler()
    {
    }
}
