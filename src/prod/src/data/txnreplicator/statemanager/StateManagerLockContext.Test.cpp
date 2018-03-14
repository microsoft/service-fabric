// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#pragma once
#define _CRT_RAND_S

using namespace Common;
using namespace std;
using namespace ktl;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class StateManagerLockContextTest
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;
    };

    BOOST_FIXTURE_TEST_SUITE(StateManagerLockContextTestSuite, StateManagerLockContextTest)

    // Test verifies Create and get Properties.
    BOOST_AUTO_TEST_CASE(LockContext_Create)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            const LONG64 expectedStateProviderId = 17;

            // Inputs and Expected Results
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Setup
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = StateManagerLockContext::Create(*expectedName, expectedStateProviderId, *metadataManagerSPtr, allocator, lockContextSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Test Verification
            VERIFY_IS_NOT_NULL(lockContextSPtr);
            VERIFY_IS_TRUE(expectedName->Compare(*lockContextSPtr->Key));
            VERIFY_ARE_EQUAL2(expectedStateProviderId, lockContextSPtr->StateProviderId);
            VERIFY_ARE_EQUAL2(0, lockContextSPtr->GrantorCount);
        }
    }

    BOOST_AUTO_TEST_CASE(Read_Local_AcquireAndRelease_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            TimeSpan timeout = TimeSpan::FromSeconds(4);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            StateManagerLockContext::SPtr lockContextSPtr = TestHelper::CreateLockContext(*metadataManagerSPtr, allocator);

            // Acquire Lock
            SyncAwait(lockContextSPtr->AcquireReadLockAsync(timeout, CancellationToken::None));

            // Release Lock
            lockContextSPtr->ReleaseLock(::Constants::InvalidTransactionId);

            // Verify
            VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
        }
    }

    BOOST_AUTO_TEST_CASE(Write_Local_AcquireAndRelease_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            TimeSpan timeout = TimeSpan::FromSeconds(4);

            // Expected results
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            StateManagerLockContext::SPtr lockContextSPtr = TestHelper::CreateLockContext(*metadataManagerSPtr, allocator);

            // Acquire Lock
            SyncAwait(lockContextSPtr->AcquireWriteLockAsync(timeout, CancellationToken::None));
            lockContextSPtr->GrantorCount++;
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            // Release Lock
            lockContextSPtr->ReleaseLock(::Constants::InvalidTransactionId);
            VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
        }
    }

    BOOST_AUTO_TEST_CASE(Write_Local_ReentrantTransaction_AcquireAndRelease_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            TimeSpan timeout = TimeSpan::FromSeconds(4);

            // Expected results
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            StateManagerLockContext::SPtr lockContextSPtr = TestHelper::CreateLockContext(*metadataManagerSPtr, allocator);

            // Acquire Lock
            SyncAwait(lockContextSPtr->AcquireWriteLockAsync(timeout, CancellationToken::None));
            lockContextSPtr->GrantorCount++;
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            lockContextSPtr->GrantorCount++;
            VERIFY_ARE_EQUAL(2, lockContextSPtr->GrantorCount);

            // Release Lock
            lockContextSPtr->ReleaseLock(::Constants::InvalidTransactionId);
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            lockContextSPtr->ReleaseLock(::Constants::InvalidTransactionId);
            VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
        }
    }

    BOOST_AUTO_TEST_CASE(Upgrade_ReadLockToWriteLockToRelease_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            TimeSpan timeout = TimeSpan::FromSeconds(4);

            // Expected results
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            StateManagerLockContext::SPtr lockContextSPtr = TestHelper::CreateLockContext(*metadataManagerSPtr, allocator);

            // Acquire Lock
            SyncAwait(lockContextSPtr->AcquireWriteLockAsync(timeout, CancellationToken::None));
            lockContextSPtr->GrantorCount++;
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            // Release Lock
            lockContextSPtr->ReleaseLock(::Constants::InvalidTransactionId);
            VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
        }

        KtlSystem::Shutdown();
    }

    // TODO: Add Dispose tests when ReaderWriterLockAsync integration is in.

    BOOST_AUTO_TEST_SUITE_END()
}
