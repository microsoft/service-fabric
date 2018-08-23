// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

StringLiteral const TraceType("GetServiceResourceListQuerySpecification");

GetServiceResourceListQuerySpecification::GetServiceResourceListQuerySpecification()
    : AggregateHealthParallelQuerySpecificationBase<ContainerServiceQueryResult, wstring>(
        QueryNames::GetServiceResourceList,
        QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false))
{
    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetApplicationServiceList,
            QueryAddresses::GetFM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetServiceResourceList,
            QueryAddresses::GetCM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServiceHealthList,
            Query::QueryAddresses::GetHM(),
            QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
}

bool GetServiceResourceListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return (querySpecification->QueryName == QueryNames::GetServiceResourceList) ||
           (querySpecification->QueryName == QueryNames::GetApplicationServiceList);
}

ErrorCode GetServiceResourceListQuerySpecification::OnParallelQueryExecutionComplete(
    Common::ActivityId const & activityId,
    map<QuerySpecificationSPtr, QueryResult> & queryResults,
    __out Transport::MessageUPtr & replyMessage)
{
    vector<ServiceQueryResult> fmResultList;
    vector<ContainerServiceQueryResult> cmResultList;
    vector<ServiceAggregatedHealthState> hmResultList;

    ErrorCode error;
    // For all inner queries, look if there is continuation token.
    MergedQueryListPager<wstring, ContainerServiceQueryResult> resultsPager;
    for (auto itQueryResult = queryResults.begin(); itQueryResult != queryResults.end(); ++itQueryResult)
    {
        ErrorCode e(ErrorCodeValue::InvalidAddress);
        if (itQueryResult->first->AddressString == QueryAddresses::GetFM().AddressString)
        {
            e = itQueryResult->second.MoveList<ServiceQueryResult>(fmResultList);
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetCM().AddressString)
        {
            e = itQueryResult->second.MoveList<ContainerServiceQueryResult>(cmResultList);
        }
        else if (itQueryResult->first->AddressString == QueryAddresses::GetHM().AddressString)
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
        QueryNames::GetServiceResourceList,
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
        wstring serviceUri = itCmResultItem->ServiceUri;
        auto itFmFind = fmResultMap.find(serviceUri);
        if (itFmFind == fmResultMap.end())
        {
            WriteInfo(
                TraceType,
                "{0}: Service {1} is present in CM result but not in FM result",
                activityId,
                serviceUri);
        }
        else
        {
            // Merging policy always prefer CM's status over FM's
            if (itCmResultItem->Status == FABRIC_QUERY_SERVICE_STATUS_UNKNOWN)
            {
                // Get ServiceStatus from FM query result
                itCmResultItem->Status = itFmFind->second.ServiceStatus;
            }
            fmResultMap.erase(itFmFind);
        }

        resultsPager.Add(move(serviceUri), move(*itCmResultItem));
    }

    resultsPager.UpdateHealthStates(activityId, hmResultMap);

    replyMessage = move(Common::make_unique<Transport::Message>(QueryResult(resultsPager.TakePager(activityId))));
    return error;
}

ErrorCode GetServiceResourceListQuerySpecification::AddEntityKeyFromHealthResult(
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

wstring GetServiceResourceListQuerySpecification::GetEntityKeyFromEntityResult(ContainerServiceQueryResult const & entityInformation)
{
    return entityInformation.ServiceUri;
}


map<wstring, ServiceQueryResult> GetServiceResourceListQuerySpecification::CreateServiceResultMap(
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
