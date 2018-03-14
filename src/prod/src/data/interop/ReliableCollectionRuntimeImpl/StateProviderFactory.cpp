// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Utilities;
using namespace TStore;
using namespace Data::Interop;

NTSTATUS StateProviderFactory::Create(
     __in KAllocator& allocator,
     __out StateProviderFactory::SPtr& factory)
{
    StoreStateProviderFactory::SPtr storeFactory = StoreStateProviderFactory::CreateStringUTF16BufferFactory(allocator);
    if (!storeFactory)
        return STATUS_INSUFFICIENT_RESOURCES;

    StateProviderFactory* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) StateProviderFactory(storeFactory.RawPtr());
    if (!pointer)
        return STATUS_INSUFFICIENT_RESOURCES;

    factory = pointer;
    return STATUS_SUCCESS;
}

NTSTATUS StateProviderFactory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept 
{
    NTSTATUS status = STATUS_SUCCESS;
    StateProviderInfo::SPtr stateProviderInfo;

    status = StateProviderInfo::Decode(
        GetThisAllocator(),
        *(factoryArguments.TypeName),
        stateProviderInfo);
    if (!NT_SUCCESS(status))
        return status;

    if (stateProviderInfo->Kind == StateProviderKind::Store)
    {
        status = CreateStringUTF16KeyBufferValueStore(factoryArguments, stateProvider);
        if (!NT_SUCCESS(status))
            return status;
    }
    else if (stateProviderInfo->Kind == StateProviderKind::ConcurrentQueue)
    {
        status = CreateBufferItemRCQ(factoryArguments, stateProvider);
        if (!NT_SUCCESS(status))
            return status;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
        // TODO add trace
    }

    auto stateProviderInfoCasted = dynamic_cast<IStateProviderInfo*>(stateProvider.RawPtr());
    if (stateProviderInfoCasted == nullptr)
        return STATUS_OBJECT_TYPE_MISMATCH;

    if (stateProviderInfo->Lang != nullptr && stateProviderInfo->LangMetadata != nullptr)
    {
        status = stateProviderInfoCasted->SetLangTypeInfo(*(stateProviderInfo->Lang), *(stateProviderInfo->LangMetadata));
        if (!NT_SUCCESS(status))
            return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS StateProviderFactory::CreateStringUTF16KeyBufferValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    return storeFactory_->Create(factoryArguments, stateProvider);
}

NTSTATUS StateProviderFactory::CreateBufferItemRCQ(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    UNREFERENCED_PARAMETER(factoryArguments);
    UNREFERENCED_PARAMETER(stateProvider);
    // TODO Call Queue Ctor
    return STATUS_SUCCESS;
}

StateProviderFactory::StateProviderFactory(__in Data::StateManager::IStateProvider2Factory* factory)
    : KObject()
    , KShared()
    , storeFactory_(factory)
{
}

StateProviderFactory::~StateProviderFactory()
{
}
