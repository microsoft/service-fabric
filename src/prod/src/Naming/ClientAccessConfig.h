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
#undef CreateService

#define DECL_ROLE_MASK(Operation) public: Transport::RoleMask::Enum Operation##Roles;

#define DEFINE_SECURITY_CONFIG(Operation, AccessLevel) \
    PRIVATE_CONFIG_ENTRY(std::wstring, L"Security/ClientAccess", Operation, AccessLevel, Common::ConfigEntryUpgradePolicy::Dynamic) \
    DECL_ROLE_MASK(Operation)

#define DEFINE_SECURITY_CONFIG_ADMIN(Operation) DEFINE_SECURITY_CONFIG(Operation, L"Admin")

#define DEFINE_SECURITY_CONFIG_USER(Operation) DEFINE_SECURITY_CONFIG(Operation, L"Admin||User")

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
        DEFINE_SECURITY_CONFIG_ADMIN( CreateName )
        // Security configuration for Naming URI deletion
        DEFINE_SECURITY_CONFIG_ADMIN( DeleteName )
        // Security configuration for Naming property write operations
        DEFINE_SECURITY_CONFIG_ADMIN( PropertyWriteBatch )

        // Security configuration for service creation
        DEFINE_SECURITY_CONFIG_ADMIN( CreateService )

        // Security configuration for service creatin from template
        DEFINE_SECURITY_CONFIG_ADMIN( CreateServiceFromTemplate )
        // Security configuration for service updates
        DEFINE_SECURITY_CONFIG_ADMIN( UpdateService )
        // Security configuration for service deletion
        DEFINE_SECURITY_CONFIG_ADMIN( DeleteService )

        // Security configuration for application type provisioning
        DEFINE_SECURITY_CONFIG_ADMIN( ProvisionApplicationType )
        // Security configuration for application creation
        DEFINE_SECURITY_CONFIG_ADMIN( CreateApplication )
        // Security configuration for application deletion
        DEFINE_SECURITY_CONFIG_ADMIN( DeleteApplication )
        // Security configuration for starting or interrupting application upgrades
        DEFINE_SECURITY_CONFIG_ADMIN( UpgradeApplication )
        // Security configuration for rolling back application upgrades
        DEFINE_SECURITY_CONFIG_ADMIN( RollbackApplicationUpgrade )
        // Security configuration for application type unprovisioning
        DEFINE_SECURITY_CONFIG_ADMIN( UnprovisionApplicationType )
        // Security configuration for resuming application upgrades with an explicit Upgrade Domain
        DEFINE_SECURITY_CONFIG_ADMIN( MoveNextUpgradeDomain )
        // Security configuration for resuming application upgrades with the current upgrade progress
        DEFINE_SECURITY_CONFIG_ADMIN( ReportUpgradeHealth )

        // Security configuration for reporting health
        DEFINE_SECURITY_CONFIG_ADMIN( ReportHealth )

        // Security configuration for MSI and/or Cluster Manifest provisioning
        DEFINE_SECURITY_CONFIG_ADMIN( ProvisionFabric )
        // Security configuration for starting cluster upgrades
        DEFINE_SECURITY_CONFIG_ADMIN( UpgradeFabric )
        // Security configuration for rolling back cluster upgrades
        DEFINE_SECURITY_CONFIG_ADMIN( RollbackFabricUpgrade )
        // Security configuration for MSI and/or Cluster Manifest unprovisioning
        DEFINE_SECURITY_CONFIG_ADMIN( UnprovisionFabric )
        // Security configuration for resuming cluster upgrades with an explicity Upgrade Domain
        DEFINE_SECURITY_CONFIG_ADMIN( MoveNextFabricUpgradeDomain )
        // Security configuration for resuming cluster upgrades with the current upgrade progress
        DEFINE_SECURITY_CONFIG_ADMIN( ReportFabricUpgradeHealth )

        // Security configuration for starting infrastructure tasks
        DEFINE_SECURITY_CONFIG_ADMIN( StartInfrastructureTask )
        // Security configuration for finishing infrastructure tasks
        DEFINE_SECURITY_CONFIG_ADMIN( FinishInfrastructureTask )

        // Security configuration for activation a node
        DEFINE_SECURITY_CONFIG_ADMIN( ActivateNode )
        // Security configuration for deactivating a node
        DEFINE_SECURITY_CONFIG_ADMIN( DeactivateNode )

        // Security configuration for deactivating multiple nodes
        DEFINE_SECURITY_CONFIG_ADMIN( DeactivateNodesBatch )
        // Security configuration for reverting deactivation on multiple nodes
        DEFINE_SECURITY_CONFIG_ADMIN( RemoveNodeDeactivations )
        // Security configuration for checking deactivation status
        DEFINE_SECURITY_CONFIG_ADMIN( GetNodeDeactivationStatus )

        // Security configuration for reporting node state removed
        DEFINE_SECURITY_CONFIG_ADMIN( NodeStateRemoved )
        // Security configuration for recovering a partition
        DEFINE_SECURITY_CONFIG_ADMIN( RecoverPartition )
        // Security configuration for recovering partitions
        DEFINE_SECURITY_CONFIG_ADMIN( RecoverPartitions )
        // Security configuration for recovering service partitions
        DEFINE_SECURITY_CONFIG_ADMIN( RecoverServicePartitions )
        // Security configuration for recovering system service partitions
        DEFINE_SECURITY_CONFIG_ADMIN( RecoverSystemPartitions )
        // Security configuration for reporting fault
        DEFINE_SECURITY_CONFIG_ADMIN( ReportFault )

        // Security configuration for infrastructure task management commands
        DEFINE_SECURITY_CONFIG_ADMIN( InvokeInfrastructureCommand )

        // Security configuration for image store client file transfer (external to cluster)
        DEFINE_SECURITY_CONFIG_ADMIN(FileContent)
        // Security configuration for image store client file download initiation (external to cluster)
        DEFINE_SECURITY_CONFIG_ADMIN(FileDownload)
        // Security configuration for image store client file list operation (internal)
        DEFINE_SECURITY_CONFIG_ADMIN(InternalList)
        // Security configuration for image store client delete operation
        DEFINE_SECURITY_CONFIG_ADMIN(Delete)

        // Upload action on the FileStoreService internally invokes
        // GetStagingLocation and GetStoreLocation actions. If the role
        // of upload is modified, modify the role of the other two too.

        // Security configuration for image store client upload operation
        DEFINE_SECURITY_CONFIG_ADMIN(Upload)
        // Security configuration for image store client staging location retrieval
        DEFINE_SECURITY_CONFIG_ADMIN(GetStagingLocation)
        // Security configuration for image store client store location retrieval
        DEFINE_SECURITY_CONFIG_ADMIN(GetStoreLocation)

        // Security configuration for starting, stopping, and restarting nodes
        DEFINE_SECURITY_CONFIG_ADMIN( NodeControl )

        // Security configuration for restarting code packages
        DEFINE_SECURITY_CONFIG_ADMIN( CodePackageControl )

       // Unreliable Transport for adding and removing behaviors
        DEFINE_SECURITY_CONFIG_ADMIN(UnreliableTransportControl)
       // Move replica
        DEFINE_SECURITY_CONFIG_ADMIN( MoveReplicaControl )
        //Predeployment api
        DEFINE_SECURITY_CONFIG_ADMIN( PredeployPackageToNode )

        // Induces data loss on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( StartPartitionDataLoss )
        // Induces quorum loss on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( StartPartitionQuorumLoss )
        // Simultaneously restarts some or all the replicas of a partition
        DEFINE_SECURITY_CONFIG_ADMIN( StartPartitionRestart )
        // Cancels a specific TestCommand - if it is in flight
        DEFINE_SECURITY_CONFIG_ADMIN( CancelTestCommand )

        // Starts Chaos - if it is not already started
        DEFINE_SECURITY_CONFIG_ADMIN( StartChaos )
        // Stops Chaos - if it has been started
        DEFINE_SECURITY_CONFIG_ADMIN( StopChaos )

        // Security configuration for starting a node transition
        DEFINE_SECURITY_CONFIG_ADMIN( StartNodeTransition )

        // Get secret values
        DEFINE_SECURITY_CONFIG_ADMIN(GetSecrets)

        // Induces StartClusterConfigurationUpgrade on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( StartClusterConfigurationUpgrade )

        // Induces GetUpgradesPendingApproval on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( GetUpgradesPendingApproval )

        // Induces StartApprovedUpgrades on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( StartApprovedUpgrades )

        // Induces GetUpgradeOrchestrationServiceState on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( GetUpgradeOrchestrationServiceState )

        // Induces SetUpgradeOrchestrationServiceState on a partition
        DEFINE_SECURITY_CONFIG_ADMIN( SetUpgradeOrchestrationServiceState )

        // Creates an compose deployment described by compose files
        DEFINE_SECURITY_CONFIG_ADMIN( CreateComposeDeployment )

        // Deletes the compose deployment
        DEFINE_SECURITY_CONFIG_ADMIN( DeleteComposeDeployment )

        // Upgrades the compose deployment
        DEFINE_SECURITY_CONFIG_ADMIN( UpgradeComposeDeployment )

        // Invoke container API
        DEFINE_SECURITY_CONFIG_ADMIN( InvokeContainerApi )

        // Creates a volume
        DEFINE_SECURITY_CONFIG_ADMIN(CreateVolume)

        // Deletes a volume
        DEFINE_SECURITY_CONFIG_ADMIN(DeleteVolume)

        // Creates a container network
        DEFINE_SECURITY_CONFIG_ADMIN(CreateNetwork)

        // Deletes a container network
        DEFINE_SECURITY_CONFIG_ADMIN(DeleteNetwork)

        // Create a gateway resource
        DEFINE_SECURITY_CONFIG_ADMIN(CreateGatewayResource)

        // Deletes a gateway resource
        DEFINE_SECURITY_CONFIG_ADMIN(DeleteGatewayResource)

        // Admin and User operations
        //

        // Security configuration for client pings
        DEFINE_SECURITY_CONFIG_USER( Ping )
        // Security configuration for queries
        DEFINE_SECURITY_CONFIG_USER( Query )

        // Security configuration for Naming URI existence checks
        DEFINE_SECURITY_CONFIG_USER( NameExists )
        // Security configuration for Naming URI enumeration
        DEFINE_SECURITY_CONFIG_USER( EnumerateSubnames )
        // Security configuration for Naming property enumeration
        DEFINE_SECURITY_CONFIG_USER( EnumerateProperties )
        // Security configuration for Naming property read operations
        DEFINE_SECURITY_CONFIG_USER( PropertyReadBatch )

        // Security configuration for long-poll service notifications and reading service descriptions
        DEFINE_SECURITY_CONFIG_USER( GetServiceDescription )
        // Security configuration for complaint-based service resolution
        DEFINE_SECURITY_CONFIG_USER( ResolveService )
        // Security configuration for resolving Naming URI owner
        DEFINE_SECURITY_CONFIG_USER( ResolveNameOwner )
        // Security configuration for resolving system services
        DEFINE_SECURITY_CONFIG_USER( ResolvePartition )
        // Security configuration for event-based service notifications
        DEFINE_SECURITY_CONFIG_USER( ServiceNotifications )
        // Security configuration for complaint-based service prefix resolution
        DEFINE_SECURITY_CONFIG_USER( PrefixResolveService )
        // Security configuration for resolving system services
        DEFINE_SECURITY_CONFIG_USER( ResolveSystemService )

        // Security configuration for polling application upgrade status
        DEFINE_SECURITY_CONFIG_USER( GetUpgradeStatus )
        // Security configuration for polling cluster upgrade status
        DEFINE_SECURITY_CONFIG_USER( GetFabricUpgradeStatus )

        // Security configuration for querying infrastructure tasks
        DEFINE_SECURITY_CONFIG_USER( InvokeInfrastructureQuery )

        // Security configuration for image store client file list operation
        DEFINE_SECURITY_CONFIG_USER( List )
        // Security configuration for reset load for a failoverUnit
        DEFINE_SECURITY_CONFIG_USER( ResetPartitionLoad )

        // Security configuration for Toggling Verbose ServicePlacement HealthReporting
        DEFINE_SECURITY_CONFIG_USER( ToggleVerboseServicePlacementHealthReporting )

        // Fetches the progress for an invoke data loss api call
        DEFINE_SECURITY_CONFIG_USER( GetPartitionDataLossProgress )
        // Fetches the progress for an invoke quorum loss api call
        DEFINE_SECURITY_CONFIG_USER( GetPartitionQuorumLossProgress )
        // Fetches the progress for a restart partition api call
        DEFINE_SECURITY_CONFIG_USER( GetPartitionRestartProgress )

        // Fetches the status of Chaos within a given time range
        DEFINE_SECURITY_CONFIG_USER( GetChaosReport )

        // Security configuration for getting progress on a node transition command
        DEFINE_SECURITY_CONFIG_USER( GetNodeTransitionProgress )

        // Induces GetClusterConfigurationUpgradeStatus on a partition
        DEFINE_SECURITY_CONFIG_USER( GetClusterConfigurationUpgradeStatus )

        // Induces GetClusterConfiguration on a partition
        DEFINE_SECURITY_CONFIG_USER( GetClusterConfiguration )

        void Initialize() override;

    private:
        static Transport::RoleMask::Enum GetRoles(std::wstring const & roles);
    };
}
