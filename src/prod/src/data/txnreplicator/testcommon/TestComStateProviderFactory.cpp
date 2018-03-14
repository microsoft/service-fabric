// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace TxnReplicator::TestCommon;

ComPointer<IFabricStateProvider2Factory> TestComStateProvider2Factory::Create(
    __in Data::StateManager::IStateProvider2Factory & factory,
    __in KAllocator & allocator)
{
    TestComStateProvider2Factory * pointer = _new(TEST_COM_STATE_PROVIDER_FACTORY_TAG, allocator) TestComStateProvider2Factory(factory);
    ASSERT_IF(pointer == nullptr, "OOM while allocating TestComStateProvider2Factory");
    ASSERT_IFNOT(NT_SUCCESS(pointer->Status()), "Unsuccessful initialization while allocating TestComStateProvider2Factory {0}", pointer->Status());

    ComPointer<IFabricStateProvider2Factory> result;
    result.SetAndAddRef(pointer);
    return result;
}

HRESULT TestComStateProvider2Factory::Create(
    __in FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
    __out HANDLE * stateProvider) // IStateProvider2* *
{
    KStringView typeString(factoryArguments->typeName);
    KUriView nameUri(factoryArguments->name);

    KString::SPtr typeStringSPtr = KString::Create(typeString, GetThisAllocator());
    KUri::CSPtr nameUriCSPtr;
    NTSTATUS status = KUri::Create(nameUri, GetThisAllocator(), nameUriCSPtr);
    if (NT_ERROR(status)) { return HRESULT_FROM_NT(status); }

    KGuid partitionId(factoryArguments->partitionId);
    LONG64 replicaId = factoryArguments->replicaId;
    FABRIC_OPERATION_DATA_BUFFER_LIST * bufferList = factoryArguments->initializationParameters;

    // Read the buffers in FABRIC_OPERATION_DATA_BUFFER_LIST and create the OperationData
    Data::Utilities::OperationData::SPtr initializationParameters = nullptr;
    if (bufferList->Count != 0)
    {
        initializationParameters = Data::Utilities::OperationData::Create(GetThisAllocator());

        for (ULONG32 i = 0; i < bufferList->Count; i++)
        {
            ULONG bufferSize = bufferList->Buffers[i].BufferSize;
            BYTE * bufferContext = bufferList->Buffers[i].Buffer;

            KBuffer::SPtr buffer;
            status = KBuffer::Create(bufferSize, buffer, GetThisAllocator());
            if (NT_ERROR(status)) { return HRESULT_FROM_NT(status); }

            BYTE * pointer = static_cast<BYTE *>(buffer->GetBuffer());
            KMemCpySafe(pointer, bufferSize, bufferContext, bufferSize);

            initializationParameters->Append(*buffer);
        }
    }

    TxnReplicator::IStateProvider2::SPtr outStateProvider;
    Data::StateManager::FactoryArguments factoryArg(*nameUriCSPtr, factoryArguments->stateProviderId, *typeStringSPtr, partitionId, replicaId, initializationParameters.RawPtr());

    status = innerFactory_->Create(factoryArg, outStateProvider);
    if (NT_ERROR(status)) { return HRESULT_FROM_NT(status); }

    *stateProvider = outStateProvider.Detach();
    return S_OK;
}

TestComStateProvider2Factory::TestComStateProvider2Factory(__in Data::StateManager::IStateProvider2Factory & factory)
    : KObject()
    , KShared()
    , innerFactory_(&factory)
{
}

TestComStateProvider2Factory::~TestComStateProvider2Factory()
{
}
