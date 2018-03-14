// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;
using namespace ktl;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class StateManagerTransactionContextTest
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;
    };

    BOOST_FIXTURE_TEST_SUITE(StateManagerTransactionContextTestSuite, StateManagerTransactionContextTest)

    // Test verifies Create and get Properties.
    BOOST_AUTO_TEST_CASE(TransactionContext_Create)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Inputs and Expected Results
            const LONG64 expectedTransactionId = 17;

            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            StateManagerLockContext::SPtr lockContextSPtr = TestHelper::CreateLockContext(*metadataManagerSPtr, allocator);

            StateManagerTransactionContext::SPtr transactionContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(expectedTransactionId, *lockContextSPtr, OperationType::Enum::Read, allocator, transactionContextSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Test Verification
            VERIFY_IS_NOT_NULL(transactionContextSPtr);
            VERIFY_ARE_EQUAL2(expectedTransactionId, transactionContextSPtr->TransactionId);
            VERIFY_ARE_EQUAL(lockContextSPtr->StateProviderId, transactionContextSPtr->LockContext->StateProviderId);
            VERIFY_ARE_EQUAL(OperationType::Enum::Read, transactionContextSPtr->OperationType);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
