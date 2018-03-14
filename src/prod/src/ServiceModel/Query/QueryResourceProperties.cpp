// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;

GlobalWString QueryResourceProperties::Node::Id = make_global<std::wstring>(L"Id");
GlobalWString QueryResourceProperties::Node::InstanceId = make_global<std::wstring>(L"InstanceId");
GlobalWString QueryResourceProperties::Node::Name = make_global<std::wstring>(L"NodeName");
GlobalWString QueryResourceProperties::Node::IsUp = make_global<std::wstring>(L"IsUp");
GlobalWString QueryResourceProperties::Node::Properties = make_global<std::wstring>(L"Properties");
GlobalWString QueryResourceProperties::Node::Domains = make_global<std::wstring>(L"Domains");
GlobalWString QueryResourceProperties::Node::ImageCacheFolder = make_global<std::wstring>(L"ImageCacheFolder");
GlobalWString QueryResourceProperties::Node::DeploymentFolder = make_global<std::wstring>(L"DeploymentFolder");
GlobalWString QueryResourceProperties::Node::Restart = make_global<std::wstring>(L"Restart");
GlobalWString QueryResourceProperties::Node::CreateFabricDump = make_global<std::wstring>(L"CreateFabricDump");
GlobalWString QueryResourceProperties::Node::NodeStatusFilter = make_global<std::wstring>(L"NodeStatusFilter");
GlobalWString QueryResourceProperties::Node::ExcludeStoppedNodeInfo = make_global<std::wstring>(L"ExcludeStoppedNodeInfo");
GlobalWString QueryResourceProperties::Node::StopDurationInSeconds = make_global<std::wstring>(L"StopDurationInSeconds");


GlobalWString QueryResourceProperties::Node::CurrentNodeName = make_global<std::wstring>(L"CurrentNodeName");
GlobalWString QueryResourceProperties::Node::NewNodeName = make_global<std::wstring>(L"NewNodeName");

GlobalWString QueryResourceProperties::ApplicationType::ApplicationTypeName = make_global<std::wstring>(L"ApplicationTypeName");
GlobalWString QueryResourceProperties::ApplicationType::ApplicationBuildPath = make_global<std::wstring>(L"ApplicationBuildPath");
GlobalWString QueryResourceProperties::ApplicationType::ApplicationTypeVersion = make_global<std::wstring>(L"ApplicationTypeVersion");

GlobalWString QueryResourceProperties::Application::ApplicationName = make_global<std::wstring>(L"ApplicationName");
GlobalWString QueryResourceProperties::Application::ApplicationId = make_global<std::wstring>(L"ApplicationId");
GlobalWString QueryResourceProperties::Application::ApplicationIds = make_global<std::wstring>(L"ApplicationIds");
GlobalWString QueryResourceProperties::Application::ApplicationVersion = make_global<std::wstring>(L"ApplicationVersion");
GlobalWString QueryResourceProperties::Application::ApplicationIsUpgrading = make_global<std::wstring>(L"ApplicationIsUpgrading");

GlobalWString QueryResourceProperties::Deployment::DeploymentName = make_global<std::wstring>(L"DeploymentName");
GlobalWString QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter = make_global<std::wstring>(L"ApplicationDefinitionKindFilter");
GlobalWString QueryResourceProperties::Deployment::ApplicationTypeDefinitionKindFilter = make_global<std::wstring>(L"ApplicationTypeDefinitionKindFilter");

GlobalWString QueryResourceProperties::Service::ServiceName = make_global<std::wstring>(L"ServiceName");
GlobalWString QueryResourceProperties::Service::ServiceTypeName = make_global<std::wstring>(L"ServiceTypeName");
GlobalWString QueryResourceProperties::Service::ServiceGroupTypeName = make_global<std::wstring>(L"ServiceGroupTypeName");
GlobalWString QueryResourceProperties::Service::IsStateful = make_global<std::wstring>(L"IsStateful");
GlobalWString QueryResourceProperties::Service::PlacementConstraints = make_global<std::wstring>(L"PlacementConstraints");
GlobalWString QueryResourceProperties::Service::Correlations = make_global<std::wstring>(L"Correlations");
GlobalWString QueryResourceProperties::Service::Metrics = make_global<std::wstring>(L"Metrics");
GlobalWString QueryResourceProperties::Service::ServiceTypeId = make_global<std::wstring>(L"ServiceTypeId");

GlobalWString QueryResourceProperties::ServiceType::ServiceTypeName = make_global<std::wstring>(L"ServiceTypeName");
GlobalWString QueryResourceProperties::ServiceType::ServiceGroupTypeName = make_global<std::wstring>(L"ServiceGroupTypeName");
GlobalWString QueryResourceProperties::ServiceType::ServiceManifestName = make_global<std::wstring>(L"ServiceManifestName");
GlobalWString QueryResourceProperties::ServiceType::SharedPackages = make_global<std::wstring>(L"SharedPackages");
GlobalWString QueryResourceProperties::ServiceType::ServicePackageSharingScope = make_global<std::wstring>(L"ServicePackageSharingScope");

GlobalWString QueryResourceProperties::ServiceManifest::ServiceManifestName = make_global<std::wstring>(L"ServiceManifestName");
GlobalWString QueryResourceProperties::ServiceManifest::ServicePackageActivationId = make_global<std::wstring>(L"ServicePackageActivationId");

GlobalWString QueryResourceProperties::ServicePackage::ServiceManifestName = make_global<std::wstring>(L"ServiceManifestName");
GlobalWString QueryResourceProperties::ServicePackage::ServicePackageActivationId = make_global<std::wstring>(L"ServicePackageActivationId");

GlobalWString QueryResourceProperties::QueryMetadata::Name = make_global<std::wstring>(L"Name");

GlobalWString QueryResourceProperties::QueryMetadata::RequiredArguments = make_global<std::wstring>(L"RequiredArguments");
GlobalWString QueryResourceProperties::QueryMetadata::OptionalArguments = make_global<std::wstring>(L"OptionalArguments");
GlobalWString QueryResourceProperties::QueryMetadata::ContinuationToken = make_global<std::wstring>(L"ContinuationToken");
GlobalWString QueryResourceProperties::QueryMetadata::MaxResults = make_global<std::wstring>(L"MaxResults");
GlobalWString QueryResourceProperties::QueryMetadata::IncludeHealthState = make_global<std::wstring>(L"IncludeHealthState");

GlobalWString QueryResourceProperties::QueryMetadata::OnlyQueryPrimaries = make_global<std::wstring>(L"OnlyQueryPrimaries");

GlobalWString QueryResourceProperties::QueryMetadata::ForceMove = make_global<std::wstring>(L"ForceMove");

GlobalWString QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters = make_global<std::wstring>(L"ExcludeApplicationParameters");

GlobalWString QueryResourceProperties::Partition::PartitionId = make_global<std::wstring>(L"PartitionId");

GlobalWString QueryResourceProperties::Replica::ReplicaId =  make_global<std::wstring>(L"ReplicaId");
GlobalWString QueryResourceProperties::Replica::InstanceId =  make_global<std::wstring>(L"InstanceId");
GlobalWString QueryResourceProperties::Replica::ReplicaStatusFilter = make_global<std::wstring>(L"ReplicaStatusFilter");

GlobalWString QueryResourceProperties::Health::ClusterHealthPolicy =  make_global<std::wstring>(L"ClusterHealthPolicy");
GlobalWString QueryResourceProperties::Health::ApplicationHealthPolicy =  make_global<std::wstring>(L"ApplicationHealthPolicy");
GlobalWString QueryResourceProperties::Health::ApplicationHealthPolicyMap = make_global<std::wstring>(L"ApplicationHealthPolicyMap");
GlobalWString QueryResourceProperties::Health::UpgradeDomainName =  make_global<std::wstring>(L"UpgradeDomainName");
GlobalWString QueryResourceProperties::Health::EventsFilter =  make_global<std::wstring>(L"EventsFilter");
GlobalWString QueryResourceProperties::Health::NodeFilters = make_global<std::wstring>(L"NodeFilters");
GlobalWString QueryResourceProperties::Health::ApplicationFilters = make_global<std::wstring>(L"ApplicationFilters");
GlobalWString QueryResourceProperties::Health::NodesFilter =  make_global<std::wstring>(L"NodesFilter");
GlobalWString QueryResourceProperties::Health::ApplicationsFilter =  make_global<std::wstring>(L"ApplicationsFilter");
GlobalWString QueryResourceProperties::Health::ServicesFilter =  make_global<std::wstring>(L"ServicesFilter");
GlobalWString QueryResourceProperties::Health::PartitionsFilter =  make_global<std::wstring>(L"PartitionsFilter");
GlobalWString QueryResourceProperties::Health::ReplicasFilter =  make_global<std::wstring>(L"ReplicasFilter");
GlobalWString QueryResourceProperties::Health::DeployedApplicationsFilter =  make_global<std::wstring>(L"DeployedApplicationsFilter");
GlobalWString QueryResourceProperties::Health::DeployedServicePackagesFilter =  make_global<std::wstring>(L"DeployedServicePackagesFilter");
GlobalWString QueryResourceProperties::Health::HealthStatsFilter = make_global<std::wstring>(L"HealthStatsFilter");

GlobalWString QueryResourceProperties::CodePackage::CodePackageName = make_global<std::wstring>(L"CodePackageName");
GlobalWString QueryResourceProperties::CodePackage::InstanceId = make_global<std::wstring>(L"InstanceId");

GlobalWString QueryResourceProperties::Repair::Scope = make_global<std::wstring>(L"Scope");
GlobalWString QueryResourceProperties::Repair::TaskIdPrefix = make_global<std::wstring>(L"TaskIdPrefix");
GlobalWString QueryResourceProperties::Repair::StateMask = make_global<std::wstring>(L"StateMask");
GlobalWString QueryResourceProperties::Repair::ExecutorName = make_global<std::wstring>(L"ExecutorName");

GlobalWString QueryResourceProperties::Cluster::CodeVersionFilter = make_global<std::wstring>(L"CodeVersion");
GlobalWString QueryResourceProperties::Cluster::ConfigVersionFilter = make_global<std::wstring>(L"ConfigVersion");

GlobalWString QueryResourceProperties::UnreliableTransportBehavior::Name = make_global<std::wstring>(L"BehaviorName");
GlobalWString QueryResourceProperties::UnreliableTransportBehavior::Behavior = make_global<std::wstring>(L"BehaviorValue");

GlobalWString QueryResourceProperties::UnreliableLeaseBehavior::Alias = make_global<std::wstring>(L"BehaviorAlias");
GlobalWString QueryResourceProperties::UnreliableLeaseBehavior::Behavior = make_global<std::wstring>(L"BehaviorValue");

GlobalWString QueryResourceProperties::Action::StateFilter = make_global<std::wstring>(L"StateFilter");
GlobalWString QueryResourceProperties::Action::TypeFilter = make_global<std::wstring>(L"TypeFilter");

GlobalWString QueryResourceProperties::ContainerInfo::InfoTypeFilter = make_global<std::wstring>(L"InfoTypeFilter");
GlobalWString QueryResourceProperties::ContainerInfo::InfoArgsFilter = make_global<std::wstring>(L"InfoArgsFilter");
