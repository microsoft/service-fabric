// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Common;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;

    StringLiteral const TraceComponent("TransactionMapTest");

    // Contains the single threaded tests for the Version Manager.
    class VersionManagerTest : VersionManagerTestBase
    {
    public:
        Awaitable<void> UnRegister_ItemRegisteredOnce_TaskMustCompleteImmediatelyAfterUnregister();
        Awaitable<void> UnRegister_ItemRegisteredTwice_TaskMustNotCompleteUntilSecondUnRegister();
        Awaitable<void> UnRegister_ItemRegisteredManyTimes_TaskMustNotCompleteUntilLastUnRegister();

        void TryRemoveVersion_NoInflightSnapshots_CanRemove();
        Awaitable<void> TryRemoveVersion_VersionAndNextVersionHigherThanOneExistingSnapshot_CanRemove();
        Awaitable<void> TryRemoveVersion_VersionAndNextVersionHigherThanTwoDuplicateExistingSnapshot_CanRemove();
        Awaitable<void> TryRemoveVersion_VersionAndNextVersionHigherThanMultipleExistingSnapshot_CanRemove();

        Awaitable<void> TryRemoveVersion_NextVersionIsHigherThanOneSnapshot_CannotRemove();
        Awaitable<void> TryRemoveVersion_NextVersionIsHigherThanTwoDuplicateSnapshot_CannotRemove();
        Awaitable<void> TryRemoveVersion_NextVersionIsHigherThanAllMultipleSnapshots_CannotRemove();
        Awaitable<void> TryRemoveVersion_NextVersionIsHigherThanSomeMultipleSnapshots_CannotRemove();

        Awaitable<void> TryRemoveVersion_NextVersionIsLowerThanOneSnapshot_CanRemove();
        Awaitable<void> TryRemoveVersion_NextVersionIsLowerThanTwoDuplicateSnapshots_CanRemove();
        Awaitable<void> TryRemoveVersion_NextVersionIsLowerThanMultipleSnapshots_CanRemove();

        void TryRemoveVersion_Multiple_OneStateProvider_NoInflightSnapshots_CanRemove();
        Awaitable<void> TryRemoveVersion_Multiple_OneStateProvider_SameRange_CannotRemove();
        Awaitable<void> TryRemoveVersion_Multiple_OneStateProvider_OneNotificationAtATime_CannotRemove();
        Awaitable<void> TryRemoveVersion_Multiple_OneStateProvider_LowEndTrain_CannotRemove();

        Awaitable<void> TryRemoveCheckpoint_NoInflightSnapshots_ReturnAlreadyCompleted();
        Awaitable<void> TryRemoveCheckpoint_AllHighSnapshots_ReturnAlreadyCompleted();
        Awaitable<void> TryRemoveCheckpoint_AllLowSnapshots_ReturnAlreadyCompleted();

        Awaitable<void> TryRemoveCheckpoint_OneSnapshot_TaskThatFiresWhen30IsComplete();
        Awaitable<void> TryRemoveCheckpoint_OneSnapshotDuplicate_TaskThatFiresWhenBoth30IsComplete();
    };

    Awaitable<void> VersionManagerTest::UnRegister_ItemRegisteredOnce_TaskMustCompleteImmediatelyAfterUnregister()
    {
        // Expected
        vector<LONG64> set0{ 10 };
        vector<LONG64> notification0{ 10 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        // Register snapshots.
        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        auto result = TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 5, 15, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        auto enumerator = result->EnumerationCompletionNotifications->GetEnumerator();
        while (enumerator->MoveNext())
        {
            EnumerationCompletionResult::SPtr current = enumerator->Current();
            VERIFY_IS_TRUE(current->IsNotificationCompleted);
            Awaitable<LONG64> notificationAwaitable = current->Notification;
            co_await notificationAwaitable;
        }

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::UnRegister_ItemRegisteredTwice_TaskMustNotCompleteUntilSecondUnRegister()
    {
        // Expected
        vector<LONG64> set0{ 10 };
        vector<LONG64> notification0{ 10 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10, 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        // Register snapshots.
        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        auto result = TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 5, 15, *expectedResult);

        versionManagerSPtr->UnRegister(10);

        auto enumerator = result->EnumerationCompletionNotifications->GetEnumerator();
        while (enumerator->MoveNext())
        {
            VERIFY_IS_FALSE(enumerator->Current()->IsNotificationCompleted);
        }

        versionManagerSPtr->UnRegister(10);

        enumerator = result->EnumerationCompletionNotifications->GetEnumerator();
        while (enumerator->MoveNext())
        {
            EnumerationCompletionResult::SPtr current = enumerator->Current();
            VERIFY_IS_TRUE(current->IsNotificationCompleted);
            Awaitable<LONG64> notificationAwaitable = current->Notification;
            co_await notificationAwaitable;
        }

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::UnRegister_ItemRegisteredManyTimes_TaskMustNotCompleteUntilLastUnRegister()
    {
        // Expected
        vector<LONG64> set0{ 10 };
        vector<LONG64> notification0{ 10 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10, 10, 10, 10, 10, 10, 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        // Register snapshots.
        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        auto result = TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 5, 15, *expectedResult);

        for (int i = 0; i < barrierResponses.size() - 1; i++)
        {
            versionManagerSPtr->UnRegister(10);

            auto enumerator = result->EnumerationCompletionNotifications->GetEnumerator();
            while (enumerator->MoveNext())
            {
                VERIFY_IS_FALSE(enumerator->Current()->IsNotificationCompleted);
            }
        }

        versionManagerSPtr->UnRegister(10);

        auto enumerator = result->EnumerationCompletionNotifications->GetEnumerator();
        while (enumerator->MoveNext())
        {
            EnumerationCompletionResult::SPtr current = enumerator->Current();
            VERIFY_IS_TRUE(current->IsNotificationCompleted);
            Awaitable<LONG64> notificationAwaitable = current->Notification;
            co_await notificationAwaitable;
        }

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    void VersionManagerTest::TryRemoveVersion_NoInflightSnapshots_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{};
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        TryRemoveVersionResult::SPtr result = TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 19, 21, *expectedResult);

        VerifyState(*versionManagerSPtr, 1, 0, 0, 0);
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_VersionAndNextVersionHigherThanOneExistingSnapshot_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 11, 19, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_VersionAndNextVersionHigherThanTwoDuplicateExistingSnapshot_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10, 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 11, 19, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_VersionAndNextVersionHigherThanMultipleExistingSnapshot_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 2, 5, 8, 8, 10, 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 11, 19, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsHigherThanOneSnapshot_CannotRemove()
    {
        // Expected
        vector<LONG64> set0{ 10 };
        vector<LONG64> notification0{ 10 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 7, 11, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsHigherThanTwoDuplicateSnapshot_CannotRemove()
    {
        // Expected
        vector<LONG64> set0{ 10 };
        vector<LONG64> notification0{ 10 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10, 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 7, 11, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsHigherThanAllMultipleSnapshots_CannotRemove()
    {
        // Expected
        vector<LONG64> set0{ 30, 40, 50 };
        vector<LONG64> notification0{ 30, 40, 50 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 30, 30, 40, 50 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 25, 55, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsHigherThanSomeMultipleSnapshots_CannotRemove()
    {
        // Expected
        vector<LONG64> set0{ 30, 40, 50 };
        vector<LONG64> notification0{ 30, 40, 50 };
        TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 30, 30, 40, 50, 50, 60, 60, 70, 80 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 25, 55, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsLowerThanOneSnapshot_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 7, 9, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsLowerThanTwoDuplicateSnapshots_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 30, 30, 40, 50, 50, 60, 60, 70, 80 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 7, 9, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_NextVersionIsLowerThanMultipleSnapshots_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 10, 10 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 7, 10, *expectedResult);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    void VersionManagerTest::TryRemoveVersion_Multiple_OneStateProvider_NoInflightSnapshots_CanRemove()
    {
        // Expected
        TryRemoveVersionResult::SPtr expectedResult = nullptr;
        NTSTATUS status = TryRemoveVersionResult::Create(true, GetAllocator(), expectedResult);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{};
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 19, 21, *expectedResult);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 6, 15, *expectedResult);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 17, 1000000, *expectedResult);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 1, 3, *expectedResult);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 7777, 15555555, *expectedResult);

        VerifyState(*versionManagerSPtr, 1, 0, 0, 0);
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_Multiple_OneStateProvider_SameRange_CannotRemove()
    {
        // Expected
        vector<LONG64> set0{ 25, 40, 50 };
        vector<LONG64> notification0{ 25, 40, 50 };
        TryRemoveVersionResult::SPtr expectedResult0 = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        vector<LONG64> set1{ 25, 40, 50 };
        vector<LONG64> notification1{};
        TryRemoveVersionResult::SPtr expectedResult1 = CreateExpectedTryRemoveResult(set1, notification1, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 25, 25, 40, 50 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 25, 55, *expectedResult0);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 25, 55, *expectedResult1);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_Multiple_OneStateProvider_OneNotificationAtATime_CannotRemove()
    {
        // Expected
        vector<LONG64> set0{ 50 };
        vector<LONG64> notification0{ 50 };
        TryRemoveVersionResult::SPtr expectedResult0 = CreateExpectedTryRemoveResult(set0, notification0, GetAllocator());

        vector<LONG64> set1{ 40, 50 };
        vector<LONG64> notification1{ 40 };
        TryRemoveVersionResult::SPtr expectedResult1 = CreateExpectedTryRemoveResult(set1, notification1, GetAllocator());

        vector<LONG64> set2{ 30, 40, 50 };
        vector<LONG64> notification2{ 30 };
        TryRemoveVersionResult::SPtr expectedResult2 = CreateExpectedTryRemoveResult(set2, notification2, GetAllocator());

        // Setup
        LONG64 stateProviderId = GetRandomLONG64();

        vector<LONG64> barrierResponses{ 30, 30, 40, 50 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 45, 55, *expectedResult0);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 35, 55, *expectedResult1);
        TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 25, 55, *expectedResult2);

        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveVersion_Multiple_OneStateProvider_LowEndTrain_CannotRemove()
    {
        // Setup
        LONG64 stateProviderId = GetRandomLONG64();
        LONG64 testNumber = 16;
        LONG64 currentHigh = 25;
        LONG64 vsn;

        vector<LONG64> barrierResponses{};
        for (LONG64 i = 1; i < testNumber + 1; i++)
        {
            barrierResponses.push_back(i * 10);
        }

        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await versionManagerSPtr->RegisterAsync(vsn);
        co_await versionManagerSPtr->RegisterAsync(vsn);
        co_await versionManagerSPtr->RegisterAsync(vsn);

        vector<LONG64> initialNotification{ barrierResponses.front() };
        vector<LONG64> initialSet{ barrierResponses.front() };
        TryRemoveVersionResult::SPtr initialeExpectedResult = CreateExpectedTryRemoveResult(initialNotification, initialSet, GetAllocator());

        auto initialResult = TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 5, 15, *initialeExpectedResult);
        vector<EnumerationCompletionResult::SPtr> tasks{};
        auto enumerator = initialResult->EnumerationCompletionNotifications->GetEnumerator();
        bool isNotEmpty = enumerator->MoveNext();
        CODING_ERROR_ASSERT(isNotEmpty);
        tasks.push_back(enumerator->Current());

        // Test
        for (LONG64 i = 1; i < testNumber - 2; i++)
        {
            vector<LONG64> tmpSet{ barrierResponses[i - 1], barrierResponses[i] };
            vector<LONG64> tmpNotifications{ barrierResponses[i] };
            TryRemoveVersionResult::SPtr expectedResult = CreateExpectedTryRemoveResult(tmpSet, tmpNotifications, GetAllocator());

            auto tmpResult = TestTryRemoveVersion(*versionManagerSPtr, stateProviderId, 5, currentHigh, *expectedResult);

            auto tmpEnumerator = initialResult->EnumerationCompletionNotifications->GetEnumerator();
            bool tmpIsNotEmpty = tmpEnumerator->MoveNext();
            CODING_ERROR_ASSERT(tmpIsNotEmpty);
            tasks.push_back(tmpEnumerator->Current());

            currentHigh += 10;

            versionManagerSPtr->UnRegister(barrierResponses[i - 1]);
            CODING_ERROR_ASSERT(tasks[i - 1]->IsNotificationCompleted);

            Awaitable<LONG64> tmpTask = tasks[i - 1]->Notification;
            LONG64 tmpVsn = co_await tmpTask;
            VERIFY_IS_TRUE(tasks[i - 1]->VisibilitySequenceNumber == tmpVsn);

            co_await versionManagerSPtr->RegisterAsync(vsn);
        }

        versionManagerSPtr->UnRegister(testNumber * 10);
        versionManagerSPtr->UnRegister((testNumber - 1) * 10);
        versionManagerSPtr->UnRegister((testNumber - 2) * 10);

        VERIFY_IS_TRUE(tasks[testNumber - 3]->IsNotificationCompleted);
        Awaitable<LONG64> task = tasks[testNumber - 3]->Notification;
        vsn = co_await task;
        VERIFY_IS_TRUE(tasks[testNumber - 3]->VisibilitySequenceNumber == vsn);

        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveCheckpoint_NoInflightSnapshots_ReturnAlreadyCompleted()
    {
        // Setup
        vector<LONG64> barrierResponses{ };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        // Test
        Awaitable<NTSTATUS> awaitable = versionManagerSPtr->TryRemoveCheckpointAsync(55, 195);

        // Verification
        VERIFY_IS_TRUE(awaitable.IsComplete());
        co_await awaitable;

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveCheckpoint_AllHighSnapshots_ReturnAlreadyCompleted()
    {
        // Setup
        vector<LONG64> barrierResponses{ 30, 30, 40, 50 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        Awaitable<NTSTATUS> awaitable = versionManagerSPtr->TryRemoveCheckpointAsync(15, 25);

        // Verification
        VERIFY_IS_TRUE(awaitable.IsComplete());
        co_await awaitable;

        // Closing
        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);
        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveCheckpoint_AllLowSnapshots_ReturnAlreadyCompleted()
    {
        // Setup
        vector<LONG64> barrierResponses{ 30, 30, 40, 50 };
        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test
        Awaitable<NTSTATUS> awaitable = versionManagerSPtr->TryRemoveCheckpointAsync(55, 75);

        // Verification
        VERIFY_IS_TRUE(awaitable.IsComplete());
        co_await awaitable;

        // Closing
        UnRegisterMultipleSnapshots(*versionManagerSPtr, barrierResponses);
        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveCheckpoint_OneSnapshot_TaskThatFiresWhen30IsComplete()
    {
        // Setup
        vector<LONG64> barrierResponses{ 30, 40, 50 };

        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test & Verification
        Awaitable<NTSTATUS> awaitable = versionManagerSPtr->TryRemoveCheckpointAsync(15, 35);
        VERIFY_IS_FALSE(awaitable.IsComplete());

        versionManagerSPtr->UnRegister(50);
        VERIFY_IS_FALSE(awaitable.IsComplete());
        
        versionManagerSPtr->UnRegister(40);
        VERIFY_IS_FALSE(awaitable.IsComplete());

        versionManagerSPtr->UnRegister(30);
        co_await awaitable;

        // Closing
        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    Awaitable<void> VersionManagerTest::TryRemoveCheckpoint_OneSnapshotDuplicate_TaskThatFiresWhenBoth30IsComplete()
    {
        // Setup
        vector<LONG64> barrierResponses{ 30, 30, 40, 50 };

        TestVersionProvider::SPtr testVersionProvider = TestVersionProvider::Create(barrierResponses, GetAllocator());
        VersionManager::SPtr versionManagerSPtr = VersionManager::Create(GetAllocator());
        versionManagerSPtr->Initialize(*testVersionProvider);

        co_await RegisterMultipleSnapshotsAsync(*versionManagerSPtr, barrierResponses);

        // Test & Verification
        Awaitable<NTSTATUS> awaitable = versionManagerSPtr->TryRemoveCheckpointAsync(15, 35);
        VERIFY_IS_FALSE(awaitable.IsComplete());

        versionManagerSPtr->UnRegister(50);
        VERIFY_IS_FALSE(awaitable.IsComplete());

        versionManagerSPtr->UnRegister(40);
        VERIFY_IS_FALSE(awaitable.IsComplete());

        versionManagerSPtr->UnRegister(30);
        VERIFY_IS_FALSE(awaitable.IsComplete());

        versionManagerSPtr->UnRegister(30);
        co_await awaitable;

        // Closing
        VerifyState(*versionManagerSPtr, DefaultRetryCount, 0, 0, 0);

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(VersionManagerTestSuite, VersionManagerTest)

    BOOST_AUTO_TEST_CASE(VM_UnRegister_ItemRegisteredOnce_TaskMustCompleteImmediatelyAfterUnregister)
    {
        SyncAwait(UnRegister_ItemRegisteredOnce_TaskMustCompleteImmediatelyAfterUnregister());
    }

    BOOST_AUTO_TEST_CASE(VM_UnRegister_ItemRegisteredTwice_TaskMustNotCompleteUntilSecondUnRegister)
    {
        SyncAwait(UnRegister_ItemRegisteredTwice_TaskMustNotCompleteUntilSecondUnRegister());
    }

    BOOST_AUTO_TEST_CASE(VM_UnRegister_ItemRegisteredManyTimes_TaskMustNotCompleteUntilLastUnRegister)
    {
        SyncAwait(UnRegister_ItemRegisteredManyTimes_TaskMustNotCompleteUntilLastUnRegister());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NoInflightSnapshots_CanRemove)
    {
        TryRemoveVersion_NoInflightSnapshots_CanRemove();
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_VersionAndNextVersionHigherThanOneExistingSnapshot_CanRemove)
    {
        SyncAwait(TryRemoveVersion_VersionAndNextVersionHigherThanOneExistingSnapshot_CanRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_VersionAndNextVersionHigherThanTwoDuplicateExistingSnapshot_CanRemove)
    {
        SyncAwait(TryRemoveVersion_VersionAndNextVersionHigherThanTwoDuplicateExistingSnapshot_CanRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_VersionAndNextVersionHigherThanMultipleExistingSnapshot_CanRemove)
    {
        SyncAwait(TryRemoveVersion_VersionAndNextVersionHigherThanMultipleExistingSnapshot_CanRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsHigherThanOneSnapshot_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsHigherThanOneSnapshot_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsHigherThanTwoDuplicateSnapshot_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsHigherThanTwoDuplicateSnapshot_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsHigherThanAllMultipleSnapshots_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsHigherThanAllMultipleSnapshots_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsHigherThanSomeMultipleSnapshots_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsHigherThanSomeMultipleSnapshots_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsLowerThanOneSnapshot_CanRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsLowerThanOneSnapshot_CanRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsLowerThanTwoDuplicateSnapshots_CanRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsLowerThanTwoDuplicateSnapshots_CanRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_NextVersionIsLowerThanMultipleSnapshots_CanRemove)
    {
        SyncAwait(TryRemoveVersion_NextVersionIsLowerThanMultipleSnapshots_CanRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_Multiple_OneStateProvider_NoInflightSnapshots_CanRemove)
    {
        TryRemoveVersion_Multiple_OneStateProvider_NoInflightSnapshots_CanRemove();
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_Multiple_OneStateProvider_SameRange_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_Multiple_OneStateProvider_SameRange_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_Multiple_OneStateProvider_OneNotificationAtATime_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_Multiple_OneStateProvider_OneNotificationAtATime_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveVersion_Multiple_OneStateProvider_LowEndTrain_CannotRemove)
    {
        SyncAwait(TryRemoveVersion_Multiple_OneStateProvider_LowEndTrain_CannotRemove());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveCheckpoint_NoInflightSnapshots_ReturnAlreadyCompleted)
    {
        SyncAwait(TryRemoveCheckpoint_NoInflightSnapshots_ReturnAlreadyCompleted());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveCheckpoint_AllHighSnapshots_ReturnAlreadyCompleted)
    {
        SyncAwait(TryRemoveCheckpoint_AllHighSnapshots_ReturnAlreadyCompleted());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveCheckpoint_AllLowSnapshots_ReturnAlreadyCompleted)
    {
        SyncAwait(TryRemoveCheckpoint_AllLowSnapshots_ReturnAlreadyCompleted());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveCheckpoint_OneSnapshot_TaskThatFiresWhen30IsComplete)
    {
        SyncAwait(TryRemoveCheckpoint_OneSnapshot_TaskThatFiresWhen30IsComplete());
    }

    BOOST_AUTO_TEST_CASE(VM_TryRemoveCheckpoint_OneSnapshotDuplicate_TaskThatFiresWhenBoth30IsComplete)
    {
        SyncAwait(TryRemoveCheckpoint_OneSnapshotDuplicate_TaskThatFiresWhenBoth30IsComplete());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
