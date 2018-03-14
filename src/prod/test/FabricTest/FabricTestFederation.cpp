// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace std;

struct CompareNodeId
{
public:
    bool operator()(FabricTestNodeSPtr const & node, NodeId id)
    {
        return (node->Id.IdValue < id.IdValue);
    }
};

FabricTestNodeSPtr const FabricTestFederation::NullNode(nullptr);

FabricTestNodeSPtr const & FabricTestFederation::GetNode(NodeId nodeId) const
{
    AcquireExclusiveLock lock(nodeListLock_);
    auto it = lower_bound(nodes_.begin(), nodes_.end(), nodeId, CompareNodeId());
    if (it != nodes_.end() && (*it)->Id == nodeId)
    {
        return *it;
    }

    return NullNode;
}

bool FabricTestFederation::ContainsNode(NodeId nodeId) const
{
    return (GetNode(nodeId) != nullptr);
}

void FabricTestFederation::AddNode(FabricTestNodeSPtr const & testNode)
{
    AcquireExclusiveLock lock(nodeListLock_);
    auto it = lower_bound(nodes_.begin(), nodes_.end(), testNode->Id, CompareNodeId());
    TestSession::FailTestIf(it != nodes_.end() && (*it)->Id == testNode->Id, "{0} already exists", testNode->Id);
    nodes_.insert(it, testNode);

    // Find the node in the deleted node list
    auto deletedNodeInfo = find_if(deletedNodes_.begin(), deletedNodes_.end(), [&testNode] (DeletedNodeEntry const & entry)
    {
        return entry.Id == testNode->Id;
    });

    if (deletedNodeInfo == deletedNodes_.end())
    {
        return;
    }

    // Node was present in the deleted node list
    // Update the node activation status based on the deleted node entry and remove the deleted node entry
    testNode->IsActivated = deletedNodeInfo->IsActivated;
    deletedNodes_.erase(deletedNodeInfo);
}

void FabricTestFederation::RemoveNode(NodeId nodeId)
{
    AcquireExclusiveLock lock(nodeListLock_);
    auto it = lower_bound(nodes_.begin(), nodes_.end(), nodeId, CompareNodeId());
    if (it != nodes_.end() && (*it)->Id == nodeId)
    {
        deletedNodes_.push_back(DeletedNodeEntry(**it));
        nodes_.erase(it);
    }
}

void FabricTestFederation::CheckDeletedNodes()
{
    AcquireReadLock lock(nodeListLock_);

    for (auto const & deletedNodeEntry : deletedNodes_)
    {
        auto & fabricNode = deletedNodeEntry.Node;
        auto & fmReplica = deletedNodeEntry.FMReplica;
        auto elapsed = DateTime::Now() - deletedNodeEntry.DeletedTime;

        CheckDeletedNode(fabricNode, elapsed);

        CheckDeletedFMReplica(fmReplica, elapsed);
    }
}

void FabricTestFederation::CheckDeletedNode(
    Fabric::FabricNodeWPtr const & nodeWPtr,
    TimeSpan const elapsed)
{
    auto nodeSPtr = nodeWPtr.lock();
    if (nodeSPtr.get() == nullptr) { return; }

    if (elapsed > FabricTestSessionConfig::GetConfig().NodeDeallocationTimeout)
    {
        nodeSPtr->TraceTrackedReferences();

        Assert::CodingError(
            "Node {0} still alive after {1}, use count {2}",
            nodeSPtr->Id, 
            elapsed, 
            nodeWPtr.use_count());
    }
    else
    {
        Trace.WriteWarning(
            "Leak", 
            "Node {0} still alive after {1}, use count {2}",
            nodeSPtr->Id, 
            elapsed, 
            nodeWPtr.use_count());
    }
}

void FabricTestFederation::CheckDeletedFMReplica(
    ComponentRootWPtr const & replicaWPtr,
    TimeSpan const elapsed)
{
    auto replicaSPtr = replicaWPtr.lock();
    if (replicaSPtr.get() == nullptr) { return; }

    if (elapsed > FabricTestSessionConfig::GetConfig().ReplicaDeallocationTimeout)
    {
        replicaSPtr->TraceTrackedReferences();

        Assert::CodingError(
            "Replica {0} still alive after {1}, use count {2}",
            replicaSPtr->TraceId,
            elapsed, 
            replicaWPtr.use_count());
    }
    else
    {
        Trace.WriteWarning(
            "Leak", 
            "Replica {0} still alive after {1}, use count {2}",
            replicaSPtr->TraceId,
            elapsed, 
            replicaWPtr.use_count());
    }
}

FabricTestNodeSPtr const & FabricTestFederation::GetNodeClosestToZero() const
{
    AcquireExclusiveLock lock(nodeListLock_);
    if (nodes_.size() == 0)
    {
        return NullNode;
    }

    FabricTestNodeSPtr const & predecessor = *(nodes_.rbegin());
    FabricTestNodeSPtr const & successor = *(nodes_.begin());

    if (NodeId::MinNodeId.MinDist(predecessor->Id) <= NodeId::MinNodeId.MinDist(successor->Id))
    {
        return predecessor;
    }
    else
    {
        return successor;
    }
}

NodeId FabricTestFederation::GetNodeIdAt(int index)
{
    AcquireExclusiveLock lock(nodeListLock_);
    TestSession::FailTestIf(index < 0 || static_cast<size_t>(index) > nodes_.size() - 1, "Invalid index provided");
    FabricTestNodeSPtr fabricNode = nodes_[index];
    return fabricNode->Id;
}

wstring const & FabricTestFederation::GetEntreeServiceAddress()
{
    AcquireExclusiveLock lock(nodeListLock_);
    FabricTestNodeSPtr fabricNode;
    TestSession::FailTestIf(Count == 0, "FabricTestFederation::GetEntreeServiceAddress called when federation is empty");

    do
    {
        int index = Random().Next();
        index %= this->Count;
        fabricNode = nodes_[index];
    }
    while(!fabricNode->Node->IsOpened);

    return fabricNode->RuntimeManager->NamingServiceListenAddress;
}

void FabricTestFederation::GetEntreeServiceAddress(__out std::vector<std::wstring> & addresses)
{
    AcquireExclusiveLock lock(nodeListLock_);
    for (FabricTestNodeSPtr const & node : nodes_)
    {
        if (node->Node->IsOpened)
        {
            addresses.push_back(node->RuntimeManager->NamingServiceListenAddress);
        }
    }
}

bool FabricTestFederation::TryFindCalculatorService(std::wstring const & serviceName, NodeId const & nodeId, CalculatorServiceSPtr & calculatorServiceSPtr)
{
    FabricTestNodeSPtr nodeSPtr = GetNode(nodeId);
    if (nodeSPtr != NullNode)
    {
        return nodeSPtr->TryFindCalculatorService(serviceName, nodeId, calculatorServiceSPtr);
    }

    return false;
}

bool FabricTestFederation::TryFindStoreService(std::wstring const & serviceName, NodeId const & nodeId, ITestStoreServiceSPtr & storeServiceSPtr)
{
    FabricTestNodeSPtr nodeSPtr = GetNode(nodeId);
    if (nodeSPtr != NullNode)
    {
        return nodeSPtr->TryFindStoreService(serviceName, nodeId, storeServiceSPtr);
    }

    return false;
}

bool FabricTestFederation::TryFindStoreService(std::wstring serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr)
{
    AcquireExclusiveLock lock(nodeListLock_);
    for ( auto iterator = nodes_.begin() ; iterator != nodes_.end(); iterator++)
    {
        if((*iterator)->TryFindStoreService(serviceLocation, storeServiceSPtr))
        {
            return true;
        }
    }

    return false;
}

bool FabricTestFederation::TryFindScriptableService(__in std::wstring const & serviceLocation, __out ITestScriptableServiceSPtr & scriptableServiceSPtr)
{
    AcquireExclusiveLock lock(nodeListLock_);

    for (auto iterator = begin(nodes_); iterator != end(nodes_); ++iterator)
    {
        if ((*iterator)->TryFindScriptableService(serviceLocation, scriptableServiceSPtr))
        {
            return true;
        }
    }

    return false;
}

bool FabricTestFederation::TryFindScriptableService(__in std::wstring const  & serviceName, Federation::NodeId const & nodeId, __out ITestScriptableServiceSPtr & scriptableServiceSPtr)
{
    FabricTestNodeSPtr nodeSPtr = GetNode(nodeId);
    if (nodeSPtr != NullNode)
    {
        return nodeSPtr->TryFindScriptableService(serviceName, nodeId, scriptableServiceSPtr);
    }

    return false;
}

void FabricTestFederation::GetBlockListForService(std::wstring placementConstraints, std::vector<Federation::NodeId> & blockList)
{
    AcquireExclusiveLock lock(nodeListLock_);
    for ( auto iterator = nodes_.begin() ; iterator != nodes_.end(); iterator++)
    {
        FabricTestNodeSPtr const& fabricTestNode = (*iterator);
        ExpressionSPtr compiledExpression = Expression::Build(placementConstraints);
        TestSession::FailTestIfNot(compiledExpression != nullptr, "Expression::Build returned null for {0}", placementConstraints);
        wstring error;
        bool result = compiledExpression->Evaluate(fabricTestNode->Node->NodeProperties, error);
        if(error.empty())
        {
            bool isNonQualifyingNode = !result;
            if(isNonQualifyingNode)
            {
                blockList.push_back(fabricTestNode->Id);
            }
        }
    }
}
