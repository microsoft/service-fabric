// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            class PendingReplicaUploadStateProcessor
            {
                DENY_COPY(PendingReplicaUploadStateProcessor);
            public:
                PendingReplicaUploadStateProcessor(
                    ReconfigurationAgent & ra,
                    Infrastructure::EntitySetCollection & setCollection,
                    Reliability::FailoverManagerId const & failoverManager);

                __declspec(property(get = get_Owner)) Reliability::FailoverManagerId const & Owner;
                Reliability::FailoverManagerId const & get_Owner() const { return owner_; }

                __declspec(property(get = get_Test_IsTimerSet)) bool Test_IsTimerSet;
                bool get_Test_IsTimerSet() { return timer_.IsSet; }

                __declspec(property(get = get_IsComplete)) bool IsComplete;
                bool get_IsComplete() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return pendingUploadState_.IsComplete;
                }

                PendingReplicaUploadState & Test_GetPendingReplicaUploadState()
                {
                    return pendingUploadState_;
                }

                void ProcessNodeUpAck(
                    std::wstring const & activityId,
                    bool isDeferredUploadRequired);

                void ProcessLastReplicaUpReply(
                    std::wstring const & activityId);

                void Close();

                void Test_OnTimer(std::wstring const & activityId) { OnTimer(activityId); }

                bool IsLastReplicaUpMessage(
                    Infrastructure::EntityEntryBaseSet const & entitiesInMessage) const;

            private:
                void OnTimer(std::wstring const & activityId);

                void Execute(
                    std::wstring const & activityId,
                    PendingReplicaUploadState::Action const & action);

                void Trace(
                    std::wstring const & activityId,
                    PendingReplicaUploadState::TraceData const & traceData);

                void TrySendLastReplicaUp(
                    std::wstring const & activityId,
                    PendingReplicaUploadState::Action const & action);

                void TryRequestUploadOnEntities(
                    std::wstring const & activityId,
                    PendingReplicaUploadState::Action const & action);

                Infrastructure::IClock const & GetClock() const;

                Reliability::FailoverManagerId const owner_;
                ReconfigurationAgent & ra_;
                Infrastructure::RetryTimer timer_;
                TimeSpanConfigEntry const * retryInterval_;
                PendingReplicaUploadState pendingUploadState_;
                mutable Common::RwLock lock_;
            };
        }
    }
}


