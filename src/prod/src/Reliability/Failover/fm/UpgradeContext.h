// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class UpgradeContext : public BackgroundThreadContext
        {
            DENY_COPY(UpgradeContext);

        public:
            virtual void Merge(BackgroundThreadContext const & context);

            virtual bool ReadyToComplete();

        protected:
            UpgradeContext(
                std::wstring const & contextId,
                std::wstring const & currentDomain);

            void ProcessReplica(FailoverUnit const& failoverUnit, Replica const& replica);
            void ProcessCandidates(FailoverManager const& fm, ApplicationInfo const& applicationInfo, FailoverUnit const& failoverUnit);

            void AddCancelNode(Federation::NodeId nodeId);
            void AddReadyNode(Federation::NodeId nodeId, NodeUpgradeProgress const& upgradeProgress, NodeInfoSPtr const& nodeInfo);
            bool AddPendingNode(Federation::NodeId nodeId, NodeUpgradeProgress const& upgradeProgress);
            bool AddWaitingNode(Federation::NodeId nodeId, NodeUpgradeProgress const& upgradeProgress);

            void PruneReadyNodes();

            void TraceNodes(FailoverManager & fm, Common::StringLiteral const& traceSource, std::wstring const& type = std::wstring()) const;

            std::wstring currentDomain_;

            std::set<Federation::NodeId> cancelNodes_;
            std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> readyNodes_;
            std::map<Federation::NodeId, NodeUpgradeProgress> pendingNodes_;
            std::map<Federation::NodeId, NodeUpgradeProgress> waitingNodes_;

            std::set<Federation::NodeId> notReadyNodes_;

        private:
            virtual bool IsReplicaSetCheckNeeded() const = 0;

            virtual bool IsReplicaWaitNeeded(Replica const& replica) const = 0;

            virtual bool IsReplicaMoveNeeded(Replica const& replica) const = 0;

            std::vector<std::pair<Federation::NodeId, std::wstring>> candidates_;
        };
    }
}
