// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        //
        // This class implements the message processing logic for the FailoverManager. It assumes that the
        // message has already been checked for staleness.
        //
        class FailoverUnitMessageProcessor
        {
            DENY_COPY(FailoverUnitMessageProcessor);

        public:

            FailoverUnitMessageProcessor(FailoverManager & fm);

            void AddInstanceReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const;
            void AddPrimaryReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const;
            void AddReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const;
            void RemoveReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const;
            void DropReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaReplyMessageBody const& body) const;
            void ReplicaDownProcessor(LockedFailoverUnitPtr & failoverUnit, ReplicaDescription const & replica) const;
            void ReconfigurationReplyProcessor(LockedFailoverUnitPtr & failoverUnit, ConfigurationReplyMessageBody const& body) const;

            void DatalossReportProcessor(LockedFailoverUnitPtr & failoverUnit, ConfigurationMessageBody const& body, std::vector<StateMachineActionUPtr> & actions) const;
            void ChangeConfigurationProcessor(LockedFailoverUnitPtr & failoverUnit, ConfigurationMessageBody const& body, Federation::NodeInstance const & from, std::vector<StateMachineActionUPtr> & actions) const;

            void RemoveInstanceReplyProcessor(LockedFailoverUnitPtr & failoverUnit, std::wstring const& action, std::unique_ptr<ReplicaReplyMessageBody> && body, Federation::NodeInstance const & from) const;
            void DeleteReplicaReplyProcessor(LockedFailoverUnitPtr & failoverUnit, std::wstring const& action, std::unique_ptr<ReplicaReplyMessageBody> && body, Federation::NodeInstance const & from) const;
            void ReplicaDroppedProcessor(LockedFailoverUnitPtr & failoverUnit, std::wstring const& action, std::unique_ptr<ReplicaMessageBody> && body, Federation::NodeInstance const & from, std::vector<StateMachineActionUPtr> & actions) const;
            void ReplicaEndpointUpdatedProcessor(LockedFailoverUnitPtr & failoverUnit, std::unique_ptr<ReplicaMessageBody> && body, Federation::NodeInstance const & from, std::vector<StateMachineActionUPtr> & actions) const;

        private:

            // < 0 better than, == 0 equal, > 0 worse than
            int CompareForPrimary(FailoverUnit const& failoverUnit, Reliability::ReplicaDescription const &r1, Reliability::ReplicaDescription const& r2) const;

            void ProcessDownReplica(LockedFailoverUnitPtr & failoverUnit, ReplicaDescription const & replica, bool isDropped, bool isDeleted) const;

            void CompleteBuildReplica(LockedFailoverUnitPtr & failoverUnit, Replica & replica, ReplicaReplyMessageBody const& body) const;

            void OnDeleteApplicationCompleted(Common::AsyncOperationSPtr const& operation, ServiceInfoSPtr const& serviceInfo) const;
            void OnDeleteServiceCompleted(Common::AsyncOperationSPtr const& operation) const;

            FailoverManager & fm_;
        };
    }
}
