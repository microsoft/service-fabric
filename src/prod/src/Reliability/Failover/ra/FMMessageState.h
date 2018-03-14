// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        /*
            This class encapsulates all the state around the messages sent by a 
            FailoverUnit to the FM where the FT is the source of the message 
            such as ReplicaDown, ReplicaUp etc

            This class is a subcomponent of the FT itself so some state
            is maintained in the FT (isDeleted) and this class just stores a pointer
            to that state
        */
        class FMMessageState
        {
        public:
            FMMessageState(
                bool const * localReplicaDeleted, 
                FailoverManagerId const & fm,
                Infrastructure::IRetryPolicySPtr const & retryPolicy);

            FMMessageState(
                bool const * localReplicaDeleted, 
                FMMessageState const & other);

            FMMessageState & operator=(
                FMMessageState const & other);

            __declspec(property(get = get_MessageStage)) FMMessageStage::Enum MessageStage;
            FMMessageStage::Enum get_MessageStage() const { return messageStage_; }
            void Test_SetMessageStage(FMMessageStage::Enum value);

            __declspec(property(get = get_FMMessageRetryPending)) Infrastructure::SetMembershipFlag & FMMessageRetryPending;
            Infrastructure::SetMembershipFlag const & get_FMMessageRetryPending() const { return fmMessageRetryPendingFlag_; }
            Infrastructure::SetMembershipFlag & Test_FMMessageRetryPending() { return fmMessageRetryPendingFlag_; }

            __declspec(property(get = get_DownReplicaInstanceId)) int64 DownReplicaInstanceId;
            int64 get_DownReplicaInstanceId() const
            {
                AssertIfInstanceIsNotSet();
                return preReopenLocalReplicaInstanceId_;
            }
            int64 Test_GetDownReplicaInstanceId() const { return preReopenLocalReplicaInstanceId_; }
            void Test_SetDownReplicaInstanceId(int64 value) { preReopenLocalReplicaInstanceId_ = value; }

            bool ShouldRetry(
                Common::StopwatchTime now,
                __out int64 & sequenceNumber) const;

            void OnRetry(
                Common::StopwatchTime now,
                int64 sequenceNumber, 
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnLastReplicaUpPending(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnLastReplicaUpAcknowledged(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void MarkReplicaEndpointUpdatePending(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void Reset(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnDropped(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnClosing(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnReplicaUp(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnReplicaDown(
                bool isPersisted,
                int64 replicaInstance,
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnLoadedFromLfum(
                int64 replicaInstance,
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnReplicaUpAcknowledged(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnReplicaDownReply(
                int64 replicaInstance,
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void OnReplicaDroppedReply(
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void AssertInvariants(FailoverUnit const & ft) const;

            std::vector<Infrastructure::SetMembershipFlag const*> Test_GetAllSets() const;

            static std::string AddField(Common::TraceEvent& traceEvent, std::string const& name);
            void FillEventData(Common::TraceEventContext& context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;

        private:
            const int64 InvalidInstance = -1;

            __declspec(property(get = get_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return *localReplicaDeleted_; }

            __declspec(property(get = get_IsInstanceSet)) bool IsInstanceSet;
            bool get_IsInstanceSet() const { return preReopenLocalReplicaInstanceId_ != InvalidInstance; }

            void TransitionMessageStage(
                FMMessageStage::Enum target,
                Infrastructure::StateUpdateContext & stateUpdateContext);

            void EnqueueFMMessageRetryAction(
                Infrastructure::StateMachineActionQueue & queue) const;

            void ClearInstanceIfSet(
                Infrastructure::StateUpdateContext & stateUpdateContext);
            
            void SetInstance(
                int64 replicaInstance,
                Infrastructure::StateUpdateContext & stateUpdateContext);
            
            void AssertIfMessageStageIsNot(
                std::initializer_list<FMMessageStage::Enum> const & values, 
                Common::StringLiteral const & tag) const; 

            void AssertIfMessageStageIs(
                std::initializer_list<FMMessageStage::Enum> const & values, 
                Common::StringLiteral const & tag) const;

            void AssertIfInstanceIsNotSet() const;
            void AssertIfInstanceIsSet() const;

            void AssertIfDeleted() const;

            Infrastructure::SetMembershipFlag fmMessageRetryPendingFlag_;
            Infrastructure::RetryState retryState_;

            FMMessageStage::Enum messageStage_;
            int64 preReopenLocalReplicaInstanceId_;
            bool const * localReplicaDeleted_;
        };
    }
}



