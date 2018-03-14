// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComStateProviderFactory.h"

using namespace Common;
using namespace Data::Utilities;
using namespace Data::Interop;

HRESULT ComStateProviderFactory::Create(
    __in KAllocator & allocator,
    __out ComPointer<IFabricStateProvider2Factory>& result)
{
    NTSTATUS status;
    StateProviderFactory::SPtr factory;
    status = StateProviderFactory::Create(allocator, factory);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    ComStateProviderFactory * pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) ComStateProviderFactory(*factory);
    if (!pointer)
        return E_OUTOFMEMORY;

    result.SetAndAddRef(pointer);
    return S_OK;
}

HRESULT ComStateProviderFactory::Create(
    __in FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
    __out HANDLE * stateProvider) // IStateProvider2* *
{
    KString::SPtr typeString = KString::Create(factoryArguments->typeName, GetThisAllocator());
    if (!typeString)
        return E_OUTOFMEMORY;

    KUriView nameUri(factoryArguments->name);
    KUri::CSPtr nameCSPtr;
    NTSTATUS status = KUri::Create(const_cast<KUriView &>(nameUri), GetThisAllocator(), (KUri::SPtr&)nameCSPtr);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    KGuid partitionGuid(factoryArguments->partitionId);
    FABRIC_OPERATION_DATA_BUFFER_LIST * bufferList = factoryArguments->initializationParameters;

    // Read the buffers in FABRIC_OPERATION_DATA_BUFFER_LIST and create the OperationData
    Data::Utilities::OperationData::SPtr initializationParameters = nullptr;
    if (bufferList->Count != 0)
    {
        initializationParameters = Data::Utilities::OperationData::Create(GetThisAllocator());
        if (!initializationParameters)
            return E_OUTOFMEMORY;

        for (ULONG32 i = 0; i < bufferList->Count; i++)
        {
            ULONG bufferSize = bufferList->Buffers[i].BufferSize;
            BYTE * bufferContext = bufferList->Buffers[i].Buffer;

            KBuffer::SPtr buffer;
            status = KBuffer::Create(bufferSize, buffer, GetThisAllocator());
            if (!NT_SUCCESS(status))
                return StatusConverter::ToHResult(status);

            BYTE * pointer = static_cast<BYTE *>(buffer->GetBuffer());
            KMemCpySafe(pointer, bufferSize, bufferContext, bufferSize);

            initializationParameters->Append(*buffer);
        }
    }

    TxnReplicator::IStateProvider2::SPtr outStateProvider;
    Data::StateManager::FactoryArguments factoryArg(*nameCSPtr, factoryArguments->stateProviderId, *typeString, partitionGuid, factoryArguments->replicaId, initializationParameters.RawPtr());

    status = innerFactory_->Create(factoryArg, outStateProvider);
    if (!NT_SUCCESS(status))
        return StatusConverter::ToHResult(status);

    *stateProvider = outStateProvider.Detach();
    return S_OK;
}

ComStateProviderFactory::ComStateProviderFactory(__in Data::StateManager::IStateProvider2Factory & factory)
    : KObject()
    , KShared()
    , innerFactory_(&factory)
{
}

ComStateProviderFactory::~ComStateProviderFactory()
{
}
