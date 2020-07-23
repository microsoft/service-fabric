// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FabricNodeConfig;
    typedef std::shared_ptr<FabricNodeConfig> FabricNodeConfigSPtr;

    class FabricNodeConfig : ComponentConfig
    {
        DECLARE_COMPONENT_CONFIG(FabricNodeConfig, "FabricNode")

    public:
        
        typedef ConfigCollection<std::wstring, std::wstring> NodePropertyCollectionMap;
        typedef ConfigCollection<std::wstring, std::wstring> KeyStringValueCollectionMap;
        typedef ConfigCollection<std::wstring, uint> NodeCapacityRatioCollectionMap;
        typedef ConfigCollection<std::wstring, uint> NodeCapacityCollectionMap;
        typedef ConfigCollection<std::wstring, std::wstring> NodeSfssRgPolicyMap;

        class NodeFaultDomainIdCollection : public std::vector<Common::Uri>
        {
        public:
            static NodeFaultDomainIdCollection Parse(Common::StringMap const & entries);
            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        };

        // This setting specifies the Id that identifies a node in the cluster for
        // internal communication. Each Node in the cluster must have a unique NodeId.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", NodeId, L"0", ConfigEntryUpgradePolicy::Static);

        // This setting specifies the version of the FabricNode. The value is set by 
        // FabricDeployer during the deployment time. 
        // The version string of the format <CodeVersion>:<ConfigVersion>:<InstanceId>.
        // This setting can be dynamically updated during the configuration only upgrades or 
        // on instance id changes.
        INTERNAL_CONFIG_ENTRY(Common::FabricVersionInstance, L"FabricNode", NodeVersion, FabricVersionInstance::Default, ConfigEntryUpgradePolicy::Dynamic);

        // Endpoint for federation layer communications between one node and another.
        // If the Node is a Seed/Voter node, then this address is also specified in the
        // votes section of all nodes.
        // Required to be set, default here is an invalid value.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", NodeAddress, L"<IPorFullyQualifiedDomainName>:<Port>", ConfigEntryUpgradePolicy::Static);

        //IPAddressOrFQDN property specified in ClusterManifest
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", IPAddressOrFQDN, L"0.0.0.0", ConfigEntryUpgradePolicy::Static);

        // Endpoint for communication between the lease drivers of different nodes.
        // This endpoint is exclusive to lease traffic.
        // Required to be set, default here is an invalid value.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", LeaseAgentAddress, L"<IPorFullyQualifiedDomainName>:<Port>", ConfigEntryUpgradePolicy::Static);
        // Address for the IPC channel between the Fabric Runtime in the user service
        // host process and Windows Fabric service on this node. 
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", RuntimeServiceAddress, L"0.0.0.0:0", ConfigEntryUpgradePolicy::Static);
        // This setting specifies the address than Naming Clients can talk to contact
        // the Naming Service in the cluster through this particular node.
        // Not having this address, clients are not able to contact the naming service through this node.
        // Required to be set, default here is an invalid value.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", ClientConnectionAddress, L"<IPorFullyQualifiedDomainName>:<Port>", ConfigEntryUpgradePolicy::Static);
        // Address for the IPC channel between Fabric gateway and Fabric
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", GatewayIpcAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address where the http gateway listens.
        // Required to be set, default here is an invalid value.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", HttpGatewayListenAddress, L"<IPorFullyQualifiedDomainName>:<Port>", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", HttpGatewayProtocol, L"", ConfigEntryUpgradePolicy::Static);
        // The address where the http app gateway listens.
        // Required to be set, if the app gateway is enabled.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", HttpApplicationGatewayListenAddress, L"<IPorFullyQualifiedDomainName>:<Port>", ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", HttpApplicationGatewayProtocol, L"", ConfigEntryUpgradePolicy::Static);

        // Path and file name to location to place shared log container.
        // Use L"" for using default path under fabric data root.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", SharedLogFilePath, L"", Common::ConfigEntryUpgradePolicy::Static);

        // Unique guid for shared log container. Use L"" if using default path under fabric data root.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", SharedLogFileId, L"", Common::ConfigEntryUpgradePolicy::Static);

        // The number of MB to allocate in the shared log container
        INTERNAL_CONFIG_ENTRY(int, L"FabricNode", SharedLogFileSizeInMB, 0, Common::ConfigEntryUpgradePolicy::Static);

        // The address that cluster manager uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", ClusterManagerReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address that repair manager uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", RepairManagerReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address that image store service uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", ImageStoreServiceReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address that naming service uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", NamingReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address that Failover manager uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", FailoverManagerReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);        
        // The address that FaultAnalysisService uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", FaultAnalysisServiceReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);        
        // The address that BackupRestoreService uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", BackupRestoreServiceReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);        
        // The address that CentralSecretService uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", CentralSecretServiceReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address that UpgradeService uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", UpgradeServiceReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);        
        // The address that EventStoreService uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", EventStoreServiceReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);
        // The address that GatewayResourceManager uses to replicate to its other replica.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", GatewayResourceManagerReplicatorAddress, L"localhost:0", ConfigEntryUpgradePolicy::Static);

        // Path used by the Fabric Service to read or write information for local operation.
        // This is the base directory for all other directories specified at the node, unless
        // those directories specify full paths. If no value is provided then it takes as base
        // the directory in which the Service is running.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", WorkingDir, L"", ConfigEntryUpgradePolicy::Static);

        // Node name
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", InstanceName, L"", ConfigEntryUpgradePolicy::Static);

        // Type of the Node defined in the cluster manifest
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", NodeType, L"", ConfigEntryUpgradePolicy::Static);

        // specifies if this node is part of the scalemin deployment or not
        INTERNAL_CONFIG_ENTRY(bool, L"FabricNode", IsScaleMin, false, ConfigEntryUpgradePolicy::Static);

        // The interval for tracing node status on each node and up nodes on FM/FMM.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricNode", StateTraceInterval, TimeSpan::FromSeconds(300), ConfigEntryUpgradePolicy::Static);

        // Whether to assert and kill the process when a node fails (because of lease for example).
        INTERNAL_CONFIG_ENTRY(bool, L"FabricNode", AssertOnNodeFailure, false, ConfigEntryUpgradePolicy::Static);

        // Timeout used for requests to FM when initializing system services
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricNode", SystemServiceInitializationTimeout, TimeSpan::FromSeconds(1), ConfigEntryUpgradePolicy::Static);
        // Retry interval used for requests to FM when initializing system services
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricNode", SystemServiceInitializationRetryInterval, TimeSpan::FromSeconds(1), ConfigEntryUpgradePolicy::Static);

        // Describes the upgrade domain a node belongs to.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"NodeDomainIds", UpgradeDomainId, L"", ConfigEntryUpgradePolicy::Static);
        // Describes the fault domains a node belongs to.
        // The fault domain is defined through a URI that describes the location of the node in the datacenter.  Fault Domain URIs are of the format fd:/fd/ followed by a URI path segment.
        PUBLIC_CONFIG_GROUP(NodeFaultDomainIdCollection, L"NodeDomainIds", NodeFaultDomainIds, ConfigEntryUpgradePolicy::Static);
        // A collection of string key-value pairs for node properties.
        PUBLIC_CONFIG_GROUP(NodePropertyCollectionMap, L"NodeProperties", NodeProperties, ConfigEntryUpgradePolicy::Static);
        // A collection of node capacities for different metrics.
        PUBLIC_CONFIG_GROUP(NodeCapacityCollectionMap, L"NodeCapacities", NodeCapacities, ConfigEntryUpgradePolicy::Static);

        // A collection of SF system services resource governance policies.
        INTERNAL_CONFIG_GROUP(NodeSfssRgPolicyMap, L"NodeSfssRgPolicies", NodeSfssRgPolicies, ConfigEntryUpgradePolicy::Dynamic);

        // Start of the application ports managed by hosting subsystem. 
        // Required if EndpointFilteringEnabled is true in Hosting.
        PUBLIC_CONFIG_ENTRY(int, L"FabricNode", StartApplicationPortRange, 0, ConfigEntryUpgradePolicy::Static);
        // End (no inclusive) of the application ports managed by hosting subsystem.
        // Required if EndpointFilteringEnabled is true in Hosting.
        PUBLIC_CONFIG_ENTRY(int, L"FabricNode", EndApplicationPortRange, 0, ConfigEntryUpgradePolicy::Static);

        /* -------------- Cluster Security -------------- */

        // Name of X.509 certificate store that contains cluster certificate for securing intra-cluster communication
#ifdef PLATFORM_UNIX
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClusterX509StoreName, L"", ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClusterX509StoreName, L"My", ConfigEntryUpgradePolicy::Dynamic);
#endif
        // Indicates how to search for cluster certificate in the store specified by ClusterX509StoreName
        // Supported values: "FindByThumbprint", "FindBySubjectName"
        // With "FindBySubjectName", when there are multiple matches, the one with the furthest expiration is used.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClusterX509FindType, L"FindByThumbprint", ConfigEntryUpgradePolicy::Dynamic);
        // Search filter value used to locate cluster certificate.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClusterX509FindValue, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClusterX509FindValueSecondary, L"", ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Client server Security -------------- */

        // Name of X.509 certificate store that contains server certificate for entree service
#ifdef PLATFORM_UNIX
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ServerAuthX509StoreName, L"", ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ServerAuthX509StoreName, L"My", ConfigEntryUpgradePolicy::Dynamic);
#endif
        // Indicates how to search for server certificate in the store specified by ServerAuthX509StoreName
        // Supported value: FindByThumbprint, FindBySubjectName
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ServerAuthX509FindType, L"FindByThumbprint", ConfigEntryUpgradePolicy::Dynamic);
        // Search filter value used to locate server certificate.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ServerAuthX509FindValue, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ServerAuthX509FindValueSecondary, L"", ConfigEntryUpgradePolicy::Dynamic);

        // Name of the X.509 certificate store that contains certificate for default admin role FabricClient.
#ifdef PLATFORM_UNIX
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClientAuthX509StoreName, L"", ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClientAuthX509StoreName, L"My", ConfigEntryUpgradePolicy::Dynamic);
#endif
        // Indicates how to search for certificate in the store specified by ClientAuthX509StoreName
        // Supported value: FindByThumbprint, FindBySubjectName
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClientAuthX509FindType, L"FindByThumbprint", ConfigEntryUpgradePolicy::Dynamic);
        // Search filter value used to locate certificate for default admin role FabricClient
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClientAuthX509FindValue, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", ClientAuthX509FindValueSecondary, L"", ConfigEntryUpgradePolicy::Dynamic);

        // Name of the X.509 certificate store that contains certificate for default user role FabricClient.
#ifdef PLATFORM_UNIX
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", UserRoleClientX509StoreName, L"", ConfigEntryUpgradePolicy::Dynamic);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", UserRoleClientX509StoreName, L"My", ConfigEntryUpgradePolicy::Dynamic);
#endif
        // Indicates how to search for certificate in the store specified by UserRoleClientX509StoreName
        // Supported value: FindByThumbprint, FindBySubjectName
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", UserRoleClientX509FindType, L"FindByThumbprint", ConfigEntryUpgradePolicy::Dynamic);
        // Search filter value used to locate certificate for default user role FabricClient
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", UserRoleClientX509FindValue, L"", ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricNode", UserRoleClientX509FindValueSecondary, L"", ConfigEntryUpgradePolicy::Dynamic);

        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricNode", StartStopFileName, L"StartStopNode.txt", ConfigEntryUpgradePolicy::Static);

        // How often the timer that checks if stopped state has expired fires
        INTERNAL_CONFIG_ENTRY(int, L"FabricNode", StoppedNodeExpiredTimerInterval, 60, ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_GROUP(KeyStringValueCollectionMap, L"LogicalApplicationDirectories", LogicalApplicationDirectories, Common::ConfigEntryUpgradePolicy::NotAllowed);

        INTERNAL_CONFIG_GROUP(KeyStringValueCollectionMap, L"LogicalNodeDirectories", LogicalNodeDirectories, Common::ConfigEntryUpgradePolicy::NotAllowed);

     public:
         void SetHttpGatewayPort(unsigned short port) { httpGatewayPort_ = port; }
         unsigned short HttpGatewayPort() const { return httpGatewayPort_; }

     private:
         unsigned short httpGatewayPort_ = 0;
    };
}
