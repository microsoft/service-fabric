// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType("GetApplicationServiceListQuerySpecification");

vector<QuerySpecificationSPtr> GetApplicationServiceListQuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    resultSPtr.push_back(make_shared<GetApplicationServiceListQuerySpecification>(false));
    resultSPtr.push_back(make_shared<GetApplicationServiceListQuerySpecification>(true));

    return move(resultSPtr);
}

GetApplicationServiceListQuerySpecification::GetApplicationServiceListQuerySpecification(
    bool adhocService)
    : AggregateHealthParallelQuerySpecificationBase<ServiceQueryResult, wstring>(
        QueryNames::GetApplicationServiceList,
        QueryAddresses::GetGateway(),
        QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
        QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false))
    , isAdhocServicesSpecification_(adhocService)
{
    // This sets the member in the parent class QuerySpecification. You need to set this because this is how QuerySpecificationStore
    // checks for uniqueness of the specificationID.
    // The check doesn't call GetSpecificationId because it's a static method.
    querySpecificationId_ = GetInternalSpecificationId(adhocService);

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
        QueryNames::GetApplicationServiceList,
        QueryAddresses::GetFM(),
        QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
        QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

    if (isAdhocServicesSpecification_)
    {
        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServiceHealthList,
            QueryAddresses::GetHM(),
            QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
    }
    else
    {
        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
            QueryNames::GetApplicationServiceList,
            QueryAddresses::GetCM(),
            QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));

        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServiceHealthList,
            QueryAddresses::GetHMViaCM(),
            QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
    }
}

bool GetApplicationServiceListQuerySpecification::IsAdhocServicesRequest(ServiceModel::QueryArgumentMap const & queryArgs)
{
    return !queryArgs.ContainsKey(QueryResourceProperties::Application::ApplicationName);
}

bool GetApplicationServiceListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetApplicationServiceList;
}

wstring GetApplicationServiceListQuerySpecification::GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs)
{
    return GetInternalSpecificationId(IsAdhocServicesRequest(queryArgs));
}

wstring GetApplicationServiceListQuerySpecification::GetInternalSpecificationId(bool isAdhoc)
{
    if (isAdhoc)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetApplicationServiceList), L"/AdHoc");
    }

    return QueryNames::ToString(QueryNames::GetApplicationServiceList);;
}

Common::ErrorCode GetApplicationServiceListQuerySpecification::AddEntityKeyFromHealthResult(
    __in ServiceModel::QueryResult & queryResult,
    __inout std::map<wstring, FABRIC_HEALTH_STATE> & healthStateMap)
{
    vector<ServiceAggregatedHealthState> healthQueryResult;
    auto error = queryResult.MoveList(healthQueryResult);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itHealthInfo = healthQueryResult.begin(); itHealthInfo != healthQueryResult.end(); ++itHealthInfo)
    {
        healthStateMap.insert(std::make_pair(move(itHealthInfo->ServiceName), itHealthInfo->AggregatedHealthState));
    }

    return error;
}

wstring GetApplicationServiceListQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::ServiceQueryResult const & entityInformation)
{
    return entityInformation.ServiceName.ToString();
}

ErrorCode GetApplicationServiceListQuerySpecification::OnParallelQueryExecutionComplete(
    Common::ActivityId const & activityId,
    map<QuerySpecificationSPtr, QueryResult> & queryResults,
    __out Transport::MessageUPtr & replyMessage)
{
    if (isAdhocServicesSpecification_)
    {
        return AggregateHealthParallelQuerySpecificationBase::OnParallelQueryExecutionComplete(activityId, queryResults, replyMessage);
    }

    ErrorCode error(ErrorCodeValue::Success);

    vector<ServiceQueryResult> fmResultList;
    vector<ServiceQueryResult> cmResultList;
    vector<ServiceAggregatedHealthState> hmResultList;

    // For all inner queries, look if there is continuation token.
    // If so, the merging will stop at the last NodeId that is contained by all queries
    MergedQueryListPager<wstring, ServiceQueryResult> resultsPager;
    for (auto itQueryResult = queryResults.begin(); itQueryResult != queryResults.end(); ++itQueryResult)
    {
        ErrorCode e(ErrorCodeValue::InvalidAddress);
        if (itQueryResult->first->AddressString == QueryAddresses::GetFM().AddressString)
        {
            e = itQueryResult->second.MoveList<ServiceQueryResult>(fmResultList);
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetCM().AddressString)
        {
            e = itQueryResult->second.MoveList<ServiceQueryResult>(cmResultList);
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetHMViaCM().AddressString)
        {
            e = itQueryResult->second.MoveList<ServiceAggregatedHealthState>(hmResultList);
        }

        if (!e.IsSuccess())
        {
            WriteInfo(
                TraceType,
                "{0}: Error '{1}' while obtaining result from sub-query with address '{2}' during merge",
                activityId,
                e,
                itQueryResult->first->AddressString);

            error = ErrorCode::FirstError(error, e);
            continue;
        }

        // Best effort update min continuation token
        // On error, use the results from the inner query as if they are all there
        resultsPager.MergePagingStatus(
            activityId,
            itQueryResult->first->AddressString,
            itQueryResult->second.MovePagingStatus()).ReadValue();
    }

    WriteInfo(
        TraceType,
        "{0}: {1}: sub-query results count = FM {2} / CM {3} / HM {4}. Merged continuation token: {5}",
        activityId,
        QueryNames::GetApplicationServiceList,
        fmResultList.size(),
        cmResultList.size(),
        hmResultList.size(),
        resultsPager.ContinuationToken);

    map<wstring, ServiceQueryResult> fmResultMap = CreateServiceResultMap(move(fmResultList));

    map<wstring, FABRIC_HEALTH_STATE> hmResultMap;
    for (auto itHmResultItem = hmResultList.begin(); itHmResultItem != hmResultList.end(); ++itHmResultItem)
    {
        hmResultMap.insert(make_pair(move(itHmResultItem->ServiceName), itHmResultItem->AggregatedHealthState));
    }

    if (!cmResultList.empty())
    {
        // Reset the error in case there was a previous error, if we have results from CM that can be sent to the client.
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceType,
                "{0} resetting error {1} because CM has results",
                activityId,
                error);
            error.Reset(ErrorCodeValue::Success);
        }
    }

    // Check CM results against FM and HM
    for (auto itCmResultItem = cmResultList.begin(); itCmResultItem != cmResultList.end(); ++itCmResultItem)
    {
        wstring serviceName = itCmResultItem->ServiceName.ToString();
        auto itFmFind = fmResultMap.find(serviceName);
        if (itFmFind == fmResultMap.end())
        {
            WriteInfo(
                TraceType,
                "{0}: Service {1} is present in CM result but not in FM result",
                activityId,
                serviceName);
        }
        else
        {
            // Get IsServiceGroup flag from CM query result
            itCmResultItem->IsServiceGroup = itFmFind->second.IsServiceGroup;
            // Merging policy always prefer CM's status over FM's
            if (itCmResultItem->ServiceStatus == FABRIC_QUERY_SERVICE_STATUS_UNKNOWN)
            {
                // Get ServiceStatus from FM query result
                itCmResultItem->ServiceStatus = itFmFind->second.ServiceStatus;
            }
            fmResultMap.erase(itFmFind);
        }

        resultsPager.Add(move(serviceName), move(*itCmResultItem));
    }

    // Check remaining FM results (that weren't found in CM)
    for (auto itFmResultItem = fmResultMap.begin(); itFmResultItem != fmResultMap.end(); ++itFmResultItem)
    {
        resultsPager.Add(move(itFmResultItem->first), move(itFmResultItem->second));
    }

    resultsPager.UpdateHealthStates(activityId, hmResultMap);

    replyMessage = move(Common::make_unique<Transport::Message>(QueryResult(resultsPager.TakePager(activityId))));
    return error;
}

map<wstring, ServiceQueryResult> GetApplicationServiceListQuerySpecification::CreateServiceResultMap(
    vector<ServiceQueryResult> && serviceQueryResults)
{
    map<wstring, ServiceQueryResult> resultMap;
    for (auto itResultItem = serviceQueryResults.begin(); itResultItem != serviceQueryResults.end(); ++itResultItem)
    {
        auto serviceName = itResultItem->ServiceName.ToString();
        resultMap.insert(make_pair(move(serviceName), move(*itResultItem)));
    }

    return move(resultMap);
}
