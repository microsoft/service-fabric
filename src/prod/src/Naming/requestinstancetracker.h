// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    template <class THashKey>
    class Hasher
    {
    public:
        static const size_t bucket_size = 1;
        static const size_t min_buckets = 1024;

        size_t operator() (THashKey const &) const;
    };

    template <class TKey>
    class RequestInstanceTracker
        : public Common::RootedObject
        , public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::General>
    {
    public:
        RequestInstanceTracker(Common::ComponentRoot const &, std::wstring const & traceSubComponent, Store::PartitionedReplicaId const &);

        bool TryAcceptRequest(
            Common::ActivityId const &,
            TKey const &, 
            __int64 requestInstance,
            __out Transport::MessageUPtr & failureReply);

        void Stop();

    private:
        struct InstanceEntry
        {
            __int64 RequestInstance;
            Common::DateTime ReceiveTimestamp;
        };

        std::wstring traceSubComponent_;

        typedef std::pair<TKey, InstanceEntry> MapPair;
        typedef std::unordered_map<TKey, InstanceEntry, Hasher<TKey>> InstanceMap;

        void SchedulePruneCallerHoldsLock();
        void PruneCallback();
        Common::TimeSpan GetExpirationInterval();

        Common::ExclusiveLock lock_;
        InstanceMap instanceMap_;
        Common::TimerSPtr pruneTimer_;
        bool stopped_;
    };

    namespace RequestInstanceTrackerHelper
    {
        Common::ErrorCode UpdateInstanceOnStaleRequestFailureReply(Transport::MessageUPtr const &, __in RequestInstance &);
    };

    typedef RequestInstanceTracker<std::wstring> StringRequestInstanceTracker;
    typedef std::unique_ptr<StringRequestInstanceTracker> StringRequestInstanceTrackerUPtr;

    typedef RequestInstanceTracker<Common::NamingUri> NameRequestInstanceTracker;
    typedef std::unique_ptr<NameRequestInstanceTracker> NameRequestInstanceTrackerUPtr;

    typedef RequestInstanceTracker<NamePropertyKey> NamePropertyRequestInstanceTracker;
    typedef std::unique_ptr<NamePropertyRequestInstanceTracker> NamePropertyRequestInstanceTrackerUPtr;
}
