// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace StateManagerTests;

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

TestStateManagerChangeHandler::SPtr TestStateManagerChangeHandler::Create(
    __in KAllocator& allocator)
{
    SPtr result = _new(TEST_TAG, allocator) TestStateManagerChangeHandler();
    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

    return result;
}

void TestStateManagerChangeHandler::VerifyExits(
    __in KArray<KUri::CSPtr> & stateProviderList,
    __in bool expectedResult)
{
    for (ULONG i = 0; i < stateProviderList.Count(); i++)
    {
        bool isExist = stateProviders_->ContainsKey(stateProviderList[i]);
        ASSERT_IFNOT(
            isExist == expectedResult,
            "Unexpected result: Expected: {0} Key: {1}",
            expectedResult,
            ToStringLiteral(*stateProviderList[i]));
    }
}

void TestStateManagerChangeHandler::VerifyState(
    __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>> & stateManagerState)
{
    ULONG count = 0;

    while(stateManagerState.MoveNext())
    {
        Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr> current = stateManagerState.Current();
        
        KUri::CSPtr name = current.Key;
        IStateProvider2::SPtr stateProvider = current.Value;

        // Verify that name exists.
        IStateProvider2::SPtr localStateProvider;
        bool isContained = stateProviders_->TryGetValue(name, localStateProvider);
        ASSERT_IFNOT(isContained, "Must be contained.");
        ASSERT_IFNOT(localStateProvider != nullptr, "Must be contained.");

        // Verify that the state providers match.
        ASSERT_IFNOT(localStateProvider->GetName().Compare(*name) == TRUE, "State Providers must match.");

        // Increment count;
        count++;
    }

    ASSERT_IFNOT(count == stateProviders_->Count, "Counts must match.");
}

void TestStateManagerChangeHandler::OnRebuilt(
    __in ITransactionalReplicator& source, 
    __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>> & stateProviders)
{
    UNREFERENCED_PARAMETER(source);
    InterlockedIncrement(&stateProviderRebuildNotificationCount_);

    stateProviders_->Clear();

    while (stateProviders.MoveNext())
    {
        Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr> current = stateProviders.Current();

        KUri::CSPtr name = current.Key;
        IStateProvider2::SPtr stateProvider = current.Value;

        stateProviders_->Add(name.RawPtr(), stateProvider);
    }

    // Verify that after enumerator reaches to end of the collection - further movenext should result to false
    ASSERT_IFNOT(stateProviders.MoveNext() == false, "MoveNext should return false.");
}

void TestStateManagerChangeHandler::OnAdded(
    __in ITransactionalReplicator& source, 
    __in ITransaction const & transaction,
    __in KUri const & stateProviderName,
    __in IStateProvider2& stateProvider)
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(transaction);
    InterlockedIncrement(&stateProviderAddNotificationCount_);

    stateProviders_->Add(&stateProviderName, &stateProvider);
}

void TestStateManagerChangeHandler::OnRemoved(
    __in ITransactionalReplicator& source,
    __in ITransaction const & transaction,
    __in KUri const & stateProviderName,
    __in IStateProvider2& stateProvider)
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(stateProvider);

    InterlockedIncrement(&stateProviderRemoveNotificationCount_);

    stateProviders_->Remove(&stateProviderName);
}

TestStateManagerChangeHandler::TestStateManagerChangeHandler()
    : KObject()
    , KShared()
{
    NTSTATUS status = ConcurrentDictionary<KUri::CSPtr, IStateProvider2::SPtr>::Create(GetThisAllocator(), stateProviders_);
    SetConstructorStatus(status);
}

TestStateManagerChangeHandler::~TestStateManagerChangeHandler()
{
}
