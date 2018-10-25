// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class INodeCache
        {
        public:
            virtual NodeInfoSPtr GetNode(Federation::NodeId id) const = 0;
        };

        class NodeCache : public INodeCache
        {
            DENY_COPY(NodeCache);
        public:
            NodeCache(
                FailoverManager & fm,
                std::vector<NodeInfoSPtr> & nodes);

            __declspec(property(get=get_IsClusterStable)) bool IsClusterStable;
            bool get_IsClusterStable() const;

            __declspec(property(get=get_IsClusterPaused)) bool IsClusterPaused;
            bool get_IsClusterPaused() const;

            __declspec(property(get=get_UpCount)) size_t UpCount;
            size_t get_UpCount() const { return static_cast<size_t>(upCount_.load()); }

            __declspec(property(get = get_NodeCount)) size_t NodeCount;
            size_t get_NodeCount() const { return static_cast<size_t>(nodeCount_.load()); }

            void Initialize(std::vector<NodeInfo> && nodes);

            void StartPeriodicTask();

            void GetNodes(std::vector<NodeInfoSPtr> & nodes) const;

            void GetNodes(Federation::NodeId continuationToken, std::vector<NodeInfoSPtr> & nodes) const;

            Common::ErrorCode GetNodesForGFUMTransfer(__out std::vector<NodeInfo> & nodeInfos) const;

            void GetNodes(std::vector<NodeInfo> & nodes) const;

            NodeInfoSPtr GetNode(Federation::NodeId id) const;

            UpgradeDomainsCSPtr GetUpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy);

            Common::ErrorCode NodeUp(
                NodeInfoSPtr && node,
                bool isVersionGatekeepingNeeded,
                _Out_ Common::FabricVersionInstance & targetVersionInstance);

            Common::ErrorCode NodeDown(Federation::NodeInstance const& nodeInstance);

            Common::ErrorCode DeactivateNodes(
                std::map<Federation::NodeId, NodeDeactivationIntent::Enum> const& nodesToDeactivate,
                std::wstring const& batchId,
                int64 sequenceNumber);

            Common::ErrorCode GetNodeDeactivationStatus(
                std::wstring const& batchId,
                __out NodeDeactivationStatus::Enum & nodeDeactivationStatus,
                __out std::vector<NodeProgress> & progressDetails) const;

            Common::ErrorCode RemoveNodeDeactivation(std::wstring const& batchId, int64 sequenceNumber);

            Common::ErrorCode UpdateNodeDeactivationStatus(
                Federation::NodeId nodeId,
                NodeDeactivationIntent::Enum deactivationIntent,
                NodeDeactivationStatus::Enum deactivationStatus,
                std::map<Federation::NodeId, NodeUpgradeProgress> && pending,
                std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> && ready,
                std::map<Federation::NodeId, NodeUpgradeProgress> && waiting);

            void AddDeactivateNodeContext(std::vector<BackgroundThreadContextUPtr> & contexts) const;

            Common::AsyncOperationSPtr BeginUpdateNodeSequenceNumber(
                Federation::NodeInstance const& nodeInstance,
                uint64 sequenceNumber,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndUpdateNodeSequenceNumber(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginReplicaUploaded(
                Federation::NodeInstance const& nodeInstance,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndReplicaUploaded(Common::AsyncOperationSPtr const& operation);

            void UpdateVersionInstanceAsync(Federation::NodeInstance const& nodeInstance, Common::FabricVersionInstance const& versionInstance);

            void ProcessNodeDeactivateReplyAsync(Federation::NodeId const& nodeId, int64 sequenceNumber);
            void ProcessNodeActivateReplyAsync(Federation::NodeId const& nodeId, int64 sequenceNumber);

            NodeDeactivationInfo GetNodeDeactivationInfo(Federation::NodeId const& nodeId) const;

            bool IsNodeUp(Federation::NodeInstance const& nodeInstance) const;

            Common::ErrorCode SetPendingDeactivateNode(Federation::NodeInstance const& nodeInstance, bool value);

            Common::AsyncOperationSPtr BeginSetPendingFabricUpgrade(
                NodeInfo const& nodeInfo,
                bool value,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndSetPendingFabricUpgrade(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginAddPendingApplicationUpgrade(
                NodeInfo const& nodeInfo,
                ServiceModel::ApplicationIdentifier const& applicationId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndAddPendingApplicationUpgrade(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginNodeStateRemoved(
                Federation::NodeId const& nodeId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndNodeStateRemoved(Common::AsyncOperationSPtr const& operation);

            Common::ErrorCode RemovePendingApplicationUpgrade(Federation::NodeInstance const& nodeInstance, ServiceModel::ApplicationIdentifier const& applicationId);

            void FinishRemoveNode(LockedNodeInfo & lockedNodeInfo, int64 commitDuration);

            static void SortUpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy, __inout std::vector<std::wstring> & upgradeDomains);
            static bool CompareUpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy, std::wstring const& upgradeDomain1, std::wstring const& upgradeDomain2);

            Common::ErrorCode GetLockedNode(
                Federation::NodeId const& nodeId,
                LockedNodeInfo & lockedNodeInfo,
                Common::TimeSpan timeout = FailoverConfig::GetConfig().LockAcquireTimeout);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:

            typedef std::shared_ptr<CacheEntry<NodeInfo>> CacheEntrySPtr;

            void InitializeNode(NodeInfoSPtr && node);

            void AddUpgradeDomain(NodeInfo const& nodeInfo);
            void RemoveUpgradeDomain(std::wstring const& upgradeDomain);

            Common::ErrorCode GetLockedNode(
                Federation::NodeId const& nodeId,
                LockedNodeInfo & lockedNodeInfo,
                bool createNewEntry,
                Common::TimeSpan timeout,
                __out bool & isNewEntry);

            void ReportHealthAfterNodeUpdate(NodeInfoSPtr const & nodeInfo, Common::ErrorCode error, bool isUpgrade = false);
            void ReportHealthForNodeDeactivation(NodeInfoSPtr const & nodeInfo, Common::ErrorCode error, bool isDeactivationComplete);

            void InitializeHealthState();

            void ProcessNodes(NodePeriodicTaskCounts & taskCounts);

            void ProcessNodeForCleanup(
                NodePeriodicTaskCounts & taskCounts,
                NodeInfoSPtr const& nodeInfo,
                CacheEntrySPtr const& cacheEntry,
                vector<LockedNodeInfo>& nodesToRemove) const;
            void RemoveNodesAsync(
                NodePeriodicTaskCounts & taskCounts,
                vector<LockedNodeInfo> & nodesToRemove);

            void ProcessNodeForActivation(NodeInfoSPtr const& nodeInfo) const;

            void ProcessNodeForCounts(
                NodeInfoSPtr const& nodeInfo,
                NodeCounts& counts,
                vector<Federation::NodeInstance>& upNodes) const;
            void TraceNodeCounts(
                NodeCounts const& counts,
                vector<Federation::NodeInstance> const& upNodes) const;

            void ProcessNodeForHealthReporting(
                NodeInfoSPtr const& nodeInfo,
                CacheEntrySPtr const& cacheEntry,
                bool shouldReportFabricUpgradeHealth,
                Common::DateTime const& upgradeStartTime,
                vector<CacheEntrySPtr>& nodesToReportFabricUpgradeHealth,
                vector<CacheEntrySPtr>& nodesToReportDeactivationStuck,
                vector<CacheEntrySPtr>& nodesToReportDeactivationComplete) const;
            void ReportHealthForNodes(
                NodePeriodicTaskCounts & taskCounts,
                bool shouldReportFabricUpgradeHealth,
                vector<CacheEntrySPtr>& nodesToReportFabricUpgradeHealth,
                vector<CacheEntrySPtr>& nodesToReportDeactivationStuck,
                vector<CacheEntrySPtr>& nodesToReportDeactivationComplete);
            void ReportNodeDeactivationHealth(
                NodePeriodicTaskCounts & taskCounts,
                CacheEntrySPtr & cacheEntry,
                bool isDeactivationComplete);

            void ProcessHealth();

            FailoverManager & fm_;
            std::map<Federation::NodeId, CacheEntrySPtr> nodes_;

            std::map<UpgradeDomainSortPolicy::Enum, UpgradeDomainsSPtr> upgradeDomains_;

            Common::DateTime lastNodeUpTime_;
            mutable bool isClusterStable_;

            Common::atomic_uint64 upCount_;
            Common::atomic_uint64 nodeCount_;

            FABRIC_SEQUENCE_NUMBER ackedSequence_;
            FABRIC_SEQUENCE_NUMBER healthSequence_;
            FABRIC_SEQUENCE_NUMBER initialSequence_;
            FABRIC_SEQUENCE_NUMBER invalidateSequence_;
            bool healthInitialized_;
      
            MUTABLE_RWLOCK(FM.NodeCache, lock_);
        };
    }
}
