// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType("GetApplicationServiceGroupMemberListQuerySpecification");

vector<QuerySpecificationSPtr> GetApplicationServiceGroupMemberListQuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    resultSPtr.push_back(make_shared<GetApplicationServiceGroupMemberListQuerySpecification>(false));
    resultSPtr.push_back(make_shared<GetApplicationServiceGroupMemberListQuerySpecification>(true));

    return move(resultSPtr);
}

GetApplicationServiceGroupMemberListQuerySpecification::GetApplicationServiceGroupMemberListQuerySpecification(
    bool adhocService)
    : AggregateHealthParallelQuerySpecificationBase<ServiceGroupMemberQueryResult, wstring>(
        QueryNames::GetApplicationServiceGroupMemberList,
        QueryAddresses::GetGateway(),
        QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false))
    , isAdhocServicesSpecification_(adhocService)
{
    querySpecificationId_ = GetInternalSpecificationId(adhocService);

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
        QueryNames::GetApplicationServiceGroupMemberList,
        QueryAddresses::GetFM(),
        QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false)));

    if (isAdhocServicesSpecification_)
    {
        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServiceHealthList,
            QueryAddresses::GetHM(),
            QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false)));
    }
    else
    {
        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
            QueryNames::GetApplicationServiceList,// CM treats Service Group as regular Service
            QueryAddresses::GetCM(),
            QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false)));

        ParallelQuerySpecifications.push_back(
            make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServiceHealthList,
            QueryAddresses::GetHMViaCM(),
            QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false)));
    }
}

bool GetApplicationServiceGroupMemberListQuerySpecification::IsAdhocServicesRequest(ServiceModel::QueryArgumentMap const & queryArgs)
{
    return !queryArgs.ContainsKey(QueryResourceProperties::Application::ApplicationName);
}

bool GetApplicationServiceGroupMemberListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetApplicationServiceGroupMemberList;
}

wstring GetApplicationServiceGroupMemberListQuerySpecification::GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs)
{
    return GetInternalSpecificationId(IsAdhocServicesRequest(queryArgs));
}

wstring GetApplicationServiceGroupMemberListQuerySpecification::GetInternalSpecificationId(bool isAdhoc)
{
    if (isAdhoc)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetApplicationServiceGroupMemberList), L"/AdHoc");
    }

    return QueryNames::ToString(QueryNames::GetApplicationServiceGroupMemberList);
}

Common::ErrorCode GetApplicationServiceGroupMemberListQuerySpecification::AddEntityKeyFromHealthResult(
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
        healthStateMap.insert(std::make_pair(itHealthInfo->ServiceName, itHealthInfo->AggregatedHealthState));
    }

    return error;
}

wstring GetApplicationServiceGroupMemberListQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::ServiceGroupMemberQueryResult const & entityInformation)
{
    return entityInformation.ServiceName.ToString();
}

ErrorCode GetApplicationServiceGroupMemberListQuerySpecification::OnParallelQueryExecutionComplete(
    Common::ActivityId const & activityId,
    map<QuerySpecificationSPtr, QueryResult> & queryResults,
    __out Transport::MessageUPtr & replyMessage)
{
    if (isAdhocServicesSpecification_)
    {
        return AggregateHealthParallelQuerySpecificationBase::OnParallelQueryExecutionComplete(activityId, queryResults, replyMessage);
    }

    ErrorCode error(ErrorCodeValue::Success);

    vector<ServiceGroupMemberQueryResult> fmResultList;
    vector<ServiceQueryResult> cmResultList;
    vector<ServiceAggregatedHealthState> hmResultList;

    for (auto itQueryResult = queryResults.begin(); itQueryResult != queryResults.end(); ++itQueryResult)
    {
        ErrorCode e(ErrorCodeValue::InvalidAddress);
        if (itQueryResult->first->AddressString == QueryAddresses::GetFM().AddressString)
        {
            e = itQueryResult->second.MoveList<ServiceGroupMemberQueryResult>(fmResultList);
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
    }

    map<wstring, ServiceGroupMemberQueryResult> fmResultMap = CreateServiceGroupMemberResultMap(move(fmResultList));
    map<wstring, ServiceQueryResult> cmResultMap = CreateServiceResultMap(move(cmResultList));

    map<wstring, FABRIC_HEALTH_STATE> hmResultMap;
    for (auto itHmResultItem = hmResultList.begin(); itHmResultItem != hmResultList.end(); ++itHmResultItem)
    {
        hmResultMap.insert(make_pair(itHmResultItem->ServiceName, itHmResultItem->AggregatedHealthState));
    }

    vector<ServiceGroupMemberQueryResult> mergedResultList;

    // Check CM results against FM and HM
    for (auto itFmResultItem = fmResultMap.begin(); itFmResultItem != fmResultMap.end(); ++itFmResultItem)
    {
        auto itCmFind = cmResultMap.find(itFmResultItem->first);
        if (itCmFind == cmResultMap.end())
        {
            WriteError(
                TraceType,
                "{0}: Service {1} is present in FM result but not in CM result",
                activityId,
                itFmResultItem->first);
        }
        else
        {
            // Get ServiceStatus from FM query result
            itFmResultItem->second.ServiceStatus = itCmFind->second.ServiceStatus;
            cmResultMap.erase(itCmFind);
        }

        auto itHmFind = hmResultMap.find(itFmResultItem->first);
        if (itHmFind == hmResultMap.end())
        {
            WriteError(
                TraceType,
                "{0}: Service {1} is present in FM result but not in HM result",
                activityId,
                itFmResultItem->first);
        }
        else
        {
            // Get HealthState from HM query result
            itFmResultItem->second.HealthState = itHmFind->second;
            hmResultMap.erase(itHmFind);
        }

        mergedResultList.push_back(move(itFmResultItem->second));
    }

    replyMessage = make_unique<Transport::Message>(QueryResult(move(mergedResultList)));
    return error;
}

map<wstring, ServiceGroupMemberQueryResult> GetApplicationServiceGroupMemberListQuerySpecification::CreateServiceGroupMemberResultMap(
    vector<ServiceGroupMemberQueryResult> && serviceGroupMemberQueryResults)
{
    map<wstring, ServiceGroupMemberQueryResult> resultMap;
    for (auto itResultItem = serviceGroupMemberQueryResults.begin(); itResultItem != serviceGroupMemberQueryResults.end(); ++itResultItem)
    {
        auto serviceName = itResultItem->ServiceName.ToString();
        resultMap.insert(make_pair(move(serviceName), move(*itResultItem)));
    }

    return move(resultMap);
}

map<wstring, ServiceQueryResult> GetApplicationServiceGroupMemberListQuerySpecification::CreateServiceResultMap(
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
