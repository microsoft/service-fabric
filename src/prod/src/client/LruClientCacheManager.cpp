// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Client;
using namespace ClientServerTransport;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Naming;

StringLiteral const TraceComponent("LruClientCacheManager");

LruClientCacheManager::LruClientCacheManager(
    __in FabricClientImpl & client,
    __in FabricClientInternalSettings & settings)
    : client_(client)
    , cache_(
        settings.PartitionLocationCacheLimit, 
        settings.PartitionLocationCacheBucketCount)
    , prefixCache_(cache_)
    , callbacks_(settings.PartitionLocationCacheBucketCount)
    , callbacksLock_()
{
}

LruClientCacheManager::~LruClientCacheManager()
{
}

AsyncOperationSPtr LruClientCacheManager::BeginGetPsd(
    NamingUri const & name,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<LruClientCacheManager::GetPsdAsyncOperation>(
        *this,
        name,
        activityHeader,
        timeout,
        callback,
        parent);
}

ErrorCode LruClientCacheManager::EndGetPsd(
    AsyncOperationSPtr const & operation, 
    __out  PartitionedServiceDescriptor & psd)
{
    return LruClientCacheManager::GetPsdAsyncOperation::End(operation, psd);
}

AsyncOperationSPtr LruClientCacheManager::BeginResolveService(
    NamingUri const & name,
    ServiceResolutionRequestData const & request, 
    ResolvedServicePartitionMetadataSPtr const & previousResultMetadata,
    FabricActivityHeader && activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<LruClientCacheManager::ResolveServiceAsyncOperation>(
        *this,
        name,
        request,
        previousResultMetadata,
        move(activityHeader),
        timeout,
        callback,
        parent);
}

ErrorCode LruClientCacheManager::EndResolveService(
    AsyncOperationSPtr const & operation, 
    __out ResolvedServicePartitionSPtr & rsp,
    __out ActivityId & activityId)
{
    return LruClientCacheManager::ResolveServiceAsyncOperation::End(operation, rsp, activityId);
}

AsyncOperationSPtr LruClientCacheManager::BeginPrefixResolveService(
    NamingUri const & name,
    ServiceResolutionRequestData const & request, 
    ResolvedServicePartitionMetadataSPtr const & previousResultMetadata,
    bool bypassCache,
    FabricActivityHeader && activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<LruClientCacheManager::PrefixResolveServiceAsyncOperation>(
        *this,
        name,
        request,
        previousResultMetadata,
        bypassCache,
        move(activityHeader),
        timeout,
        callback,
        parent);
}

ErrorCode LruClientCacheManager::EndPrefixResolveService(
    AsyncOperationSPtr const & operation, 
    __out ResolvedServicePartitionSPtr & rsp,
    __out ActivityId & activityId)
{
    return LruClientCacheManager::PrefixResolveServiceAsyncOperation::End(operation, rsp, activityId);
}


LruClientCacheCallbackSPtr LruClientCacheManager::GetCacheCallback(
    NamingUri const & name)
{
    AcquireReadLock lock(callbacksLock_);

    auto it = callbacks_.find(name);
    if (it != callbacks_.end())
    {
        return it->second;
    }

    return LruClientCacheCallbackSPtr();
}

void LruClientCacheManager::ClearCacheCallbacks()
{
    AcquireWriteLock lock(callbacksLock_);

    callbacks_.clear();
}

void LruClientCacheManager::EnsureCacheCallback(
    NamingUri const & name, 
    LruClientCacheCallback::RspUpdateCallback const & callback)
{
    AcquireWriteLock lock(callbacksLock_);

    auto it = callbacks_.find(name);
    if (it == callbacks_.end())
    {
        auto handler = make_shared<LruClientCacheCallback>(callback);

        auto inner = callbacks_.insert(make_pair(
            name,
            handler));

        if (!inner.second)
        {
            inner.first->second->IncrementRefCount();

            this->WriteWarning(TraceComponent, "LruClientCacheCallback already exists for '{0}'", name);
            Assert::TestAssert();
        }
    }
    else
    {
        it->second->IncrementRefCount();
    }
}

void LruClientCacheManager::ReleaseCacheCallback(
    NamingUri const & name) 
{
    AcquireWriteLock lock(callbacksLock_);

    auto it = callbacks_.find(name);
    if (it != callbacks_.end())
    {
        auto count = it->second->DecrementRefCount();
        if (count < 0)
        {
            this->WriteWarning(
                TraceComponent,
                "Invalid LruClientCacheCallback ref-count for '{0}': count={1}", 
                name, 
                count);
            Assert::TestAssert();
        }

        if (count <= 0)
        {

            callbacks_.erase(it);
        }
    }
}

bool LruClientCacheManager::TryUpdateOrGetPsdCacheEntry(
    NamingUri const & name,
    PartitionedServiceDescriptor const & psd,
    ActivityId const &,
    __out LruClientCacheEntrySPtr & cacheEntry)
{
    cacheEntry = make_shared<LruClientCacheEntry>(name, psd);

    return cache_.TryPutOrGet(cacheEntry);
}

AsyncOperationSPtr LruClientCacheManager::BeginUpdateCacheEntries(
    multimap<NamingUri, ResolvedServicePartitionSPtr> && rsps,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ParallelUpdateCacheEntriesAsyncOperation>(
        *this,
        move(rsps),
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode LruClientCacheManager::EndUpdateCacheEntries(
    AsyncOperationSPtr const & operation,
    __out vector<Naming::ResolvedServicePartitionSPtr> & result)
{
    return ParallelUpdateCacheEntriesAsyncOperation::End(operation, result);
}

bool LruClientCacheManager::TryGetEntry(
    NamingUri const & name,
    PartitionKey const & key,
    __out ResolvedServicePartitionSPtr & rsp)
{
    LruClientCacheEntrySPtr cacheEntry;
    if (cache_.TryGet(name, cacheEntry))
    {
        // Incorrect notification registrations shouldn't 
        // invalidate cache entry on read.
        //
        return cacheEntry->TryGetRsp(name, key, rsp).IsSuccess();
    }

    return false;
}

void LruClientCacheManager::GetAllEntriesForName(
    NamingUri const & name, 
    __out vector<ResolvedServicePartitionSPtr> & result)
{
    LruClientCacheEntrySPtr cacheEntry;
    if (cache_.TryGet(name, cacheEntry))
    {
        cacheEntry->GetAllRsps(result);
    }
}

void LruClientCacheManager::ClearCacheEntriesWithName(
    NamingUri const & name,
    ActivityId const &)
{
    cache_.TryRemove(name);
}

void LruClientCacheManager::UpdateCacheOnError(
    NamingUri const & name,
    PartitionKey const & key,
    ActivityId const & activityId,
    ErrorCode const error)
{
    if (ErrorIndicatesInvalidService(error))
    {
        // All the entries for this service are invalid,
        // so remove them from cache.
        //
        this->ClearCacheEntriesWithName(name, activityId);
    }
    else if (ErrorIndicatesInvalidPartition(error))
    {
        // Remove the cache entry for this partition;
        // Keep the rest of the entries for this names, 
        // as that information may be still valid.
        //
        LruClientCacheEntrySPtr cacheEntry;
        if (cache_.TryGet(name, cacheEntry))
        {
            cacheEntry->TryRemoveRsp(key);
        }
    }
}

AsyncOperationSPtr LruClientCacheManager::BeginUpdateCacheEntries(
    __in ServiceNotification & notification,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessServiceNotificationAsyncOperation>(
        *this,
        notification,
        timeout,
        callback,
        parent);
}

ErrorCode LruClientCacheManager::EndUpdateCacheEntries(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceNotificationResultSPtr> & results)
{
    return ProcessServiceNotificationAsyncOperation::End(operation, results);
}

bool LruClientCacheManager::ErrorIndicatesInvalidService(
    ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    // All the entries for this service are invalid,
    // so remove them from cache.
    //
    case ErrorCodeValue::NameNotFound:
    case ErrorCodeValue::UserServiceNotFound:
    case ErrorCodeValue::PartitionNotFound:
        return true;

    default:
        return false;
    }
}

bool LruClientCacheManager::ErrorIndicatesInvalidPartition(
    ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    // Remove the cache entry for this partition;
    // Keep the rest of the entries for this name, 
    // as that information may be still valid.
    //
    case ErrorCodeValue::ServiceOffline:
    case ErrorCodeValue::InvalidServicePartition:
        return true;

    default:
        return false;
    }
}

