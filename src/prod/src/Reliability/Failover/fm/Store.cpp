// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Management::NetworkInventoryManager;
using namespace std;
using namespace Store;

template ErrorCode FailoverManagerStore::LoadAll<vector<NodeInfoSPtr>>(vector<NodeInfoSPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<FailoverUnitUPtr>>(vector<FailoverUnitUPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<ApplicationInfoSPtr>>(vector<ApplicationInfoSPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<ServiceTypeSPtr>>(vector<ServiceTypeSPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<ServiceInfoSPtr>>(vector<ServiceInfoSPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<LoadInfoSPtr>>(vector<LoadInfoSPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<InBuildFailoverUnitUPtr>>(vector<InBuildFailoverUnitUPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<NodeInfoSPtr>>(vector<NodeInfoSPtr> & data) const;

template ErrorCode FailoverManagerStore::UpdateData<ApplicationInfo>(ApplicationInfo & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<ServiceInfo>(ServiceInfo & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<ServiceType>(ServiceType & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<FailoverUnit>(FailoverUnit & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<NodeInfo>(NodeInfo & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<LoadInfo>(LoadInfo & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<InBuildFailoverUnit>(InBuildFailoverUnit & data, __out int64 & commitDuration) const;

template AsyncOperationSPtr FailoverManagerStore::BeginUpdateData<ApplicationInfo>(ApplicationInfo & data, AsyncCallback const& callback, AsyncOperationSPtr const& state) const;
template AsyncOperationSPtr FailoverManagerStore::BeginUpdateData<ServiceInfo>(ServiceInfo & data, AsyncCallback const& callback, AsyncOperationSPtr const& state) const;
template AsyncOperationSPtr FailoverManagerStore::BeginUpdateData<ServiceType>(ServiceType & data, AsyncCallback const& callback, AsyncOperationSPtr const& state) const;
template AsyncOperationSPtr FailoverManagerStore::BeginUpdateData<FailoverUnit>(FailoverUnit & data, AsyncCallback const& callback, AsyncOperationSPtr const& state) const;
template AsyncOperationSPtr FailoverManagerStore::BeginUpdateData<NodeInfo>(NodeInfo & data, AsyncCallback const& callback, AsyncOperationSPtr const& state) const;

template ErrorCode FailoverManagerStore::EndUpdateData<ApplicationInfo>(ApplicationInfo & data, AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::EndUpdateData<ServiceInfo>(ServiceInfo & data, AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::EndUpdateData<ServiceType>(ServiceType & data, AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::EndUpdateData<FailoverUnit>(FailoverUnit & data, AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::EndUpdateData<NodeInfo>(NodeInfo & data, AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const;


template ErrorCode FailoverManagerStore::LoadAll<vector<std::shared_ptr<NIMNetworkNodeAllocationStoreData>>>(vector<std::shared_ptr<NIMNetworkNodeAllocationStoreData>> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<std::shared_ptr<NIMNetworkDefinitionStoreData>>>(vector<std::shared_ptr<NIMNetworkDefinitionStoreData>> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<NIMNetworkMACAddressPoolStoreDataSPtr>>(vector<NIMNetworkMACAddressPoolStoreDataSPtr> & data) const;
template ErrorCode FailoverManagerStore::LoadAll<vector<NIMNetworkIPv4AddressPoolStoreDataSPtr>>(vector<NIMNetworkIPv4AddressPoolStoreDataSPtr> & data) const;

template ErrorCode FailoverManagerStore::UpdateData<NIMNetworkNodeAllocationStoreData>(NIMNetworkNodeAllocationStoreData & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<NIMNetworkDefinitionStoreData>(NIMNetworkDefinitionStoreData & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<NIMNetworkMACAddressPoolStoreData>(NIMNetworkMACAddressPoolStoreData & data, __out int64 & commitDuration) const;
template ErrorCode FailoverManagerStore::UpdateData<NIMNetworkIPv4AddressPoolStoreData>(NIMNetworkIPv4AddressPoolStoreData & data, __out int64 & commitDuration) const;


StringLiteral const TraceStore("Store");

size_t const FailoverManagerStore::StoreDataBufferSize(Reliability::Constants::FMStoreBufferSize);

wstring const FailoverManagerStore::ApplicationType(L"ApplicationType");
wstring const FailoverManagerStore::ServiceInfoType(L"ServiceInfoType");
wstring const FailoverManagerStore::ServiceTypeType(L"ServiceTypeType");
wstring const FailoverManagerStore::FailoverUnitType(L"FailoverUnitType");
wstring const FailoverManagerStore::NodeInfoType(L"NodeInfoType");
wstring const FailoverManagerStore::LoadInfoType(L"LoadInfoType");
wstring const FailoverManagerStore::InBuildFailoverUnitType(L"InBuildFailoverUnitType");
wstring const FailoverManagerStore::FabricVersionInstanceType(L"FabricVersionInstanceType");
wstring const FailoverManagerStore::FabricUpgradeType(L"FabricUpgradeType");
wstring const FailoverManagerStore::StringType(L"StringType");

wstring const FailoverManagerStore::FabricVersionInstanceKey(L"FabricVersionInstanceKey");
wstring const FailoverManagerStore::FabricUpgradeKey(L"FabricUpgradeKey");

wstring const FailoverManagerStore::NetworkDefinitionType(L"NetworkDefinitionType");
wstring const FailoverManagerStore::NetworkNodeType(L"NetworkNodeType");
wstring const FailoverManagerStore::NetworkMACAddressPoolType(L"NetworkMACAddressPoolType");
wstring const FailoverManagerStore::NetworkIPv4AddressPoolType(L"NetworkIPv4AddressPoolType");



struct BasicStringKeyBody : public Serialization::FabricSerializable
{
    BasicStringKeyBody() { }

    FABRIC_FIELDS_01(key_);

    wstring key_;
};

class FailoverManagerStore::IReplicatedStoreHolder : public ComponentRoot
{
private:
    IReplicatedStoreHolder(IReplicatedStoreUPtr && replicatedStore)
        : ComponentRoot()
        , replicatedStore_(move(replicatedStore))
    {
    }

public:

    static RootedObjectPointer<IReplicatedStore> CreateRootedObjectPointer(IReplicatedStoreUPtr && replicatedStore)
    {
        auto holder = shared_ptr<IReplicatedStoreHolder>(new IReplicatedStoreHolder(move(replicatedStore)));
        return RootedObjectPointer<IReplicatedStore>(holder->replicatedStore_.get(), holder->CreateComponentRoot());
    }

private:
    IReplicatedStoreUPtr replicatedStore_;
};

FailoverManagerStore::FailoverManagerStore(RootedObjectPointer<IReplicatedStore> && replicatedStore)
    : storeSPtr_(make_shared<RootedStore>(move(replicatedStore)))
    , storeWPtr_(storeSPtr_)
{
}

FailoverManagerStore::FailoverManagerStore(IReplicatedStoreUPtr && replicatedStore)
    : storeSPtr_(make_shared<RootedStore>(IReplicatedStoreHolder::CreateRootedObjectPointer(move(replicatedStore))))
    , storeWPtr_(storeSPtr_)
{
}

bool FailoverManagerStore::get_IsThrottleNeeded() const 
{ 
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return false; }
    auto rootedStore = *rootedStoreSPtr;

    return rootedStore->IsThrottleNeeded(); 
}

ErrorCode FailoverManagerStore::BeginTransaction(IStoreBase::TransactionSPtr & txSPtr) const
{
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    return static_cast<IStoreBase*>(rootedStore.get())->CreateTransaction(txSPtr);
}

ErrorCode FailoverManagerStore::BeginSimpleTransaction(IStoreBase::TransactionSPtr & txSPtr) const
{
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    return rootedStore->CreateSimpleTransaction(txSPtr);
}

template <class T>
ErrorCode FailoverManagerStore::LoadAll(T & data) const
{
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    wstring type = GetCollectionTraits<T>::GetStoreType();

    IStoreBase::TransactionSPtr tx;
    error = BeginTransaction(tx);
    if (error.IsSuccess())
    {
        IStoreBase::EnumerationSPtr enumeration;
        error = rootedStore->CreateEnumerationByTypeAndKey(tx, type, L"", enumeration);

        bool done = !error.IsSuccess();
        while (!done)
        {
            error = enumeration->MoveNext();
            if (error.IsSuccess())
            {
                error = GetCollectionTraits<T>::Add(data, enumeration);
            }
            
            if (!error.IsSuccess())
            {
                done = true;

                if (error.ReadValue() == ErrorCodeValue::EnumerationCompleted)
                {
                    error = tx->Commit();
                }
            }
        }

        if (!error.IsSuccess())
        {
            tx->Rollback();
        }
    }

    return error;
}

template <class T>
ErrorCode FailoverManagerStore::UpdateData(T & data, __out int64 & commitDuration) const
{
    Stopwatch stopwatch;
    stopwatch.Start();

    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginSimpleTransaction(tx);
    if (error.IsSuccess())
    {
        error = UpdateData(tx, data);

        int64 operationLSN = 0;

        if (error.IsSuccess())
        {
            error = tx->Commit(operationLSN);
        }

        if (error.IsSuccess())
        {
            data.PostCommit(operationLSN);
        }
        else
        {
            tx->Rollback();
        }
    }

    stopwatch.Stop();
    commitDuration = stopwatch.ElapsedMilliseconds;

    return error;
}

template <class T>
ErrorCode FailoverManagerStore::UpdateData(IStoreBase::TransactionSPtr const& tx, T & data) const
{
    ErrorCode error(ErrorCodeValue::Success);

    __if_exists(T::PostUpdate)
    {
        data.PostUpdate(DateTime::Now());
    }

    if (data.PersistenceState == PersistenceState::ToBeInserted)
    {
        error = TryInsertData<T>(tx, data, T::GetStoreType(), data.GetStoreKey());
    }
    else if (data.PersistenceState == PersistenceState::ToBeUpdated)
    {
        error = TryUpdateData<T>(tx, data, T::GetStoreType(), data.GetStoreKey(), data.OperationLSN);
    }
    else if (data.PersistenceState == PersistenceState::ToBeDeleted)
    {
        error = TryDeleteData(tx, T::GetStoreType(), data.GetStoreKey(), data.OperationLSN);
    }
    else
    {
        Assert::CodingError("Invalid PersistenceState: {0}", data.PersistenceState);
    }

    return error;
}

template <typename TData>
AsyncOperationSPtr FailoverManagerStore::BeginUpdateData(
    TData & data,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state) const
{
    Stopwatch stopwatch;
    stopwatch.Start();

    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginSimpleTransaction(tx);
    
    if (error.IsSuccess())
    {
        ASSERT_IF(tx.get() == nullptr, "Transaction is null.");

        error = UpdateData(tx, data);
        if (error.IsSuccess())
        {
            auto transaction = tx.get();
            return transaction->BeginCommit(TimeSpan::MaxValue, OperationContextCallback<pair<IStoreBase::TransactionSPtr, Stopwatch>>(callback, make_pair(move(tx), stopwatch)), state);
        }

        tx->Rollback();
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
}

template <typename TData>
ErrorCode FailoverManagerStore::EndUpdateData(TData & data, AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const
{
    unique_ptr<OperationContext<pair<IStoreBase::TransactionSPtr, Stopwatch>>> context = updateOperation->PopOperationContext<pair<IStoreBase::TransactionSPtr, Stopwatch>>();
    if (context)
    {
        int64 operationLSN = 0;

        ErrorCode error = context->Context.first->EndCommit(updateOperation, operationLSN);

        if (error.IsSuccess())
        {
            data.PostCommit(operationLSN);
        }

        context->Context.second.Stop();
        commitDuration = context->Context.second.ElapsedMilliseconds;

        return error;
    }
    
    return CompletedAsyncOperation::End(updateOperation);
}

ErrorCode FailoverManagerStore::GetKeyValues(__out map<wstring, wstring>& keyValues) const
{
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    IStoreBase::TransactionSPtr txSPtr;
    IStoreBase::EnumerationSPtr enumSPtr;

    error = BeginTransaction(txSPtr);
    if (error.IsSuccess())
    {
        error = rootedStore->CreateEnumerationByTypeAndKey(txSPtr, StringType, L"", enumSPtr);
    }

	if (error.IsSuccess())
	{
        bool done = false;
        while (!done)
        {
            error = enumSPtr->MoveNext();
            if (error.IsSuccess())
            {
                wstring key;
                if (!(error = enumSPtr->CurrentKey(key)).IsSuccess())
                {
                    break;
                }

                BasicStringKeyBody body;
                if (!(error = ReadCurrentData(enumSPtr, body)).IsSuccess())
                {
                    break;
                }
                keyValues.insert(pair<wstring, wstring>(key, body.key_));
            }
            else
            {
                done = true;
            }
        }

        if(error.IsSuccess() || error.ReadValue() == ErrorCodeValue::EnumerationCompleted)
        {
            error = txSPtr->Commit();
        }

        if (!error.IsSuccess())
        {
            txSPtr->Rollback();
        }
    }

    return error;
}

ErrorCode FailoverManagerStore::UpdateKeyValue(wstring const& key, wstring const& value) const
{
    IStoreBase::TransactionSPtr txSPtr;
    ErrorCode error = BeginTransaction(txSPtr);

    if (error.IsSuccess())
    {
        BasicStringKeyBody existing;
        BasicStringKeyBody body;
        body.key_ = value;

        error = InternalGetData<BasicStringKeyBody>(txSPtr, StringType, key, existing);
        if (error.IsSuccess())
        {
            error = TryUpdateData(txSPtr, body, StringType, key, 0);
        }
        else if (error.IsError(ErrorCodeValue::FMStoreKeyNotFound))
        {
            error = TryInsertData(txSPtr, body, StringType, key);
        }

        if (error.IsSuccess())
        {
            error = txSPtr->Commit();
        }

        if (!error.IsSuccess())
        {
            txSPtr->Rollback();
        }
    }

    return error;
}

ErrorCode FailoverManagerStore::GetFailoverUnit(FailoverUnitId const& failoverUnitId, FailoverUnitUPtr & failoverUnit) const
{
    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginTransaction(tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = InternalGetFailoverUnit(tx, failoverUnitId, failoverUnit);
    if (!error.IsSuccess() && error.ReadValue() == ErrorCodeValue::FMStoreKeyNotFound)
    {
        return ErrorCode(ErrorCodeValue::FMFailoverUnitNotFound);
    }

    return error;
}

ErrorCode FailoverManagerStore::InternalGetFailoverUnit(
    IStoreBase::TransactionSPtr const& tx,
    FailoverUnitId const& failoverUnitId,
    FailoverUnitUPtr & failoverUnit) const
{
    wstring idString;
    StringWriter(idString).Write(failoverUnitId);

    FailoverUnit data;
    ErrorCode error = this->InternalGetData(tx, FailoverUnitType, idString, data);

    if (error.IsSuccess())
    {
        failoverUnit = make_unique<FailoverUnit>(move(data));
    }

    return error;
}

ErrorCode FailoverManagerStore::UpdateFailoverUnits(IStoreBase::TransactionSPtr const& txSPtr, vector<FailoverUnitUPtr> const& failoverUnits) const
{
    ErrorCode error(ErrorCodeValue::Success);

    for (FailoverUnitUPtr const& failoverUnit : failoverUnits)
    {
        error = UpdateData(txSPtr, *failoverUnit);
        if (!error.IsSuccess())
        {
            break;
        }
    }

    return error;
}

AsyncOperationSPtr FailoverManagerStore::BeginUpdateServiceAndFailoverUnits(
    ApplicationInfoSPtr const& applicationInfo,
    ServiceTypeSPtr const& serviceType,
    ServiceInfo & serviceInfo,
    vector<FailoverUnitUPtr> const& failoverUnits,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state) const
{
    Stopwatch stopwatch;
    stopwatch.Start();

    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginTransaction(tx);

    if (error.IsSuccess())
    {
        if (applicationInfo != nullptr)
        {
            error = UpdateData(tx, *applicationInfo);
        }

        if (error.IsSuccess() && serviceType != nullptr)
        {
            error = UpdateData(tx, *serviceType);
        }

        if (error.IsSuccess())
        {
            error = UpdateData(tx, serviceInfo);

            if (error.IsSuccess())
            {
                error = UpdateFailoverUnits(tx, failoverUnits);

                if (error.IsSuccess())
                {
                    auto transaction = tx.get();
                    return transaction->BeginCommit(TimeSpan::MaxValue, OperationContextCallback<pair<IStoreBase::TransactionSPtr, Stopwatch>>(callback, make_pair(move(tx), stopwatch)), state);
                }
            }
        }

        if (!error.IsSuccess())
        {
            tx->Rollback();
        }
    }

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
}

ErrorCode FailoverManagerStore::EndUpdateServiceAndFailoverUnits(
    AsyncOperationSPtr const& operation,
    ApplicationInfoSPtr const& applicationInfo,
    ServiceTypeSPtr const& serviceType,
    ServiceInfo & serviceInfo,
    vector<FailoverUnitUPtr> const& failoverUnits,
    __out int64 & commitDuration) const
{
    unique_ptr<OperationContext<pair<IStoreBase::TransactionSPtr, Stopwatch>>> context = operation->PopOperationContext<pair<IStoreBase::TransactionSPtr, Stopwatch>>();

    if (context)
    {
        int64 operationLSN = 0;

        ErrorCode error = context->Context.first->EndCommit(operation, operationLSN);

        if (error.IsSuccess())
        {
            if (applicationInfo)
            {
                applicationInfo->PostCommit(operationLSN);
            }

            if (serviceType)
            {
                serviceType->PostCommit(operationLSN);
            }

            serviceInfo.PostCommit(operationLSN);

            for (FailoverUnitUPtr const& failoverUnit : failoverUnits)
            {
                failoverUnit->PostCommit(operationLSN);
            }
        }

        context->Context.second.Stop();
        commitDuration = context->Context.second.ElapsedMilliseconds;

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

ErrorCode FailoverManagerStore::DeleteInBuildFailoverUnitAndUpdateFailoverUnit(InBuildFailoverUnitUPtr const& inBuildFailoverUnit, FailoverUnitUPtr const& failoverUnit) const
{
    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginSimpleTransaction(tx);

    if (error.IsSuccess())
    {
        if (inBuildFailoverUnit)
        {
            wstring failoverUnitId = inBuildFailoverUnit->IdString;
            error = TryDeleteData(tx, InBuildFailoverUnitType, failoverUnitId, inBuildFailoverUnit->OperationLSN);
        }

        int64 operationLSN = 0;

        if (error.IsSuccess())
        {
            error = UpdateData(tx, *failoverUnit);

            if (error.IsSuccess())
            {
                error = tx->Commit(operationLSN);
            }
        }

        if (error.IsSuccess())
        {
            failoverUnit->PostCommit(operationLSN);
        }
        else
        {
            tx->Rollback();
        }
    }

    return error;
}

ErrorCode FailoverManagerStore::UpdateFabricVersionInstance(FabricVersionInstance const& versionInstance) const
{
    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginTransaction(tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    FabricVersionInstance oldVersionInstance;
    error = GetFabricVersionInstance(tx, oldVersionInstance);

    // We do not need the store to provide versioning for FabricVersionInstance.
    // We achieve this using the InstanceId portion of FabricVersionInstance.
    if (!error.IsSuccess() && error.ReadValue() == ErrorCodeValue::FMStoreKeyNotFound)
    {
        // Insert
        error = TryInsertData<FabricVersionInstance>(tx, versionInstance, FabricVersionInstanceType, FabricVersionInstanceKey);
    }
    else if (error.IsSuccess())
    {
        // Update
        if (oldVersionInstance.InstanceId <= versionInstance.InstanceId)
        {
            error = TryUpdateData<FabricVersionInstance>(tx, versionInstance, FabricVersionInstanceType, FabricVersionInstanceKey, ILocalStore::SequenceNumberIgnore);
        }
        else
        {
            error = ErrorCode(ErrorCodeValue::Success);
        }
    }

    if (error.IsSuccess())
    {
        error = tx->Commit();
    }
    else
    {
        tx->Rollback();
    }

    return error;
}

ErrorCode FailoverManagerStore::GetFabricVersionInstance(FabricVersionInstance & versionInstance) const
{
    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginTransaction(tx);
    
    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetFabricVersionInstance(tx, versionInstance);
    
    if (error.IsSuccess())
    {
        tx->Commit();
    }
    else
    {
        tx->Rollback();
    }

    return error;
}

ErrorCode FailoverManagerStore::GetFabricVersionInstance(IStoreBase::TransactionSPtr const& tx, FabricVersionInstance & versionInstance) const
{
    return this->InternalGetData(tx, FabricVersionInstanceType, FabricVersionInstanceKey, versionInstance);
}

ErrorCode FailoverManagerStore::UpdateFabricUpgrade(FabricUpgrade const& upgrade) const
{
    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginSimpleTransaction(tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (upgrade.PersistenceState == PersistenceState::ToBeInserted)
    {
        error = TryInsertData<FabricUpgrade>(tx, upgrade, FabricUpgradeType, FabricUpgradeKey);
    }
    else if (upgrade.PersistenceState == PersistenceState::ToBeUpdated)
    {
        error = TryUpdateData<FabricUpgrade>(tx, upgrade, FabricUpgradeType, FabricUpgradeKey, ILocalStore::SequenceNumberIgnore);
    }
    else if (upgrade.PersistenceState == PersistenceState::ToBeDeleted)
    {
        error = TryDeleteData(tx, FabricUpgradeType, FabricUpgradeKey, ILocalStore::SequenceNumberIgnore);
    }
    else
    {
        Assert::CodingError("Invalid PersistenceState: {0}", upgrade.PersistenceState);
    }

    if (error.IsSuccess())
    {
        error = tx->Commit();
    }
    else
    {
        tx->Rollback();
    }

    return error;
}

ErrorCode FailoverManagerStore::GetFabricUpgrade(FabricUpgradeUPtr & upgrade) const
{
    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginTransaction(tx);
    
    if (!error.IsSuccess())
    {
        return error;
    }

    FabricUpgrade fabricUpgrade;
    error = GetFabricUpgrade(tx, fabricUpgrade);
    
    if (error.IsSuccess())
    {
        upgrade = make_unique<FabricUpgrade>(move(fabricUpgrade));
        tx->Commit();
    }
    else
    {
        tx->Rollback();
    }

    return error;
}

ErrorCode FailoverManagerStore::GetFabricUpgrade(IStoreBase::TransactionSPtr const& tx, FabricUpgrade & upgrade) const
{
    return this->InternalGetData(tx, FabricUpgradeType, FabricUpgradeKey, upgrade);
}

ErrorCode FailoverManagerStore::UpdateNodes(vector<NodeInfoSPtr> & nodes) const
{
    if (nodes.empty())
    {
        return ErrorCodeValue::Success;
    }

    IStoreBase::TransactionSPtr tx;
    ErrorCode error = BeginSimpleTransaction(tx);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (NodeInfoSPtr const& node : nodes)
    {
        error = UpdateData(tx, *node);
        if (!error.IsSuccess())
        {
            break;
        }
    }

    int64 operationLSN = 0;

    if (error.IsSuccess())
    {
        error = tx->Commit(operationLSN);
    }

    if (error.IsSuccess())
    {
        for (auto it = nodes.begin(); it != nodes.end(); it++)
        {
            (*it)->PostCommit(operationLSN);
        }
    }
    else
    {
        tx->Rollback();
    }

    return error;
}

template <class T>
ErrorCode FailoverManagerStore::InternalGetData(
    IStoreBase::TransactionSPtr const& tx,
    wstring const & type,
    wstring const & key,
    T & result) const
{
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    vector<byte> buffer;
    _int64 operationLsn;
    error = rootedStore->ReadExact(tx, type, key, buffer, operationLsn);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCodeValue::FMStoreKeyNotFound;
    }

    if (error.IsSuccess())
    {
        error = FabricSerializer::Deserialize(result, static_cast<ULONG>(buffer.size()), buffer.data());

        __if_exists(T::PostRead)
        {
            if (error.IsSuccess())
            {
                result.PostRead(operationLsn);
            }
        }
    }

    return error;
}

template <class T>
ErrorCode FailoverManagerStore::TryInsertData(
    IStoreBase::TransactionSPtr const & tx, 
    T const & data, 
    wstring const & type,
    wstring const & key) const
{                       
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    // TODO: Consider creating a buffer pool for performance
    vector<byte> buffer;
    buffer.reserve(StoreDataBufferSize);

    error = FabricSerializer::Serialize(&data, buffer);
    if (error.IsSuccess())
    {
        error = rootedStore->Insert(
            tx, 
            type,
            key,
            &(*buffer.begin()), 
            buffer.size());
    }

    return error;
}

template <class T>
ErrorCode FailoverManagerStore::TryUpdateData(
    IStoreBase::TransactionSPtr const & tx, 
    T const & data, 
    wstring const & type, 
    wstring const & key, 
    int64 currentSequenceNumber) const
{                       
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    // TODO: Consider creating a buffer pool for performance
    vector<byte> buffer;
    buffer.reserve(StoreDataBufferSize);

    error = FabricSerializer::Serialize(&data, buffer);
    if (error.IsSuccess())
    {
        error = rootedStore->Update(
            tx, 
            type,
            key,
            currentSequenceNumber,
            key,
            &(*buffer.begin()), 
            buffer.size());
    }
    return error;
}

ErrorCode FailoverManagerStore::TryDeleteData(
    IStoreBase::TransactionSPtr const & tx, 
    wstring const & type, 
    wstring const & key, 
    int64 currentSequenceNumber) const
{                       
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (!error.IsSuccess()) { return error; }
    auto rootedStore = *rootedStoreSPtr;

    return rootedStore->Delete(
        tx, 
        type,
        key,
        currentSequenceNumber);
}

template <class T>
ErrorCode FailoverManagerStore::ReadCurrentData(IStoreBase::EnumerationSPtr const & enumeration, __out T & result)
{
    vector<byte> bytes;
    bytes.reserve(StoreDataBufferSize);
    ErrorCode error;
    if (!(error = enumeration->CurrentValue(bytes)).IsSuccess())
    {
        return error;
    }

    int64 operationLSN;
    if (!(error = enumeration->CurrentOperationLSN(operationLSN)).IsSuccess())
    {
        return error;
    }

    error = FabricSerializer::Deserialize(result, static_cast<ULONG>(bytes.size()), bytes.data());

    __if_exists(T::PostRead)
    {
        if (error.IsSuccess())
        {
            result.PostRead(operationLSN);
        }
    }

    return error;
}

ErrorCode FailoverManagerStore::DeleteStore(unique_ptr<ILocalStore> & localStore)
{
    wstring emptyKey = L"";
    _int64 checkSequenceNumber = 0;
    IStoreBase::TransactionSPtr txSPtr;
    ErrorCode error = localStore->CreateTransaction(txSPtr);

    if (!error.IsSuccess())
    {
        return error;
    }

    error = localStore->Delete(txSPtr, ServiceInfoType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, FailoverUnitType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, NodeInfoType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, LoadInfoType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, StringType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, InBuildFailoverUnitType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, ServiceTypeType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, ApplicationType, emptyKey, checkSequenceNumber);
    if(!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, FabricVersionInstanceType, emptyKey, checkSequenceNumber);
    if (!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = localStore->Delete(txSPtr, FabricUpgradeType, emptyKey, checkSequenceNumber);
    if (!error.IsSuccess()) 
    {
        txSPtr->Rollback();
        return error;
    }

    error = txSPtr->Commit();

    return error;
}

void FailoverManagerStore::Dispose(bool isStoreCloseNeeded)
{
    RootedStoreSPtr rootedStoreSPtr;
    auto error = this->TryGetStore(rootedStoreSPtr);
    if (error.IsSuccess() && isStoreCloseNeeded) 
    {
        auto rootedStore = *rootedStoreSPtr;

        rootedStore->Close();
    }

    storeSPtr_.reset();
}

ErrorCode FailoverManagerStore::TryGetStore(__out RootedStoreSPtr & result) const
{
    result = storeWPtr_.lock();

    return (result.get() == nullptr) ? ErrorCodeValue::FMStoreNotUsable : ErrorCodeValue::Success;
}
