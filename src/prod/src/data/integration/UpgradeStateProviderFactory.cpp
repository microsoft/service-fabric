// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Data::Integration;
using namespace TxnReplicator::TestCommon;
#define UPGRADE_TEST_PROVIDER_FACTORY_TAG 'psTU'

UpgradeStateProviderFactory::SPtr UpgradeStateProviderFactory::Create(
    __in KAllocator& allocator)
{
    SPtr result = _new(UPGRADE_TEST_PROVIDER_FACTORY_TAG, allocator) UpgradeStateProviderFactory(false);
    THROW_ON_ALLOCATION_FAILURE(result);
    THROW_ON_FAILURE(result->Status());

    return result;
}

UpgradeStateProviderFactory::SPtr UpgradeStateProviderFactory::Create(
    __in KAllocator& allocator,
    __in bool isBackCompatTest)
{
    SPtr result = _new(UPGRADE_TEST_PROVIDER_FACTORY_TAG, allocator) UpgradeStateProviderFactory(isBackCompatTest);
    THROW_ON_ALLOCATION_FAILURE(result);
    THROW_ON_FAILURE(result->Status());

    return result;
}

NTSTATUS UpgradeStateProviderFactory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept
{
    KString::CSPtr typeName = factoryArguments.TypeName;
    std::wstring t = L"TStore";

    KString::SPtr searchTarget;
    NTSTATUS status = KString::Create(searchTarget, GetThisAllocator(), t.c_str());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    ULONG pos;

    bool tstoreStrFound = typeName->Search(*searchTarget, pos);
    if (tstoreStrFound)
    {
        return innerFactory_->Create(factoryArguments, stateProvider);
    }
    else
    {
        TestStateProvider::SPtr testStateProviderSPtr = TestStateProvider::Create(
            factoryArguments, 
            isBackCompatTest_,
            GetThisAllocator());
        stateProvider = testStateProviderSPtr.RawPtr();

        return STATUS_SUCCESS;
    }
}

UpgradeStateProviderFactory::UpgradeStateProviderFactory(
    __in bool isBackCompatTest)
    : KObject()
    , KShared()
    , isBackCompatTest_(isBackCompatTest)
{
    if (isBackCompatTest_)
    {
        innerFactory_ = TStore::StoreStateProviderFactory::CreateIntIntFactory(GetThisAllocator());
    }
    else
    {
        innerFactory_ = TStore::StoreStateProviderFactory::CreateNullableStringUTF8BufferFactory(GetThisAllocator());
    }
}

UpgradeStateProviderFactory::~UpgradeStateProviderFactory()
{
}
