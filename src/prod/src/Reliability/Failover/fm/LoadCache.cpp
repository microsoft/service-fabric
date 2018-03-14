// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StringLiteral const TraceLoadCache("LoadCache");

LoadCache::LoadCache(FailoverManager& fm, FailoverManagerStore& fmStore, LoadBalancingComponent::IPlacementAndLoadBalancing & plb, vector<LoadInfoSPtr> & loads)
    :   fm_(fm),
        fmStore_(fmStore),
        plb_(plb), 
        maxBatchSize_(static_cast<size_t>(FailoverConfig::GetConfig().MaxLoadUpdateBatchSize))
{
    for (size_t i = 0; i < loads.size(); i++)
    {
        loads_.insert(make_pair(FailoverUnitId(loads[i]->LoadDescription.FailoverUnitId), move(loads[i])));
    }

    for (int i = 0; i < FailoverConfig::GetConfig().MaxParallelLoadUpdates; i++)
    {
        activeLists_.push_back(make_unique<PersistenceList>());
    }
}

void LoadCache::GetAllFailoverUnitIds(std::vector<FailoverUnitId> & failoverUnitIds)
{
    AcquireReadLock grab(lock_);

    failoverUnitIds.reserve(loads_.size());
    for (auto it = loads_.begin(); it != loads_.end(); it++)
    {
        failoverUnitIds.push_back(it->first);
    }
}

void LoadCache::GetAllLoads(vector<LoadInfoSPtr> & loads)
{
    AcquireReadLock grab(lock_);

    for(auto it = loads_.begin(); it != loads_.end(); ++it)
    {
        loads.push_back(it->second);
    }
}

LoadInfoSPtr LoadCache::GetLoads(FailoverUnitId const& failoverUnitId) const
{
    AcquireReadLock grab(lock_);

    auto it = loads_.find(failoverUnitId);
    if (it != loads_.end())
    {
        return it->second;
    }

    return LoadInfoSPtr();
}

void LoadCache::UpdateLoads(LoadBalancingComponent::LoadOrMoveCostDescription && loads)
{
    AcquireExclusiveLock grab(lock_);

    FailoverUnitId failoverUnitId(loads.FailoverUnitId);

    auto it = loads_.find(failoverUnitId);
    if (it == loads_.end())
    {
        loads_.insert(make_pair(failoverUnitId, make_shared<LoadInfo>(move(loads))));
        Enqueue(failoverUnitId);
    }
    else if (it->second->MergeLoads(move(loads)))
    {
        Enqueue(failoverUnitId);
    }
}

void LoadCache::DeleteLoads(FailoverUnitId const& failoverUnitId)
{
    AcquireExclusiveLock grab(lock_);

    auto it = loads_.find(failoverUnitId);
    if (it != loads_.end() && it->second->MarkForDeletion())
    {
        Enqueue(failoverUnitId);
    }
}

void LoadCache::Enqueue(FailoverUnitId const & failoverUnitId)
{
    pendingQueue_.push(failoverUnitId);
}

void LoadCache::PeriodicPersist()
{
    AcquireExclusiveLock grab(lock_);

    fm_.Events.PeriodicTaskBeginNoise(fm_.Id, PeriodicTaskName::LoadCache);

    if (pendingQueue_.size() > 0)
    {
        fm_.WriteInfo(TraceLoadCache, "{0} load entries pending", pendingQueue_.size());
        StartPersist();
    }
}

void LoadCache::StartPersist()
{
    for (size_t i = 0; i < activeLists_.size() && pendingQueue_.size() > 0; i++)
    {
        if (activeLists_[i]->size() == 0)
        {
            ErrorCode result = PersistBatchAsync(i);
            if (!result.IsSuccess())
            {
                fm_.WriteWarning(TraceLoadCache, "Load report failed to persist: {0}", result);
                return;
            }
        }
    }
}

ErrorCode LoadCache::PersistBatchAsync(size_t listIndex)
{
    PersistenceListUPtr const & list = activeLists_[listIndex];
    IStoreBase::TransactionSPtr transactionSPtr;
    ErrorCode error = fmStore_.BeginTransaction(transactionSPtr);
    if (!error.IsSuccess())
    {
        return error;
    }

    while (pendingQueue_.size() > 0 && list->size() < maxBatchSize_)
    {
        FailoverUnitId id = pendingQueue_.front();
        list->push_back(id);
        pendingQueue_.pop();

        auto it = loads_.find(id);
        ASSERT_IF(it == loads_.end(),
            "Load for {0} not found", id);

        it->second->StartPersist();

        error = fmStore_.UpdateData(transactionSPtr, *(it->second));
        if (!error.IsSuccess())
        {
            OnPersistCompleted(listIndex, false, 0);

            transactionSPtr->Rollback();
            return error;
        }
    }

    transactionSPtr->BeginCommit(
        TimeSpan::MaxValue,
        [this, transactionSPtr, listIndex] (AsyncOperationSPtr const & contextSPtr) mutable
        {
            int64 operationLSN = 0;
            ErrorCode result = transactionSPtr->EndCommit(contextSPtr, operationLSN);

            auto root = this->fm_.CreateComponentRoot();
            LoadCache* loadCache = this;
            size_t index = listIndex;
            Threadpool::Post([loadCache, index, operationLSN, result, root]
            {
                loadCache->PersistCallback(index, operationLSN, result);
            });
        },
        fm_.CreateAsyncOperationRoot());

    return error;
}

void LoadCache::PersistCallback(size_t listIndex, int64 operationLSN, ErrorCode result)
{
    AcquireExclusiveLock grab(lock_);

    if (result.IsSuccess())
    {
        fm_.WriteInfo(TraceLoadCache, "{0} load entries persisted",
            activeLists_[listIndex]->size());
    }
    else
    {
        fm_.WriteWarning(TraceLoadCache, "Load report failed to persist: {0}",
            result);
    }

    OnPersistCompleted(listIndex, result.IsSuccess(), operationLSN);
}

void LoadCache::OnPersistCompleted(size_t listIndex, bool isSuccess, int64 operationLSN)
{
    PersistenceListUPtr const & list = activeLists_[listIndex];
    for (auto id : *list)
    {
        auto it = loads_.find(id);
        ASSERT_IF(it == loads_.end(),
            "Load for {0} not found", id);

        auto peristenceState = it->second->PersistenceState;

        if (isSuccess)
        {
            it->second->PostCommit(operationLSN);
        }

        bool isDeleted = peristenceState == PersistenceState::ToBeDeleted;

        bool pendingUpdate = it->second->OnPersistCompleted(isSuccess, isDeleted, plb_);

        if (isDeleted)
        {
            loads_.erase(it);
        }
        else if (pendingUpdate)
        {
            pendingQueue_.push(id);
        }
    }

    list->clear();

    if (isSuccess && pendingQueue_.size() >= maxBatchSize_)
    {
        StartPersist();
    }
}

void LoadCache::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    for (auto const& pair : loads_)
    {
        writer.Write(*pair.second);
    }
}

