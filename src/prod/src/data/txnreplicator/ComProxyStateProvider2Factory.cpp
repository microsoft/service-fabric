// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data;
using namespace TxnReplicator;

NTSTATUS ComProxyStateProvider2Factory::Create(
    __in ComPointer<IFabricStateProvider2Factory> & factory,
    __in KAllocator & allocator,
    __out ComProxyStateProvider2Factory::SPtr & result)
{
    ComProxyStateProvider2Factory::SPtr tmpPtr(_new(COM_PROXY_STATE_PROVIDER_FACTORY, allocator)ComProxyStateProvider2Factory(factory));

    if (!tmpPtr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(tmpPtr->Status()))
    {
        return tmpPtr->Status();
    }

    result = Ktl::Move(tmpPtr);
    return STATUS_SUCCESS;
}

NTSTATUS ComProxyStateProvider2Factory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out IStateProvider2::SPtr& stateProvider) noexcept
{
    LPCWSTR comTypeName = *factoryArguments.TypeName;
    FABRIC_URI comName = factoryArguments.Name->Get(KUriView::eRaw);
    Utilities::OperationData::CSPtr initParameters = factoryArguments.get_InitializationParameters();

    FABRIC_OPERATION_DATA_BUFFER_LIST comDataBufferList;

    // If the initParameters is nullptr, set the dataBufferList count to 0 and Buffers to nullptr
    if(initParameters != nullptr)
    {
        comDataBufferList.Count = factoryArguments.InitializationParameters->BufferCount;

        // Initialize the ReferenceArray with size equal to BufferCount
        auto list = heap_.AddArray<FABRIC_OPERATION_DATA_BUFFER>(factoryArguments.InitializationParameters->BufferCount);
        FABRIC_OPERATION_DATA_BUFFER * bufferList = list.GetRawArray();

        // Set each item in the list
        for (ULONG32 i = 0; i < factoryArguments.InitializationParameters->BufferCount; i++)
        {
            FABRIC_OPERATION_DATA_BUFFER buffer;
            buffer.BufferSize = (*factoryArguments.InitializationParameters)[i]->QuerySize();
            buffer.Buffer = static_cast<BYTE *>(const_cast<void*>((*factoryArguments.InitializationParameters)[i]->GetBuffer()));
            bufferList[i] = buffer;
        }

        comDataBufferList.Buffers = bufferList;
    }
    else
    {
        comDataBufferList.Count = 0;
        comDataBufferList.Buffers = nullptr;
    }

    FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS comFactoryArguments;
    comFactoryArguments.name = comName;
    comFactoryArguments.typeName = comTypeName;
    comFactoryArguments.initializationParameters = &comDataBufferList;
    comFactoryArguments.partitionId = factoryArguments.PartitionId;
    comFactoryArguments.replicaId = factoryArguments.ReplicaId;
    comFactoryArguments.stateProviderId = factoryArguments.StateProviderId;

    void* rawStateProvider;
    HRESULT hResult = comFactory_->Create(&comFactoryArguments, &rawStateProvider);
    if (!SUCCEEDED(hResult))
    {
        NTSTATUS status = Utilities::StatusConverter::Convert(hResult);
        return status;
    }

    IStateProvider2 * ptr = static_cast<IStateProvider2 *>(rawStateProvider);
    ASSERT_IF(ptr == nullptr, "Pointer returned from IFabricStateProvider2Factory is not pointing to an IStateProvider2.");

    stateProvider.Attach(ptr);

    return STATUS_SUCCESS;
}

ComProxyStateProvider2Factory::ComProxyStateProvider2Factory(__in ComPointer<IFabricStateProvider2Factory> & factory)
    : KObject()
    , KShared()
    , comFactory_(factory)
    , heap_()
{
}

ComProxyStateProvider2Factory::~ComProxyStateProvider2Factory()
{
}

