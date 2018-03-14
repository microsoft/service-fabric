// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

namespace StateManagerTests
{
    class MetadataOperationDataTest
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;
    public:
        VOID Test_Serialization_Success(__in KAllocator & allocator);
    };

    VOID MetadataOperationDataTest::Test_Serialization_Success(__in KAllocator & allocator)
    {
        // Inputs and Expected Results
        const LONG64 expectedStateProviderId = 17;
        KStringView expectedNameString(L"fabric:/parent/child");
        KUri::SPtr expectedStateProviderName;
        KUri::Create(expectedNameString, allocator, expectedStateProviderName);
        const ApplyType::Enum expectedApplyType = ApplyType::Insert;

        // Setup
        MetadataOperationData::CSPtr metadataOperationDataSPtr = TestHelper::CreateMetadataOperationData(
            expectedStateProviderId,
            *expectedStateProviderName,
            ApplyType::Insert,
            allocator);

        // Test Input
        VERIFY_ARE_EQUAL2(expectedStateProviderId, metadataOperationDataSPtr->StateProviderId);
        VERIFY_ARE_EQUAL(*expectedStateProviderName, *metadataOperationDataSPtr->StateProviderName);
        VERIFY_ARE_EQUAL(expectedApplyType, metadataOperationDataSPtr->ApplyType);

        // Serialization and Deserialization
        OperationData const * replicatorOperationDataPtr = static_cast<OperationData const *>(metadataOperationDataSPtr.RawPtr());
        MetadataOperationData::CSPtr outputOperationDataSPtr = TestHelper::CreateMetadataOperationData(allocator, replicatorOperationDataPtr);

        // Test Output
        VERIFY_ARE_EQUAL2(expectedStateProviderId, outputOperationDataSPtr->StateProviderId);
        VERIFY_ARE_EQUAL(*expectedStateProviderName, *outputOperationDataSPtr->StateProviderName);
        VERIFY_ARE_EQUAL(expectedApplyType, outputOperationDataSPtr->ApplyType);
    }

    BOOST_FIXTURE_TEST_SUITE(MetadataOperationDataTestSuite, MetadataOperationDataTest)

    BOOST_AUTO_TEST_CASE(Serialization_NullUserOperationData_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            Test_Serialization_Success(allocator);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
