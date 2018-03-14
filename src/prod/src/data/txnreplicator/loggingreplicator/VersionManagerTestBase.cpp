// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

ULONG VersionManagerTestBase::HashFunction(
    __in const TxnReplicator::EnumerationCompletionResult::SPtr & Key)
{
    return Key->GetHashCode();
}

BOOLEAN VersionManagerTestBase::Equals(
    __in TxnReplicator::EnumerationCompletionResult::SPtr const & one,
    __in TxnReplicator::EnumerationCompletionResult::SPtr const & two)
{
    return *one == *two ? TRUE : FALSE;
}

LONG64 VersionManagerTestBase::get_BeforeUnRegisterFlag() const
{
    return beforeUnRegisterFlag_;
}

LONG64 VersionManagerTestBase::get_AfterUnRegisterFlag() const
{
    return afterUnRegisterFlag_;
}

TxnReplicator::TryRemoveVersionResult::SPtr VersionManagerTestBase::CreateExpectedTryRemoveResult(
    __in std::vector<LONG64>& set,
    __in std::vector<LONG64>& tasks,
    __in KAllocator & allocator)
{
    NTSTATUS status;

    Data::Utilities::KHashSet<LONG64>::SPtr enumerationSet = nullptr;

    if (set.size() > 0)
    {
        status = Data::Utilities::KHashSet<LONG64>::Create(
            static_cast<ULONG>(set.size()),
            K_DefaultHashFunction,
            allocator,
            enumerationSet);
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to allocate KHashSet for enumerationSet");

        for (auto item : set)
        {
            enumerationSet->TryAdd(item);
        }
    }

    Data::Utilities::KHashSet<TxnReplicator::EnumerationCompletionResult::SPtr>::SPtr notificationSet = nullptr;

    if (tasks.size() > 0)
    {
        status = Data::Utilities::KHashSet<TxnReplicator::EnumerationCompletionResult::SPtr>::Create(
            static_cast<ULONG>(set.size()),
            HashFunction,
            Equals,
            allocator,
            notificationSet);
        ASSERT_IFNOT(NT_SUCCESS(status), "Failed to allocate KHashSet for notificationSet");

        for (auto item : tasks)
        {
            ktl::AwaitableCompletionSource<LONG64>::SPtr acs = nullptr;
            status = ktl::AwaitableCompletionSource<LONG64>::Create(GetAllocator(), KTL_TAG_TEST, acs);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to allocate AwaitableCompletionSource for enumerationCompletionResult");
            acs->SetResult(0);

            TxnReplicator::EnumerationCompletionResult::SPtr enumerationCompletionResultSPtr = nullptr;
            status = TxnReplicator::EnumerationCompletionResult::Create(item, *acs, GetAllocator(), enumerationCompletionResultSPtr);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to allocate EnumerationCompletionResult");

            notificationSet->TryAdd(enumerationCompletionResultSPtr);
        }
    }

    TxnReplicator::TryRemoveVersionResult::SPtr result = nullptr;
    TxnReplicator::TryRemoveVersionResult::Create(false, enumerationSet.RawPtr(), notificationSet.RawPtr(), allocator, result);
    return result;
}

ktl::Awaitable<void> VersionManagerTestBase::RegisterMultipleSnapshotsAsync(
    __in Data::LoggingReplicator::VersionManager& versionManager, 
    __in std::vector<LONG64>& barrierResponses)
{
    for (LONG64 barrierResponse : barrierResponses)
    {
        LONG64 visibilityNumber;
        co_await versionManager.RegisterAsync(visibilityNumber);
        ASSERT_IFNOT(visibilityNumber == barrierResponse, "Test bug. Incorrect visibility sequence number.");
    }

    co_return;
}

void VersionManagerTestBase::UnRegisterMultipleSnapshots(
    __in Data::LoggingReplicator::VersionManager& versionManager, 
    __in std::vector<LONG64>& barrierResponses,
    __in_opt bool withFlag)
{
    if (withFlag)
    {
        ASSERT_IFNOT(0 == beforeUnRegisterFlag_, "Test bug. Incorrect visibility sequence number.");
        beforeUnRegisterFlag_ = 1;
    }

    for (LONG64 barrierResponse : barrierResponses)
    {
        versionManager.UnRegister(barrierResponse);
    }

    if (withFlag)
    {
        ASSERT_IFNOT(0 == afterUnRegisterFlag_, "Test bug. Incorrect visibility sequence number.");
        afterUnRegisterFlag_ = 1;
    }
}

TxnReplicator::TryRemoveVersionResult::SPtr VersionManagerTestBase::TestTryRemoveVersion(
    __in Data::LoggingReplicator::VersionManager & versionManager, 
    __in LONG64 stateProviderId, 
    __in LONG64 version, 
    __in LONG64 versionNext, 
    __in TxnReplicator::TryRemoveVersionResult & expectedResult)
{
    TxnReplicator::TryRemoveVersionResult::SPtr result;
    versionManager.TryRemoveVersion(stateProviderId, version, versionNext, result);

    VerifyTryRemoveResult(*result, expectedResult);

    return result;
}

void VersionManagerTestBase::VerifyTryRemoveResult(
    __in TxnReplicator::TryRemoveVersionResult& inputResult,
    __in TxnReplicator::TryRemoveVersionResult& expectedResult)
{
    ASSERT_IFNOT(
        inputResult == expectedResult,
        "VerifyTryRemoveResult failed");

    if (inputResult.EnumerationCompletionNotifications == nullptr)
    {
        return;
    }

    ASSERT_IFNOT(
        inputResult.EnumerationCompletionNotifications->Count() == expectedResult.EnumerationCompletionNotifications->Count(),
        "Input {0} == Expected {1}",
        inputResult.EnumerationCompletionNotifications->Count(),
        expectedResult.EnumerationCompletionNotifications->Count());

    Data::IEnumerator<TxnReplicator::EnumerationCompletionResult::SPtr>::SPtr enumerator = inputResult.EnumerationCompletionNotifications->GetEnumerator();
    while (enumerator->MoveNext())
    {
        TxnReplicator::EnumerationCompletionResult::SPtr currentItem = enumerator->Current();
        bool containsVisibilityLSN = ContainsNotification(*expectedResult.EnumerationCompletionNotifications, currentItem->VisibilitySequenceNumber);
        ASSERT_IFNOT(
            containsVisibilityLSN, 
            "Does not contain {0}", 
             currentItem->VisibilitySequenceNumber);
        ASSERT_IFNOT(
            enumerator->Current()->IsNotificationCompleted == false, 
            "Notification should not have completed {0}",
            currentItem->VisibilitySequenceNumber);        
    }
}

bool VersionManagerTestBase::ContainsNotification(
    __in Data::Utilities::KHashSet<TxnReplicator::EnumerationCompletionResult::SPtr> const & set,
    __in LONG64 enumeration)
{
    Data::IEnumerator<TxnReplicator::EnumerationCompletionResult::SPtr>::SPtr enumerator = set.GetEnumerator();

    while (enumerator->MoveNext())
    {
        TxnReplicator::EnumerationCompletionResult::SPtr currentItem = enumerator->Current();
        if (currentItem->VisibilitySequenceNumber == enumeration)
        {
            return true;
        }
    }

    return false;
}

void VersionManagerTestBase::VerifyState(
    __in Data::LoggingReplicator::VersionManager& versionManager,
    __in LONG32 retryCount,
    __in LONG32 versionCount,
    __in LONG32 notificationCount,
    __in LONG32 registeredNotificationsCount)
{
    for (LONG32 i = 0; i < retryCount; i++)
    {
        bool result = versionManager.Test_VerifyState(versionCount, notificationCount, registeredNotificationsCount);
        if (result)
        {
            return;
        }

        KTimer::SPtr timer;
        NTSTATUS status = KTimer::Create(timer, GetAllocator(), KTL_TAG_TEST);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ktl::Awaitable<NTSTATUS> awaitable = KTimer::StartTimerAsync(GetAllocator(), KTL_TAG_TEST, DefaultBackoff, nullptr);
        status = ktl::SyncAwait(awaitable);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    CODING_ERROR_ASSERT(false);
}

KAllocator& VersionManagerTestBase::GetAllocator()
{
    return underlyingSystemPtr_->PagedAllocator();
}

LONG64 VersionManagerTestBase::GetRandomLONG64()
{
    return static_cast<LONG64>(random_.Next());
}

VersionManagerTestBase::VersionManagerTestBase()
{
    NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystemPtr_);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    underlyingSystemPtr_->SetStrictAllocationChecks(TRUE);
}

VersionManagerTestBase::~VersionManagerTestBase()
{
    underlyingSystemPtr_->Shutdown();
}
