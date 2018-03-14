// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TestCommon;
using namespace FederationTestCommon;
using namespace FabricTest;

// It will add a new behavior for given nodeIdStr, seviceName.
// If the behavior to be added is already present
// then method will exit with error.
void BehaviorStore::AddBehavior(
    __in wstring const& nodeIdStr,
    __in wstring const& serviceName,
    __in wstring const& strBehavior)
{
    // Test if nodeIdStr can represent nodeId(s)
    TestSession::FailTestIfNot(IsSupportedNodeStr(nodeIdStr),
        "String can not represent node(s). No appropriate interpretation found.");
    TestSession::FailTestIfNot(IsSupportedServiceStr(serviceName),
        "String can not represent service(s). No appropriate interpretation found.");

    // Find the corresponding node or create one.
    NodeServiceBehaviorSet::iterator iteratorNode = behaviorMap_.find(nodeIdStr);
    if (behaviorMap_.end() == iteratorNode)
    {
        pair<NodeServiceBehaviorSet::iterator, bool> ret =
            behaviorMap_.insert(pair<wstring, ServiceBehaviorSet>(nodeIdStr, ServiceBehaviorSet()));
        TestSession::FailTestIfNot(ret.second, "ret.second");
        iteratorNode = ret.first;
    }

    // Find the corresponding service-node or create one.
    ServiceBehaviorSet::iterator iteratorService = iteratorNode->second.find(serviceName);
    if (iteratorNode->second.end() == iteratorService)
    {
        pair<map<wstring, BehaviorSet>::iterator, bool> ret =
            iteratorNode->second.insert(pair<wstring, BehaviorSet>(serviceName, BehaviorSet()));
        TestSession::FailTestIfNot(ret.second, "ret.second");
        iteratorService = ret.first;
    }

    // Add the behavior iff it is not present otherwise report error.
    BehaviorSet::iterator iteratorBehavior = iteratorService->second.find(strBehavior);
    TestSession::FailTestIfNot(iteratorService->second.end() == iteratorBehavior, "Behavior existed");
    iteratorService->second.insert(strBehavior);

    TestSession::WriteInfo(
        TraceSource,
        "ApiBehaviorHelper::AddBehavior - nodeIdStr({0}) serviceName({1}) BehaviorType({2})",
        nodeIdStr,
        serviceName,
        strBehavior);
}

// It will remove a behavior which was previously added for given nodeIdStr, seviceName.
// If the behavior to be removed is not found then method will cause test to fail.
void BehaviorStore::RemoveBehavior(
    __in wstring const& nodeIdStr,
    __in wstring const& serviceName,
    __in wstring const& strBehavior)
{
    // Test if nodeIdStr can represent nodeId(s)
    TestSession::FailTestIfNot(IsSupportedNodeStr(nodeIdStr),
        "String is can not represent node(s). No appropriate interpretation found.");
    TestSession::FailTestIfNot(IsSupportedServiceStr(serviceName),
        "String can not represent service(s). No appropriate interpretation found.");

    // Find the corresponding node.
    NodeServiceBehaviorSet::iterator iteratorNode = behaviorMap_.find(nodeIdStr);
    TestSession::FailTestIf(behaviorMap_.end() == iteratorNode, "Behavior not existing: Node");

    // Find the corresponding service-node.
    ServiceBehaviorSet::iterator iteratorService = iteratorNode->second.find(serviceName);
    TestSession::FailTestIf(iteratorNode->second.end() == iteratorService, "Behavior not existing: Service");

    // Remove the behavior if found otherwise report Error.
    BehaviorSet::iterator iteratorBehavior = iteratorService->second.find(strBehavior);
    TestSession::FailTestIf(iteratorService->second.end() == iteratorBehavior, "Behavior not existing: BehaviorType");

    // Remove the behavior.
    iteratorService->second.erase(iteratorBehavior);

    TestSession::WriteInfo(
        TraceSource,
        "ApiBehaviorHelper::RemoveBehavior - nodeIdStr({0}) serviceName({1}) BehaviorType({2})",
        nodeIdStr,
        serviceName,
        strBehavior);
}

bool BehaviorStore::IsBehaviorSet(
    __in wstring const& nodeIdStr,
    __in wstring const& serviceName,
    __in wstring const& strBehavior) const
{
    // Test if nodeIdStr can represent nodeId(s)
    TestSession::FailTestIfNot(IsSupportedNodeStr(nodeIdStr),
        "String is can not represent node(s). No appropriate interpretation found.");
    TestSession::FailTestIfNot(IsSupportedServiceStr(serviceName),
        "String can not represent service(s). No appropriate interpretation found.");

    bool BehaviorFound = false; // Not found yet
    wstring nodeNameSearchList[] = {nodeIdStr, L"*"};

    // Search for the nodeIdStr then * if needed
    for(const auto& currentNodeName : nodeNameSearchList)
    {
        /// Search for node: currentNodeName
        auto iteratorNode = behaviorMap_.find(currentNodeName);
        if(behaviorMap_.end() != iteratorNode)
        {// Node name matches
            BehaviorFound = IsBehaviorSetForNode(iteratorNode, serviceName, strBehavior);
        }

        // Stop searching if success
        if (BehaviorFound) break;
    }

    return BehaviorFound;
}

bool BehaviorStore::IsBehaviorSetForNode(
    NodeServiceBehaviorSet::const_iterator const& iteratorNode,
    __in wstring const& serviceName,
    __in wstring const& strBehavior) const
{
    bool result = false; // Not found yet
    wstring serviceNameSearchList[] = {serviceName, L"*"};
    for(const auto& currentSvc : serviceNameSearchList)
    {
        /// Search for serviceName
        auto iteratorService = iteratorNode->second.find(currentSvc);

        if(iteratorNode->second.end() != iteratorService)
        {// Service name matches
            result = IsBehaviorSetForService(iteratorService, strBehavior);
        }

        // Break if success
        if (result) break;
    }

    return result;
}

bool BehaviorStore::IsBehaviorSetForService(
    ServiceBehaviorSet::const_iterator const& iteratorService,
    __in wstring const& strBehavior) const
{
    bool result = false; // Not found yet.

    /// Search for particular behavior
    auto iteratorBehavior = iteratorService->second.find(strBehavior);

    if (iteratorService->second.end() != iteratorBehavior)
    {
        // Behavior type matches
        result = true;
    }
    return result;
}

// Verify if string is supported as NodeId
bool BehaviorStore::IsSupportedNodeStr(
    __in wstring const& nodeIdStr) const
{
    if(nodeIdStr.compare(L"*") == 0) return true;

    // Fail-- Exception if string can't be parsed as Federation::NodeId
    CommonDispatcher::ParseNodeId(nodeIdStr);

    return true;
}

bool BehaviorStore::IsSupportedServiceStr(__in wstring const& serviceNameStr) const
{
    if(serviceNameStr.compare(L"*") == 0) return true;

    // Fail-- Exception if string can't be parsed as a Uri.
    NamingUri uri;
    if (!NamingUri::TryParse(serviceNameStr, uri))
    {
        TestSession::WriteError(TraceSource, "Invalid NamingUri: ", serviceNameStr);
        return false;
    }
    return true;
}


BehaviorStore::BehaviorStore()
{
    // constructor
}

StringLiteral const BehaviorStore::TraceSource("FabricTest.BehaviorStore");
