// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;

GlobalWString QueryResourceProperties::Node::Id = make_global<wstring>(L"Id");
GlobalWString QueryResourceProperties::Node::InstanceId = make_global<wstring>(L"InstanceId");
GlobalWString QueryResourceProperties::Node::Name = make_global<wstring>(L"NodeName");
GlobalWString QueryResourceProperties::Node::IsUp = make_global<wstring>(L"IsUp");
GlobalWString QueryResourceProperties::Node::Properties = make_global<wstring>(L"Properties");
GlobalWString QueryResourceProperties::Node::Domains = make_global<wstring>(L"Domains");
GlobalWString QueryResourceProperties::Node::ImageCacheFolder = make_global<wstring>(L"ImageCacheFolder");
GlobalWString QueryResourceProperties::Node::DeploymentFolder = make_global<wstring>(L"DeploymentFolder");
GlobalWString QueryResourceProperties::Node::Restart = make_global<wstring>(L"Restart");
GlobalWString QueryResourceProperties::Node::CreateFabricDump = make_global<wstring>(L"CreateFabricDump");
GlobalWString QueryResourceProperties::Node::NodeStatusFilter = make_global<wstring>(L"NodeStatusFilter");
GlobalWString QueryResourceProperties::Node::ExcludeStoppedNodeInfo = make_global<wstring>(L"ExcludeStoppedNodeInfo");
GlobalWString QueryResourceProperties::Node::StopDurationInSeconds = make_global<wstring>(L"StopDurationInSeconds");


GlobalWString QueryResourceProperties::Node::CurrentNodeName = make_global<wstring>(L"CurrentNodeName");
GlobalWString QueryResourceProperties::Node::NewNodeName = make_global<wstring>(L"NewNodeName");

GlobalWString QueryResourceProperties::ApplicationType::ApplicationTypeName = make_global<wstring>(L"ApplicationTypeName");
GlobalWString QueryResourceProperties::ApplicationType::ApplicationBuildPath = make_global<wstring>(L"ApplicationBuildPath");
GlobalWString QueryResourceProperties::ApplicationType::ApplicationTypeVersion = make_global<wstring>(L"ApplicationTypeVersion");

GlobalWString QueryResourceProperties::Application::ApplicationName = make_global<wstring>(L"ApplicationName");
GlobalWString QueryResourceProperties::Application::ApplicationId = make_global<wstring>(L"ApplicationId");
GlobalWString QueryResourceProperties::Application::ApplicationIds = make_global<wstring>(L"ApplicationIds");
GlobalWString QueryResourceProperties::Application::ApplicationVersion = make_global<wstring>(L"ApplicationVersion");
GlobalWString QueryResourceProperties::Application::ApplicationIsUpgrading = make_global<wstring>(L"ApplicationIsUpgrading");

GlobalWString QueryResourceProperties::Deployment::DeploymentName = make_global<wstring>(L"DeploymentName");
GlobalWString QueryResourceProperties::Deployment::InstanceName = make_global<wstring>(L"InstanceName");
GlobalWString QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter = make_global<wstring>(L"ApplicationDefinitionKindFilter");
GlobalWString QueryResourceProperties::Deployment::ApplicationTypeDefinitionKindFilter = make_global<wstring>(L"ApplicationTypeDefinitionKindFilter");
GlobalWString QueryResourceProperties::Deployment::DeploymentTypeFilter = make_global<wstring>(L"DeploymentTypeFilter");
GlobalWString QueryResourceProperties::Deployment::InstanceIdFilter = make_global<wstring>(L"InstanceIdFilter");

GlobalWString QueryResourceProperties::Service::ServiceName = make_global<wstring>(L"ServiceName");
GlobalWString QueryResourceProperties::Service::ServiceTypeName = make_global<wstring>(L"ServiceTypeName");
GlobalWString QueryResourceProperties::Service::ServiceGroupTypeName = make_global<wstring>(L"ServiceGroupTypeName");
GlobalWString QueryResourceProperties::Service::IsStateful = make_global<wstring>(L"IsStateful");
GlobalWString QueryResourceProperties::Service::PlacementConstraints = make_global<wstring>(L"PlacementConstraints");
GlobalWString QueryResourceProperties::Service::Correlations = make_global<wstring>(L"Correlations");
GlobalWString QueryResourceProperties::Service::Metrics = make_global<wstring>(L"Metrics");
GlobalWString QueryResourceProperties::Service::ServiceTypeId = make_global<wstring>(L"ServiceTypeId");
GlobalWString QueryResourceProperties::Service::ServiceNames = make_global<wstring>(L"ServiceNames");

GlobalWString QueryResourceProperties::ServiceType::ServiceTypeName = make_global<wstring>(L"ServiceTypeName");
GlobalWString QueryResourceProperties::ServiceType::ServiceGroupTypeName = make_global<wstring>(L"ServiceGroupTypeName");
GlobalWString QueryResourceProperties::ServiceType::ServiceManifestName = make_global<wstring>(L"ServiceManifestName");
GlobalWString QueryResourceProperties::ServiceType::SharedPackages = make_global<wstring>(L"SharedPackages");
GlobalWString QueryResourceProperties::ServiceType::ServicePackageSharingScope = make_global<wstring>(L"ServicePackageSharingScope");

GlobalWString QueryResourceProperties::ServiceManifest::ServiceManifestName = make_global<wstring>(L"ServiceManifestName");
GlobalWString QueryResourceProperties::ServiceManifest::ServicePackageActivationId = make_global<wstring>(L"ServicePackageActivationId");

GlobalWString QueryResourceProperties::ServicePackage::ServiceManifestName = make_global<wstring>(L"ServiceManifestName");
GlobalWString QueryResourceProperties::ServicePackage::ServicePackageActivationId = make_global<wstring>(L"ServicePackageActivationId");

GlobalWString QueryResourceProperties::QueryMetadata::Name = make_global<wstring>(L"Name");

GlobalWString QueryResourceProperties::QueryMetadata::RequiredArguments = make_global<wstring>(L"RequiredArguments");
GlobalWString QueryResourceProperties::QueryMetadata::OptionalArguments = make_global<wstring>(L"OptionalArguments");
GlobalWString QueryResourceProperties::QueryMetadata::ContinuationToken = make_global<wstring>(L"ContinuationToken");
GlobalWString QueryResourceProperties::QueryMetadata::MaxResults = make_global<wstring>(L"MaxResults");
GlobalWString QueryResourceProperties::QueryMetadata::IncludeHealthState = make_global<wstring>(L"IncludeHealthState");

GlobalWString QueryResourceProperties::QueryMetadata::OnlyQueryPrimaries = make_global<wstring>(L"OnlyQueryPrimaries");

GlobalWString QueryResourceProperties::QueryMetadata::ForceMove = make_global<wstring>(L"ForceMove");

GlobalWString QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters = make_global<wstring>(L"ExcludeApplicationParameters");

GlobalWString QueryResourceProperties::Partition::PartitionId = make_global<wstring>(L"PartitionId");

GlobalWString QueryResourceProperties::Replica::ReplicaId =  make_global<wstring>(L"ReplicaId");
GlobalWString QueryResourceProperties::Replica::InstanceId =  make_global<wstring>(L"InstanceId");
GlobalWString QueryResourceProperties::Replica::ReplicaStatusFilter = make_global<wstring>(L"ReplicaStatusFilter");

GlobalWString QueryResourceProperties::Health::ClusterHealthPolicy =  make_global<wstring>(L"ClusterHealthPolicy");
GlobalWString QueryResourceProperties::Health::ApplicationHealthPolicy =  make_global<wstring>(L"ApplicationHealthPolicy");
GlobalWString QueryResourceProperties::Health::ApplicationHealthPolicyMap = make_global<wstring>(L"ApplicationHealthPolicyMap");
GlobalWString QueryResourceProperties::Health::UpgradeDomainName =  make_global<wstring>(L"UpgradeDomainName");
GlobalWString QueryResourceProperties::Health::EventsFilter =  make_global<wstring>(L"EventsFilter");
GlobalWString QueryResourceProperties::Health::NodeFilters = make_global<wstring>(L"NodeFilters");
GlobalWString QueryResourceProperties::Health::ApplicationFilters = make_global<wstring>(L"ApplicationFilters");
GlobalWString QueryResourceProperties::Health::NodesFilter =  make_global<wstring>(L"NodesFilter");
GlobalWString QueryResourceProperties::Health::ApplicationsFilter =  make_global<wstring>(L"ApplicationsFilter");
GlobalWString QueryResourceProperties::Health::ServicesFilter =  make_global<wstring>(L"ServicesFilter");
GlobalWString QueryResourceProperties::Health::PartitionsFilter =  make_global<wstring>(L"PartitionsFilter");
GlobalWString QueryResourceProperties::Health::ReplicasFilter =  make_global<wstring>(L"ReplicasFilter");
GlobalWString QueryResourceProperties::Health::DeployedApplicationsFilter =  make_global<wstring>(L"DeployedApplicationsFilter");
GlobalWString QueryResourceProperties::Health::DeployedServicePackagesFilter =  make_global<wstring>(L"DeployedServicePackagesFilter");
GlobalWString QueryResourceProperties::Health::HealthStatsFilter = make_global<wstring>(L"HealthStatsFilter");

GlobalWString QueryResourceProperties::CodePackage::CodePackageName = make_global<wstring>(L"CodePackageName");
GlobalWString QueryResourceProperties::CodePackage::InstanceId = make_global<wstring>(L"InstanceId");

GlobalWString QueryResourceProperties::Repair::Scope = make_global<wstring>(L"Scope");
GlobalWString QueryResourceProperties::Repair::TaskIdPrefix = make_global<wstring>(L"TaskIdPrefix");
GlobalWString QueryResourceProperties::Repair::StateMask = make_global<wstring>(L"StateMask");
GlobalWString QueryResourceProperties::Repair::ExecutorName = make_global<wstring>(L"ExecutorName");

GlobalWString QueryResourceProperties::Cluster::CodeVersionFilter = make_global<wstring>(L"CodeVersion");
GlobalWString QueryResourceProperties::Cluster::ConfigVersionFilter = make_global<wstring>(L"ConfigVersion");

GlobalWString QueryResourceProperties::UnreliableTransportBehavior::Name = make_global<wstring>(L"BehaviorName");
GlobalWString QueryResourceProperties::UnreliableTransportBehavior::Behavior = make_global<wstring>(L"BehaviorValue");

GlobalWString QueryResourceProperties::UnreliableLeaseBehavior::Alias = make_global<wstring>(L"BehaviorAlias");
GlobalWString QueryResourceProperties::UnreliableLeaseBehavior::Behavior = make_global<wstring>(L"BehaviorValue");

GlobalWString QueryResourceProperties::Action::StateFilter = make_global<wstring>(L"StateFilter");
GlobalWString QueryResourceProperties::Action::TypeFilter = make_global<wstring>(L"TypeFilter");

GlobalWString QueryResourceProperties::ContainerInfo::InfoTypeFilter = make_global<std::wstring>(L"InfoTypeFilter");
GlobalWString QueryResourceProperties::ContainerInfo::InfoArgsFilter = make_global<std::wstring>(L"InfoArgsFilter");
GlobalWString QueryResourceProperties::ContainerInfo::ContainerName = make_global<std::wstring>(L"ContainerName");

GlobalWString QueryResourceProperties::VolumeResource::VolumeName = make_global<wstring>(L"VolumeName");

GlobalWString QueryResourceProperties::Network::NetworkName = make_global<std::wstring>(L"NetworkName");
GlobalWString QueryResourceProperties::Network::NetworkStatusFilter = make_global<std::wstring>(L"NetworkStatusFilter");
GlobalWString QueryResourceProperties::Network::NetworkType = make_global<std::wstring>(L"NetworkType");

GlobalWString QueryResourceProperties::GatewayResource::GatewayName = make_global<wstring>(L"GatewayName");
