// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;
using namespace TStoreTests;
using namespace Common;


inline ULONG IntHashFuncImp(const int& val)
{
    return ~val;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateLongStringUTF8Factory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::LongStringUTF8);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateIntIntFactory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::IntInt);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateStringUTF8BufferFactory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::StringUTF8Buffer);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateNullableStringUTF8BufferFactory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::NullableStringUTF8Buffer);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateStringUTF16BufferFactory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::StringUTF16Buffer);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateLongBufferFactory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::LongBuffer);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}

StoreStateProviderFactory::SPtr StoreStateProviderFactory::CreateBufferBufferFactory(__in KAllocator& allocator)
{
    SPtr result = _new(STORE_STATE_PROVIDER_FACTORY_TAG, allocator) StoreStateProviderFactory(FactoryDataType::BufferBuffer);
    THROW_ON_ALLOCATION_FAILURE(result);
    Diagnostics::Validate(result->Status());
    return result;
}


NTSTATUS StoreStateProviderFactory::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept
{
    try
    {
        switch (type_)
        {
        case FactoryDataType::LongStringUTF8:
            CreateLongKeyStringUTF8ValueStore(factoryArguments, stateProvider);
            break;

        case FactoryDataType::StringUTF8Buffer:
            CreateStringUTF8KeyBufferValueStore(factoryArguments, stateProvider);
            break;

        case FactoryDataType::NullableStringUTF8Buffer:
            CreateNullableStringUTF8KeyBufferValueStore(factoryArguments, stateProvider);
            break;

        case FactoryDataType::StringUTF16Buffer:
            CreateStringUTF16KeyBufferValueStore(factoryArguments, stateProvider);
            break;

        case FactoryDataType::IntInt:
            CreateIntKeyIntValueStore(factoryArguments, stateProvider);
            break;

        case FactoryDataType::LongBuffer:
            CreateLongKeyBufferValueStore(factoryArguments, stateProvider);
            break;

        case FactoryDataType::BufferBuffer:
            CreateBufferKeyBufferValueStore(factoryArguments, stateProvider);
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

void StoreStateProviderFactory::CreateIntKeyIntValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427:TStore needs to take in const KuriView.
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<IntComparer> comparerSPtr = nullptr;
    status = IntComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    TestStateSerializer<int>::SPtr keyStateSerializer = nullptr;
    TestStateSerializer<int>::SPtr valueStateSerializer = nullptr;

    status = TestStateSerializer<int>::Create(GetThisAllocator(), keyStateSerializer);
    Diagnostics::Validate(status);
    status = TestStateSerializer<int>::Create(GetThisAllocator(), valueStateSerializer);
    Diagnostics::Validate(status);

    Data::TStore::Store<int, int>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<int, int>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        IntHashFuncImp,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);
    stateProvider = storeSPtr.RawPtr();
}

void StoreStateProviderFactory::CreateLongKeyStringUTF8ValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427: TStore needs to take in const KuriView.
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<LongComparer> comparerSPtr = nullptr;
    status = LongComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    TestStateSerializer<LONG64>::SPtr keyStateSerializer = nullptr;
    StringStateSerializer::SPtr valueStateSerializer = nullptr;

    status = TestStateSerializer<LONG64>::Create(GetThisAllocator(), keyStateSerializer);
    Diagnostics::Validate(status);
    status = StringStateSerializer::Create(GetThisAllocator(), valueStateSerializer, Data::Utilities::UTF8);
    Diagnostics::Validate(status);

    Data::TStore::Store<LONG64, KString::SPtr>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<LONG64, KString::SPtr>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        K_DefaultHashFunction,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);

    storeSPtr->EnableSweep = true;

    stateProvider = storeSPtr.RawPtr();
}

void StoreStateProviderFactory::CreateStringUTF8KeyBufferValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427: TStore needs to take in const KuriView.
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<KStringComparer> comparerSPtr = nullptr;
    status = KStringComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    StringStateSerializer::SPtr keyStateSerializer = nullptr;
    KBufferSerializer::SPtr valueStateSerializer = nullptr;

    status = StringStateSerializer::Create(GetThisAllocator(), keyStateSerializer, Data::Utilities::UTF8);
    Diagnostics::Validate(status);
    status = KBufferSerializer::Create(GetThisAllocator(), valueStateSerializer);
    Diagnostics::Validate(status);

    Data::TStore::Store<KString::SPtr, KBuffer::SPtr>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<KString::SPtr, KBuffer::SPtr>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        K_DefaultHashFunction,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);

    storeSPtr->EnableSweep = true;

    stateProvider = storeSPtr.RawPtr();
}

void StoreStateProviderFactory::CreateNullableStringUTF8KeyBufferValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427: TStore needs to take in const KuriView.
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<KStringComparer> comparerSPtr = nullptr;
    status = KStringComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    NullableStringStateSerializer::SPtr keyStateSerializer = nullptr;
    KBufferSerializer::SPtr valueStateSerializer = nullptr;

    status = NullableStringStateSerializer::Create(GetThisAllocator(), keyStateSerializer, Data::Utilities::UTF8);
    Diagnostics::Validate(status);
    status = KBufferSerializer::Create(GetThisAllocator(), valueStateSerializer);
    Diagnostics::Validate(status);

    Data::TStore::Store<KString::SPtr, KBuffer::SPtr>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<KString::SPtr, KBuffer::SPtr>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        K_DefaultHashFunction,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);

    storeSPtr->EnableSweep = true;

    stateProvider = storeSPtr.RawPtr();
}

void StoreStateProviderFactory::CreateStringUTF16KeyBufferValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427: TStore needs to take in const KuriView.
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<KStringComparer> comparerSPtr = nullptr;
    status = KStringComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    StringStateSerializer::SPtr keyStateSerializer = nullptr;
    KBufferSerializer::SPtr valueStateSerializer = nullptr;

    status = StringStateSerializer::Create(GetThisAllocator(), keyStateSerializer, Data::Utilities::UTF16);
    Diagnostics::Validate(status);
    status = KBufferSerializer::Create(GetThisAllocator(), valueStateSerializer);
    Diagnostics::Validate(status);

    Data::TStore::Store<KString::SPtr, KBuffer::SPtr>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<KString::SPtr, KBuffer::SPtr>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        K_DefaultHashFunction,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);

    storeSPtr->EnableSweep = true;

    stateProvider = storeSPtr.RawPtr();
}

void StoreStateProviderFactory::CreateLongKeyBufferValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427: TStore needs to take in const KuriView.
    KUri::SPtr stateProviderName;
    NTSTATUS status = KUri::Create(const_cast<KUri &>(*factoryArguments.Name), GetThisAllocator(), stateProviderName);
    Diagnostics::Validate(status);

    KSharedPtr<LongComparer> comparerSPtr = nullptr;
    status = LongComparer::Create(GetThisAllocator(), comparerSPtr);
    Diagnostics::Validate(status);

    TestStateSerializer<LONG64>::SPtr keyStateSerializer = nullptr;
    KBufferSerializer::SPtr valueStateSerializer = nullptr;

    status = TestStateSerializer<LONG64>::Create(GetThisAllocator(), keyStateSerializer);
    Diagnostics::Validate(status);
    status = KBufferSerializer::Create(GetThisAllocator(), valueStateSerializer);
    Diagnostics::Validate(status);

    Data::TStore::Store<LONG64, KBuffer::SPtr>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<LONG64, KBuffer::SPtr>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        K_DefaultHashFunction,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);

    storeSPtr->EnableSweep = true;

    stateProvider = storeSPtr.RawPtr();
}

void StoreStateProviderFactory::CreateBufferKeyBufferValueStore(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __out TxnReplicator::IStateProvider2::SPtr & stateProvider)
{
    // #9805427: TStore needs to take in const KuriView.
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

    Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr storeSPtr = nullptr;

    status = Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::Create(
        *PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()),
        *comparerSPtr,
        KBufferComparer::Hash,
        GetThisAllocator(),
        *stateProviderName,
        factoryArguments.StateProviderId,
        *keyStateSerializer,
        *valueStateSerializer,
        storeSPtr);

    Diagnostics::Validate(status);

    storeSPtr->EnableSweep = true;

    stateProvider = storeSPtr.RawPtr();
}



StoreStateProviderFactory::StoreStateProviderFactory(__in FactoryDataType type)
    : KObject()
    , KShared()
    , type_(type)
{
}

StoreStateProviderFactory::~StoreStateProviderFactory()
{
}
