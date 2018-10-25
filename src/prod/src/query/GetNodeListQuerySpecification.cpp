// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Query;
using namespace Federation;

StringLiteral const TraceType("GetNodeListQuerySpecification");
double const GetNodeListQuerySpecification::CMGetNodeListTimeoutPercentage = 0.8;

vector<QuerySpecificationSPtr> GetNodeListQuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    resultSPtr.push_back(make_shared<GetNodeListQuerySpecification>(false));
    resultSPtr.push_back(make_shared<GetNodeListQuerySpecification>(true));

    return move(resultSPtr);
}

GetNodeListQuerySpecification::GetNodeListQuerySpecification(bool excludeFAS)
    : ParallelQuerySpecification(
        QueryNames::GetNodeList,
        QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Node::Name, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::Node::NodeStatusFilter, false),
        QueryArgument(Query::QueryResourceProperties::Node::ExcludeStoppedNodeInfo, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false))
{
    querySpecificationId_ = GetInternalSpecificationId(excludeFAS);

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetNodeList,
            QueryAddresses::GetCM(),
            QueryArgument(Query::QueryResourceProperties::Node::Name, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::Node::NodeStatusFilter, false),
            QueryArgument(Query::QueryResourceProperties::Node::ExcludeStoppedNodeInfo, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));


    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetNodeList,
            Query::QueryAddresses::GetFM(),
            QueryArgument(Query::QueryResourceProperties::Node::Name, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::Node::NodeStatusFilter, false),
            QueryArgument(Query::QueryResourceProperties::Node::ExcludeStoppedNodeInfo, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetAggregatedNodeHealthList,
            QueryAddresses::GetHM(),
            QueryArgument(Query::QueryResourceProperties::Node::Name, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::Node::ExcludeStoppedNodeInfo, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    if (!excludeFAS)
    {
        // There is no reason to add a MaxResults option here as there is no current use case
        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
                QueryNames::GetNodeList,
                QueryAddresses::GetTS(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::Node::NodeStatusFilter, false),
                QueryArgument(Query::QueryResourceProperties::Node::ExcludeStoppedNodeInfo, false)));
    }
}

bool GetNodeListQuerySpecification::ExcludeStoppedNodeInfo(ServiceModel::QueryArgumentMap const & queryArgs)
{
    wstring value;
    if (queryArgs.TryGetValue(QueryResourceProperties::Node::ExcludeStoppedNodeInfo, value))
    {
        if (value == L"True")
        {
            return true;
        }
    }

    return false;
}

wstring GetNodeListQuerySpecification::GetSpecificationId(QueryArgumentMap const& queryArgs)
{
    return GetInternalSpecificationId(ExcludeStoppedNodeInfo(queryArgs));
}

wstring GetNodeListQuerySpecification::GetInternalSpecificationId(bool excludeFas)
{
    if (excludeFas)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetNodeList), L"/NOFAS");
    }

    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetNodeList), L"/FAS");
}

// Merge the results together from the parallel calls.
// Return a message that is created from a MergedQueryListPager object
//
// A map is passed in with all the query results.
// For each of the query results from the map, call MergePagingStatus.
// MergePagingStatus will choose the smallest continuation token, and set that as official continuation
// token for that query.
// Then merge the results for the different subsystems together.
Common::ErrorCode GetNodeListQuerySpecification::OnParallelQueryExecutionComplete(
    Common::ActivityId const & activityId,
    std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
    __out Transport::MessageUPtr & replyMessage)
{
    vector<NodeQueryResult> fmResultList;
    vector<NodeQueryResult> cmResultList;
    vector<NodeAggregatedHealthState> healthResultList;
    vector<NodeQueryResult> fasResultList;

    // For all inner queries, look if there is continuation token.
    // If so, the merging will stop at the last NodeId that is contained by all queries
    MergedQueryListPager<NodeId, NodeQueryResult> resultsPager;
    ErrorCode fmReplyErrorCode(ErrorCodeValue::Success);
    ErrorCode cmReplyErrorCode(ErrorCodeValue::Success);
    ErrorCode hmReplyErrorCode(ErrorCodeValue::Success);
    ErrorCode fasErrorCode(ErrorCodeValue::InvalidAddress);
    for (auto itQueryResult = queryResults.begin(); itQueryResult != queryResults.end(); ++itQueryResult)
    {
        if (itQueryResult->first->AddressString == QueryAddresses::GetFM().AddressString)
        {
            fmReplyErrorCode = itQueryResult->second.MoveList<NodeQueryResult>(fmResultList);
            if (!fmReplyErrorCode.IsSuccess())
            {
                WriteInfo(
                    TraceType,
                    "{0}: Error '{1}' while obtaining result from sub-query with address '{2}' during merge",
                    activityId,
                    fmReplyErrorCode,
                    itQueryResult->first->AddressString);
            }
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetCM().AddressString)
        {
            cmReplyErrorCode = itQueryResult->second.MoveList<NodeQueryResult>(cmResultList);
            if (!cmReplyErrorCode.IsSuccess())
            {
                WriteInfo(
                    TraceType,
                    "{0}: Error '{1}' while obtaining result from sub-query with address '{2}' during merge",
                    activityId,
                    cmReplyErrorCode,
                    itQueryResult->first->AddressString);
            }
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetHM().AddressString)
        {
            hmReplyErrorCode = itQueryResult->second.MoveList<NodeAggregatedHealthState>(healthResultList);
            if (!hmReplyErrorCode.IsSuccess())
            {
                WriteInfo(
                    TraceType,
                    "{0}: Error '{1}' while obtaining result from sub-query with address '{2}' during merge",
                    activityId,
                    hmReplyErrorCode,
                    itQueryResult->first->AddressString);
            }
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetTS().AddressString)
        {
            // Note about "GetTS" above.  The acronym was renamed from TS to FAS, but it was missed for GetTS and the query
            // address underneath, which cannot be changed now for compatibility
            fasErrorCode = itQueryResult->second.MoveList<NodeQueryResult>(fasResultList);
            if (!fasErrorCode.IsSuccess())
            {
                WriteInfo(
                    TraceType,
                    "{0}: Error '{1}' while obtaining result from sub-query with address '{2}' during merge",
                    activityId,
                    fasErrorCode,
                    itQueryResult->first->AddressString);
            }
        }

        // Best effort update min continuation token
        // On error, use the results from the inner query as if they are all there
        resultsPager.MergePagingStatus(
            activityId,
            itQueryResult->first->AddressString,
            itQueryResult->second.MovePagingStatus()).ReadValue();

        if (!fmReplyErrorCode.IsSuccess() && !cmReplyErrorCode.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "{0}: Error '{1}' from sub-query with address '{2}'. Error '{3} from sub-query with address '{4}'",
                activityId,
                fmReplyErrorCode,
                QueryAddresses::GetFM().AddressString,
                cmReplyErrorCode,
                QueryAddresses::GetCM().AddressString);
            return fmReplyErrorCode; // return error since FM and CM failed to return node list
        }
        else if (!fmReplyErrorCode.IsSuccess() || !cmReplyErrorCode.IsSuccess() || !hmReplyErrorCode.IsSuccess())
        {
            continue;
        }
    }

    WriteInfo(
        TraceType,
        "{0}: {1}: FM returned {2} results, CM {3} results, HM {4} results, TS {5} results. Merged continuation token: {6}",
        activityId,
        QueryNames::GetNodeList,
        fmResultList.size(),
        cmResultList.size(),
        healthResultList.size(),
        fasResultList.size(),
        resultsPager.ContinuationToken);

    map<NodeId, NodeQueryResult> cmResultMap = CreateNodeResultMap(move(cmResultList));
    map<NodeId, FABRIC_HEALTH_STATE> healthResultMap;

    for (auto itHealthResultItem = healthResultList.begin(); itHealthResultItem != healthResultList.end(); ++itHealthResultItem)
    {
        healthResultMap.insert(make_pair(move(itHealthResultItem->NodeId), itHealthResultItem->AggregatedHealthState));
    }

    // For all FM results, look for CM entries and add them to result
    for (auto & itFmResultItem : fmResultList)
    {
        auto itCm = cmResultMap.find(itFmResultItem.NodeId);
        if (itCm == cmResultMap.end())
        {
            WriteNoise(
                TraceType,
                "{0}: Node {1}, id {2} is present in FM result but not in CM result",
                activityId,
                itFmResultItem.NodeName,
                itFmResultItem.NodeId);
        }
        else
        {
            itFmResultItem.NodeName = itCm->second.NodeName;
            cmResultMap.erase(itCm);
        }

        // If the node is marked down by FM, populate the IsStopped property.  Otherwise, do nothing.
        if (itFmResultItem.NodeStatus == FABRIC_QUERY_NODE_STATUS_DOWN)
        {
            auto itStoppedNodeMatch = find_if(fasResultList.begin(), fasResultList.end(), [&itFmResultItem](NodeQueryResult const & n) {return itFmResultItem.NodeName == n.NodeName; });
            if (itStoppedNodeMatch != fasResultList.end())
            {
                WriteNoise(
                    TraceType,
                    "{0}: Node {1}, id {2} status={3}, marking stopped",
                    activityId,
                    itFmResultItem.NodeName,
                    itFmResultItem.NodeId,
                    itFmResultItem.NodeStatus);
                itFmResultItem.IsStopped = true;
            }
        }

        resultsPager.Add(itFmResultItem.NodeId, move(itFmResultItem));
    }

    // Add remaining items from CM that were not found in FM
    for (auto & itCmResultItem : cmResultMap)
    {
        WriteNoise(
            TraceType,
            "{0}: Node {1}, id {2} is present in CM result but not in FM result",
            activityId,
            itCmResultItem.second.NodeName,
            itCmResultItem.first);
        resultsPager.Add(itCmResultItem.first, move(itCmResultItem.second));
    }

    // Insert health data into the resultsPager.
    resultsPager.UpdateHealthStates(activityId, healthResultMap);

    replyMessage = move(Common::make_unique<Transport::Message>(QueryResult(resultsPager.TakePager(activityId))));
    return ErrorCode::Success();
}

map<NodeId, ServiceModel::NodeQueryResult> GetNodeListQuerySpecification::CreateNodeResultMap(
    vector<ServiceModel::NodeQueryResult> && nodeQueryResults)
{
    map<NodeId, ServiceModel::NodeQueryResult> resultMap;
    for (auto itResultItem = nodeQueryResults.begin(); itResultItem != nodeQueryResults.end(); ++itResultItem)
    {
        resultMap.insert(make_pair(move(itResultItem->NodeId), move(*itResultItem)));
    }

    return move(resultMap);
}

TimeSpan GetNodeListQuerySpecification::GetParallelQueryExecutionTimeout(QuerySpecificationSPtr const& querySpecification, TimeSpan const& totalRemainingTime)
{
    if (querySpecification->QueryName == QueryNames::GetAggregatedNodeHealthList)
    {
        return QueryConfig::GetConfig().GetAggregatedHealthQueryTimeout(totalRemainingTime);
    }
    else if (querySpecification->QueryName == QueryNames::GetNodeList &&
        querySpecification->AddressString == QueryAddresses::GetCM().AddressString)
    {
        return TimeSpan::FromSeconds(totalRemainingTime.TotalSeconds() * CMGetNodeListTimeoutPercentage);
    }
    else if (querySpecification->QueryName == QueryNames::GetNodeList &&
        querySpecification->AddressString == QueryAddresses::GetTS().AddressString)
    {
        return QueryConfig::GetConfig().GetStoppedNodeQueryTimeout(totalRemainingTime);
    }
    else
    {
        return totalRemainingTime;
    }
}
