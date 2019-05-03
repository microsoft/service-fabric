// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

vector<QuerySpecificationSPtr> QuerySpecification::CreateSpecifications()
{
    vector<QuerySpecificationSPtr> resultSPtr;

    resultSPtr.reserve(QueryNames::LAST_QUERY_PLUS_ONE);

    //
    // QueryName enum is a sequentially increasing enum.
    //
    for (auto queryName = (QueryNames::Enum)(QueryNames::FIRST_QUERY_MINUS_ONE + 1);
        queryName < QueryNames::LAST_QUERY_PLUS_ONE;
        queryName = (QueryNames::Enum)(queryName+1))
    {
        switch (queryName)
        {
        case QueryNames::GetQueries:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetGateway()));
                break;
            }

        case QueryNames::GetApplicationList:
            {
                resultSPtr.push_back(make_shared<GetApplicationListQuerySpecification>());
                break;
            }

        case QueryNames::GetApplicationTypeList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, false)));
                break;
            }

        case QueryNames::GetApplicationTypePagedList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetCM(),
                QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, false),
                QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeVersion, false),
                QueryArgument(Query::QueryResourceProperties::Deployment::ApplicationTypeDefinitionKindFilter, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetNodeList:
            {
                auto tempResults = GetNodeListQuerySpecification::CreateSpecifications();
                for (auto it = tempResults.begin(); it != tempResults.end(); ++it)
                {
                    resultSPtr.push_back(move(*it));
                }
                break;
            }

        case QueryNames::GetApplicationServiceList:
            {
                auto tempResults = move(GetApplicationServiceListQuerySpecification::CreateSpecifications());
                for (auto it = tempResults.begin(); it != tempResults.end(); ++it)
                {
                    resultSPtr.push_back(move(*it));
                }
                break;
            }

        case QueryNames::GetApplicationServiceGroupMemberList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetFM(),
                    QueryArgument(QueryResourceProperties::Application::ApplicationName, false),
                    QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false)));
                break;
            }

        case QueryNames::GetServiceTypeList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, true),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeVersion, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false)));
                break;
            }

        case QueryNames::GetServiceGroupMemberTypeList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, true),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeVersion, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceGroupTypeName, false)));
                break;
            }

        case QueryNames::GetSystemServicesList:
            {
                resultSPtr.push_back(make_shared<GetSystemServiceListQuerySpecification>());
                break;
            }

        case QueryNames::GetServicePartitionList:
            {
                auto tempResults = move(GetServicePartitionListQuerySpecification::CreateSpecifications());
                for (auto it = tempResults.begin(); it != tempResults.end(); ++it)
                {
                    resultSPtr.push_back(move(*it));
                }
                break;
            }

        case QueryNames::GetServicePartitionReplicaList:
            {
                auto tempResults = move(GetServicePartitionReplicaListQuerySpecification::CreateSpecifications());
                for (auto it = tempResults.begin(); it != tempResults.end(); ++it)
                {
                    resultSPtr.push_back(move(*it));
                }
                break;
            }

        case QueryNames::GetNodeHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Id, true, L"id"),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true, L"name"),
                    QueryArgument(Query::QueryResourceProperties::Health::ClusterHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false)));
                break;
            }
        case QueryNames::GetServicePartitionHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::HealthStatsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ReplicasFilter, false)));
                break;
            }

        case QueryNames::GetServicePartitionReplicaHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId),
                    QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false)));
                break;
            }

        case QueryNames::GetPartitionLoadInformation:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetFM(),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId)));
                break;
            }

        case QueryNames::GetServiceHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Service::ServiceName),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::HealthStatsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::PartitionsFilter, false)));
                break;
            }

        case QueryNames::GetApplicationHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ServicesFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::HealthStatsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::DeployedApplicationsFilter, false)));
                break;
            }

        case QueryNames::GetClusterHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Health::ClusterHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicyMap, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::NodesFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::HealthStatsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationsFilter, false)));
                break;
            }

        case QueryNames::GetDeployedApplicationHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName),
                    QueryArgument(Query::QueryResourceProperties::Node::Id, true, L"id"),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true, L"name"),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::HealthStatsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::Health::DeployedServicePackagesFilter, false)));
                break;
            }

        case QueryNames::GetDeployedServicePackageHealth:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName),
                    QueryArgument(Query::QueryResourceProperties::ServicePackage::ServiceManifestName),
                    QueryArgument(Query::QueryResourceProperties::Node::Id, true, L"id"),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true, L"name"),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::Health::EventsFilter, false),
                    QueryArgument(Query::QueryResourceProperties::ServicePackage::ServicePackageActivationId, false)));
                break;
            }

        case QueryNames::GetAggregatedNodeHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ClusterHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
                break;
            }

        case QueryNames::GetAggregatedDeployedApplicationHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false)));
                break;
            }

        case QueryNames::GetAggregatedDeployedApplicationsOnNodeHealthPagedList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetHM(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
            break;
        }

        case QueryNames::GetAggregatedDeployedServicePackageHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Id, false),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, false),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false)));
                break;
            }

        case QueryNames::GetAggregatedServicePartitionReplicaHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, true),
                    QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId, false),
                    QueryArgument(Query::QueryResourceProperties::Replica::InstanceId, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
                break;
            }

        case QueryNames::GetAggregatedServicePartitionHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
                break;
            }

        case QueryNames::GetAggregatedServiceHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::Service::ServiceName, false),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false),
                    QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicy, false),
                    QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
                break;
            }

        case QueryNames::GetClusterHealthChunk:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetHM(),
                QueryArgument(Query::QueryResourceProperties::Health::ClusterHealthPolicy, false),
                QueryArgument(Query::QueryResourceProperties::Health::ApplicationHealthPolicyMap, false),
                QueryArgument(Query::QueryResourceProperties::Health::NodeFilters, false),
                QueryArgument(Query::QueryResourceProperties::Health::ApplicationFilters, false)));
            break;
        }

        case QueryNames::GetAggregatedApplicationHealthList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, false),
                    QueryArgument(Query::QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, false),
                    QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false)));
                break;
            }

        case QueryNames::GetApplicationManifest:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, true),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeVersion, true)));
                break;
            }

        case QueryNames::GetClusterManifest:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::Cluster::ConfigVersionFilter, false)));
                break;
            }

        case QueryNames::GetClusterVersion:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetCM()));
            break;
        }

        case QueryNames::GetApplicationListDeployedOnNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false)));
                break;
            }

        case QueryNames::GetApplicationPagedListDeployedOnNode:
        {
            auto tempResults = move(GetApplicationPagedListDeployedOnNodeQuerySpecification::CreateSpecifications());
            for (auto it = tempResults.begin(); it != tempResults.end(); ++it)
            {
                resultSPtr.push_back(move(*it));
            }
            break;
        }

        case QueryNames::GetServiceManifest:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, true),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeVersion, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceManifest::ServiceManifestName, true)));
                break;
            }

        case QueryNames::GetServiceManifestListDeployedOnNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceManifestName, false)));
                break;
            }

        case QueryNames::GetServiceTypeListDeployedOnNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceManifestName, false),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceTypeName, false)));
                break;
            }

        case QueryNames::GetCodePackageListDeployedOnNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceManifestName, false),
                    QueryArgument(Query::QueryResourceProperties::CodePackage::CodePackageName, false)));
                break;
            }

        case QueryNames::GetServiceReplicaListDeployedOnNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetRA(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceManifestName, false),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false)));
                break;
            }

        case QueryNames::AddUnreliableTransportBehavior:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::UnreliableTransportBehavior::Name, true),
                    QueryArgument(Query::QueryResourceProperties::UnreliableTransportBehavior::Behavior, true)));
                break;
            }

        case QueryNames::RemoveUnreliableTransportBehavior:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::UnreliableTransportBehavior::Name, true)));
                break;
            }

        case QueryNames::AddUnreliableLeaseBehavior:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetTestability(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::UnreliableLeaseBehavior::Alias, true),
                    QueryArgument(Query::QueryResourceProperties::UnreliableLeaseBehavior::Behavior, true)));
                break;
            }

        case QueryNames::RemoveUnreliableLeaseBehavior:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetTestability(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::UnreliableLeaseBehavior::Alias, true)));
                break;
            }

        case QueryNames::GetTransportBehaviors:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetTestability(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true)));
                break;
            }

        case QueryNames::StopNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Node::InstanceId, true),
                    QueryArgument(Query::QueryResourceProperties::Node::Restart, true),
                    QueryArgument(Query::QueryResourceProperties::Node::CreateFabricDump, false),
                    QueryArgument(Query::QueryResourceProperties::Node::StopDurationInSeconds, false)));
                break;
            }

        case QueryNames::RestartDeployedCodePackage:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceManifest::ServiceManifestName, true),
                    QueryArgument(Query::QueryResourceProperties::CodePackage::CodePackageName, true),
                    QueryArgument(Query::QueryResourceProperties::CodePackage::InstanceId, true),
                    QueryArgument(Query::QueryResourceProperties::ServicePackage::ServicePackageActivationId, false)));
                break;
            }

        case QueryNames::GetContainerInfoDeployedOnNode:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetHosting(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                QueryArgument(Query::QueryResourceProperties::ServiceManifest::ServiceManifestName, true),
                QueryArgument(Query::QueryResourceProperties::CodePackage::CodePackageName, true),
                QueryArgument(Query::QueryResourceProperties::ContainerInfo::InfoTypeFilter, true),
                QueryArgument(Query::QueryResourceProperties::ContainerInfo::InfoArgsFilter, false)));
            break;
        }

        case QueryNames::InvokeContainerApiOnNode:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetHosting(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                QueryArgument(Query::QueryResourceProperties::ServiceManifest::ServiceManifestName, true),
                QueryArgument(Query::QueryResourceProperties::CodePackage::CodePackageName, true),
                QueryArgument(Query::QueryResourceProperties::CodePackage::InstanceId, true),
                QueryArgument(Query::QueryResourceProperties::ContainerInfo::InfoArgsFilter, true)));
            break;
        }

        case QueryNames::GetInfrastructureTask:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM()));
                break;
            }

        case QueryNames::GetDeployedServiceReplicaDetail:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetRA(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, true),
                    QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId, true)));
                break;
            }

        case QueryNames::GetRepairList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetRM(),
                    QueryArgument(Query::QueryResourceProperties::Repair::Scope, true),
                    QueryArgument(Query::QueryResourceProperties::Repair::TaskIdPrefix, true),
                    QueryArgument(Query::QueryResourceProperties::Repair::StateMask, true),
                    QueryArgument(Query::QueryResourceProperties::Repair::ExecutorName, true)));
                break;
            }

        case QueryNames::GetClusterLoadInformation:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    QueryNames::GetClusterLoadInformation,
                    Query::QueryAddresses::GetFM()));
                break;
            }

        case QueryNames::GetProvisionedFabricCodeVersionList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::Cluster::CodeVersionFilter, false)));
                break;
            }

        case QueryNames::GetProvisionedFabricConfigVersionList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::Cluster::ConfigVersionFilter, false)));
                break;
            }

        case QueryNames::GetDeletedApplicationsList:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::Application::ApplicationIds, true)));
                break;
            }
        case QueryNames::DeployServicePackageToNode:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetHosting(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, true),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeVersion, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceManifest::ServiceManifestName, true),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::ServicePackageSharingScope, false),
                    QueryArgument(Query::QueryResourceProperties::ServiceType::SharedPackages, false)));
                break;
            }
        case QueryNames::GetProvisionedApplicationTypePackages:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetCM(),
                    QueryArgument(Query::QueryResourceProperties::ApplicationType::ApplicationTypeName, true)));
                break;
            }

        case QueryNames::GetFMNodeList:
            {
                auto specification = make_shared<QuerySpecification>(
                    QueryNames::GetNodeList,
                    Query::QueryAddresses::GetFM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, false));

                specification->querySpecificationId_ = QueryNames::ToString(queryName);
                resultSPtr.push_back(move(specification));

                break;
            }

        case QueryNames::GetNodeLoadInformation:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    QueryNames::GetNodeLoadInformation,
                    Query::QueryAddresses::GetFM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name)));
                break;
            }

        case QueryNames::GetReplicaLoadInformation:
            {
                resultSPtr.push_back(make_shared<QuerySpecification>(
                    queryName,
                    Query::QueryAddresses::GetFM(),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId),
                    QueryArgument(Query::QueryResourceProperties::Replica::ReplicaId)));
                break;
            }

        case QueryNames::GetUnplacedReplicaInformation:
          {

              auto specification = make_shared<QuerySpecification>(
                  queryName,
                  Query::QueryAddresses::GetFM(),
                  QueryArgument(QueryResourceProperties::Service::ServiceName, true),
                  QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false),
                  QueryArgument(QueryResourceProperties::QueryMetadata::OnlyQueryPrimaries, false));
              specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetUnplacedReplicaInformation), L"/FM");
              resultSPtr.push_back(move(specification));

              specification = make_shared<QuerySpecification>(
                  queryName,
                  Query::QueryAddresses::GetFMM(),
                  QueryArgument(QueryResourceProperties::Service::ServiceName, true),
                  QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, false),
                  QueryArgument(QueryResourceProperties::QueryMetadata::OnlyQueryPrimaries, false));
              specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetUnplacedReplicaInformation), L"/FMM");
              resultSPtr.push_back(move(specification));

            break;
          }

        case QueryNames::MovePrimary:
        {
                auto specification = make_shared<QuerySpecification>(
                    QueryNames::MovePrimary,
                    Query::QueryAddresses::GetFM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, false),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, true),
                    QueryArgument(QueryResourceProperties::QueryMetadata::ForceMove, false));

                specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::MovePrimary), L"/FM");
                resultSPtr.push_back(move(specification));

                specification = make_shared<QuerySpecification>(
                    QueryNames::MovePrimary,
                    Query::QueryAddresses::GetFMM(),
                    QueryArgument(Query::QueryResourceProperties::Node::Name, false),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, true),
                    QueryArgument(QueryResourceProperties::QueryMetadata::ForceMove, false));

                specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::MovePrimary), L"/FMM");
                resultSPtr.push_back(move(specification));

                break;
        }

        case QueryNames::MoveSecondary:
        {
               auto specification = make_shared<QuerySpecification>(
                   QueryNames::MoveSecondary,
                   Query::QueryAddresses::GetFM(),
                   QueryArgument(Query::QueryResourceProperties::Node::CurrentNodeName, true),
                   QueryArgument(Query::QueryResourceProperties::Node::NewNodeName, false),
                   QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, true),
                   QueryArgument(QueryResourceProperties::QueryMetadata::ForceMove, false));

                specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::MoveSecondary), L"/FM");
                resultSPtr.push_back(move(specification));

                specification = make_shared<QuerySpecification>(
                    QueryNames::MoveSecondary,
                    Query::QueryAddresses::GetFMM(),
                    QueryArgument(Query::QueryResourceProperties::Node::CurrentNodeName, true),
                    QueryArgument(Query::QueryResourceProperties::Node::NewNodeName, false),
                    QueryArgument(Query::QueryResourceProperties::Partition::PartitionId, true),
                    QueryArgument(QueryResourceProperties::QueryMetadata::ForceMove, false));

                specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::MoveSecondary), L"/FMM");
                resultSPtr.push_back(move(specification));

                break;
        }

        case QueryNames::GetApplicationLoadInformation:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetApplicationLoadInformation,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName)));
            break;
        }

        case QueryNames::GetTestCommandList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetTestCommandList,
                Query::QueryAddresses::GetTS(),
                QueryArgument(Query::QueryResourceProperties::Action::StateFilter),
                QueryArgument(Query::QueryResourceProperties::Action::TypeFilter)));
            break;
        }

        case QueryNames::GetClusterConfiguration:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetUOS()));
            break;
        }

        case QueryNames::GetServiceName:
        {
            auto specification = make_shared<QuerySpecification>(
                QueryNames::GetServiceName,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Partition::PartitionId));

            specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetServiceName), L"/FM");
            resultSPtr.push_back(move(specification));

            specification = make_shared<QuerySpecification>(
                QueryNames::GetServiceName,
                Query::QueryAddresses::GetFMM(),
                QueryArgument(Query::QueryResourceProperties::Partition::PartitionId));

            specification->querySpecificationId_ = MAKE_QUERY_SPEC_ID(QueryNames::ToString(QueryNames::GetServiceName), L"/FMM");
            resultSPtr.push_back(move(specification));

            break;
        }

        case QueryNames::GetApplicationName:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetApplicationName,
                Query::QueryAddresses::GetGateway(),
                QueryArgument(Query::QueryResourceProperties::Service::ServiceName)));
            break;
        }

        case QueryNames::GetComposeDeploymentStatusList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetComposeDeploymentStatusList,
                Query::QueryAddresses::GetCM(),
                QueryArgument(Query::QueryResourceProperties::Deployment::DeploymentName, false),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetComposeDeploymentUpgradeProgress:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetComposeDeploymentUpgradeProgress,
                Query::QueryAddresses::GetCM(),
                QueryArgument(Query::QueryResourceProperties::Deployment::DeploymentName)));
            break;
        }

        case QueryNames::GetDeployedCodePackageListByApplication:
        {
            // No-op: GetDeployedCodePackageParallelQuerySpecification's QuerySpecification list is dynamic, need not save in the store as a singleton copy.
            break;
        }

        case QueryNames::GetReplicaListByServiceNames:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetReplicaListByServiceNames,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Service::ServiceNames, true)));
            break;
        }

        case QueryNames::GetApplicationResourceList:
        {
            resultSPtr.push_back(make_shared<GetApplicationResourceListQuerySpecification>());
            break;
        }

        case QueryNames::GetServiceResourceList:
        {
            resultSPtr.push_back(make_shared<GetServiceResourceListQuerySpecification>());
            break;
        }
        case QueryNames::GetContainerCodePackageLogs:
        {
            resultSPtr.push_back(make_shared<GetContainerCodePackageLogsQuerySpecification>());
            break;
        }
        case QueryNames::GetReplicaResourceList:
        {
            resultSPtr.push_back(make_shared<GetReplicaResourceListQuerySpecification>());
            break;
        }

        case QueryNames::GetApplicationUnhealthyEvaluation:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                Query::QueryAddresses::GetHMViaCM(),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetVolumeResourceList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                QueryAddresses::GetCM(),
                QueryArgument(Query::QueryResourceProperties::VolumeResource::VolumeName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }
        case QueryNames::GetGatewayResourceList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                queryName,
                QueryAddresses::GetGRM(),
                QueryArgument(Query::QueryResourceProperties::GatewayResource::GatewayName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetNetworkList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetNetworkList,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Network::NetworkName, false),
                QueryArgument(Query::QueryResourceProperties::Network::NetworkStatusFilter, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetNetworkApplicationList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetNetworkApplicationList,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Network::NetworkName, true),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetNetworkNodeList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetNetworkNodeList,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Network::NetworkName, true),
                QueryArgument(Query::QueryResourceProperties::Node::Name, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetApplicationNetworkList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetApplicationNetworkList,
                Query::QueryAddresses::GetFM(),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, true),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetDeployedNetworkList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetDeployedNetworkList,
                Query::QueryAddresses::GetHosting(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

        case QueryNames::GetDeployedNetworkCodePackageList:
        {
            resultSPtr.push_back(make_shared<QuerySpecification>(
                QueryNames::GetDeployedNetworkCodePackageList,
                Query::QueryAddresses::GetHosting(),
                QueryArgument(Query::QueryResourceProperties::Node::Name, true),
                QueryArgument(Query::QueryResourceProperties::Network::NetworkName, true),
                QueryArgument(Query::QueryResourceProperties::Application::ApplicationName, false),
                QueryArgument(Query::QueryResourceProperties::ServiceType::ServiceManifestName, false),
                QueryArgument(Query::QueryResourceProperties::CodePackage::CodePackageName, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, false),
                QueryArgument(Query::QueryResourceProperties::QueryMetadata::MaxResults, false)));
            break;
        }

    default:
        Assert::CodingError("Query specification unknown for query : {0}", queryName);
        }
    }

    return move(resultSPtr);
}

//
// Return an unique identifier for the query specification.
//
wstring QuerySpecification::GetSpecificationId(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs)
{
    //
    // For parallel queries, the query specification address might change based on the argument,
    // Each of those specifications should have a unique id.
    //
    switch (queryName)
    {
    case QueryNames::GetApplicationServiceList:
        return GetApplicationServiceListQuerySpecification::GetSpecificationId(queryArgs);

    case QueryNames::GetServicePartitionList:
        return GetServicePartitionListQuerySpecification::GetSpecificationId(queryArgs);

    case QueryNames::GetServicePartitionReplicaList:
        return GetServicePartitionReplicaListQuerySpecification::GetSpecificationId(queryArgs);

    case QueryNames::MovePrimary:
        return SwapReplicaQuerySpecificationHelper::GetSpecificationId(queryArgs, QueryNames::MovePrimary);

    case QueryNames::MoveSecondary:
        return SwapReplicaQuerySpecificationHelper::GetSpecificationId(queryArgs, QueryNames::MoveSecondary);

    case QueryNames::GetUnplacedReplicaInformation:
        return GetUnplacedReplicaInformationQuerySpecificationHelper::GetSpecificationId(queryArgs, QueryNames::GetUnplacedReplicaInformation);

    case QueryNames::GetNodeList:
        return GetNodeListQuerySpecification::GetSpecificationId(queryArgs);

    case QueryNames::GetServiceName:
        return GetServiceNameQuerySpecificationHelper::GetSpecificationId(queryArgs, QueryNames::GetServiceName);

    case QueryNames::GetApplicationPagedListDeployedOnNode:
        return GetApplicationPagedListDeployedOnNodeQuerySpecification::GetSpecificationId(queryArgs);

    default:
        return QueryNames::ToString(queryName);
    }
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);

    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);
    arguments_.push_back(argument1);

    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);
    arguments_.push_back(argument1);
    arguments_.push_back(argument2);

    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);
    arguments_.push_back(argument1);
    arguments_.push_back(argument2);
    arguments_.push_back(argument3);

    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3,
    QueryArgument const & argument4,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);
    arguments_.push_back(argument1);
    arguments_.push_back(argument2);
    arguments_.push_back(argument3);
    arguments_.push_back(argument4);

    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3,
    QueryArgument const & argument4,
    QueryArgument const & argument5,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);
    arguments_.push_back(argument1);
    arguments_.push_back(argument2);
    arguments_.push_back(argument3);
    arguments_.push_back(argument4);
    arguments_.push_back(argument5);

    InitializeArgumentSets();
}

QuerySpecification::QuerySpecification(
    QueryNames::Enum queryName,
    QueryAddressGenerator const & queryAddress,
    QueryArgument const & argument0,
    QueryArgument const & argument1,
    QueryArgument const & argument2,
    QueryArgument const & argument3,
    QueryArgument const & argument4,
    QueryArgument const & argument5,
    QueryArgument const & argument6,
    Query::QueryType::Enum queryType)
    : queryName_(queryName)
    , querySpecificationId_(QueryNames::ToString(queryName))
    , queryAddressGenerator_(queryAddress)
    , arguments_()
    , queryType_(queryType)
    , argumentSets_()
{
    arguments_.push_back(argument0);
    arguments_.push_back(argument1);
    arguments_.push_back(argument2);
    arguments_.push_back(argument3);
    arguments_.push_back(argument4);
    arguments_.push_back(argument5);
    arguments_.push_back(argument6);

    InitializeArgumentSets();
}

void QuerySpecification::InitializeArgumentSets()
{
    set<wstring> remainingSets;

    for (auto itArgument = arguments_.begin(); itArgument != arguments_.end(); ++itArgument)
    {
        for (wstring const & name : itArgument->ArgumentSets)
        {
            if (find(argumentSets_.begin(), argumentSets_.end(), name) == argumentSets_.end())
            {
                argumentSets_.push_back(name);
                remainingSets.insert(name);
            }

            if (itArgument->ArgumentSets.size() == 1)
            {
                remainingSets.erase(name);
            }
        }
    }

    ASSERT_IF(remainingSets.size() > 0, "Invalid argument set for query {0}", queryName_);
}

Common::ErrorCode QuerySpecification::GenerateAddress(
    Common::ActivityId const & activityId,
    ServiceModel::QueryArgumentMap const & queryArgs,
    _Out_ wstring & address)
{
    return queryAddressGenerator_.GenerateQueryAddress(activityId, queryArgs, address);
}

Common::ErrorCode QuerySpecification::HasRequiredArguments(QueryArgumentMap const & queryArgs)
{
    if (arguments_.size() == 0 && queryArgs.Size == 0)
    {
        return ErrorCode::Success();
    }

    wstring setName;
    if (argumentSets_.size() > 0)
    {
        for (auto itArgument = arguments_.begin(); itArgument != arguments_.end(); ++itArgument)
        {
            if (itArgument->ArgumentSets.size() == 1 && queryArgs.ContainsKey(itArgument->Name))
            {
                setName = itArgument->ArgumentSets[0];
                break;
            }
        }

        if (setName.size() == 0)
        {
            QueryEventSource::EventSource->QueryUnknownArgumentSet(
                QueryNames::ToString(queryName_));
        }
    }

    // Creating a copy as we need to remove the found entries. The entries left will be extra arguments.
    // We should not allow extra arguments to pass through.
    QueryArgumentMap queryArgsCopy = queryArgs;

    for (auto itArgument = arguments_.begin(); itArgument != arguments_.end(); ++itArgument)
    {
        if (setName.size() == 0 ||
            itArgument->ArgumentSets.size() == 0 ||
            find(itArgument->ArgumentSets.begin(), itArgument->ArgumentSets.end(), setName) != itArgument->ArgumentSets.end())
        {
            if (queryArgsCopy.ContainsKey(itArgument->Name))
            {
                queryArgsCopy.Erase(itArgument->Name);
            }
            else if (itArgument->IsRequired)
            {
                wstring queryName = QueryNames::ToString(queryName_);
                QueryEventSource::EventSource->QueryArgsMissing(
                    itArgument->Name,
                    queryName);
                return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_QUERY_RC(Query_Missing_Required_Argument), queryName, itArgument->Name));
            }
        }
    }

    // Extra argument.
    if (queryArgsCopy.Size > 0)
    {
        wstring queryName = QueryNames::ToString(queryName_);
        QueryEventSource::EventSource->QueryUnknownArgument(
            queryArgsCopy.FirstKey(),
            queryName);
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_QUERY_RC(Query_Unknown_Argument), queryName, queryArgsCopy.FirstKey()));
    }

    return ErrorCode::Success();
}