// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace std;
    using namespace Transport;
    using namespace ServiceModel;
	
	template class Hasher<wstring>;
    template class Hasher<NamingUri>;
    template class Hasher<NamePropertyKey>;

    template<>
    size_t Hasher<wstring>::operator() (wstring const & key) const { return NamingUtility::GetHash(key); }
    template<>
    size_t Hasher<NamingUri>::operator() (NamingUri const & name) const { return name.GetHash(); }
    template<>
    size_t Hasher<NamePropertyKey>::operator() (NamePropertyKey const & property) const { return NamingUtility::GetHash(property.Key); }

    StringLiteral const TraceComponent("RequestInstanceTracker");

    int const EntryExpirationFactor = 2;

    template <class TKey>
    RequestInstanceTracker<TKey>::RequestInstanceTracker(
        ComponentRoot const & root, 
        std::wstring const & traceSubComponent,
        Store::PartitionedReplicaId const & partitionedReplicaId)
        : RootedObject(root)
        , Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::General>(partitionedReplicaId)
        , traceSubComponent_(traceSubComponent)
        , lock_()
        , instanceMap_()
        , pruneTimer_()
        , stopped_(false)
    {
    }

    template <class TKey>
    bool RequestInstanceTracker<TKey>::TryAcceptRequest(
        ActivityId const & activityId,
        TKey const & key, 
        __int64 requestInstance,
        __out MessageUPtr & failureReply)
    {
        if (requestInstance < 0)
        {
            return true;
        }

        auto receiveTimestamp = DateTime::Now();

        AcquireExclusiveLock lock(lock_);

        auto findIter = instanceMap_.find(key);
        if (findIter == instanceMap_.end())
        {
            InstanceEntry entry = { requestInstance, receiveTimestamp };
            instanceMap_.insert(MapPair(key, move(entry)));
        }
        else
        {
            InstanceEntry & entry = findIter->second;

            if (requestInstance <= entry.RequestInstance)
            {
                WriteInfo(
                    TraceComponent,
                    traceSubComponent_,
                    "{0} rejecting stale/duplicate request({1}): {2} <= {3}",
                    Store::ReplicaActivityId(this->PartitionedReplicaId, activityId),
                    key,
                    requestInstance,
                    entry.RequestInstance);

                failureReply = NamingMessage::GetStaleRequestFailureReply(StaleRequestFailureReplyBody(entry.RequestInstance));

                return false;
            }

            entry.RequestInstance = requestInstance;
            entry.ReceiveTimestamp = receiveTimestamp;
        }

        this->SchedulePruneCallerHoldsLock();

        return true;
    }

    ErrorCode RequestInstanceTrackerHelper::UpdateInstanceOnStaleRequestFailureReply(
        MessageUPtr const & reply,
        __in RequestInstance & requestInstance)
    {
        if (reply->Action == NamingMessage::NamingStaleRequestFailureReplyAction)
        {
            StaleRequestFailureReplyBody body;
            if (!reply->GetBody(body))
            {
                // Not InvalidMessage since the problem is with our reply
                // rather than the request
                //
                return ErrorCodeValue::OperationFailed;
            }

            requestInstance.UpdateToHigherInstance(body.Instance);

            return ErrorCodeValue::StaleRequest;
        }

        return ErrorCodeValue::Success;
    }

    template <class TKey>
    void RequestInstanceTracker<TKey>::SchedulePruneCallerHoldsLock()
    {
        if (!pruneTimer_ && !stopped_)
        {
            auto root = this->Root.CreateComponentRoot();
            pruneTimer_ = Timer::Create(TimerTagDefault, [this, root](TimerSPtr const& timer) 
                { 
                    timer->Cancel();
                    this->PruneCallback(); 
                },
                true); // allow concurrency

            auto delay = ServiceModelConfig::GetConfig().MaxOperationTimeout;

            WriteInfo(
                TraceComponent,
                traceSubComponent_,
                "{0} scheduling prune: delay = {1}",
                this->TraceId,
                delay);

            pruneTimer_->Change(delay);
        }
    }

    template <class TKey>
    void RequestInstanceTracker<TKey>::PruneCallback()
    {
        auto expirationInterval = this->GetExpirationInterval();
        auto now = DateTime::Now();
        size_t prePruneCount = 0;
        size_t postPruneCount = 0;

        WriteInfo(
            TraceComponent,
            traceSubComponent_,
            "{0} starting prune: expiration interval = {1}",
            this->TraceId,
            expirationInterval);

        InstanceMap prunedMap;
        {
            AcquireExclusiveLock lock(lock_);

            prePruneCount = instanceMap_.size();

            // remove_if() does not compile with hash_map
            //
            for (auto iter = instanceMap_.begin(); iter != instanceMap_.end(); ++iter)
            {
                if (iter->second.ReceiveTimestamp.AddWithMaxValueCheck(expirationInterval) > now)
                {
                    prunedMap.insert(move(*iter));
                }
            }

            instanceMap_.swap(prunedMap);

            pruneTimer_.reset();

            if (!instanceMap_.empty())
            {
                postPruneCount = instanceMap_.size();

                this->SchedulePruneCallerHoldsLock();
            }
        }

        WriteInfo(
            TraceComponent,
            traceSubComponent_,
            "{0} finished pruning: pre = {1} post = {2}",
            this->TraceId,
            prePruneCount,
            postPruneCount);
    }

    // Allow dynamic adjustment if config is changed
    //
    template <class TKey>
    TimeSpan RequestInstanceTracker<TKey>::GetExpirationInterval()
    {
        // The routing layer message expiration mechanism currently does not account for network latency
        // so this assumes that network latency is negligible compared to the max operation timeout
        //
        return TimeSpan::FromTicks(ServiceModelConfig::GetConfig().MaxOperationTimeout.Ticks * EntryExpirationFactor);
    }

    template <class TKey>
    void RequestInstanceTracker<TKey>::Stop()
    {
        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(lock_);

            snap = pruneTimer_;

            stopped_ = true;
        }

        if (snap)
        {
            snap->Cancel();
        }
    }
    
    template class RequestInstanceTracker<wstring>;
    template class RequestInstanceTracker<NamingUri>;
    template class RequestInstanceTracker<NamePropertyKey>;
}

