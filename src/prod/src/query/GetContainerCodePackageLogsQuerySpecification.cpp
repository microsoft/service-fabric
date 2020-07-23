//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Query;

StringLiteral const TraceType("GetContainerCodePackageLogsQuerySpecification");

GetContainerCodePackageLogsQuerySpecification::GetContainerCodePackageLogsQuerySpecification()
    : SequentialQuerySpecification(
        QueryNames::GetContainerCodePackageLogs,
        QueryAddresses::GetGateway(),
        QueryArgument(QueryResourceProperties::Application::ApplicationName, true),
        QueryArgument(QueryResourceProperties::Service::ServiceName, true),
        QueryArgument(QueryResourceProperties::Replica::ReplicaId, true),
        QueryArgument(QueryResourceProperties::CodePackage::CodePackageName, true),
        QueryArgument(Query::QueryResourceProperties::ContainerInfo::InfoArgsFilter, false))
{
}

ErrorCode GetContainerCodePackageLogsQuerySpecification::GetNext(
    QuerySpecificationSPtr const & previousQuerySpecification,
    unordered_map<QueryNames::Enum, QueryResult> & queryResults,
    QueryArgumentMap & queryArgs,
    QuerySpecificationSPtr & nextQuery,
    Transport::MessageUPtr & replyMessage)
{
    switch (previousQuerySpecification->QueryName)
    {
        case QueryNames::GetContainerCodePackageLogs:
        {
            nextQuery = QuerySpecificationStore::Get().GetSpecification(QueryNames::GetServicePartitionList, queryArgs);
            break;
        }
        case QueryNames::GetServicePartitionList:
        {
            vector<ServicePartitionQueryResult> previousQueryList;
            auto error = queryResults[previousQuerySpecification->QueryName].MoveList<ServicePartitionQueryResult>(previousQueryList);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (previousQueryList.empty())
            {
                return ErrorCodeValue::PartitionNotFound;
            }

            wstring replicaName;
            queryArgs.TryGetValue(QueryResourceProperties::Replica::ReplicaId, replicaName);

            wstring partitionId;
            for (auto const &partitionResult : previousQueryList)
            {
                if (partitionResult.PartitionInformation.PartitionName.empty() ||
                    partitionResult.PartitionInformation.PartitionName != replicaName)
                {
                    continue;
                }

                partitionId = partitionResult.PartitionInformation.PartitionId.ToString();
            }

            if (partitionId.empty())
            {
                return ErrorCodeValue::NameNotFound;
            }

            WriteInfo(
                TraceType,
                "Starting get service partition query for partition {0}",
                partitionId);

            QueryArgumentMap argsMap;
            argsMap.Insert(
                QueryResourceProperties::Partition::PartitionId,
                partitionId);

            queryArgs = move(argsMap);
            nextQuery = QuerySpecificationStore::Get().GetSpecification(QueryNames::GetServicePartitionReplicaList, queryArgs);
            break;
        }
        case QueryNames::GetServicePartitionReplicaList:
        {
            queryArgs.Insert(
                QueryResourceProperties::ContainerInfo::InfoTypeFilter,
                StringUtility::ToWString(ContainerInfoType::Enum::Logs));

            vector<ServiceReplicaQueryResult> previousQueryList;
            auto error = queryResults[previousQuerySpecification->QueryName].MoveList<ServiceReplicaQueryResult>(previousQueryList);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (previousQueryList.empty())
            {
                return ErrorCodeValue::ReplicaDoesNotExist;
            }

            wstring applicationNameUri;
            queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameUri);

            wstring serviceNameUri;
            queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceNameUri);

            auto serviceManifestName = ContainerGroupDeploymentUtility::GetServiceManifestName(GetRelativeServiceName(applicationNameUri, serviceNameUri));

            queryArgs.Insert(
                QueryResourceProperties::ServiceType::ServiceManifestName,
                serviceManifestName);

            auto targetNodeName = (*previousQueryList.begin()).NodeName;
            if (previousQueryList.size() > 1) // More than one replica found in the partition, pick the primary for now.
            {
                for(auto const &replicaQueryResult : previousQueryList)
                {
                    if (replicaQueryResult.ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY)
                    {
                        targetNodeName = replicaQueryResult.NodeName;
                        break;
                    }
                }
            }

            queryArgs.Insert(
                QueryResourceProperties::Node::Name,
                targetNodeName);

            nextQuery = QuerySpecificationStore::Get().GetSpecification(QueryNames::GetContainerInfoDeployedOnNode, queryArgs);
            break;
        }
        case QueryNames::GetContainerInfoDeployedOnNode:
        {
            replyMessage = Common::make_unique<Transport::Message>(queryResults[previousQuerySpecification->QueryName]);
            return ErrorCodeValue::EnumerationCompleted;
        }
        default:
        {
            replyMessage = QueryMessage::GetQueryFailure(QueryResult(ErrorCodeValue::InvalidArgument));
            return ErrorCodeValue::InvalidArgument;
        }
    }

    replyMessage = QueryMessage::GetQueryReply(QueryResult(ErrorCodeValue::Success));
    return ErrorCodeValue::Success;
}


wstring GetContainerCodePackageLogsQuerySpecification::GetRelativeServiceName(wstring const & appName, wstring const & absoluteServiceName)
{
    wstring serviceName(absoluteServiceName);

    StringUtility::Replace<wstring>(serviceName, appName, L"");
    StringUtility::Trim<wstring>(serviceName, L"/");

    return serviceName;
}

