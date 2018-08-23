// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class DeactivateNodesContext : public UpgradeContext
        {
            DENY_COPY(DeactivateNodesContext);

        public:

            DeactivateNodesContext(
                std::map<Federation::NodeId, NodeDeactivationIntent::Enum> const& nodesToDeactivate,
                std::wstring const& batchId);

            virtual BackgroundThreadContextUPtr CreateNewContext() const;

            void Initialize(FailoverManager & fm);

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit);

            virtual void Merge(BackgroundThreadContext const& context);

            virtual bool ReadyToComplete();

            virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted);

        private:

            void CheckSeedNodes(FailoverManager & fm);

            virtual bool IsReplicaSetCheckNeeded() const;

            virtual bool IsReplicaWaitNeeded(Replica const& replica) const;

            virtual bool IsReplicaMoveNeeded(Replica const& replica) const;

            bool IsRemoveNodeOrDataReplicaWaitNeeded(NodeInfoSPtr const & nodeInfo) const;

            void SendNodeDeactivateMessage(FailoverManager & fm, NodeInfo const& nodeInfo) const;

            std::map<Federation::NodeId, NodeDeactivationIntent::Enum> nodesToDeactivate_;
            std::wstring batchId_;

            std::set<Federation::NodeId> nodesWithUpReplicas_;

            FailoverUnitId failoverUnitId_;

            static std::wstring const SeedNodeTag;
        };
    }
}
