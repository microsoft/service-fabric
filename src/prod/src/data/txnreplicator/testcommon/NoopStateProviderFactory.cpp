// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator::TestCommon;

NoopStateProviderFactory::SPtr NoopStateProviderFactory::Create(
    __in KAllocator& allocator)
{
    SPtr result = _new(TEST_STATE_PROVIDER_FACTORY_TAG, allocator) NoopStateProviderFactory();
    THROW_ON_ALLOCATION_FAILURE(result);
    THROW_ON_FAILURE(result->Status());

    return result;
}

NTSTATUS NoopStateProviderFactory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept
{
    KString::CSPtr typeName = factoryArguments.TypeName;

    if (typeName->Compare(NoopStateProvider::TypeName) == 0)
    {
        auto testStateProviderSPtr = NoopStateProvider::Create(factoryArguments, GetThisAllocator());
        stateProvider = testStateProviderSPtr.RawPtr();

        return STATUS_SUCCESS;
    }

    ASSERT_IF(true, "NoopStateProviderFactory not implemented for type");

    return STATUS_NOT_IMPLEMENTED;
}

NoopStateProviderFactory::NoopStateProviderFactory()
    : KObject()
    , KShared()
{
}

NoopStateProviderFactory::~NoopStateProviderFactory()
{
}
