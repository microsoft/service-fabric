//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceType("GetReplicaResourceListQuerySpecification");

// TODO: find out how to track original activity id.
GetReplicaResourceListQuerySpecification::GetReplicaResourceListQuerySpecification()
    : SequentialQuerySpecification(
        QueryNames::Enum::GetReplicaResourceList,
        QueryAddresses::GetGateway(),
        QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
        QueryArgument(Query::QueryResourceProperties::Service::ServiceName, true),
        QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
        QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false))
{
}

ErrorCode GetReplicaResourceListQuerySpecification::GetNext(
    QuerySpecificationSPtr const & previousQuerySpecification,
    __in std::unordered_map<QueryNames::Enum, ServiceModel::QueryResult> & queryResults,
    __inout QueryArgumentMap & queryArgs,
    __out Query::QuerySpecificationSPtr & nextQuery,
    __out Transport::MessageUPtr & replyMessage)
{
    switch (previousQuerySpecification->QueryName)
    {
        case QueryNames::Enum::GetReplicaResourceList:
        {
            nextQuery = QuerySpecificationStore::Get().GetSpecification(
                QueryNames::Enum::GetServiceResourceList,
                queryArgs);
            break;
        }
        case QueryNames::Enum::GetServiceResourceList:
        {
            vector<ModelV2::ContainerServiceQueryResult> serviceResourceResult;
            auto error = queryResults[QueryNames::Enum::GetServiceResourceList].GetList<ModelV2::ContainerServiceQueryResult>(serviceResourceResult);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (serviceResourceResult.size() == 0)
            {
                replyMessage = QueryMessage::GetQueryReply(queryResults[QueryNames::Enum::GetServiceResourceList]);
                return ErrorCodeValue::EnumerationCompleted;
            }

            ASSERT_IF(serviceResourceResult.size() != 1, "serviceResourceResult size is not 1 actually size is", serviceResourceResult.size());

            NamesArgument serviceNames;
            for (auto const & serviceResource : serviceResourceResult)
            {
                serviceNames.Names.push_back(serviceResource.ServiceUri);
            }

            wstring serviceNamesSerialized;
            error = JsonHelper::Serialize(serviceNames, serviceNamesSerialized);
            if (!error.IsSuccess())
            {
                return error;
            }

            QueryArgumentMap argsMap;
            argsMap.Insert(
                QueryResourceProperties::Service::ServiceNames,
                serviceNamesSerialized);

            wstring replicaIdFilter;
            if (queryArgs.TryGetValue(QueryResourceProperties::Replica::ReplicaId, replicaIdFilter))
            {
                // This is actually named partition filter.
                argsMap.Insert(
                    QueryResourceProperties::Replica::ReplicaId,
                    StringUtility::ToWString(replicaIdFilter));
            }

            queryArgs = move(argsMap);

            nextQuery = QuerySpecificationStore::Get().GetSpecification(
                QueryNames::Enum::GetReplicaListByServiceNames,
                queryArgs);

            break;
        }
        case QueryNames::Enum::GetReplicaListByServiceNames:
        {
            vector<ModelV2::ContainerServiceQueryResult> serviceResourceResult;
            auto error = queryResults[QueryNames::Enum::GetServiceResourceList].GetList<ModelV2::ContainerServiceQueryResult>(serviceResourceResult);
            if (!error.IsSuccess())
            {
                return error;
            }

            vector<ReplicasByServiceQueryResult> replicaResult;
            error = queryResults[QueryNames::Enum::GetReplicaListByServiceNames].GetList<ReplicasByServiceQueryResult>(replicaResult);
            if (!error.IsSuccess())
            {
                return error;
            }

            vector<wstring> nodeNames;
            for (auto const & replicasByService : replicaResult)
            {
                vector<ReplicaInfoResult> const & replicaInfos = replicasByService.ReplicaInfos;
                // In a stable situation, there should be only 1 replica within a node. We need to handle when there are more if replicas are moving or |node| < |replica|.
                // TODO: More replica within 1 CGS/CG on the same node.
                for (auto const & replicaInfo : replicaInfos)
                {
                    nodeNames.push_back(replicaInfo.NodeName);
                }
            }

            if (nodeNames.size() > 0)
            {
                nextQuery = make_shared<GetDeployedCodePackageParallelQuerySpecification>(
                        serviceResourceResult[0].ApplicationUri,
                        nodeNames);
                break;
            }
            else
            {
                ListPager<ReplicaResourceQueryResult> resultList;
                replyMessage = QueryMessage::GetQueryReply(QueryResult(move(resultList)));

                return ErrorCodeValue::EnumerationCompleted;
            }
        }
        case QueryNames::Enum::GetDeployedCodePackageListByApplication:
        {
            vector<ModelV2::ContainerServiceQueryResult> serviceResourceResult;
            auto error = queryResults[QueryNames::Enum::GetServiceResourceList].MoveList<ModelV2::ContainerServiceQueryResult>(serviceResourceResult);
            if (!error.IsSuccess())
            {
                return error;
            }

            ModelV2::ContainerServiceQueryResult serviceResource = serviceResourceResult[0];

            vector<ReplicasByServiceQueryResult> replicaResult;
            error = queryResults[QueryNames::Enum::GetReplicaListByServiceNames].MoveList<ReplicasByServiceQueryResult>(replicaResult);
            if (!error.IsSuccess())
            {
                return error;
            }

            vector<DeployedCodePackageQueryResult> codePackagesResult;
            error = queryResults[QueryNames::Enum::GetDeployedCodePackageListByApplication].MoveList<DeployedCodePackageQueryResult>(codePackagesResult);
            if (!error.IsSuccess())
            {
                return error;
            }

            ListPager<ReplicaResourceQueryResult> resultList;

            wstring maxResultsString;
            if (queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString))
            {
                int64 maxResults = std::numeric_limits<int64>::max();
                auto int64Parser = StringUtility::ParseIntegralString<int64, true>();
                if (int64Parser.Try(maxResultsString, maxResults, 0))
                {
                    resultList.SetMaxResults(maxResults);
                }
            }

            wstring serviceName = replicaResult[0].ServiceName;

            // Size has been checked to be > 0
            // If an instance filtered by GetReplicaListByServiceNames paging or GetDeployedCodePackage timed out, we don't return InstanceView
            // We enumarate based on resultResult(FM) since this query is meant for the live deployed view. If FM's result is bigger/smaller than CM, we still go ahead and return the size of FM's replica result.
            for (ReplicaInfoResult const & replicaInfoResult : replicaResult[0].ReplicaInfos)
            {
                ReplicaResourceQueryResult queryResult(replicaInfoResult.PartitionName); // Partition name is replica name.

                wstring nodeName = replicaInfoResult.NodeName;
                // TODO: Should we enumerate based on codePackageResult? The mismatch could happen if service is upgrading and node view vs. CM view is different.
                for (ModelV2::ContainerCodePackageDescription & codePackageDescription : serviceResource.Properties.CodePackages)
                {
                    wstring codePackageName = codePackageDescription.Name;
                    auto codePackage = find_if(codePackagesResult.begin(), codePackagesResult.end(),
                        [&nodeName, &codePackageName, &serviceName](DeployedCodePackageQueryResult const & item)
                        {
                            return
                                item.NodeName == nodeName &&
                                item.CodePackageName == codePackageName &&
                                item.ServiceNameInternalUseOnly == serviceName;
                        });

                    if (codePackage != codePackagesResult.end())
                    {
                        CodePackageEntryPoint const & mainEntryPoint = codePackage->EntryPoint;

                        ModelV2::InstanceState previousState(
                            EntryPointStatus::ToString(EntryPointStatus::Invalid),
                            0);
                        ModelV2::InstanceState currentState(
                            EntryPointStatus::ToString(mainEntryPoint.EntryPointStatus),
                            mainEntryPoint.Statistics.LastExitCode);

                        codePackageDescription.InstanceViewSPtr = make_shared<ModelV2::CodePackageInstanceView>(
                            mainEntryPoint.Statistics.ActivationCount,
                            currentState,
                            previousState);
                    }
                }

                queryResult.CodePackages = serviceResource.Properties.CodePackages;
                queryResult.OsType = serviceResource.Properties.OsType;
                queryResult.NetworkRefs = serviceResource.Properties.NetworkRefs;

                resultList.TryAdd(move(queryResult));
            }

            replyMessage = QueryMessage::GetQueryReply(QueryResult(move(resultList)));
            return ErrorCodeValue::EnumerationCompleted;
        }
        default:
            replyMessage = QueryMessage::GetQueryFailure(QueryResult(ErrorCodeValue::InvalidArgument));
            return ErrorCodeValue::InvalidArgument;
    }

    replyMessage = QueryMessage::GetQueryReply(QueryResult(ErrorCodeValue::Success));
    return ErrorCodeValue::Success;
}

bool GetReplicaResourceListQuerySpecification::ShouldContinueSequentialQuery(
    Query::QuerySpecificationSPtr const & lastExecutedQuerySpecification,
    Common::ErrorCode const & error)
{
    // Instance view can be empty. Continue to return without code package information.
    if (lastExecutedQuerySpecification->QueryName == QueryNames::Enum::GetDeployedCodePackageListByApplication)
    {
        return true;
    }
    else
    {
        return SequentialQuerySpecification::ShouldContinueSequentialQuery(lastExecutedQuerySpecification, error);
    }
}
