// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Collections;
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

    RCQStateProviderFactory::SPtr rcqFactory = RCQStateProviderFactory::CreateBufferFactory(allocator);
    if (!rcqFactory)
        return STATUS_INSUFFICIENT_RESOURCES;

    StateProviderFactory* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) StateProviderFactory(storeFactory.RawPtr(), rcqFactory.RawPtr());
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
        status = storeFactory_->Create(factoryArguments, stateProvider);
        if (!NT_SUCCESS(status))
            return status;
    }
    else if (stateProviderInfo->Kind == StateProviderKind::ConcurrentQueue)
    {
        status = rcqFactory_->Create(factoryArguments, stateProvider);
        if (!NT_SUCCESS(status))
            return status;
    }
    else if (stateProviderInfo->Kind == StateProviderKind::ReliableDictionary_Compat)
    {
        KStringView uriLastSegment;
        ULONG uriSegmentCount = const_cast<KUri*>(factoryArguments.Name.RawPtr())->GetSegmentCount();
        const_cast<KUri*>(factoryArguments.Name.RawPtr())->GetSegment(uriSegmentCount - 1, uriLastSegment);
        if (uriLastSegment.Compare(L"dataStore") == 0)
        {
            // Create internal TStore component of ReliableDictionary
            status = storeFactory_->Create(factoryArguments, stateProvider);
            if (!NT_SUCCESS(status))
                return status;
        }
        else
        {
            // Create dummy wrapper ReliableDictionary state provider
            CompatRDStateProvider::SPtr compatRDStateProvider;
            status = CompatRDStateProvider::Create(
                factoryArguments,
                GetThisAllocator(),
                compatRDStateProvider);
            if (!NT_SUCCESS(status))
                return status;

            stateProvider = static_cast<TxnReplicator::IStateProvider2*>(compatRDStateProvider.RawPtr());
        }
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

StateProviderFactory::StateProviderFactory(
    __in Data::StateManager::IStateProvider2Factory* storeFactory,
    __in Data::StateManager::IStateProvider2Factory* rcqFactory)
    : KObject()
    , KShared()
    , storeFactory_(storeFactory)
    , rcqFactory_(rcqFactory)
{
}

StateProviderFactory::~StateProviderFactory()
{
}
