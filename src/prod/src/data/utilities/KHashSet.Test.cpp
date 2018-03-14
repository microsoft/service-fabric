// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class KHashSetTests
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        KHashSetTests()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~KHashSetTests()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        KUri::CSPtr GenerateName()
        {
            KString::SPtr rootString = nullptr;;
            NTSTATUS status = KString::Create(rootString, GetAllocator(), L"fabric:/parent/");
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KGuid guid;
            guid.CreateNew();
            KLocalString<40> stateProviderId;
            BOOLEAN isSuccess = stateProviderId.FromGUID(static_cast<GUID>(guid));
            CODING_ERROR_ASSERT(isSuccess == TRUE);

            isSuccess = rootString->Concat(stateProviderId);
            CODING_ERROR_ASSERT(isSuccess == TRUE);

            KAllocator& allocator = ktlSystem_->NonPagedAllocator();
            KUri::SPtr output = nullptr;
            status = KUri::Create(*rootString, allocator, output);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return output.RawPtr();
        }

    private:
        KtlSystem* ktlSystem_;
        Common::Random random_;
    };

    BOOLEAN Equals(const KUri::CSPtr & keyOne, const KUri::CSPtr & keyTwo)
    {
        return *keyOne == *keyTwo ? TRUE : FALSE;
    }

    BOOST_FIXTURE_TEST_SUITE(KHashSetTestSuite, KHashSetTests)

    BOOST_AUTO_TEST_CASE(ContainsKey_Empty_False)
    {
        NTSTATUS status;
        KAllocator& allocator = GetAllocator();

        KUri::CSPtr testName;
        status = KUri::Create(L"fabric:/test", allocator, testName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KHashSet<KUri::CSPtr>::SPtr hashSetSPtr = nullptr;
        status = KHashSet<KUri::CSPtr>::Create(32, K_DefaultHashFunction, Equals, allocator, hashSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool isContained = hashSetSPtr->ContainsKey(testName);
        VERIFY_IS_FALSE(isContained);

        VERIFY_IS_TRUE(hashSetSPtr->Count() == 0);
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_NotEmpty_True)
    {
        NTSTATUS status;
        KAllocator& allocator = GetAllocator();

        KUri::CSPtr testName;
        status = KUri::Create(L"fabric:/test", allocator, testName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KHashSet<KUri::CSPtr>::SPtr hashSetSPtr = nullptr;
        status = KHashSet<KUri::CSPtr>::Create(32, K_DefaultHashFunction, Equals, allocator, hashSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool isAdded = hashSetSPtr->TryAdd(testName);
        VERIFY_IS_TRUE(isAdded);

        bool isContained = hashSetSPtr->ContainsKey(testName);
        VERIFY_IS_TRUE(isContained);

        VERIFY_IS_TRUE(hashSetSPtr->Count() == 1);
    }

    BOOST_AUTO_TEST_CASE(Add_AlreadyExists_DuplicateRejected)
    {
        NTSTATUS status;
        KAllocator& allocator = GetAllocator();

        KUri::CSPtr testName1;
        status = KUri::Create(L"fabric:/test", allocator, testName1);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KUri::CSPtr testName2;
        status = KUri::Create(L"fabric:/test", allocator, testName2);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KHashSet<KUri::CSPtr>::SPtr hashSetSPtr = nullptr;
        status = KHashSet<KUri::CSPtr>::Create(32, K_DefaultHashFunction, Equals, allocator, hashSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        bool isAdded = hashSetSPtr->TryAdd(testName1);
        VERIFY_IS_TRUE(isAdded);

        isAdded = hashSetSPtr->TryAdd(testName2);
        VERIFY_IS_FALSE(isAdded);

        VERIFY_IS_TRUE(hashSetSPtr->Count() == 1);
    }

    BOOST_AUTO_TEST_CASE(Enumerator_Empty_Empty)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KAllocator& allocator = GetAllocator();

        KHashSet<LONG64>::SPtr hashSetSPtr = nullptr;
        status = KHashSet<LONG64>::Create(32, K_DefaultHashFunction, allocator, hashSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Data::IEnumerator<LONG64>::SPtr enumerator = hashSetSPtr->GetEnumerator();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        VERIFY_IS_FALSE(enumerator->MoveNext());
        VERIFY_IS_FALSE(enumerator->MoveNext());
    }

    BOOST_AUTO_TEST_CASE(Enumerator_NotEmpty_AllItemsReturned)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KAllocator& allocator = GetAllocator();

        // Expected Count
        const LONG64 expectedCount = 1024;

        KHashSet<LONG64>::SPtr hashSetSPtr = nullptr;
        status = KHashSet<LONG64>::Create(32, K_DefaultHashFunction, allocator, hashSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KHashSet<LONG64>::SPtr testSetSPtr = nullptr;
        status = KHashSet<LONG64>::Create(32, K_DefaultHashFunction, allocator, testSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (LONG64 i = 0; i < expectedCount; i++)
        {
            bool isAdded = hashSetSPtr->TryAdd(i);
            CODING_ERROR_ASSERT(isAdded);
        }

        Data::IEnumerator<LONG64>::SPtr enumerator = hashSetSPtr->GetEnumerator();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        while (enumerator->MoveNext())
        {
            bool isAdded = testSetSPtr->TryAdd(enumerator->Current());
            CODING_ERROR_ASSERT(isAdded);
        }

        VERIFY_IS_FALSE(enumerator->MoveNext());
        VERIFY_IS_TRUE(testSetSPtr->Equals(*hashSetSPtr));
    }

    BOOST_AUTO_TEST_CASE(Enumerator_KUriCSPtr_NotEmpty_AllItemsReturned)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KAllocator& allocator = GetAllocator();

        // Expected Count
        const LONG64 expectedCount = 1024;

        KHashSet<KUri::CSPtr>::SPtr hashSetSPtr = nullptr;
        status = KHashSet<KUri::CSPtr>::Create(32, K_DefaultHashFunction, Equals, allocator, hashSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KHashSet<KUri::CSPtr>::SPtr testSetSPtr = nullptr;
        status = KHashSet<KUri::CSPtr>::Create(32, K_DefaultHashFunction, Equals, allocator, testSetSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (LONG64 i = 0; i < expectedCount; i++)
        {
            KUri::CSPtr testName = GenerateName();
            bool isAdded = hashSetSPtr->TryAdd(testName);
            CODING_ERROR_ASSERT(isAdded);
        }

        Data::IEnumerator<KUri::CSPtr>::SPtr enumerator = hashSetSPtr->GetEnumerator();
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        while (enumerator->MoveNext())
        {
            bool isAdded = testSetSPtr->TryAdd(enumerator->Current());
            CODING_ERROR_ASSERT(isAdded);
        }

        VERIFY_IS_FALSE(enumerator->MoveNext());
        VERIFY_IS_TRUE(testSetSPtr->Equals(*hashSetSPtr));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
