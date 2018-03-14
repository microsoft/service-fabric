// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Reliability;
    using namespace std;

    BroadcastWaiters::BroadcastWaiters(
        GatewayEventSource const & trace,
        wstring const & traceId)
        : trace_(trace)
        , traceId_(traceId)
        , waiters_() 
        , isClosed_(false)
        , lock_()
    {
    }

    ErrorCode BroadcastWaiters::Add(
        Common::AsyncOperationSPtr const & owner,
        OnBroadcastReceivedCallback const & callback,
        BroadcastRequestVector && requests)
    {
        AcquireExclusiveLock lock(lock_);
        if (isClosed_)
        {
            return ErrorCode(ErrorCodeValue::ObjectClosed);
        }

        // Do not register the operation after the async operation is completed,
        // as we may not un-register if OnCompleted already executed,
        // which will lead to leaks.
        // Since removing requests are done under the same lock,
        // checking the async operation state here will prevent any races.
        if (owner->IsCompleted)
        {
            return ErrorCode(ErrorCodeValue::InvalidState);
        }
        
        for (auto it = requests.begin(); it != requests.end(); ++it)
        {
            NamingUri const & name = it->Name;
            trace_.BroadcastEventManagerAddRequest(
                traceId_,
                *it);

            if (waiters_.find(name) == waiters_.end())
            {
                waiters_.insert(BroadcastWaiterEntry(name, BroadcastWaiterVector()));
            }
            
            waiters_[name].push_back(make_shared<WaiterEntry>(owner, callback, it->Cuid));
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    void BroadcastWaiters::Remove(
        Common::AsyncOperationSPtr const & owner,
        set<NamingUri> && names)
    {
        if (!names.empty())
        {
            AcquireExclusiveLock lock(lock_);
            for (auto it = names.begin(); it != names.end(); ++it)
            {
                RemoveCallerHoldsLock(owner, *it);
            }
        }

        names.clear();
    }

    void BroadcastWaiters::RemoveCallerHoldsLock(
        Common::AsyncOperationSPtr const & owner,
        NamingUri const & name)
    {
        auto it = waiters_.find(name);
        if (it != waiters_.end())
        {
            BroadcastWaiterVector newEntries;
            for (auto itInner = it->second.begin(); itInner != it->second.end(); ++itInner)
            {
                if ((*itInner)->Owner != owner)
                {
                    newEntries.push_back(move(*itInner));
                }
            }

            it->second.swap(newEntries);
        }
    }

    void BroadcastWaiters::Notify(UserServiceCuidMap const & updates)
    {
        // Find the list of names with their associated CUIDs that need to be raised
        // to the waiters.
        // One waiter will be notified of all pertinent changes.
        map<WaiterEntrySPtr, UserServiceCuidMap> waitersToNotify;

        {
            AcquireExclusiveLock lock(lock_);
            for (auto it = updates.begin(); it != updates.end(); ++it)
            {
                NamingUri const & name = it->first;
                vector<ConsistencyUnitId> const & cuids = it->second;
        
                wstring cuidString;
                StringWriter writer(cuidString);
                for (auto itInner = cuids.begin(); itInner != cuids.end(); ++itInner)
                {
                    writer << *itInner << L";";
                }

                trace_.BroadcastEventManagerReceivedUpdate(
                    traceId_,
                    name,
                    cuidString);

                auto itName = waiters_.find(name);
                if (itName != waiters_.end())
                {
                    for (auto itInner = itName->second.begin(); itInner != itName->second.end(); ++itInner)
                    {
                        std::vector<ConsistencyUnitId> relevantCuids;
                        if ((*itInner)->ShouldNotify(cuids, relevantCuids))
                        {
                            auto itWaitersToNotify = waitersToNotify.find(*itInner);
                            if (itWaitersToNotify == waitersToNotify.end())
                            {
                                UserServiceCuidMap param;
                                param[name] = relevantCuids;
                                waitersToNotify.insert(
                                    pair<WaiterEntrySPtr, UserServiceCuidMap>(*itInner, move(param)));
                            }
                            else
                            {
                                itWaitersToNotify->second.insert(
                                    pair<NamingUri, vector<ConsistencyUnitId>>(name, move(relevantCuids)));
                            }
                        }
                    }
                }
            }
        }

        for (auto it = waitersToNotify.begin(); it != waitersToNotify.end(); ++it)
        {
            it->first->Notify(it->second);
        }
    }

    void BroadcastWaiters::Close()
    {
        map<NamingUri, BroadcastWaiterVector> waitersToCancel;

        {
            AcquireExclusiveLock lock(lock_);
            isClosed_ = true;
            waiters_.swap(waitersToCancel);
        }
        
        for (auto it = waitersToCancel.begin(); it != waitersToCancel.end(); ++it)
        {
            for (auto itInner = it->second.begin(); itInner != it->second.end(); ++itInner)
            {
                (*itInner)->Cancel();
            }
        }
    }

    // Test hooks
    bool BroadcastWaiters::Test_Contains(
        NamingUri const & name, 
        Reliability::ConsistencyUnitId const & cuid) const
    {
        AcquireExclusiveLock lock(lock_);
        auto itName = waiters_.find(name);
        if (itName == waiters_.end())
        {
            return false;
        }

        BroadcastWaiterVector const & registeredWaiters = itName->second;
        for (auto it = registeredWaiters.begin(); it != registeredWaiters.end(); ++it)
        {
            if ((*it)->Cuid.IsInvalid || (cuid == (*it)->Cuid))
            {
                // Registered for all cuids or the specified one
                return true;
            }
        }

        return false;
    }

    // Inner class
         
    BroadcastWaiters::WaiterEntry::WaiterEntry(
        Common::AsyncOperationSPtr const & owner,
        OnBroadcastReceivedCallback const & callback,
        Reliability::ConsistencyUnitId cuid)
        : owner_(owner)
        , callback_(callback)
        , cuid_(cuid)
    {
    }

    void BroadcastWaiters::WaiterEntry::Cancel()
    {
        owner_->TryComplete(owner_, ErrorCode(ErrorCodeValue::ObjectClosed));
        owner_.reset();
    }

    bool BroadcastWaiters::WaiterEntry::ShouldNotify(
        std::vector<ConsistencyUnitId> const & cuids,
        __out std::vector<ConsistencyUnitId> & relevantCuids) const
    {
        if (cuid_.IsInvalid)
        {
            relevantCuids = cuids;
            return true;
        }
        else
        {
            auto it = find(cuids.begin(), cuids.end(), cuid_);
            if (it != cuids.end())
            {
                relevantCuids.push_back(cuid_);
                return true;
            }
        }

        return false;
    }

    void BroadcastWaiters::WaiterEntry::Notify(UserServiceCuidMap const & changedCuids)
    {
        callback_(changedCuids, owner_);
    }
}
