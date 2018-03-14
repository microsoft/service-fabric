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
    class NamedOperationDataTest
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;
    public:
        VOID Test_Serialization_Success(
            __in SerializationMode::Enum mode,
            __in KAllocator & allocator,
            __in_opt OperationData const * const userOperationDataPtr);
    };

    VOID NamedOperationDataTest::Test_Serialization_Success(
        __in SerializationMode::Enum mode,
        __in KAllocator & allocator,
        __in_opt OperationData const * const userOperationDataPtr)
    {
        // Inputs and Expected Results
        const LONG64 expectedStateProviderId = 17;

        // Setup
        NamedOperationData::CSPtr namedOperationDataSPtr = TestHelper::CreateNamedOperationData(
            expectedStateProviderId,
            mode,
            allocator,
            userOperationDataPtr);

        // Test Input
        VERIFY_ARE_EQUAL2(expectedStateProviderId, namedOperationDataSPtr->StateProviderId);
        VERIFY_IS_TRUE(TestHelper::Equals(userOperationDataPtr, namedOperationDataSPtr->UserOperationDataCSPtr.RawPtr()));

        // Serialization and Deserialization
        OperationData const * replicatorOperationDataPtr = static_cast<OperationData const *>(namedOperationDataSPtr.RawPtr());
        NamedOperationData::CSPtr outputOperationDataSPtr = TestHelper::CreateNamedOperationData(allocator, replicatorOperationDataPtr);

        // Test Output
        VERIFY_ARE_EQUAL2(expectedStateProviderId, outputOperationDataSPtr->StateProviderId);
        VERIFY_IS_TRUE(TestHelper::Equals(userOperationDataPtr, outputOperationDataSPtr->UserOperationDataCSPtr.RawPtr()));
    }

    BOOST_FIXTURE_TEST_SUITE(NamedOperationDataTestSuite, NamedOperationDataTest)

    BOOST_AUTO_TEST_CASE(Serialization_NullUserOperationData_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData const * userOperationDataPtr = nullptr;

            Test_Serialization_Success(SerializationMode::Native, allocator, userOperationDataPtr);
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_NullUserOperationData_Managed_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData const * userOperationDataPtr = nullptr;

            Test_Serialization_Success(SerializationMode::Managed, allocator, userOperationDataPtr);
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_EmptyUserOperationData_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr userOperationDataSPtr = OperationData::Create(allocator);

            Test_Serialization_Success(SerializationMode::Native, allocator, userOperationDataSPtr.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_EmptyUserOperationData_Managed_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr userOperationDataSPtr = OperationData::Create(allocator);

            Test_Serialization_Success(SerializationMode::Managed, allocator, userOperationDataSPtr.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_OneBufferUserOperationData_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr userOperationDataSPtr = OperationData::Create(allocator);
            KBuffer::SPtr tmpBufferSPtr = nullptr;
            NTSTATUS status = KBuffer::Create(16, tmpBufferSPtr, allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            userOperationDataSPtr->Append(*tmpBufferSPtr);

            Test_Serialization_Success(SerializationMode::Native, allocator, userOperationDataSPtr.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_OneBufferUserOperationData_Managed_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr userOperationDataSPtr = OperationData::Create(allocator);
            KBuffer::SPtr tmpBufferSPtr = nullptr;
            NTSTATUS status = KBuffer::Create(16, tmpBufferSPtr, allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            userOperationDataSPtr->Append(*tmpBufferSPtr);

            Test_Serialization_Success(SerializationMode::Managed, allocator, userOperationDataSPtr.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_MultipleBufferUserOperationData_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr userOperationDataSPtr = OperationData::Create(allocator);
            for (int i = 0; i < 64; i++)
            {
                KBuffer::SPtr tmpBufferSPtr = nullptr;
                NTSTATUS status = KBuffer::Create(16, tmpBufferSPtr, allocator);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                userOperationDataSPtr->Append(*tmpBufferSPtr);
            }

            Test_Serialization_Success(SerializationMode::Native, allocator, userOperationDataSPtr.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(Serialization_MultipleBufferUserOperationData_Native_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            OperationData::SPtr userOperationDataSPtr = OperationData::Create(allocator);
            for (int i = 0; i < 64; i++)
            {
                KBuffer::SPtr tmpBufferSPtr = nullptr;
                NTSTATUS status = KBuffer::Create(16, tmpBufferSPtr, allocator);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                userOperationDataSPtr->Append(*tmpBufferSPtr);
            }

            Test_Serialization_Success(SerializationMode::Managed, allocator, userOperationDataSPtr.RawPtr());
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
