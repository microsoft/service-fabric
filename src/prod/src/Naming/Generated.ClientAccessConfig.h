
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    //
    // NOTE: *Config.h files are normally parsed for PUBLIC_CONFIG_ENTRY macros
    //       in order to generate a CSV file containing all known product configs.
    //       To avoid repetitive use of PUBLIC_CONFIG_ENTRY(...), this file
    //       defines some security config macros.
    //
    //       To avoid special-case handling of this *Config.h file in the
    //       CSV generation perl script, we produce a preprocessed *Config.h
    //       file during build for the CSV generator to parse (see src\prod\src\Naming\EntreeService\GenerateHeader\GenerateHeader.proj).
    //
    //       The CSV generation script is found at src\prod\tools\ConfigurationValidator\GenerateConfigurationsCSV.pl .
    //
    //       Please submit this generated file as well when changing these
    //       config settings.
    //

// A Win32 CreateService() function interferes with macro preprocessing below
//












    // This contains access control information for client operations.
    // Each access control entry is a key-value pair, where the key is the name
    // of an operation, and its value is list of role names: e.g. "User||Admin"
    // means both user and admin roles can execute that operation
    //
    class ClientAccessConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ClientAccessConfig, "Security/ClientAccess")

    public:

        //
        // Admin-only operations
        //

        // Security configuration for Naming URI creation
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateName, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateNameRoles;
        // Security configuration for Naming URI deletion
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteName, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteNameRoles;
        // Security configuration for Naming property write operations
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", PropertyWriteBatch, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum PropertyWriteBatchRoles;

        // Security configuration for service creation
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateService, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateServiceRoles;

        // Security configuration for service creatin from template
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateServiceFromTemplate, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateServiceFromTemplateRoles;
        // Security configuration for service updates
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UpdateService, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UpdateServiceRoles;
        // Security configuration for service deletion
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteService, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteServiceRoles;

        // Security configuration for application type provisioning
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ProvisionApplicationType, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ProvisionApplicationTypeRoles;
        // Security configuration for application creation
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateApplication, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateApplicationRoles;
        // Security configuration for application deletion
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteApplication, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteApplicationRoles;
        // Security configuration for starting or interrupting application upgrades
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UpgradeApplication, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UpgradeApplicationRoles;
        // Security configuration for rolling back application upgrades
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RollbackApplicationUpgrade, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RollbackApplicationUpgradeRoles;
        // Security configuration for application type unprovisioning
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UnprovisionApplicationType, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UnprovisionApplicationTypeRoles;
        // Security configuration for resuming application upgrades with an explicit Upgrade Domain
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", MoveNextUpgradeDomain, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum MoveNextUpgradeDomainRoles;
        // Security configuration for resuming application upgrades with the current upgrade progress
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ReportUpgradeHealth, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ReportUpgradeHealthRoles;

        // Security configuration for reporting health
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ReportHealth, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ReportHealthRoles;

        // Security configuration for MSI and/or Cluster Manifest provisioning
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ProvisionFabric, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ProvisionFabricRoles;
        // Security configuration for starting cluster upgrades
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UpgradeFabric, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UpgradeFabricRoles;
        // Security configuration for rolling back cluster upgrades
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RollbackFabricUpgrade, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RollbackFabricUpgradeRoles;
        // Security configuration for MSI and/or Cluster Manifest unprovisioning
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UnprovisionFabric, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UnprovisionFabricRoles;
        // Security configuration for resuming cluster upgrades with an explicity Upgrade Domain
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", MoveNextFabricUpgradeDomain, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum MoveNextFabricUpgradeDomainRoles;
        // Security configuration for resuming cluster upgrades with the current upgrade progress
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ReportFabricUpgradeHealth, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ReportFabricUpgradeHealthRoles;

        // Security configuration for starting infrastructure tasks
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartInfrastructureTask, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartInfrastructureTaskRoles;
        // Security configuration for finishing infrastructure tasks
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", FinishInfrastructureTask, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum FinishInfrastructureTaskRoles;

        // Security configuration for activation a node
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ActivateNode, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ActivateNodeRoles;
        // Security configuration for deactivating a node
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeactivateNode, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeactivateNodeRoles;

        // Security configuration for deactivating multiple nodes
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeactivateNodesBatch, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeactivateNodesBatchRoles;
        // Security configuration for reverting deactivation on multiple nodes
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RemoveNodeDeactivations, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RemoveNodeDeactivationsRoles;
        // Security configuration for checking deactivation status
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetNodeDeactivationStatus, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetNodeDeactivationStatusRoles;

        // Security configuration for reporting node state removed
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", NodeStateRemoved, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum NodeStateRemovedRoles;
        // Security configuration for recovering a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RecoverPartition, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RecoverPartitionRoles;
        // Security configuration for recovering partitions
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RecoverPartitions, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RecoverPartitionsRoles;
        // Security configuration for recovering service partitions
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RecoverServicePartitions, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RecoverServicePartitionsRoles;
        // Security configuration for recovering system service partitions
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", RecoverSystemPartitions, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum RecoverSystemPartitionsRoles;
        // Security configuration for reporting fault
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ReportFault, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ReportFaultRoles;

        // Security configuration for infrastructure task management commands
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", InvokeInfrastructureCommand, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum InvokeInfrastructureCommandRoles;

        // Security configuration for image store client file transfer (external to cluster)
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", FileContent, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum FileContentRoles;
        // Security configuration for image store client file download initiation (external to cluster)
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", FileDownload, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum FileDownloadRoles;
        // Security configuration for image store client file list operation (internal)
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", InternalList, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum InternalListRoles;
        // Security configuration for image store client delete operation
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", Delete, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteRoles;

        // Upload action on the FileStoreService internally invokes
        // GetStagingLocation and GetStoreLocation actions. If the role
        // of upload is modified, modify the role of the other two too.

        // Security configuration for image store client upload operation
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", Upload, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UploadRoles;
        // Security configuration for image store client staging location retrieval
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetStagingLocation, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetStagingLocationRoles;
        // Security configuration for image store client store location retrieval
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetStoreLocation, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetStoreLocationRoles;

        // Security configuration for starting, stopping, and restarting nodes
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", NodeControl, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum NodeControlRoles;

        // Security configuration for restarting code packages
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CodePackageControl, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CodePackageControlRoles;

       // Unreliable Transport for adding and removing behaviors
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UnreliableTransportControl, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UnreliableTransportControlRoles;
       // Move replica
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", MoveReplicaControl, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum MoveReplicaControlRoles;
        //Predeployment api
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", PredeployPackageToNode, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum PredeployPackageToNodeRoles;

        // Induces data loss on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartPartitionDataLoss, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartPartitionDataLossRoles;
        // Induces quorum loss on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartPartitionQuorumLoss, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartPartitionQuorumLossRoles;
        // Simultaneously restarts some or all the replicas of a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartPartitionRestart, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartPartitionRestartRoles;
        // Cancels a specific TestCommand - if it is in flight
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CancelTestCommand, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CancelTestCommandRoles;

        // Starts Chaos - if it is not already started
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartChaos, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartChaosRoles;
        // Stops Chaos - if it has been started
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StopChaos, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StopChaosRoles;

        // Security configuration for starting a node transition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartNodeTransition, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartNodeTransitionRoles;

        // Get secret values
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetSecrets, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetSecretsRoles;

        // Induces StartClusterConfigurationUpgrade on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartClusterConfigurationUpgrade, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartClusterConfigurationUpgradeRoles;

        // Induces GetUpgradesPendingApproval on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetUpgradesPendingApproval, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetUpgradesPendingApprovalRoles;

        // Induces StartApprovedUpgrades on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", StartApprovedUpgrades, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum StartApprovedUpgradesRoles;

        // Induces GetUpgradeOrchestrationServiceState on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetUpgradeOrchestrationServiceState, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetUpgradeOrchestrationServiceStateRoles;

        // Induces SetUpgradeOrchestrationServiceState on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", SetUpgradeOrchestrationServiceState, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum SetUpgradeOrchestrationServiceStateRoles;

        // Creates an compose deployment described by compose files
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateComposeDeployment, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateComposeDeploymentRoles;

        // Deletes the compose deployment
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteComposeDeployment, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteComposeDeploymentRoles;

        // Upgrades the compose deployment
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", UpgradeComposeDeployment, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum UpgradeComposeDeploymentRoles;

        // Invoke container API
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", InvokeContainerApi, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum InvokeContainerApiRoles;

        // Creates a volume
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateVolume, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateVolumeRoles;

        // Deletes a volume
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteVolume, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteVolumeRoles;

        // Creates a container network
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateNetwork, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateNetworkRoles;

        // Deletes a container network
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteNetwork, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteNetworkRoles;

        // Create a gateway resource
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", CreateGatewayResource, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum CreateGatewayResourceRoles;

        // Deletes a gateway resource
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", DeleteGatewayResource, L"Admin", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum DeleteGatewayResourceRoles;

        // Admin and User operations
        //

        // Security configuration for client pings
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", Ping, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum PingRoles;
        // Security configuration for queries
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", Query, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum QueryRoles;

        // Security configuration for Naming URI existence checks
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", NameExists, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum NameExistsRoles;
        // Security configuration for Naming URI enumeration
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", EnumerateSubnames, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum EnumerateSubnamesRoles;
        // Security configuration for Naming property enumeration
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", EnumerateProperties, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum EnumeratePropertiesRoles;
        // Security configuration for Naming property read operations
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", PropertyReadBatch, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum PropertyReadBatchRoles;

        // Security configuration for long-poll service notifications and reading service descriptions
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetServiceDescription, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetServiceDescriptionRoles;
        // Security configuration for complaint-based service resolution
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ResolveService, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ResolveServiceRoles;
        // Security configuration for resolving Naming URI owner
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ResolveNameOwner, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ResolveNameOwnerRoles;
        // Security configuration for resolving system services
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ResolvePartition, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ResolvePartitionRoles;
        // Security configuration for event-based service notifications
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ServiceNotifications, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ServiceNotificationsRoles;
        // Security configuration for complaint-based service prefix resolution
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", PrefixResolveService, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum PrefixResolveServiceRoles;
        // Security configuration for resolving system services
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ResolveSystemService, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ResolveSystemServiceRoles;

        // Security configuration for polling application upgrade status
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetUpgradeStatus, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetUpgradeStatusRoles;
        // Security configuration for polling cluster upgrade status
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetFabricUpgradeStatus, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetFabricUpgradeStatusRoles;

        // Security configuration for querying infrastructure tasks
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", InvokeInfrastructureQuery, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum InvokeInfrastructureQueryRoles;

        // Security configuration for image store client file list operation
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", List, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ListRoles;
        // Security configuration for reset load for a failoverUnit
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ResetPartitionLoad, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ResetPartitionLoadRoles;

        // Security configuration for Toggling Verbose ServicePlacement HealthReporting
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", ToggleVerboseServicePlacementHealthReporting, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum ToggleVerboseServicePlacementHealthReportingRoles;

        // Fetches the progress for an invoke data loss api call
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetPartitionDataLossProgress, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetPartitionDataLossProgressRoles;
        // Fetches the progress for an invoke quorum loss api call
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetPartitionQuorumLossProgress, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetPartitionQuorumLossProgressRoles;
        // Fetches the progress for a restart partition api call
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetPartitionRestartProgress, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetPartitionRestartProgressRoles;

        // Fetches the status of Chaos within a given time range
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetChaosReport, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetChaosReportRoles;

        // Security configuration for getting progress on a node transition command
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetNodeTransitionProgress, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetNodeTransitionProgressRoles;

        // Induces GetClusterConfigurationUpgradeStatus on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetClusterConfigurationUpgradeStatus, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetClusterConfigurationUpgradeStatusRoles;

        // Induces GetClusterConfiguration on a partition
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", GetClusterConfiguration, L"Admin||User", Common::ConfigEntryUpgradePolicy::Dynamic) public: Transport::RoleMask::Enum GetClusterConfigurationRoles;

        void Initialize() override;

    private:
        static Transport::RoleMask::Enum GetRoles(std::wstring const & roles);
    };
}
