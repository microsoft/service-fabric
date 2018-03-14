// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

StringLiteral const SetTimerTag("SetTimer");
StringLiteral const UpdateTimerTag("UpdateTimer");
StringLiteral const CleanupCallbackTag("CleanupCallback");

void LocalFailoverUnitProxyMap::Open(ReconfigurationAgentProxyId const & id)
{
    id_ = id;
}

void LocalFailoverUnitProxyMap::AddFailoverUnitProxyCallerHoldsLock(FailoverUnitProxySPtr &&fuProxy)
{
    FailoverUnitId fuId = fuProxy->FailoverUnitId;

    failoverUnitProxies_.insert(
        make_pair(fuId, move(fuProxy))
    );

    SetCleanupTimerCallerHoldsLock();
}

void LocalFailoverUnitProxyMap::AddFailoverUnitProxy(FailoverUnitProxySPtr &fuProxy)
{
    AcquireWriteLock grab(lock_);

    FailoverUnitProxySPtr fuProxyToMove = fuProxy;

    AddFailoverUnitProxyCallerHoldsLock(std::move(fuProxyToMove));
}

vector<FailoverUnitId> LocalFailoverUnitProxyMap::GetAllIds() const
{
    vector<FailoverUnitId> rv;
    
    AcquireReadLock grab(lock_);

    rv.reserve(failoverUnitProxies_.size());
    
    for (auto const & it : failoverUnitProxies_)
    {
        rv.push_back(it.first);
    }

    return rv;
}

FailoverUnitProxySPtr LocalFailoverUnitProxyMap::GetOrCreateFailoverUnitProxy(
    ReconfigurationAgentProxy & reconfigurationAgentProxy,
    FailoverUnitDescription const & ftDesc,
    std::wstring const & runtimeId,
    bool createFlag,
    bool & wasCreated)
{
    auto const & failoverUnitId = ftDesc.FailoverUnitId;

    wasCreated = false;

    {
        AcquireReadLock grab(lock_);

        if(!closed_)
        {
            auto it = failoverUnitProxies_.find(failoverUnitId);
            if (it != failoverUnitProxies_.end())
            {
                LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(it->second);
                if (lockedFailoverUnitProxyPtr->IsClosed)
                {
                    if (!createFlag)
                    {
                        return nullptr;
                    }
                }
                else
                {
                    return it->second;
                }
            }
        }
    }

    {
        AcquireWriteLock grab(lock_);

        if(!closed_)
        {
            // Check for race
            auto it = failoverUnitProxies_.find(failoverUnitId);

            if (it == failoverUnitProxies_.end())
            {
                if(!createFlag)
                {
                    return nullptr;
                }

                // No race, do create
                AddFailoverUnitProxyCallerHoldsLock(make_shared<FailoverUnitProxy>(
                    ftDesc,
                    reconfigurationAgentProxy,
                    runtimeId,
                    reconfigurationAgentProxy.Root.CreateAsyncOperationRoot()));

                it = failoverUnitProxies_.find(failoverUnitId);

                wasCreated = true;
            }
            else
            {
                LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(FailoverUnitProxySPtr(it->second));
                if (lockedFailoverUnitProxyPtr->IsClosed)
                {
                    if (!createFlag)
                    {
                        return nullptr;
                    }

                    auto cleanup = removedFailoverUnitProxies_.find(failoverUnitId);
                    if (cleanup != removedFailoverUnitProxies_.end())
                    {
                        lockedFailoverUnitProxyPtr->Reuse(ftDesc, runtimeId);

                        // Reclaim failoverunitproxy
                        removedFailoverUnitProxies_.erase(cleanup);
                    }
                }
            }

            ASSERT_IF(
                it == failoverUnitProxies_.end(), 
                "Cannot find FailoverUnit proxy in Local FailoverUnitProxy Map");

            return it->second;
        }
    }

    return nullptr;
}

FailoverUnitProxySPtr LocalFailoverUnitProxyMap::GetFailoverUnitProxy(FailoverUnitId const & failoverUnitId) const
{
    AcquireReadLock grab(lock_);
    if(!closed_)
    {
        auto it = failoverUnitProxies_.find(failoverUnitId);
        if (it != failoverUnitProxies_.end())
        {
            return it->second;
        }
    }

    return nullptr;
}

void LocalFailoverUnitProxyMap::RemoveFailoverUnitProxy(FailoverUnitId const & failoverUnitId)
{
    AcquireWriteLock grab(lock_);
    if(!closed_)
    {
        auto it = failoverUnitProxies_.find(failoverUnitId);
        if(it != failoverUnitProxies_.end())
        {
            removedFailoverUnitProxies_.insert(failoverUnitId);
        }
    }
}

void LocalFailoverUnitProxyMap::RegisterFailoverUnitProxyForCleanupEvent(FailoverUnitId const & failoverUnitId)
{
    AcquireWriteLock grab(lock_);
    if(!closed_)
    {
        registeredFailoverUnitProxiesForCleanupEvent_.push_back(failoverUnitId);
    }
}

void LocalFailoverUnitProxyMap::UnregisterFailoverUnitProxyForCleanupEvent(FailoverUnitId const & failoverUnitId)
{
    AcquireWriteLock grab(lock_);
    if(!closed_)
    {
        auto it = remove_if(
            registeredFailoverUnitProxiesForCleanupEvent_.begin(),
            registeredFailoverUnitProxiesForCleanupEvent_.end(),
            [&failoverUnitId] (FailoverUnitId const& fuId)
            {
                return failoverUnitId == fuId;
            });

        registeredFailoverUnitProxiesForCleanupEvent_.erase(it, registeredFailoverUnitProxiesForCleanupEvent_.end());
    }
}

map<FailoverUnitId, FailoverUnitProxySPtr> && LocalFailoverUnitProxyMap::PrivatizeLFUPM()
{
    {
        AcquireWriteLock grab(lock_);
        ASSERT_IF(closed_, "PrivatizeLFUPM called on already closed LocalFailoverUnitProxyMap");

        if (cleanupTimerSPtr_ != nullptr)
        {
            cleanupTimerSPtr_->Cancel();
            cleanupTimerSPtr_ = nullptr;
        }

        closed_ = true;

        removedFailoverUnitProxies_.clear();
        registeredFailoverUnitProxiesForCleanupEvent_.clear();

        return std::move(failoverUnitProxies_);
    }
}

void LocalFailoverUnitProxyMap::SetCleanupTimerCallerHoldsLock()
{
    RAPEventSource::Events->LFUPMTimerChange(SetTimerTag, *this);

    if (closed_)
    {
        // Component has been shutdown
        return;
    }

    if (!cleanupTimerSPtr_)
    {
        auto root = Root.CreateComponentRoot();

        // Create the timer
        cleanupTimerSPtr_ = Common::Timer::Create(
            TimerTagDefault,
            [this, root] (Common::TimerSPtr const &)
            { 
                this->CleanupCallback();
            });

        cleanupTimerSPtr_->Change(FailoverConfig::GetConfig().FailoverUnitProxyCleanupInterval);
    }
}

void LocalFailoverUnitProxyMap::UpdateCleanupTimer()
{
    {
        AcquireWriteLock grab(lock_);

        RAPEventSource::Events->LFUPMTimerChange(UpdateTimerTag, *this);

        if (closed_)
        {
            // Component has been shutdown
            return;
        }

        ASSERT_IFNOT(cleanupTimerSPtr_, "UpdateTimer: timer is not set");

        cleanupTimerSPtr_->Change(FailoverConfig::GetConfig().FailoverUnitProxyCleanupInterval);
    }
}

void LocalFailoverUnitProxyMap::CleanupCallback()
{
    vector<FailoverUnitId> registeredFailoverUnitProxiesForCleanupEvent;

    {
        AcquireReadLock grab(lock_);

        RAPEventSource::Events->LFUPMTimerChange(CleanupCallbackTag, *this);

        if (closed_)
        {
            return;
        }

        registeredFailoverUnitProxiesForCleanupEvent = vector<FailoverUnitId>(
            registeredFailoverUnitProxiesForCleanupEvent_.cbegin(),
            registeredFailoverUnitProxiesForCleanupEvent_.cend());
    }

    {
        AcquireWriteLock grab(lock_);

        for (auto iter = removedFailoverUnitProxies_.begin(); iter != removedFailoverUnitProxies_.end();)
        {
            auto fupIter = failoverUnitProxies_.find(*iter);
            ASSERT_IF(fupIter == failoverUnitProxies_.end(), "Cannot have item in removed list and not in FUP");
            
            auto fup = fupIter->second;

            LockedFailoverUnitProxyPtr lockedFUP(fup);
            if (lockedFUP->TryDelete())
            {
                failoverUnitProxies_.erase(fupIter);
                iter = removedFailoverUnitProxies_.erase(iter);
                continue;
            }

            iter++;
        }
    }

    for (auto iter = registeredFailoverUnitProxiesForCleanupEvent.begin(); iter != registeredFailoverUnitProxiesForCleanupEvent.end(); ++iter)
    {
        FailoverUnitProxySPtr failoverUnitProxy = GetFailoverUnitProxy(*iter);
        if (failoverUnitProxy != nullptr)
        {
            failoverUnitProxy->CleanupEventHandler();
        }
    }

    UpdateCleanupTimer();
}

void LocalFailoverUnitProxyMap::WriteTo(TextWriter& writer, FormatOptions const &options) const
{
    UNREFERENCED_PARAMETER(options);

    {
        writer.WriteLine("{0} {1} {2} {3}", 
            id_, failoverUnitProxies_.size(), removedFailoverUnitProxies_.size(), registeredFailoverUnitProxiesForCleanupEvent_.size());
    }
}

void LocalFailoverUnitProxyMap::WriteToEtw(uint16 contextSequenceId) const
{
    RAPEventSource::Events->LFUPM(contextSequenceId, 
        id_, failoverUnitProxies_.size(), removedFailoverUnitProxies_.size(), registeredFailoverUnitProxiesForCleanupEvent_.size());
}
