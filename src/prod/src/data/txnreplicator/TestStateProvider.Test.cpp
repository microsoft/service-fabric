// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TransactionalReplicatorTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data;
    using namespace TxnReplicator;
    using namespace TxnReplicator::TestCommon;
    using namespace Data::Utilities;

    class TestStateProviderTests
    {
        CommonConfig commonConfig; // load the config object as its needed for the tracing to work

    public:
        void Test_UndoOperationData(
            __in KString const & value,
            __in FABRIC_SEQUENCE_NUMBER version,
            __in KAllocator & allocator);

    private:
        KtlSystem * underlyingSystem_;

    private:
        KtlSystem * CreateKtlSystem()
        {
            KtlSystem* underlyingSystem;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            underlyingSystem->SetStrictAllocationChecks(TRUE);

            return underlyingSystem;
        }
    };

    void TestStateProviderTests::Test_UndoOperationData(
        __in KString const & value,
        __in FABRIC_SEQUENCE_NUMBER version,
        __in KAllocator & allocator)
    {
        NTSTATUS status;

        KGuid partitionId; 
        partitionId.CreateNew();

        PartitionedReplicaId::SPtr pid = PartitionedReplicaId::Create(partitionId, 7, allocator);

        // Serialize the undoperationdata.
        TestStateProvider::UndoOperationData::CSPtr undoOperationData;
        status = TestStateProvider::UndoOperationData::Create(*pid, value, version, allocator, undoOperationData);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Deserialize
        OperationData const * operationData = dynamic_cast<OperationData const *>(undoOperationData.RawPtr());

        TestStateProvider::UndoOperationData::CSPtr outputOperationData;
        status = TestStateProvider::UndoOperationData::Create(*pid, operationData, allocator, outputOperationData);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(outputOperationData->Value->Compare(value) == 0);
        VERIFY_ARE_EQUAL(version, outputOperationData->Version);
    }

    BOOST_FIXTURE_TEST_SUITE(TestStateProviderTestSuite, TestStateProviderTests)

    BOOST_AUTO_TEST_CASE(Undo_Empty_0_SUCCess)
    {
        KtlSystem* underlyingSystem;
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem->SetStrictAllocationChecks(TRUE);

        KFinally([&] { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KString::SPtr inputString;
            status = KString::Create(inputString, allocator, L"");
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            Test_UndoOperationData(*inputString, 0, allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(Undo_NotEmoty_10000_SUCCess)
    {
        KtlSystem* underlyingSystem;
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem->SetStrictAllocationChecks(TRUE);

        KFinally([&] { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KString::SPtr inputString;
            status = KString::Create(
                inputString, 
                allocator, 
                L"I am a happy undo operation data. I am mischievous and proud of it");
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            Test_UndoOperationData(*inputString, 10000, allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(Undo_Large_MAX_SUCCess)
    {
        KtlSystem* underlyingSystem;
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        underlyingSystem->SetStrictAllocationChecks(TRUE);

        KFinally([&] { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KString::SPtr inputString;
            status = KString::Create(
                inputString,
                allocator,
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will." // 113 chars.
                L"One who pretends to know, never will. One who pretends to know, never will. One who pretends to know, never will."); // *39.
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            Test_UndoOperationData(*inputString, LONG64_MAX, allocator);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
