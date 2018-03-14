// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator::TestCommon;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent = "TestStateProviderFactory";

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
    KString::CSPtr typeName = factoryArguments.TypeName;
    KStringView expectedTypeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());
    if (typeName->Compare(expectedTypeName) == 0)
    {
        auto testStateProviderSPtr = TestStateProvider::Create(factoryArguments, GetThisAllocator());
        stateProvider = testStateProviderSPtr.RawPtr();
        Trace.WriteInfo(
            TraceComponent,
            "PartitionID {0}, ReplicaID {1}, StateProviderID {2}, StateProviderName {3}, TestStateProviderFactory creates state provider for type {4}",
            Common::Guid(factoryArguments.PartitionId),
            factoryArguments.ReplicaId,
            factoryArguments.StateProviderId,
            ToStringLiteral(*(factoryArguments.Name)),
            ToStringLiteral(*typeName));
        return STATUS_SUCCESS;
    }

    CODING_ASSERT(
        "PartitionID {0}, ReplicaID {1}, StateProviderID {2}, StateProviderName {3}, TestStateProviderFactory not implemented for type {4} because expected type is {5}",
        Common::Guid(factoryArguments.PartitionId),
        factoryArguments.ReplicaId,
        factoryArguments.StateProviderId,
        ToStringLiteral(*(factoryArguments.Name)),
        ToStringLiteral(*typeName),
        ToStringLiteral(expectedTypeName));
    return STATUS_NOT_IMPLEMENTED;
}

TestStateProviderFactory::TestStateProviderFactory()
    : KObject()
    , KShared()
{
}

TestStateProviderFactory::~TestStateProviderFactory()
{
}
