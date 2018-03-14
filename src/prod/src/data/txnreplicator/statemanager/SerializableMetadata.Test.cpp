// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace StateManagerTests
{
    using namespace std;
    using namespace Common;
    using namespace ktl;
    using namespace Data::StateManager;
    using namespace Data::Utilities;

    class SerializableMetadataTest
    {
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    public:
        void TestCreateAndGetter (__in bool isInitParameterNull)
        {
            NTSTATUS status;

            KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
            KFinally([&]() { KtlSystem::Shutdown(); });
            {
                KAllocator& allocator = underlyingSystem->NonPagedAllocator();

                // Expected
                KUriView expectedNameView = L"fabric:/sps/sp";
                KUri::CSPtr expectedNameCSPtr;
                status = KUri::Create(expectedNameView, allocator, expectedNameCSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                KStringView expectedTypeStringView = TestStateProvider::TypeName;
                KString::SPtr expectedTypeCSPtr;
                status = KString::Create(expectedTypeCSPtr, allocator, expectedTypeStringView);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                OperationData::SPtr initParameters = isInitParameterNull ? nullptr : TestHelper::CreateInitParameters(allocator);

                LONG64 expectedStateProviderId = 6;
                LONG64 expectedParentId = 16;
                LONG64 expectedCreateLSN = 19;
                LONG64 expectedDeleteLSN = 87;

                MetadataMode::Enum expectedMetadataMode = MetadataMode::Enum::Active;

                // Setup
                SerializableMetadata::CSPtr serializablemetadataSPtr = nullptr;
                status = SerializableMetadata::Create(*expectedNameCSPtr, *expectedTypeCSPtr, expectedStateProviderId, expectedParentId, expectedCreateLSN, expectedDeleteLSN, expectedMetadataMode, allocator, initParameters.RawPtr(), serializablemetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                // Test
                CODING_ERROR_ASSERT(expectedNameView.Get(KUriView::eRaw) == serializablemetadataSPtr->Name->Get(KUriView::eRaw));
                CODING_ERROR_ASSERT(expectedTypeStringView.Compare(*serializablemetadataSPtr->TypeString) == 0);

                if(isInitParameterNull)
                {
                    CODING_ERROR_ASSERT(serializablemetadataSPtr->InitializationParameters == nullptr);
                }
                else
                {
                    TestHelper::VerifyInitParameters(*serializablemetadataSPtr->InitializationParameters);
                }

                CODING_ERROR_ASSERT(expectedStateProviderId == serializablemetadataSPtr->StateProviderId);
                CODING_ERROR_ASSERT(expectedCreateLSN == serializablemetadataSPtr->CreateLSN);
                CODING_ERROR_ASSERT(expectedDeleteLSN == serializablemetadataSPtr->DeleteLSN);
                CODING_ERROR_ASSERT(expectedMetadataMode == serializablemetadataSPtr->MetadataMode);
            }
        }
    };

    BOOST_FIXTURE_TEST_SUITE(SerializableMetadataTestSuite, SerializableMetadataTest)

    // Create with null initialization parameter, Get call should return SerializableMetadata with null initialization parameter
    BOOST_AUTO_TEST_CASE(CreateAndGetter_InitParameterNull_SUCCESS)
    {
        TestCreateAndGetter(true);
    }

    // Create with initialization parameter, Get call should return SerializableMetadata with the same initialization parameter
    BOOST_AUTO_TEST_CASE(CreateAndGetter_WithInitParameter_SUCCESS)
    {
        TestCreateAndGetter(false);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
