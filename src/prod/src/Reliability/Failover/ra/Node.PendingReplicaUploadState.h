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
            // This class manages all the pending replica upload state
            // Pending replica upload is started after node up ack
            // and completes after the last replica up reply is received
            // This class is not thread safe - the caller has to handle thread safety
            class PendingReplicaUploadState
            {
            public:
                struct TraceData
                {
                    explicit TraceData(Reliability::FailoverManagerId const & owner) :
                        PendingReplicaCount(0),
                        IsComplete(false),
                        Owner(owner)
                    {
                    }

                    uint64 PendingReplicaCount;
                    bool IsComplete;
                    Reliability::FailoverManagerId Owner;
                    Common::TimeSpan Elapsed;
                    std::string RandomPendingEntityTraceId;

                    static std::string AddField(Common::TraceEvent& traceEvent, std::string const& name);
                    void FillEventData(Common::TraceEventContext& context) const;
                    void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;
                };

                struct Action
                {
                    explicit Action(Reliability::FailoverManagerId const & owner) :
                    SendLastReplicaUp(false),
                    InvokeUploadOnEntities(false),
                    TraceDataObj(owner)
                    {
                    }

                    bool SendLastReplicaUp;
                    bool InvokeUploadOnEntities;
                    Infrastructure::EntityEntryBaseList Entities;
                    TraceData TraceDataObj;
                };

                PendingReplicaUploadState(
                    Infrastructure::EntitySet::Parameters const & setParameters,
                    Infrastructure::EntitySetCollection & setCollection,
                    TimeSpanConfigEntry const & reopenSuccessWaitDurationConfig);

                __declspec(property(get = get_IsComplete)) bool IsComplete;
                bool get_IsComplete() const { return isComplete_; }

                __declspec(property(get = get_FailoverManager)) Reliability::FailoverManagerId const & FailoverManager;
                Reliability::FailoverManagerId const & get_FailoverManager() const { return pendingReplicas_.Id.FailoverManager; }

                __declspec(property(get = get_HasPendingReplicas)) bool HasPendingReplicas;
                bool get_HasPendingReplicas() const { return pendingReplicas_.Size != 0; }

                Action OnNodeUpAckProcessed(
                    std::wstring const & activityId,
                    Common::StopwatchTime now,
                    bool isDeferredUploadRequired);

                Action OnTimer(
                    std::wstring const & activityId,
                    Common::StopwatchTime now);

                Action OnLastReplicaUpReply(
                    std::wstring const & activityId,
                    Common::StopwatchTime now);

                bool IsLastReplicaUpMessage(
                    Infrastructure::EntityEntryBaseSet const & entitiesInMessage) const;

           private:
                void AssertIfNodeUpAckNotProcessed(
                    std::wstring const & activityId,
                    Common::StringLiteral const & tag) const;

                void AssertIfNodeUpAckProcessed(
                    std::wstring const & activityId,
                    Common::StringLiteral const & tag) const;

                __declspec(property(get = get_HasNodeUpAckBeenProcessed)) bool HasNodeUpAckBeenProcessed;
                bool get_HasNodeUpAckBeenProcessed() const { return nodeUpAckTime_ != Common::StopwatchTime::Zero; }

                Action CreateSendLastReplicaUpAction(Common::StopwatchTime now) const;

                Action CreateNoOpAction(Common::StopwatchTime now) const;

                Action CreateMarkForUploadAction(
                    Infrastructure::EntityEntryBaseList && entities,
                    Common::StopwatchTime now) const;

                Infrastructure::EntitySet pendingReplicas_;
                Common::StopwatchTime nodeUpAckTime_;
                TimeSpanConfigEntry const * reopenSuccessWaitDurationConfig_;
                bool isComplete_;
                bool hasUploadBeenRequested_;
                bool isDeferredUploadRequired_;
            };
        }
    }
}
