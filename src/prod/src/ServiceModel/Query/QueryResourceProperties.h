// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    namespace QueryResourceProperties
    {
        class Node
        {
        public:
            static Common::GlobalWString Id;
            static Common::GlobalWString InstanceId;
            static Common::GlobalWString Name;
            static Common::GlobalWString IsUp;
            static Common::GlobalWString Properties;
            static Common::GlobalWString Domains;
            static Common::GlobalWString ImageCacheFolder;
            static Common::GlobalWString DeploymentFolder;
            static Common::GlobalWString Restart;
            static Common::GlobalWString CurrentNodeName;
            static Common::GlobalWString NewNodeName;
            static Common::GlobalWString CreateFabricDump;
            static Common::GlobalWString NodeStatusFilter;
            static Common::GlobalWString ExcludeStoppedNodeInfo;
            static Common::GlobalWString StopDurationInSeconds;
        };

        class ApplicationType
        {
        public:
            static Common::GlobalWString ApplicationTypeName;
            static Common::GlobalWString ApplicationBuildPath;
            static Common::GlobalWString ApplicationTypeVersion;
        };

        class Application
        {
        public:
            static Common::GlobalWString ApplicationName;
            static Common::GlobalWString ApplicationId;
            static Common::GlobalWString ApplicationVersion;
            static Common::GlobalWString ApplicationIsUpgrading;
            static Common::GlobalWString ApplicationIds;
        };

        class Deployment
        {
        public:
            static Common::GlobalWString DeploymentName;
            static Common::GlobalWString InstanceName;
            static Common::GlobalWString ApplicationDefinitionKindFilter;
            static Common::GlobalWString ApplicationTypeDefinitionKindFilter;
            static Common::GlobalWString DeploymentTypeFilter;
            static Common::GlobalWString InstanceIdFilter;
        };

        class Service
        {
        public:
            static Common::GlobalWString ServiceName;
            static Common::GlobalWString ServiceTypeName;
            static Common::GlobalWString ServiceGroupTypeName;
            static Common::GlobalWString IsStateful;
            static Common::GlobalWString PlacementConstraints;
            static Common::GlobalWString Correlations;
            static Common::GlobalWString Metrics;
            static Common::GlobalWString ServiceTypeId;
            static Common::GlobalWString ServiceNames;
        };

        class ServiceType
        {
        public:
            static Common::GlobalWString ServiceTypeName;
            static Common::GlobalWString ServiceGroupTypeName;
            static Common::GlobalWString ServiceManifestName;
            static Common::GlobalWString SharedPackages;
            static Common::GlobalWString ServicePackageSharingScope;
        };

        class ServiceManifest
        {
        public:
            static Common::GlobalWString ServiceManifestName;
            static Common::GlobalWString ServicePackageActivationId;
        };

        class ServicePackage
        {
        public:
            static Common::GlobalWString ServiceManifestName;
            static Common::GlobalWString ServicePackageActivationId;
        };

        class QueryMetadata
        {
        public:
            static Common::GlobalWString Name;
            static Common::GlobalWString RequiredArguments;
            static Common::GlobalWString OptionalArguments;
            static Common::GlobalWString OnlyQueryPrimaries;
            static Common::GlobalWString ForceMove;
            static Common::GlobalWString ContinuationToken;
            static Common::GlobalWString ExcludeApplicationParameters;
            static Common::GlobalWString MaxResults;
            static Common::GlobalWString IncludeHealthState;
        };

        class Partition
        {
        public:
            static Common::GlobalWString PartitionId;
        };

        class Replica
        {
        public:
            static Common::GlobalWString ReplicaId;
            static Common::GlobalWString InstanceId;
            static Common::GlobalWString ReplicaStatusFilter;
        };

        class Health
        {
        public:
            static Common::GlobalWString ClusterHealthPolicy;
            static Common::GlobalWString ApplicationHealthPolicy;
            static Common::GlobalWString ApplicationHealthPolicyMap;
            static Common::GlobalWString UpgradeDomainName;
            static Common::GlobalWString EventsFilter;
            static Common::GlobalWString NodeFilters;
            static Common::GlobalWString ApplicationFilters;
            static Common::GlobalWString NodesFilter;
            static Common::GlobalWString ApplicationsFilter;
            static Common::GlobalWString ServicesFilter;
            static Common::GlobalWString PartitionsFilter;
            static Common::GlobalWString ReplicasFilter;
            static Common::GlobalWString DeployedApplicationsFilter;
            static Common::GlobalWString DeployedServicePackagesFilter;
            static Common::GlobalWString HealthStatsFilter;
        };

        class CodePackage
        {
        public:
            static Common::GlobalWString CodePackageName;
            static Common::GlobalWString InstanceId;
        };

        class Repair
        {
        public:
            static Common::GlobalWString Scope;
            static Common::GlobalWString TaskIdPrefix;
            static Common::GlobalWString StateMask;
            static Common::GlobalWString ExecutorName;
        };

        class Cluster
        {
        public:
            static Common::GlobalWString CodeVersionFilter;
            static Common::GlobalWString ConfigVersionFilter;
        };

        class UnreliableTransportBehavior
        {
        public:
            static Common::GlobalWString  Name;
            static Common::GlobalWString  Behavior;
        };

        class UnreliableLeaseBehavior
        {
        public:
            static Common::GlobalWString  Alias;
            static Common::GlobalWString  Behavior;
        };

        class Action
        {
        public:
            static Common::GlobalWString StateFilter;
            static Common::GlobalWString TypeFilter;
        };

        class ContainerInfo
        {
        public:
            static Common::GlobalWString InfoTypeFilter;
            static Common::GlobalWString InfoArgsFilter;
            static Common::GlobalWString ContainerName;
        };

	class VolumeResource
        {
        public:
            static Common::GlobalWString VolumeName;
        };

        class Network
        {
        public:
            static Common::GlobalWString NetworkName;
            static Common::GlobalWString NetworkType;
            static Common::GlobalWString NetworkStatusFilter;
        };

        class GatewayResource
        {
        public:
            static Common::GlobalWString GatewayName;
        };
    }
}
