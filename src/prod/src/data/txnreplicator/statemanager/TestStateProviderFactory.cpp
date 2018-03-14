// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace StateManagerTests;

TestStateProviderFactory::SPtr TestStateProviderFactory::Create(
    __in KAllocator& allocator)
{
    SPtr result = _new(TEST_STATE_PROVIDER_FACTORY_TAG, allocator) TestStateProviderFactory();
    THROW_ON_ALLOCATION_FAILURE(result);
    THROW_ON_FAILURE(result->Status());

    return result;
}

NTSTATUS TestStateProviderFactory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept
{
    TestStateProvider::SPtr testStateProvider = nullptr;
    NTSTATUS status = TestStateProvider::Create(
        factoryArguments,
        GetThisAllocator(),
        faultAPI_,
        testStateProvider);
    if (NT_SUCCESS(status) == false)
    {
        stateProvider = nullptr;
        return status;
    }

    stateProvider = testStateProvider.RawPtr();
    return status;
}

TestStateProviderFactory::TestStateProviderFactory()
    : KObject()
    , KShared()
    , faultAPI_(FaultStateProviderType::FaultAPI::None)
{
}

TestStateProviderFactory::~TestStateProviderFactory()
{
}
