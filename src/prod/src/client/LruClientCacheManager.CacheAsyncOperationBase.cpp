// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Client;
using namespace Transport;
using namespace Reliability;
using namespace std;
using namespace Naming;

StringLiteral const TraceComponent("CacheManager.CacheAsyncOperationBase");

LruClientCacheManager::CacheAsyncOperationBase::CacheAsyncOperationBase(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    FabricActivityHeader && activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : AsyncOperation(callback, parent)
  , owner_(owner)
  , cache_(owner.cache_)
  , client_(owner.client_)
  , name_(name)
  , nameWithoutMembersSPtr_()
  , nameWithoutMembersLock_()
  , timeoutHelper_(timeout)
  , activityHeader_(move(activityHeader))
  , traceId_(activityHeader_.ActivityId.ToString())
{
}

LruClientCacheManager::CacheAsyncOperationBase::CacheAsyncOperationBase(
    __in LruClientCacheManager & owner,
    NamingUri const & name,
    FabricActivityHeader const & activityHeader,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
  : AsyncOperation(callback, parent)
  , owner_(owner)
  , cache_(owner.cache_)
  , client_(owner.client_)
  , name_(name)
  , nameWithoutMembersSPtr_()
  , nameWithoutMembersLock_()
  , timeoutHelper_(timeout)
  , activityHeader_(activityHeader)
  , traceId_(activityHeader_.ActivityId.ToString())
{
}

wstring const & LruClientCacheManager::CacheAsyncOperationBase::get_ClientContextId() const
{
    return client_.TraceContext;
}

LruClientCacheCallbackSPtr LruClientCacheManager::CacheAsyncOperationBase::GetCacheCallback(NamingUri const & cacheEntryName) const
{
    return owner_.GetCacheCallback(cacheEntryName);
}

void LruClientCacheManager::CacheAsyncOperationBase::AddActivityHeader(ClientServerRequestMessageUPtr & message)
{
    message->Headers.Replace(activityHeader_);
}

bool LruClientCacheManager::CacheAsyncOperationBase::HasCachedPsd() const
{
    LruClientCacheEntrySPtr unused;
    return cache_.TryGet(this->GetNameWithoutMembers(), unused);
}

NamingUri const & LruClientCacheManager::CacheAsyncOperationBase::GetNameWithoutMembers() const
{
    // All functional interactions with the cache or Naming must take into account
    // service groups. Tracing should use the full name that contains the member for debugging.
    //
    if (!name_.HasQueryOrFragment)
    {
        return name_;
    }

    {
        AcquireReadLock lock(nameWithoutMembersLock_);

        if (nameWithoutMembersSPtr_)
        {
            return *nameWithoutMembersSPtr_;
        }
    }

    {
        AcquireWriteLock lock(nameWithoutMembersLock_);

        if (nameWithoutMembersSPtr_.get() == nullptr)
        {
            nameWithoutMembersSPtr_ = make_shared<NamingUri>(name_.GetTrimQueryAndFragmentName());

            this->WriteInfo(
                TraceComponent, 
                "{0}: mapping service group name: {1} -> {2}",
                this->TraceId, 
                name_, 
                *nameWithoutMembersSPtr_);
        }

        return *nameWithoutMembersSPtr_;
    }
}

void LruClientCacheManager::CacheAsyncOperationBase::GetCachedPsd(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = cache_.BeginTryGet(
        this->GetNameWithoutMembers(),
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const & operation) { this->OnGetCachedPsdComplete(operation, false); },
        thisSPtr);
    this->OnGetCachedPsdComplete(operation, true);
}

void LruClientCacheManager::CacheAsyncOperationBase::OnGetCachedPsdComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    bool isFirstWaiter;
    LruClientCacheEntrySPtr cacheEntry;

    auto error = this->EndGetCachedPsd(operation, isFirstWaiter, cacheEntry);
    
    if (error.IsSuccess())
    {
        if (isFirstWaiter)
        {
            this->OnPsdCacheMiss(thisSPtr);
        }
        else
        {
            this->OnProcessCachedPsd(cacheEntry, thisSPtr);
        }
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: reading PSD cache failed: {1} error = {2}", 
            this->TraceId, 
            name_, 
            error);

        this->TryComplete(thisSPtr, error);
    }
}

ErrorCode LruClientCacheManager::CacheAsyncOperationBase::EndGetCachedPsd(
    AsyncOperationSPtr const & operation,
    __out bool & isFirstWaiter,
    __out LruClientCacheEntrySPtr & cacheEntry)
{
    return cache_.EndTryGet(operation, isFirstWaiter, cacheEntry);
}

void LruClientCacheManager::CacheAsyncOperationBase::OnPsdCacheMiss(AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheMiss(
        this->ClientContextId,
        this->ActivityId, 
        name_,
        false); // prefix

    auto operation = client_.BeginInternalGetServiceDescription(
        this->GetNameWithoutMembers(),
        activityHeader_,
        timeoutHelper_.GetRemainingTime(),
        [this] (AsyncOperationSPtr const & operation) { this->OnGetPsdComplete(operation, false); },
        thisSPtr);
    this->OnGetPsdComplete(operation, true);
}

void LruClientCacheManager::CacheAsyncOperationBase::OnGetPsdComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    LruClientCacheEntrySPtr cacheEntry;
    auto error = client_.EndInternalGetServiceDescription(operation, cacheEntry);

    if (error.IsSuccess())
    {
        this->OnProcessRefreshedPsd(cacheEntry, thisSPtr);
    }
    else
    {
        this->OnPsdError(error, thisSPtr);
    }
}

void LruClientCacheManager::CacheAsyncOperationBase::OnPsdError(
    ErrorCode const & error,
    AsyncOperationSPtr const & thisSPtr)
{
    this->WriteInfo(TraceComponent, "{0}: Get PSD failed: {1} error = {2}", this->TraceId, name_, error);

    // Best-effort fail and cancel waiters
    //
    if (LruClientCacheManager::ErrorIndicatesInvalidService(error))
    {
        cache_.FailWaiters(this->GetNameWithoutMembers(), error);
    }
    else
    {
        cache_.CancelWaiters(this->GetNameWithoutMembers());
    }

    this->TryComplete(thisSPtr, error);
}

void LruClientCacheManager::CacheAsyncOperationBase::InvalidateCachedPsd(
    LruClientCacheEntrySPtr const & cacheEntry,
    AsyncOperationSPtr const & thisSPtr)
{
    FabricClientImpl::Trace.PsdCacheInvalidation(
        this->ClientContextId,
        this->ActivityId, 
        cacheEntry->Name,
        cacheEntry->Description.Version);

    auto operation = cache_.BeginTryInvalidate(
        cacheEntry,
        timeoutHelper_.GetRemainingTime(),
        [this](AsyncOperationSPtr const & operation) { this->OnInvalidateCachedPsdComplete(operation, false); },
        thisSPtr);
    this->OnInvalidateCachedPsdComplete(operation, true);
}

void LruClientCacheManager::CacheAsyncOperationBase::OnInvalidateCachedPsdComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    bool isFirstWaiter;
    LruClientCacheEntrySPtr cacheEntry;

    auto error = cache_.EndTryInvalidate(operation, isFirstWaiter, cacheEntry);

    if (error.IsSuccess())
    {
        if (isFirstWaiter)
        {
            this->OnPsdCacheMiss(thisSPtr);
        }
        else
        {
            this->OnProcessRefreshedPsd(cacheEntry, thisSPtr);
        }
    }
    else
    {
        this->WriteInfo(
            TraceComponent, 
            "{0}: invalidating PSD cache failed: {1} error = {2}", 
            this->TraceId, 
            name_, 
            error);

        this->TryComplete(thisSPtr, error);
    }
}

NamingUri const & LruClientCacheManager::CacheAsyncOperationBase::GetCacheEntryName(LruClientCacheEntrySPtr const & cacheEntry)
{
    // Need to pass the fullname for Service Group endpoint parsing
    //
    return this->FullName.Fragment.empty() ? cacheEntry->Name : this->FullName;
}
