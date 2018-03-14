// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class DeadlockDetector 
        : public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

    public:
        DeadlockDetector(PartitionedReplicaTraceComponent const &);

        void TrackAssertEvent(std::wstring const & eventName, Common::TimeSpan const timeout);
        void TrackAssertEvent(std::wstring const & eventName, Common::Guid const & eventId, Common::TimeSpan const timeout);

        void UntrackAssertEvent(std::wstring const & eventName);
        void UntrackAssertEvent(std::wstring const & eventName, Common::Guid const & eventId);

    private:

        void OnAssertEvent(std::wstring const & eventName, Common::Guid const & eventId, Common::TimeSpan const timeout);

        std::unordered_map<std::wstring, Common::TimerSPtr> stringEvents_;
        std::unordered_map<Common::Guid, Common::TimerSPtr, Guid::Hasher> guidEvents_;
        Common::ExclusiveLock eventsLock_;
    };

    typedef std::shared_ptr<DeadlockDetector> DeadlockDetectorSPtr;
}
