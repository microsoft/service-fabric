// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Store;
using namespace Management::FileStoreService;
using namespace Serialization;
using namespace Api;
using namespace Naming;

StringLiteral const TraceComponent("ReplicatedStoreWrapper");

ReplicatedStoreWrapper::ReplicatedStoreWrapper(RequestManager & root)
    : RootedObject(root)
    , owner_(root)
{
}

ReplicatedStoreWrapper::~ReplicatedStoreWrapper()
{
}

ErrorCode ReplicatedStoreWrapper::CreateTransaction(ActivityId const & activityId, StoreTransactionUPtr & tx)
{
    uint64 currentState = owner_.GetState();
    if(currentState == RequestManager::Deactivated ||
        currentState == RequestManager::Failed ||
        currentState == RequestManager::Aborted)
    {
        return ErrorCodeValue::FileStoreServiceNotReady;
    }
    else
    {
        StoreTransaction tempStoreTx = StoreTransaction::Create(owner_.ReplicaObj.ReplicatedStore, owner_.ReplicaObj.PartitionedReplicaId, activityId);
        tx = make_unique<StoreTransaction>(move(tempStoreTx));
        return ErrorCodeValue::Success;
    }
}

ErrorCode ReplicatedStoreWrapper::CreateSimpleTransaction(ActivityId const & activityId, StoreTransactionUPtr & tx)
{
    uint64 currentState = owner_.GetState();
    if(currentState == RequestManager::Deactivated ||
        currentState == RequestManager::Failed ||
        currentState == RequestManager::Aborted)
    {
        return ErrorCodeValue::FileStoreServiceNotReady;
    }
    else
    {
        StoreTransaction tempStoreTx = StoreTransaction::CreateSimpleTransaction(owner_.ReplicaObj.ReplicatedStore, owner_.ReplicaObj.PartitionedReplicaId, activityId);
        tx = make_unique<StoreTransaction>(move(tempStoreTx));
        return ErrorCodeValue::Success;
    }
}

ErrorCode ReplicatedStoreWrapper::ReadExact(ActivityId const & activityId, __inout Store::StoreData & storeData)
{
    StoreTransactionUPtr tx;
    auto error = this->CreateTransaction(activityId, tx);

    if (error.IsSuccess()) 
    {
        error = this->ReadExactImpl(*tx, storeData);

        tx->CommitReadOnly();
    }

    return error;
}

ErrorCode ReplicatedStoreWrapper::ReadExact(
    Store::StoreTransaction const & storeTx, 
    __inout Store::StoreData & storeData)
{
    if (FileStoreServiceConfig::GetConfig().EnableTStore || Store::StoreConfig::GetConfig().EnableTStore)
    {
        return this->ReadExactImpl(storeTx, storeData);
    }
    else
    {
        // In the V1 stack, simple transactions are used to
        // batch writes. Since simple transactions do not support
        // reads, we need to create a separate transaction
        // for reading.
        //
        return this->ReadExact(storeTx.ActivityId, storeData);
    }
}

ErrorCode ReplicatedStoreWrapper::ReadExactImpl(
    Store::StoreTransaction const & storeTx, 
    __inout Store::StoreData & storeData)
{
    return storeTx.ReadExact(storeData);
}


ErrorCode ReplicatedStoreWrapper::TryReadOrInsertIfNotFound(
    ActivityId const & activityId, 
    Store::StoreTransaction const & storeTx, 
    __inout Store::StoreData & result, 
    __out bool & readExisting)
{
    ErrorCode error = this->ReadExact(storeTx, result);

    if(error.IsError(ErrorCodeValue::NotFound))
    {
        error = storeTx.Insert(result);
        WriteTrace(
            error.ToLogLevel(),
            TraceComponent,
            "{0}: Insert:{1}, Error:{2}",
            activityId,
            result.Key,
            error);

        readExisting = false;
    }
    else if(error.IsSuccess())
    {
        readExisting = true;
    }

    return error;
}

