// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

vector<QuerySpecificationSPtr> GetServicePartitionReplicaListQuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    resultSPtr.push_back(make_shared<GetServicePartitionReplicaListQuerySpecification>(false));
    resultSPtr.push_back(make_shared<GetServicePartitionReplicaListQuerySpecification>(true));

    return move(resultSPtr);
}

GetServicePartitionReplicaListQuerySpecification::GetServicePartitionReplicaListQuerySpecification(
    bool FMMQuery)
    : AggregateHealthParallelQuerySpecificationBase<ServiceModel::ServiceReplicaQueryResult, int64>(
        Query::QueryNames::GetServicePartitionReplicaList,
        Query::QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Partition::PartitionId),
        QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId, false),
        QueryArgument(Query::QueryResourceProperties::Replica::InstanceId, false),
        QueryArgument(Query::QueryResourceProperties::Replica::ReplicaStatusFilter, (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false))
{
    querySpecificationId_ = GetInternalSpecificationId(FMMQuery);

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
        QueryNames::GetServicePartitionReplicaList,
        FMMQuery ? Query::QueryAddresses::GetFMM() : Query::QueryAddresses::GetFM(),
        QueryArgument(Query::QueryResourceProperties::Partition::PartitionId),
        QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId, false),
        QueryArgument(Query::QueryResourceProperties::Replica::InstanceId, false),
        QueryArgument(Query::QueryResourceProperties::Replica::ReplicaStatusFilter, (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));

    ParallelQuerySpecifications.push_back(
        make_shared<QuerySpecification>(
        QueryNames::GetAggregatedServicePartitionReplicaHealthList,
            Query::QueryAddresses::GetHM(),
            QueryArgument(Query::QueryResourceProperties::Partition::PartitionId),
            QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId, false),
            QueryArgument(Query::QueryResourceProperties::Replica::InstanceId, false),
            QueryArgument(Query::QueryResourceProperties::Replica::ReplicaStatusFilter, (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT),
            QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
}

bool GetServicePartitionReplicaListQuerySpecification::ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs)
{
    wstring partitionName;
    QueryAddressGenerator address = Query::QueryAddresses::GetFM();
    if (queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionName))
    {
        if (partitionName == Reliability::Constants::FMServiceConsistencyUnitId.ToString())
        {
            return true;
        }
    }

    return false;
}

wstring GetServicePartitionReplicaListQuerySpecification::GetSpecificationId(QueryArgumentMap const& queryArgs)
{
    return GetInternalSpecificationId(ShouldSendToFMM(queryArgs));
}

wstring GetServicePartitionReplicaListQuerySpecification::GetInternalSpecificationId(bool sendToFMM)
{
    if (sendToFMM)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetServicePartitionReplicaList), L"/FMM");
    }

    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetServicePartitionReplicaList), L"/FM");
}

bool GetServicePartitionReplicaListQuerySpecification::IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification)
{
    return querySpecification->QueryName == QueryNames::GetServicePartitionReplicaList;
}

Common::ErrorCode GetServicePartitionReplicaListQuerySpecification::AddEntityKeyFromHealthResult(
    __in ServiceModel::QueryResult & queryResult,
    __inout std::map<int64, FABRIC_HEALTH_STATE> & healthStateMap)
{
    vector<ReplicaAggregatedHealthState> healthQueryResult;
    auto error = queryResult.MoveList(healthQueryResult);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto itHealthInfo = healthQueryResult.begin(); itHealthInfo != healthQueryResult.end(); ++itHealthInfo)
    {
        healthStateMap.insert(std::make_pair(itHealthInfo->ReplicaId, itHealthInfo->AggregatedHealthState));
    }

    return error;
}

int64 GetServicePartitionReplicaListQuerySpecification::GetEntityKeyFromEntityResult(ServiceModel::ServiceReplicaQueryResult const & entityInformation)
{
    return entityInformation.Kind == FABRIC_SERVICE_KIND_STATEFUL ? entityInformation.ReplicaId : entityInformation.InstanceId;
}
