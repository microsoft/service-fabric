// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace StateManagerTests
{
    using namespace Data::StateManager;
    using namespace Data::Utilities;

    class MetadataTest
    {
        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(StateManagerTestSuite, MetadataTest)

    BOOST_AUTO_TEST_CASE(Create_ExplicitCreateAndDeleteLSN_SUCCESS)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUriView expectedName(L"fabric:/sps/sp");
            KUri::CSPtr expectedNameCSPtr;
            status = KUri::Create(expectedName, allocator, expectedNameCSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KStringView expectedTypeString(TestStateProvider::TypeName);
            KString::SPtr expectedTypeCSPtr;
            status = KString::Create(expectedTypeCSPtr, allocator, expectedTypeString);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            OperationData::SPtr initParameters = TestHelper::CreateInitParameters(allocator);

            KGuid partitionId;
            partitionId.CreateNew();
            LONG64 replicaId = 0;
            LONG64 expectedStateProviderId = 6;
            LONG64 expectedParentId = 16;
            LONG64 expectedCreateLSN = 19;
            LONG64 expectedDeleteLSN = 87;

            FactoryArguments factoryArguments(*expectedNameCSPtr, expectedStateProviderId, *expectedTypeCSPtr, partitionId, replicaId, initParameters.RawPtr());

            TestStateProvider::SPtr expectedStateProvider2 = nullptr;
            status = TestStateProvider::Create(
                factoryArguments,
                allocator, 
                FaultStateProviderType::None,
                expectedStateProvider2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            MetadataMode::Enum expectedMetadataMode = MetadataMode::Enum::Active;
            bool expectedTransientCreate = true;

            // Setup
            Metadata::SPtr metadataSPtr = nullptr;
            status = Metadata::Create(
                *expectedNameCSPtr, 
                *expectedTypeCSPtr, 
                *expectedStateProvider2, 
                initParameters.RawPtr(),
                expectedStateProviderId, 
                expectedParentId, 
                expectedCreateLSN, 
                expectedDeleteLSN, 
                expectedMetadataMode, 
                expectedTransientCreate, 
                allocator, 
                metadataSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Test
            VERIFY_IS_TRUE(metadataSPtr->Name->Compare(expectedName));
            VERIFY_IS_TRUE(expectedTypeString.Compare(*metadataSPtr->TypeString) == 0);

            VERIFY_IS_NOT_NULL(metadataSPtr->StateProvider.RawPtr());
            TestHelper::VerifyInitParameters(*metadataSPtr->InitializationParameters);
            VERIFY_ARE_EQUAL2(expectedStateProviderId, metadataSPtr->StateProviderId);
            VERIFY_ARE_EQUAL2(expectedParentId, metadataSPtr->ParentId);
            VERIFY_ARE_EQUAL2(expectedCreateLSN, metadataSPtr->CreateLSN);
            VERIFY_ARE_EQUAL2(expectedDeleteLSN, metadataSPtr->DeleteLSN);

            VERIFY_ARE_EQUAL(expectedMetadataMode, metadataSPtr->MetadataMode);
            VERIFY_ARE_EQUAL2(expectedTransientCreate, metadataSPtr->TransientCreate);
            VERIFY_IS_FALSE(metadataSPtr->TransientDelete);
        }
    }

    BOOST_AUTO_TEST_CASE(CreateAndGetter_DefaultCreateAndDeleteLSN_SUCCESS)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUriView expectedName(L"fabric:/sps/sp");
            KUri::CSPtr expectedNameCSPtr;
            status = KUri::Create(expectedName, allocator, expectedNameCSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KStringView expectedTypeString(TestStateProvider::TypeName);
            KString::SPtr expectedTypeCSPtr;
            status = KString::Create(expectedTypeCSPtr, allocator, expectedTypeString);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            OperationData const * const expectedBuffer = nullptr;

            KGuid partitionId;
            partitionId.CreateNew();
            LONG64 replicaId = 0;
            LONG64 expectedStateProviderId = 6;
            LONG64 expectedParentId = 16;
            LONG64 expectedCreateLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
            LONG64 expectedDeleteLSN = FABRIC_INVALID_SEQUENCE_NUMBER;

            FactoryArguments factoryArguments(*expectedNameCSPtr, expectedStateProviderId, *expectedTypeCSPtr, partitionId, replicaId, expectedBuffer);

            TestStateProvider::SPtr expectedStateProvider2 = nullptr;
            status = TestStateProvider::Create(
                factoryArguments,
                allocator, 
                FaultStateProviderType::None,
                expectedStateProvider2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            MetadataMode::Enum expectedMetadataMode = MetadataMode::Enum::Active;
            bool expectedTransientCreate = true;

            // Setup
            Metadata::SPtr metadataSPtr = nullptr;
            status = Metadata::Create(
                *expectedNameCSPtr,
                *expectedTypeCSPtr,
                *expectedStateProvider2,
                expectedBuffer,
                expectedStateProviderId,
                expectedParentId,
                expectedMetadataMode,
                expectedTransientCreate,
                allocator,
                metadataSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Test
            VERIFY_IS_TRUE(metadataSPtr->Name->Compare(expectedName));
            VERIFY_IS_TRUE(expectedTypeString.Compare(*metadataSPtr->TypeString) == 0);

            VERIFY_IS_NOT_NULL(metadataSPtr->StateProvider.RawPtr());
            VERIFY_IS_NULL(metadataSPtr->InitializationParameters);
            VERIFY_ARE_EQUAL2(expectedStateProviderId, metadataSPtr->StateProviderId);
            VERIFY_ARE_EQUAL2(expectedParentId, metadataSPtr->ParentId);
            VERIFY_ARE_EQUAL2(expectedCreateLSN, metadataSPtr->CreateLSN);
            VERIFY_ARE_EQUAL2(expectedDeleteLSN, metadataSPtr->DeleteLSN);

            VERIFY_ARE_EQUAL(expectedMetadataMode, metadataSPtr->MetadataMode);
            VERIFY_ARE_EQUAL2(expectedTransientCreate, metadataSPtr->TransientCreate);
            VERIFY_IS_FALSE(metadataSPtr->TransientDelete);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
