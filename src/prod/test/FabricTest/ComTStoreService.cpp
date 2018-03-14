// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;

#define TEST_COM_STATE_PROVIDER_FACTORY_TAG 'fpCT'

//
// The Test Com StateProvider2Factory implements the IFabricStateProvider2Factory interface.
// Its purpose is to convert the internal KTL TestStateProviderFactory in to COM based interface.
// This is used to create state providers (IStateProvider2).
//
class TestComStateProvider2Factory final
    : public KObject<TestComStateProvider2Factory>
    , public KShared<TestComStateProvider2Factory>
    , public IFabricStateProvider2Factory
{
    K_FORCE_SHARED(TestComStateProvider2Factory)

    K_BEGIN_COM_INTERFACE_LIST(TestComStateProvider2Factory)
    COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider2Factory)
    COM_INTERFACE_ITEM(IID_IFabricStateProvider2Factory, IFabricStateProvider2Factory)
    K_END_COM_INTERFACE_LIST()

public:
    static ComPointer<IFabricStateProvider2Factory> Create(
        __in Data::TStore::StoreStateProviderFactory& innerFactory,
        __in KAllocator & allocator);

    // Returns a pointer to a HANDLE (PHANDLE).
    // HANDLE is IStateProvider2*.
    STDMETHOD_(HRESULT, Create)(
        /* [in] */ FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
        /* [retval][out] */ HANDLE * stateProvider) override;

private:
    TestComStateProvider2Factory(__in Data::TStore::StoreStateProviderFactory & innerFactory);

private:
    Data::TStore::StoreStateProviderFactory::SPtr innerFactory_;
};

ComPointer<IFabricStateProvider2Factory> TestComStateProvider2Factory::Create(
    __in Data::TStore::StoreStateProviderFactory & factory,
    __in KAllocator & allocator)
{
    TestComStateProvider2Factory * pointer = _new(TEST_COM_STATE_PROVIDER_FACTORY_TAG, allocator) TestComStateProvider2Factory(factory);
    CODING_ERROR_ASSERT(pointer != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(pointer->Status()))

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

TestComStateProvider2Factory::TestComStateProvider2Factory(__in Data::TStore::StoreStateProviderFactory & factory)
    : KObject()
    , KShared()
    , innerFactory_(&factory)
{
}

TestComStateProvider2Factory::~TestComStateProvider2Factory()
{
}

ComTStoreService::ComTStoreService(TStoreService & innerService)
    : ComTXRService(innerService)
{
}

ComPointer<IFabricStateProvider2Factory> ComTStoreService::CreateStateProvider2Factory(
    __in std::wstring const & nodeId,
    __in std::wstring const & serviceName,
    __in KAllocator & allocator)
{
    UNREFERENCED_PARAMETER(nodeId);
    UNREFERENCED_PARAMETER(serviceName);
    Data::TStore::StoreStateProviderFactory::SPtr factory = Data::TStore::StoreStateProviderFactory::CreateLongStringUTF8Factory(allocator);

    return TestComStateProvider2Factory::Create(*factory, allocator);
}
