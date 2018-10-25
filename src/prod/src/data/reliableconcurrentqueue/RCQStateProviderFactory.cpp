//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Collections;
using namespace Common;

RCQStateProviderFactory::SPtr RCQStateProviderFactory::CreateBufferFactory(__in KAllocator& allocator)
{
    SPtr result = _new(RCQ_STATE_PROVIDER_FACTORY_TAG, allocator) RCQStateProviderFactory(FactoryDataType::Buffer);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

NTSTATUS RCQStateProviderFactory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept
{
    try
    {
        switch (type_)
        {
        case FactoryDataType::Buffer:
            CreateBufferValueRCQ(factoryArguments, stateProvider);
            break;

        default:
            break;
        }
    }
    catch (ktl::Exception & e)
    {
        return e.GetStatus();
    }

    return STATUS_SUCCESS;
}

void RCQStateProviderFactory::CreateBufferValueRCQ(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<KBufferComparer> comparerSPtr = nullptr;
    status = KBufferComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    KBufferSerializer::SPtr keyStateSerializer = nullptr;
    KBufferSerializer::SPtr valueStateSerializer = nullptr;

    status = KBufferSerializer::Create(GetThisAllocator(), keyStateSerializer);
    Diagnostics::Validate(status);
    status = KBufferSerializer::Create(GetThisAllocator(), valueStateSerializer);
    Diagnostics::Validate(status);

    Data::Collections::ReliableConcurrentQueue<KBuffer::SPtr>::SPtr rcqSPtr = nullptr;

    status = Data::Collections::ReliableConcurrentQueue<KBuffer::SPtr>::Create(
        GetThisAllocator(),
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *valueStateSerializer,
        rcqSPtr);

    Diagnostics::Validate(status);

    rcqSPtr->EnableSweep = true;

    stateProvider = rcqSPtr.RawPtr();
}

RCQStateProviderFactory::RCQStateProviderFactory(__in FactoryDataType type)
    : KObject()
    , KShared()
    , type_(type)
{
}

RCQStateProviderFactory::~RCQStateProviderFactory()
{
}
