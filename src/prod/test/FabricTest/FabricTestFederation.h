// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    struct DeletedNodeEntry
    {
        DeletedNodeEntry(FabricTestNode const & testNode)
            :   Id(testNode.Id),
                Node(),
                DeletedTime(Common::DateTime::Now()),
                IsActivated(testNode.IsActivated)
        {
            std::shared_ptr<FabricNodeWrapper> fabricNodeWrapper;
            if (testNode.TryGetFabricNodeWrapper(fabricNodeWrapper))
            {
                auto fabricNode = fabricNodeWrapper->Node;
                Node = fabricNode;
                FMReplica = fabricNode->Test_Reliability.Test_GetFailoverManagerService();
            }
        }

        Federation::NodeId Id;
        Fabric::FabricNodeWPtr Node;
        ComponentRootWPtr FMReplica;
        Common::DateTime DeletedTime;
        bool IsActivated;
    };

    class FabricTestFederation
    {
        DENY_COPY(FabricTestFederation)

    public:     
        FabricTestFederation() 
        {
        }

        __declspec (property(get=get_Count)) int Count;
        int get_Count() { return static_cast<int>(nodes_.size()); }

        bool ContainsNode(Federation::NodeId nodeId) const;
        FabricTestNodeSPtr const & GetNode(Federation::NodeId nodeId) const;
        std::vector<FabricTestNodeSPtr> const & GetNodes() const { return nodes_; }

        FabricTestNodeSPtr const & GetNodeClosestToZero() const;

        void AddNode(FabricTestNodeSPtr const & testNode);
        void RemoveNode(Federation::NodeId nodeId);

        void CheckDeletedNodes();

        Federation::NodeId GetNodeIdAt(int index);
        std::wstring const & GetEntreeServiceAddress();
        void GetEntreeServiceAddress(__out std::vector<std::wstring> & addresses);

        bool TryFindStoreService(std::wstring serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr);
        bool TryFindStoreService(std::wstring const & serviceName, Federation::NodeId const & nodeId, ITestStoreServiceSPtr & storeServiceSPtr);
        bool TryFindCalculatorService(std::wstring const & serviceName, Federation::NodeId const & nodeId, CalculatorServiceSPtr & calculatorServiceSPtr);

        bool TryFindScriptableService(__in std::wstring const & serviceLocation, __out ITestScriptableServiceSPtr & scriptableServiceSPtr);
        bool TryFindScriptableService(__in std::wstring const & serviceName, Federation::NodeId const & nodeId, __out ITestScriptableServiceSPtr & scriptableServiceSPtr);

        void GetBlockListForService(std::wstring placementConstraints, std::vector<Federation::NodeId> & blockList);

        void ForEachFabricTestNode(std::function<void(FabricTestNodeSPtr const&)> processor) const
        {
            Common::AcquireExclusiveLock lock(nodeListLock_);
            for_each(nodes_.begin(), nodes_.end(), processor);
        }

    private:
        void CheckDeletedNode(Fabric::FabricNodeWPtr const &, TimeSpan const elapsed);
        void CheckDeletedFMReplica(Common::ComponentRootWPtr const & replicaWPtr, TimeSpan const elapsed);

        std::vector<FabricTestNodeSPtr> nodes_;
        std::vector<DeletedNodeEntry> deletedNodes_;
        mutable Common::ExclusiveLock nodeListLock_;

        static FabricTestNodeSPtr const NullNode;
    };
};
