// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Sockets;
    using System.Security;

    internal abstract class FabricValidator
    {
        public static FabricEvents.ExtensionsEvents TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageBuilder);

        public bool DoMachineAndNodeHaveSameIPInScaleMin { get; private set; }

        private FabricSettingsValidator fabricSettingsValidator;

        public bool IsSingleMachineDeployment { get; private set; }

        public DeploymentOperations DeploymentOperation { get; set; }

        internal SecureString ImageStoreConnectionString
        { 
            get { return fabricSettingsValidator.ImageStoreConnectionString; } 
        }

        internal bool IsRunAsPolicyEnabled
        {
            get { return fabricSettingsValidator.IsRunAsPolicyEnabled; }
        }

        internal bool IsV2NodeIdGeneratorEnabled
        {
            get { return fabricSettingsValidator.IsV2NodeIdGeneratorEnabled; }
        }

        internal string NodeIdGeneratorVersion
        {
            get { return fabricSettingsValidator.NodeIdGeneratorVersion; }
        }

        internal SettingsType HostSettings { get; private set; }

        internal SecureString DiagnosticsFileStoreConnectionString
        {
            get { return fabricSettingsValidator.DiagnosticsFileStoreConnectionString; }
        }

        internal string LogContainer
        {
            get { return fabricSettingsValidator.LogContainer; }
        }

        internal string CrashDumpContainer
        {
            get { return fabricSettingsValidator.CrashDumpContainer; }
        }

        internal SecureString DiagnosticsTableStoreConnectionString
        {
            get { return fabricSettingsValidator.DiagnosticsTableStoreConnectionString; }
        }

        internal string DiagnosticsTableStoreName
        {
            get { return fabricSettingsValidator.DiagnosticsTableStoreName; }
        }

        internal bool IsDCAEnabled { get { return fabricSettingsValidator.IsDCAEnabled; } }

        internal bool IsDCAFileStoreEnabled { get { return fabricSettingsValidator.IsDCAFileStoreEnabled; } }

        internal bool IsDCATableStoreEnabled { get { return fabricSettingsValidator.IsDCATableStoreEnabled; } }

        internal bool ShouldRegisterSpnForMachineAccount { get { return fabricSettingsValidator.ShouldRegisterSpnForMachineAccount; } }

        internal bool IsAppLogCollectionEnabled
        {
            get { return fabricSettingsValidator.IsAppLogCollectionEnabled; }
        }

        internal int AppLogDirectoryQuotaInMB
        {
            get { return fabricSettingsValidator.AppLogDirectoryQuotaInMB; }
        }

#if !DotNetCoreClrLinux
        internal List<DCASettingsValidator.PluginInfo> DCAConsumers
        {
            get { return fabricSettingsValidator.DCAConsumers; }
        }
#endif

        internal bool IsHttpGatewayEnabled { get { return fabricSettingsValidator.IsHttpGatewayEnabled; } }

        internal bool IsHttpAppGatewayEnabled { get { return fabricSettingsValidator.IsHttpAppGatewayEnabled; } }

        internal bool IsTVSEnabled { get { return fabricSettingsValidator.IsTVSEnabled; } }

        internal string StoreName { get; private set; }

        protected int NonSeedVoteCount { get { return fabricSettingsValidator.NonSeedNodeVoteCount; } }

        protected ClusterManifestType clusterManifest;

        protected List<InfrastructureNodeType> infrastructureInformation;

        protected bool IsScaleMin;

        public static FabricValidator Create(ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructure, WindowsFabricSettings windowsFabricSettings, DeploymentOperations deploymentOperation)
        {
#if DotNetCoreClrLinux
            var isServer = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux;
#else
            var isServer = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer;
#endif
            if (isServer)
            {
                return new FabricValidatorServer(clusterManifest, infrastructure, windowsFabricSettings);
            }
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                return new FabricValidatorAzure(clusterManifest, infrastructure, windowsFabricSettings);
            }
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureBlackbird)
            {
                return new FabricValidatorBlackbird(clusterManifest, infrastructure, windowsFabricSettings);
            }
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
            {
                var paasValidator = new FabricValidatorPaaS(clusterManifest, infrastructure, windowsFabricSettings);
                paasValidator.DeploymentOperation = deploymentOperation;
                return paasValidator;
            }

            throw new InvalidOperationException("The Infrastucture type is not supported");
        }

        public FabricValidator(ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructureInformation, WindowsFabricSettings windowsFabricSettings)
        {
            ReleaseAssert.AssertIf(clusterManifest == null, "clusterManifest should be non-null");
            this.clusterManifest = clusterManifest;
            this.infrastructureInformation = infrastructureInformation;
            this.StoreName = windowsFabricSettings.StoreName;
            this.DoMachineAndNodeHaveSameIPInScaleMin = true;
            this.IsSingleMachineDeployment = true;

            this.fabricSettingsValidator = new FabricSettingsValidator(
                windowsFabricSettings,
                clusterManifest,
                this.infrastructureInformation);
        }

        public void Validate()
        {
            if (!FabricValidatorUtility.IsValidFileName(this.clusterManifest.Version))
            {
                throw new ArgumentException("cluster manifest version is not valid");
            }

            fabricSettingsValidator.ValidateSettings();
            this.ValidateNodeTypes();
            this.ValidateNodeList();
            fabricSettingsValidator.ValidateImageStore(this.IsSingleMachineDeployment);
            this.ValidateInfrastructure();
            ValidateClusterManifestVersion(this.clusterManifest);
        }

        public IEnumerable<KeyValuePair<string, string>> CompareAndAnalyze(ClusterManifestType newClusterManifest, string nodeTypeNameFilter)
        {
            return fabricSettingsValidator.CompareSettings(newClusterManifest, nodeTypeNameFilter);
        }

        private void ValidateNodeTypes()
        {
            if (this.clusterManifest.NodeTypes == null)
            {
                if (this.DeploymentOperation == DeploymentOperations.Update && this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
                {
                    return;
                }

                FabricValidator.TraceSource.WriteWarning(
                FabricValidatorUtility.TraceTag,
                StringResources.Info_FabricValidatorNodeTypeNull);
                return;
            }

#if DotNetCoreClrLinux
            var isServer = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux;
#else
            var isServer = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer;
#endif
            //Validate StartPort and EndPort number and range in nodetypes
            if (isServer || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                foreach (ClusterManifestTypeNodeType fabricNode in this.clusterManifest.NodeTypes)
                {
                    try
                    {
                        ValidatePortRangHelper(fabricNode.Name,
                            fabricNode.Endpoints.ApplicationEndpoints.StartPort,
                            fabricNode.Endpoints.ApplicationEndpoints.EndPort); 

                        if (fabricNode.Endpoints.EphemeralEndpoints != null)
                        {
                            // validate dynamic port range
                            ValidatePortRangHelper(fabricNode.Name, fabricNode.Endpoints.EphemeralEndpoints.StartPort, fabricNode.Endpoints.EphemeralEndpoints.EndPort);
                            ValidateNoOverlappingPortsRange(fabricNode.Name,
                                fabricNode.Endpoints.ApplicationEndpoints.StartPort,
                                fabricNode.Endpoints.ApplicationEndpoints.EndPort,
                                fabricNode.Endpoints.EphemeralEndpoints.StartPort,
                                fabricNode.Endpoints.EphemeralEndpoints.EndPort);
                        }

                        //Validate the other five ports
                        ValidateEndpointsPortsHelper(fabricNode.Endpoints.ClientConnectionEndpoint.Port, fabricNode.Name, "ClientConnectionEndpoint");
                        ValidateEndpointsPortsHelper(fabricNode.Endpoints.LeaseDriverEndpoint.Port, fabricNode.Name, "LeaseDriverEndpoint");
                        ValidateEndpointsPortsHelper(fabricNode.Endpoints.ClusterConnectionEndpoint.Port, fabricNode.Name, "ClusterConnectionEndpoint");
                        ValidateEndpointsPortsHelper(fabricNode.Endpoints.HttpGatewayEndpoint.Port, fabricNode.Name, "HttpGatewayEndpoint");
                        // TODO Add validation to http appgateway endpoint
                        //ValidateEndpointsPortsHelper(fabricNode.Endpoints.HttpApplicationGatewayEndpoint.Port, fabricNode.Name, "HttpApplicationGatewayEndpoint");
                        ValidateEndpointsPortsHelper(fabricNode.Endpoints.ServiceConnectionEndpoint.Port, fabricNode.Name, "ServiceConnectionEndpoint");

                        if (fabricNode.KtlLoggerSettings != null)
                        {
                            ValidateKtlLoggerSettingsHelper(fabricNode.KtlLoggerSettings.SharedLogFilePath != null ? fabricNode.KtlLoggerSettings.SharedLogFilePath.Value : "",
                                                            fabricNode.KtlLoggerSettings.SharedLogFileId != null ? fabricNode.KtlLoggerSettings.SharedLogFileId.Value : "",
                                                            fabricNode.KtlLoggerSettings.SharedLogFileSizeInMB != null ? fabricNode.KtlLoggerSettings.SharedLogFileSizeInMB.Value : 0);
                        }

                        // Validate SF system service resource governance policies
                        ValidateSfssRgPoliciesSettingsHelper(fabricNode.SfssRgPolicies, fabricNode.Name);

                    }
                    catch (NullReferenceException)
                    {
                        FabricValidator.TraceSource.WriteWarning(
                        FabricValidatorUtility.TraceTag,
                        StringResources.Info_FabricValidatorPortsNull);
                        return;
                    }
                }
            }
        }


        private void ValidateKtlLoggerSettingsHelper(String sharedLogFilePath, String sharedLogFileId, int sharedLogFileSizeInMB)
        {
            KtlLoggerConfigurationValidator.ValidateKtlLoggerSharedPathAndId(sharedLogFilePath, sharedLogFileId, "NodeTypes");

            if ((sharedLogFileSizeInMB < 512) && (sharedLogFileSizeInMB != 0))
            {
                throw new ArgumentException(
                  string.Format(
                     StringResources.Error_FabricValidatorSharedLogFileSizeTooSmall, sharedLogFileSizeInMB));
            }
        }

        private void ValidateSfssRgPoliciesSettingsHelper(KeyValuePairType[] sfssRgPolicies, string nodeTypeName)
        {
            foreach (var rgPolicy in sfssRgPolicies)
            {
                try
                {
                    if (rgPolicy.Name.Contains("ProcessCpusetCpus"))
                    {
                        string[] cores = rgPolicy.Value.Split(',');
                        foreach (var core in cores)
                        {
                            Int32 coreId = Convert.ToInt32(core);
                            // Allowed affinitizations to first 64 cores
                            if (coreId < 0 || coreId > 63)
                            {
                                throw new ArgumentException(
                                    string.Format(
                                         StringResources.SfssRgPolicyError_InvalidValueRange, rgPolicy.Name, nodeTypeName, 0, 63));
                            }
                        }

                    }
                    else if (rgPolicy.Name.Contains("ProcessCpuShares"))
                    {
                        Int32 value = Convert.ToInt32(rgPolicy.Value);
                        // Allowed values for the Cpu rates are up to 10000 (i.e. 100%)
                        if (value < 0 || value > 10000)
                        {
                            throw new ArgumentException(
                            string.Format(
                                 StringResources.SfssRgPolicyError_InvalidValueRange, rgPolicy.Name, nodeTypeName, 0, 10000));
                        }
                    }
                    else if (rgPolicy.Name.Contains("ProcessMemoryInMB"))
                    {
                        Int32 value = Convert.ToInt32(rgPolicy.Value);
                        // Allowed working memory limit up to 100GB
                        const int memoryLimit = 100 * 1024;
                        if (value < 0 || value > memoryLimit)
                        {
                            throw new ArgumentException(
                            string.Format(
                                 StringResources.SfssRgPolicyError_InvalidValueRange, rgPolicy.Name, nodeTypeName, 0, memoryLimit));

                        }
                    }
                    else if (rgPolicy.Name.Contains("ProcessMemorySwapInMB"))
                    {
                        Int32 value = Convert.ToInt32(rgPolicy.Value);
                        // Allowed commited memory limit up to 1TB
                        const int memoryLimit = 1024 * 1024;
                        if (value < 0 || value > memoryLimit)
                        {
                            throw new ArgumentException(
                            string.Format(
                                    StringResources.SfssRgPolicyError_InvalidValueRange, rgPolicy.Name, nodeTypeName, 0, memoryLimit));

                        }
                    }
                }
                catch (FormatException e)
                {
                    throw new ArgumentException(
                        string.Format(
                             StringResources.SfssRgPolicyError_InvalidValueType, rgPolicy.Name, nodeTypeName, e));
                }
                catch (OverflowException e)
                {
                    throw new ArgumentException(
                        string.Format(
                             StringResources.SfssRgPolicyError_InvalidValueType, rgPolicy.Name, nodeTypeName, e));
                }
            }
        }


        private void ValidateEndpointsPortsHelper(String port, String nodeName, String portType)
        {
            int intPort;
            try
            {
                intPort = Convert.ToInt32(port);
                if (intPort < 0)
                {
                    throw new ArgumentException(
                    string.Format(
                         StringResources.Warning_FabricValidatorPortsNegative, nodeName, portType));
                }
            }
            catch (FormatException e)
            {
                throw new ArgumentException(
                    string.Format(
                         StringResources.Warning_FabricValidatorPortsNotDigit, nodeName, portType, e));
            }
            catch (OverflowException e)
            {
                throw new ArgumentException(
                    string.Format(
                        StringResources.Warning_FabricValidatorPortsNotInt32, nodeName, portType, e));
            }
        }

        private void ValidatePortRangHelper(string nodeName, int startPort, int endPort)
        {
            // when ports are not set from cluster manifest.
            if (startPort == 0 && endPort == 0)
            {
                return; 
            }

            if (startPort < 0 || endPort < 0)
            {
                throw new ArgumentException(
                    string.Format(
                        StringResources.Warning_FabricValidatorStartEndPortNegative, nodeName));
            }
            if (startPort > endPort)
            {
                throw new ArgumentException(
                    string.Format(
                        StringResources.Warning_FabricValidatorPortRangeInvalid, nodeName));
            }

            if(startPort < 1024 || endPort > 65535)
            {
                throw new ArgumentException(string.Format( StringResources.Warning_FabricValidatorPortRangeInvalid, nodeName)); 
            }
        }

        private void ValidateNoOverlappingPortsRange(string nodeName,
            int applicationStartPort,
            int applicationEndPort,
            int dynamicStartPort,
            int dynamicEndPort
            )
        {
            if((dynamicEndPort < applicationStartPort && dynamicEndPort > applicationEndPort) || 
                (applicationStartPort <dynamicEndPort &&  applicationEndPort > dynamicEndPort))
            {
                throw new ArgumentException(string.Format(StringResources.Warning_FabricValidatorPortRangeOverlapping, nodeName));
            }
        }

        private void ValidateNodeList()
        {

            if (this.infrastructureInformation == null)
            {
                FabricValidator.TraceSource.WriteWarning(
                    FabricValidatorUtility.TraceTag,
                    "Skipping node list validation as it is empty");
                return;
            }

            //Validate StartPort and EndPort number and range
            if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure)
            {
                foreach (InfrastructureNodeType infra in this.infrastructureInformation)
                {
                    try
                    {
                        if (infra.Endpoints.ApplicationEndpoints.StartPort < 0 || infra.Endpoints.ApplicationEndpoints.EndPort < 0)
                        {
                            throw new ArgumentException(
                                string.Format(
                                    StringResources.Warning_FabricValidatorStartEndPortNegative, infra.NodeName));
                        }

                        if (infra.Endpoints.ApplicationEndpoints.StartPort > infra.Endpoints.ApplicationEndpoints.EndPort)
                        {
                            throw new ArgumentException(
                                string.Format(
                                    StringResources.Warning_FabricValidatorPortRangeInvalid, infra.NodeName));
                        }

                        //Validate the other five ports
                        ValidateEndpointsPortsHelper(infra.Endpoints.ClientConnectionEndpoint.Port, infra.NodeName, "ClientConnectionEndpoint");
                        ValidateEndpointsPortsHelper(infra.Endpoints.LeaseDriverEndpoint.Port, infra.NodeName, "LeaseDriverEndpoint");
                        ValidateEndpointsPortsHelper(infra.Endpoints.ClusterConnectionEndpoint.Port, infra.NodeName, "ClusterConnectionEndpoint");
                        ValidateEndpointsPortsHelper(infra.Endpoints.HttpGatewayEndpoint.Port, infra.NodeName, "HttpGatewayEndpoint");
                        // TODO add validation for http app gateway endpoint
                        // ValidateEndpointsPortsHelper(infra.Endpoints.HttpApplicationGatewayEndpoint.Port, infra.NodeName, "HttpApplicationGatewayEndpoint");
                        ValidateEndpointsPortsHelper(infra.Endpoints.ServiceConnectionEndpoint.Port, infra.NodeName, "ServiceConnectionEndpoint");
                    }
                    catch (NullReferenceException)
                    {
                        FabricValidator.TraceSource.WriteWarning(
                        FabricValidatorUtility.TraceTag,
                        StringResources.Info_FabricValidatorPortsNull);
                        return;
                    }
                }
            }

            
#if DotNetCoreClrLinux
            var isServer = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux;
#else
            var isServer = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer;
#endif
            if (isServer || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                foreach (InfrastructureNodeType infra in this.infrastructureInformation)
                {
                    if (Helpers.isIPV6AddressAndNoBracket(infra.IPAddressOrFQDN)) 
                    {
                        throw new ArgumentException(StringResources.Error_BracketsAroundIPV6AddressAreMandatory, infra.NodeName);
                    }
                }
            }

            int voteCount = this.NonSeedVoteCount;
            ClusterManifestTypeInfrastructurePaaS paasInfrastructure = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructurePaaS;
            if(paasInfrastructure != null)
            {
                voteCount += paasInfrastructure.Votes.Count();
            }

            bool matchingLocalNodeFound = false;
            string prevMachine = this.infrastructureInformation[0].IPAddressOrFQDN;

            if (this.IsScaleMin && !NetworkApiHelper.IsNodeForThisMachine(this.infrastructureInformation[0]))
            {
                this.DoMachineAndNodeHaveSameIPInScaleMin = false;
            }

            bool hasLoopbackAddressDefined = false;
            HashSet<string> nodeNameDuplicateDetector = new HashSet<string>();
            List<string> faultDomains = new List<string>();
            foreach (InfrastructureNodeType fabricNode in this.infrastructureInformation)
            {
                ClusterManifestTypeNodeType nodeType = this.clusterManifest.NodeTypes.FirstOrDefault(nodeTypeVar => FabricValidatorUtility.EqualsIgnoreCase(nodeTypeVar.Name, fabricNode.NodeTypeRef));

                if (nodeType == null)
                {
                    if (!(this.DeploymentOperation == DeploymentOperations.Update && this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS))
                    {
                        throw new ArgumentException(
                            string.Format(
                            "Node '{0}' has attribute NodeTypeRef='{1}'. However, no NodeType with name {1} exists.",
                            fabricNode.NodeName,
                            fabricNode.NodeTypeRef));
                    }
                }

                if (fabricNode.IsSeedNode)
                {
                    voteCount++;
                }

                faultDomains.Add(fabricNode.FaultDomain);
                if (!FabricValidatorUtility.IsValidFileName(fabricNode.NodeName))
                {
                  throw new ArgumentException(
                        string.Format(
                        "Invalid character found in node name {0}. Node name should be a valid file name.",
                        fabricNode.NodeName));
                }

                if (!FabricValidatorUtility.IsValidIPAddressOrFQDN(fabricNode.IPAddressOrFQDN))
                {
                  throw new ArgumentException(
                        string.Format(
                        "Malformed IPAddressOrFQDN found {0}.",
                        fabricNode.IPAddressOrFQDN));
                }

                if (nodeNameDuplicateDetector.Contains(fabricNode.NodeName))
                {
                  throw new ArgumentException(
                        string.Format(
                        "Duplicate node name {0} found.",
                        fabricNode.NodeName));
                }
                else
                {
                    nodeNameDuplicateDetector.Add(fabricNode.NodeName);
                }

                if (fabricNode.IPAddressOrFQDN != prevMachine &&
                    !FabricValidatorUtility.IsNodeAddressLoopback(fabricNode))
                {
                    this.IsSingleMachineDeployment = false;
                }

                if (!hasLoopbackAddressDefined)
                {
                    hasLoopbackAddressDefined = FabricValidatorUtility.IsNodeAddressLoopback(fabricNode);
                }

                FabricValidator.TraceSource.WriteNoise(
                    FabricValidatorUtility.TraceTag,
                    "fabricNode is {0}", fabricNode);                    

                if (!FabricValidatorUtility.IsNodeForThisMachine(fabricNode))
                {
                    continue;
                }

                if (matchingLocalNodeFound && !this.IsScaleMin)
                {
                  throw new ArgumentException(
                        string.Format(
                        "The IPAddressorFQDN value in node '{0}' is {1}. Another node is also configured to run on the same machine. " +
                        "Running multiple nodes on the same machine is not valid in the specified infrastructure.",
                        fabricNode.NodeName,
                        fabricNode.IPAddressOrFQDN));
                }
                else
                {
                    matchingLocalNodeFound = true;
                    FabricValidatorUtility.ValidateDomainUri(fabricNode.FaultDomain, fabricNode.NodeName);
                }
            }

            int fdPathCount = -1;
            foreach (string faultDomain in faultDomains)
            {
                if (!string.IsNullOrEmpty(faultDomain))
                {
                    int currentfdPathCount = faultDomain.Split('/').Length;
                    if (fdPathCount == -1)
                    {
                        fdPathCount = currentfdPathCount;
                    }
                    if (fdPathCount != currentfdPathCount)
                    {
                        throw new ArgumentException("All FaultDomains should have the same depth.");
                    }
                }
            }

            if (!matchingLocalNodeFound)
            {
                FabricValidator.TraceSource.WriteWarning(
                    FabricValidatorUtility.TraceTag,
                    "None of the declared nodes is for the current machine.");
            }

            if (!this.IsSingleMachineDeployment && hasLoopbackAddressDefined)
            {
                throw new ArgumentException("Even though the deployment is split over multiple machines there is at least one node with a loopback address");
            }

            if (voteCount == 0)
            {
                throw new ArgumentException("No votes specified. Either seed nodes or sql vote specification is required for cluster to bootstrap");
            }

            if (!this.DoMachineAndNodeHaveSameIPInScaleMin)
            {
                FabricValidator.TraceSource.WriteWarning(
                    FabricValidatorUtility.TraceTag,
                    StringResources.Warning_FirstNodeAndMachineHaveDifferentIP);
                return;
            }
        }

        public static void ValidateClusterManifestVersion(ClusterManifestType manifest)
        {
            if (string.IsNullOrEmpty(manifest.Version) || manifest.Version.Contains(":"))
            {
                throw new ArgumentException(
                string.Format(
                    StringResources.ImageBuilderError_InvalidClusterManifestVersion));
            }
        }

        protected abstract void ValidateInfrastructure();
    }
}