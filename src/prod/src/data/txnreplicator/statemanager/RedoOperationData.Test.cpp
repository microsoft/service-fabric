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
    class RedoOperationDataTest
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;
    public:
        void Test_Serialization_Success(
            __in KAllocator & allocator, 
            __in_opt OperationData const * const initializationParameters, 
            __in_opt SerializationMode::Enum mode = SerializationMode::Enum::Native);
    };

    void RedoOperationDataTest::Test_Serialization_Success(
        __in KAllocator & allocator, 
        __in_opt OperationData const * const initializationParameters,
        __in_opt SerializationMode::Enum mode)
    {
        NTSTATUS status;

        // Inputs and Expected Results
        KStringView expectedTypeName(L"TestStateProvider");
        KString::SPtr expectedTypeNameSPtr;
        status = KString::Create(expectedTypeNameSPtr, allocator, expectedTypeName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        const LONG64 expectedParentId = 17;

        // Setup
        RedoOperationData::CSPtr redoOperationDataSPtr = TestHelper::CreateRedoOperationData(
            *TestHelper::CreatePartitionedReplicaId(allocator),
            *expectedTypeNameSPtr,
            initializationParameters,
            expectedParentId,
            allocator,
            mode);

        // Test Input
        CODING_ERROR_ASSERT(expectedTypeName.Compare(*redoOperationDataSPtr->TypeName) == 0);
        CODING_ERROR_ASSERT(expectedParentId == redoOperationDataSPtr->ParentId);
        VERIFY_IS_TRUE(TestHelper::Equals(initializationParameters, redoOperationDataSPtr->InitializationParameters.RawPtr()));

        // Serialization and Deserialization
        OperationData const * replicatorOperationDataPtr = static_cast<OperationData const *>(redoOperationDataSPtr.RawPtr());
        RedoOperationData::CSPtr outputOperationDataSPtr = TestHelper::CreateRedoOperationData(
            *TestHelper::CreatePartitionedReplicaId(allocator),
            allocator, 
            replicatorOperationDataPtr);

        // Test Output
        CODING_ERROR_ASSERT(expectedTypeName.Compare(*outputOperationDataSPtr->TypeName) == 0);
        CODING_ERROR_ASSERT(expectedParentId == outputOperationDataSPtr->ParentId);
        VERIFY_IS_TRUE(TestHelper::Equals(initializationParameters, outputOperationDataSPtr->InitializationParameters.RawPtr()));
    }
    
    BOOST_FIXTURE_TEST_SUITE(RedoOperationDataTestSuite, RedoOperationDataTest)

    // Test with InitParams is null
    BOOST_AUTO_TEST_CASE(Serialization_NullInitParams_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            Test_Serialization_Success(allocator, nullptr);
        }
    }

    // Test with InitParams is not null but has 0 item
    BOOST_AUTO_TEST_CASE(Serialization_EmptyInitParams_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr initParameters = TestHelper::CreateInitParameters(allocator, 0);

            Test_Serialization_Success(allocator, initParameters.RawPtr());
        }
    }

    // Test with InitParams has 5 buffers, each buffer size is 16
    BOOST_AUTO_TEST_CASE(Serialization_NotEmptyInitParams_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            ULONG32 const bufferCount = 5;
            ULONG32 const bufferSize = 16;

            OperationData::SPtr initParameters = TestHelper::CreateInitParameters(allocator, bufferCount, bufferSize);

            Test_Serialization_Success(allocator, initParameters.RawPtr());
        }
    }

    // Test Managed RedoOperationData with InitParams is null
    BOOST_AUTO_TEST_CASE(Serialization_NullInitParams_Managed_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            Test_Serialization_Success(allocator, nullptr, SerializationMode::Managed);
        }
    }

    // Test Managed RedoOperationData with InitParams is not null but has 0 item
    BOOST_AUTO_TEST_CASE(Serialization_EmptyInitParams_Managed_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr initParameters = TestHelper::CreateInitParameters(allocator, 0);

            Test_Serialization_Success(allocator, initParameters.RawPtr(), SerializationMode::Managed);
        }
    }

    // Test Managed RedoOperationData with InitParams has 1 buffer and buffer size is 64
    BOOST_AUTO_TEST_CASE(Serialization_NotEmptyInitParams_Managed_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            ULONG32 const bufferCount = 1;
            ULONG32 const bufferSize = 64;

            OperationData::SPtr initParameters = TestHelper::CreateInitParameters(allocator, bufferCount, bufferSize);

            Test_Serialization_Success(allocator, initParameters.RawPtr(), SerializationMode::Managed);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
