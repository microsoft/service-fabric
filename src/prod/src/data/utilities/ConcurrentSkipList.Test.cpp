// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../testcommon/TestCommon.Public.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::Utilities;

    class ConcurrentSkipListTest
    {
    public:
        template <typename T>
        class Comparer 
            : public KObject<DefaultComparer<T>>
            , public KShared<DefaultComparer<T>>
            , public IComparer<T>
        {
            K_FORCE_SHARED(Comparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out typename IComparer<T>::SPtr & result)
            {
                result = _new(KTL_TAG_TEST, allocator) Comparer();
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            int Compare(__in const T& lhs, __in const T& rhs) const override
            {
                return lhs < rhs ? -1 : lhs > rhs ? 1 : 0;
            }
        };

    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        ConcurrentSkipListTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~ConcurrentSkipListTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;
    };

    template <typename T>
    ConcurrentSkipListTest::Comparer<T>::Comparer()
    {
    }

    template <typename T>
    ConcurrentSkipListTest::Comparer<T>::~Comparer()
    {
    }

    template<typename TKey, typename TValue>
    TValue IncrementValue(__in TKey const & key, __in TValue const & val)
    {
        UNREFERENCED_PARAMETER(key);
        return val + 1;
    }

    BOOST_FIXTURE_TEST_SUITE(ConcurrentSkipListTestSuite, ConcurrentSkipListTest)

        BOOST_AUTO_TEST_CASE(Api_AddGetUpdateRemove_Success)
        {
            NTSTATUS status;
            KAllocator& allocator = ConcurrentSkipListTest::GetAllocator();

            // Expected
            ULONG expectedKey = 10;
            ULONG expectedValue = 20;

            ConcurrentSkipList<ULONG, ULONG>::SPtr ConcurrentSkipListSPtr = nullptr;
            IComparer<ULONG>::SPtr comparer;
            status = Comparer<ULONG>::Create(allocator, comparer);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
            status = ConcurrentSkipList<ULONG, ULONG>::Create(comparer, allocator, ConcurrentSkipListSPtr);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

            // TryAdd
            bool addResult = ConcurrentSkipListSPtr->TryAdd(expectedKey, expectedValue);
            CODING_ERROR_ASSERT(addResult == true);

            // Contains
            CODING_ERROR_ASSERT(ConcurrentSkipListSPtr->ContainsKey(expectedKey));

            // TryGetValue
            ULONG outValue = 0;
            bool getResult = ConcurrentSkipListSPtr->TryGetValue(expectedKey, outValue);
            CODING_ERROR_ASSERT(getResult == true);
            CODING_ERROR_ASSERT(outValue == expectedValue);

            // Update with value
            ULONG updatedValue = IncrementValue<ULONG, ULONG>(expectedKey, expectedValue);
            ConcurrentSkipListSPtr->Update(expectedKey, updatedValue);
            getResult = ConcurrentSkipListSPtr->TryGetValue(expectedKey, outValue);
            CODING_ERROR_ASSERT(getResult == true);
            CODING_ERROR_ASSERT(outValue == updatedValue);
            ConcurrentSkipListSPtr->Update(expectedKey, expectedValue);
            getResult = ConcurrentSkipListSPtr->TryGetValue(expectedKey, outValue);
            CODING_ERROR_ASSERT(getResult == true);
            CODING_ERROR_ASSERT(outValue == expectedValue);

            // Update with func
            ConcurrentSkipListSPtr->Update(expectedKey, IncrementValue<ULONG, ULONG>);
            getResult = ConcurrentSkipListSPtr->TryGetValue(expectedKey, outValue);
            CODING_ERROR_ASSERT(getResult == true);
            CODING_ERROR_ASSERT(outValue == updatedValue);

            // Enumerator
            IEnumerator<KeyValuePair<ULONG, ULONG>>::SPtr enumerator;
            status = ConcurrentSkipList<ULONG, ULONG>::Enumerator::Create(ConcurrentSkipListSPtr, enumerator);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);
            CODING_ERROR_ASSERT(enumerator->MoveNext());
            CODING_ERROR_ASSERT(enumerator->Current().Key == expectedKey);
            CODING_ERROR_ASSERT(!enumerator->MoveNext());

            // Remove
            ULONG value;
            bool removeResult = ConcurrentSkipListSPtr->TryRemove(expectedKey, value);
            CODING_ERROR_ASSERT(removeResult == true);
        }

        BOOST_AUTO_TEST_SUITE_END()
}
