// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

vector<QuerySpecificationSPtr> GetServicePartitionListQuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    resultSPtr.push_back(make_shared<GetServicePartitionListQuerySpecification>(false));
    resultSPtr.push_back(make_shared<GetServicePartitionListQuerySpecification>(true));

    return move(resultSPtr);
}

GetServicePartitionListQuerySpecification::GetServicePartitionListQuerySpecification(
    bool FMMQuery)
    : AggregateHealthParallelQuerySpecificationBase<ServiceModel::ServicePartitionQueryResult, Common::Guid>(
        Query::QueryNames::GetServicePartitionList,
        Query::QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
        QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false))
{
    // TODO: Set the query iD
    querySpecificationId_ = GetInternalSpecificationId(FMMQuery);

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
        QueryNames::GetServicePartitionList,
        FMMQuery ? Query::QueryAddresses::GetFMM():Query::QueryAddresses::GetFM(),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
        QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
            QueryNames::GetAggregatedServicePartitionHealthList,
            Query::QueryAddresses::GetHM(),
            QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
            QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
}

bool GetServicePartitionListQuerySpecification::ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs)
{
    wstring serviceName;
    wstring partitionId;
    if (queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceName))
    {
        if (serviceName == SystemServiceApplicationNameHelper::PublicFMServiceName)
        {
            return true;
        }
    }
    if (queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionId))
    {
        Guid partitionIdGuid(partitionId);
        if (partitionIdGuid == ServiceModel::Constants::FMServiceGuid)
        {
            return true;
        }
    }

    return false;
}

bool GetServicePartitionListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetServicePartitionList;
}

wstring GetServicePartitionListQuerySpecification::GetSpecificationId(QueryArgumentMap const& queryArgs)
{
    return GetInternalSpecificationId(ShouldSendToFMM(queryArgs));
}

wstring GetServicePartitionListQuerySpecification::GetInternalSpecificationId(bool sendToFMM)
{
    if (sendToFMM)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetServicePartitionList), L"/FMM");
    }

    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetServicePartitionList), L"/FM");
}

Common::ErrorCode GetServicePartitionListQuerySpecification::AddEntityKeyFromHealthResult(
    __in ServiceModel::QueryResult & queryResult,
    __inout std::map<Common::Guid, FABRIC_HEALTH_STATE> & healthStateMap)
{
    vector<PartitionAggregatedHealthState> healthQueryResult;
    auto error = queryResult.MoveList(healthQueryResult);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itHealthInfo = healthQueryResult.begin(); itHealthInfo != healthQueryResult.end(); ++itHealthInfo)
    {
        healthStateMap.insert(std::make_pair(itHealthInfo->PartitionId, itHealthInfo->AggregatedHealthState));
    }

    return error;
}

Common::Guid GetServicePartitionListQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::ServicePartitionQueryResult const & entityInformation)
{
    return entityInformation.PartitionId;
}
