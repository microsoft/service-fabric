// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TpccService;
using namespace TxnReplicator;
using namespace Data::TStore;
using namespace Data::Utilities;

static const StringLiteral TraceComponent("TestComStateProvider2Factory");

ComPointer<IFabricStateProvider2Factory> TestComStateProvider2Factory::Create(
    __in FABRIC_REPLICA_ID replicaId,
    __in KAllocator & allocator)
{
    TestComStateProvider2Factory * pointer = _new('xxxx', allocator) TestComStateProvider2Factory(replicaId);
    CODING_ERROR_ASSERT(pointer != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(pointer->Status()))

    ComPointer<IFabricStateProvider2Factory> result;
    result.SetAndAddRef(pointer);
    return result;
}

HRESULT STDMETHODCALLTYPE TestComStateProvider2Factory::Create(
    /* [in] */ const FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS *factoryArguments,
    /* [retval][out] */ void **stateProvider)
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

    if (Service::warehousesSPName == factoryArguments->name) {
        WarehouseStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::districtsSPName == factoryArguments->name) {
        DistrictStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::newOrdersSPName == factoryArguments->name) {
        NewOrderStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::customersSPName == factoryArguments->name) {
        CustomerStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::ordersSPName == factoryArguments->name) {
        OrderStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::orderLineSPName == factoryArguments->name) {
        OrderLineStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::historySPName == factoryArguments->name) {
        HistoryStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::stockSPName == factoryArguments->name) {
        StockStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else if (Service::itemsSPName == factoryArguments->name) {
        ItemStoreUtil::CreateStore(
            factoryArg,
            replicaId_,
            this->GetThisAllocator(),
            outStateProvider);
    }
    else {
        Trace.WriteError(TraceComponent, "Invalid store name {0}", factoryArguments->name);
        KInvariant(false);
    }

    *stateProvider = outStateProvider.Detach();
    return S_OK;
}

TestComStateProvider2Factory::TestComStateProvider2Factory(FABRIC_REPLICA_ID replicaId)
    : KObject()
    , KShared()
    , replicaId_(replicaId)
{
}

TestComStateProvider2Factory::~TestComStateProvider2Factory()
{
}
