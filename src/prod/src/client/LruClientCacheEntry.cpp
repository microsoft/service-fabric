// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Client;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;
using namespace Naming;

StringLiteral const TraceComponent("LruClientCacheEntry");

NamingUri::Hasher LruClientCacheEntry::KeyHasher;

LruClientCacheEntry::LruClientCacheEntry(
    NamingUri const & name, 
    PartitionedServiceDescriptor && psd)
    : LruCacheEntryBase(name)
    , psd_(move(psd))
    , rspEntries_()
    , rspEntriesLock_()
    , waitersList_()
    , waitersListLock_()
{
    this->Initialize();
}

LruClientCacheEntry::LruClientCacheEntry(
    NamingUri const & name, 
    PartitionedServiceDescriptor const & psd)
    : LruCacheEntryBase(name)
    , psd_(psd)
    , rspEntries_()
    , rspEntriesLock_()
    , waitersList_()
    , waitersListLock_()
{
    this->Initialize();
}

void LruClientCacheEntry::Initialize()
{
    rspEntries_.resize(psd_.Service.PartitionCount);
    waitersList_.resize(psd_.Service.PartitionCount);
}

size_t LruClientCacheEntry::GetHash(NamingUri const & key) 
{ 
    return KeyHasher(key);
}

bool LruClientCacheEntry::AreEqualKeys(NamingUri const & left, NamingUri const & right) 
{ 
    return KeyHasher(left, right);
}

bool LruClientCacheEntry::ShouldUpdateUnderLock(
    LruClientCacheEntry const & existing, 
    LruClientCacheEntry const & incoming)
{
    return (existing.psd_.Version < incoming.psd_.Version);
}

bool LruClientCacheEntry::ShouldUpdateUnderLock(
    ResolvedServicePartitionSPtr const & existing,
    ResolvedServicePartitionSPtr const & incoming)
{
    return (existing->CompareVersionNoValidation(*incoming) < 0);
}

ErrorCode LruClientCacheEntry::TryPutOrGetRsp(
    NamingUri const & fullName,
    __inout ResolvedServicePartitionSPtr & rsp,
    LruClientCacheCallbackSPtr const & callback)
{
    int pIndex = -1;
    bool update = false;
    ResolvedServicePartitionCacheEntrySPtr rspEntry;

    auto error = this->GetPartitionIndex(rsp, pIndex);
    if (!error.IsSuccess()) { return error; }

    {
        AcquireWriteLock lock(rspEntriesLock_);

        auto existing = rspEntries_[pIndex];
        if (existing)
        {
            if (ShouldUpdateUnderLock(existing->UnparsedRsp, rsp))
            {
                update = true;
            }
            else
            {
                ResolvedServicePartitionSPtr parsedExisting;
                if (!existing->TryGetParsedRsp(fullName, parsedExisting))
                {
                    return ErrorCodeValue::UserServiceNotFound;
                }

                this->WriteNoise(
                    TraceComponent, 
                    "Ignoring stale RSP ({0}): {1} < {2}", 
                    pIndex,
                    *rsp,
                    *parsedExisting);

                rsp = parsedExisting;
            }
        }
        else
        {
            update = true;
        }

        if (update)
        {
            rspEntry = make_shared<ResolvedServicePartitionCacheEntry>(rsp);

            // Replaces output parameter 'rsp' with member RSP for service groups
            //
            if (!rspEntry->TryGetParsedRsp(fullName, rsp))
            {
                return ErrorCodeValue::UserServiceNotFound;
            }

            rspEntries_[pIndex] = rspEntry;
        }
    }

    if (update)
    {
        this->UpdateWaitersAndCallback(pIndex, rspEntry, callback);
    }

    return error;
}

ErrorCode LruClientCacheEntry::TryRemoveRsp(
    PartitionKey const & key)
{
    int pIndex = -1;

    auto error = this->GetPartitionIndex(key, pIndex);
    if (!error.IsSuccess()) { return error; }

    {
        AcquireWriteLock lock(rspEntriesLock_);

        if (rspEntries_[pIndex])
        {
            rspEntries_[pIndex].reset();
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode LruClientCacheEntry::TryGetRsp(
    NamingUri const & fullName,
    Naming::PartitionKey const & key, 
    __out Naming::ResolvedServicePartitionSPtr & rsp)
{
    int pIndex = -1;

    auto error = this->GetPartitionIndex(key, pIndex);
    if (!error.IsSuccess()) { return error; }

    {
        AcquireReadLock lock(rspEntriesLock_);

        auto existing = rspEntries_[pIndex];

        if (existing)
        {
            if (!existing->TryGetParsedRsp(fullName, rsp))
            {
                return ErrorCodeValue::UserServiceNotFound;
            }

            return ErrorCodeValue::Success;
        }
    }

    return ErrorCodeValue::NotFound;
}

void LruClientCacheEntry::GetAllRsps(
    __out vector<ResolvedServicePartitionSPtr> & result)
{
    result.clear();

    AcquireReadLock lock(rspEntriesLock_);

    for (auto const & rspEntry : rspEntries_)
    {
        if (rspEntry)
        { 
            // Used by notification, which current performs its own
            // extraction of member RSPs
            //
            result.push_back(rspEntry->UnparsedRsp);
        }
    }
}

AsyncOperationSPtr LruClientCacheEntry::BeginTryGetRsp(
    PartitionKey const & key, 
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    int pIndex = -1;
    WaiterAsyncOperationSPtr waiter;

    auto error = this->GetPartitionIndex(key, pIndex);

    if (error.IsSuccess()) 
    { 
        AcquireReadLock lock(rspEntriesLock_);

        auto existing = rspEntries_[pIndex];

        if (existing)
        {
            waiter = WaiterAsyncOperation::Create(
                existing,
                callback,
                parent);
        }
        else
        {
            waiter = this->AddWaiter(pIndex, timeout, callback, parent);
        }
    }
    else
    {
        waiter = WaiterAsyncOperation::Create(
            error,
            callback,
            parent);
    }

    waiter->StartOutsideLock(waiter);

    return waiter;
}

ErrorCode LruClientCacheEntry::EndTryGetRsp(
    NamingUri const & fullName,
    AsyncOperationSPtr const & operation,
    __out bool & isFirstWaiter,
    __out ResolvedServicePartitionSPtr & rsp)
{
    ResolvedServicePartitionCacheEntrySPtr rspEntry;
    auto error = WaiterAsyncOperation::End(operation, isFirstWaiter, rspEntry);

    if (error.IsSuccess() && !isFirstWaiter)
    {
        // Parsing of service group member RSP occurs on End() since the waiters
        // for different members of the same service group will be waiting
        // on the same RSP entry. Hence each waiter must parse the cached
        // RSP (which is the same) based on its own member information (which is different).
        //
        // e.g. fabric:/SG_Name#Member0 and fabric:/SG_Name#Member1 may parse
        // different member RSPs using the same RSP cache entry for
        // fabric:/SG_Name.
        //
        if (!rspEntry || !rspEntry->TryGetParsedRsp(fullName, rsp))
        {
            error = ErrorCodeValue::UserServiceNotFound;
        }
    }

    return error;
}

AsyncOperationSPtr LruClientCacheEntry::BeginTryInvalidateRsp(
    ResolvedServicePartitionSPtr const & rsp,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    int pIndex = -1;
    WaiterAsyncOperationSPtr waiter;

    auto error = this->GetPartitionIndex(rsp, pIndex);

    if (error.IsSuccess()) 
    {
        AcquireWriteLock lock(rspEntriesLock_);

        auto existing = rspEntries_[pIndex];

        // Can't compare pointers here since the incoming RSP could have been
        // parsed to produce a service group member RSP
        //
        if (!existing || existing->UnparsedRsp->CompareVersionNoValidation(*rsp) <= 0)
        {
            rspEntries_[pIndex].reset();

            waiter = this->AddWaiter(pIndex, timeout, callback, parent);
        }
        else
        {
            waiter = WaiterAsyncOperation::Create(
                existing,
                callback,
                parent);
        }
    }
    else
    {
        waiter = WaiterAsyncOperation::Create(
            error,
            callback,
            parent);
    }

    waiter->StartOutsideLock(waiter);

    return waiter;
}

ErrorCode LruClientCacheEntry::EndTryInvalidateRsp(
    NamingUri const & fullName,
    AsyncOperationSPtr const & operation,
    __out bool & isFirstWaiter,
    __out ResolvedServicePartitionSPtr & rsp)
{
    ResolvedServicePartitionCacheEntrySPtr rspEntry;
    auto error = WaiterAsyncOperation::End(operation, isFirstWaiter, rspEntry);

    if (error.IsSuccess() && !isFirstWaiter)
    {
        if (!rspEntry || !rspEntry->TryGetParsedRsp(fullName, rsp))
        {
            error = ErrorCodeValue::UserServiceNotFound;
        }
    }

    return error;
}

LruClientCacheEntry::WaiterAsyncOperationSPtr LruClientCacheEntry::AddWaiter(
    int pIndex,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    RspWaiterListSPtr list;
    {
        AcquireExclusiveLock lock(waitersListLock_);

        list = waitersList_[pIndex];

        if (!list)
        {
            list = make_shared<RspWaiterList>();

            waitersList_[pIndex] = list;
        }
    }

    return list->AddWaiter(
        timeout,
        callback,
        parent);
}

ErrorCode LruClientCacheEntry::CancelWaiters(PartitionKey const & key)
{
    return this->FailWaiters(key, ErrorCodeValue::OperationCanceled);
}

ErrorCode LruClientCacheEntry::FailWaiters(PartitionKey const & key, ErrorCode const & error)
{
    int pIndex = -1;
    
    auto innerError = this->GetPartitionIndex(key, pIndex);
    if (!innerError.IsSuccess()) { return innerError; }

    auto list = this->TakeWaiters(pIndex);

    if (list)
    {
        list->CompleteWaiters(error);
    }

    return ErrorCodeValue::Success;
}

LruClientCacheEntry::RspWaiterListSPtr LruClientCacheEntry::TakeWaiters(int pIndex)
{
    RspWaiterListSPtr list;
    {
        AcquireExclusiveLock lock(waitersListLock_);

        list = move(waitersList_[pIndex]);
    }

    return list;
}

void LruClientCacheEntry::UpdateWaiters(
    int pIndex, 
    ResolvedServicePartitionCacheEntrySPtr const & rspEntry)
{
    auto list = this->TakeWaiters(pIndex);

    if (list)
    {
        list->CompleteWaiters(rspEntry);
    }
}

void LruClientCacheEntry::UpdateWaitersAndCallback(
    int pIndex, 
    ResolvedServicePartitionCacheEntrySPtr const & rspEntry,
    LruClientCacheCallbackSPtr const & callback)
{
    this->UpdateWaiters(pIndex, rspEntry);

    if (callback)
    {
        // Notification feature currently does its own service group member RSP parsing
        //
        Threadpool::Post([callback, rspEntry] { (*callback)(rspEntry->UnparsedRsp); });
    }
}

bool LruClientCacheEntry::TryGetPartitionKeyFromRsp(
    ResolvedServicePartitionSPtr const & rsp,
    __out PartitionKey & key)
{
    auto const & partitionInfo = rsp->PartitionData;

    switch (partitionInfo.PartitionScheme)
    {
    case FABRIC_PARTITION_SCHEME_SINGLETON:
        key = PartitionKey();
        break;

    case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
        key = PartitionKey(partitionInfo.LowKey);
        break;

    case FABRIC_PARTITION_SCHEME_NAMED:
        key = PartitionKey(partitionInfo.PartitionName);
        break;

    default:
        this->WriteWarning(
            TraceComponent, 
            "Unknown partition scheme for RSP: {0}",
            *rsp);

        return false;
    }

    return true;
}

ErrorCode LruClientCacheEntry::GetPartitionIndex(
    ResolvedServicePartitionSPtr const & rsp,
    __out int & pIndex)
{
    PartitionKey key;
    if (!TryGetPartitionKeyFromRsp(rsp, key))
    {
        return ErrorCodeValue::InvalidServicePartition;
    }
    else
    {
        return this->GetPartitionIndex(key, pIndex);
    }
}

ErrorCode LruClientCacheEntry::GetPartitionIndex(
    PartitionKey const & key,
    __out int & pIndex)
{
    if (psd_.TryGetPartitionIndex(key, pIndex))
    {
        return ErrorCodeValue::Success;
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "Invalid partition key for PSD: {0}, {1}",
            key,
            psd_);

        return ErrorCodeValue::InvalidServicePartition;
    }
}
