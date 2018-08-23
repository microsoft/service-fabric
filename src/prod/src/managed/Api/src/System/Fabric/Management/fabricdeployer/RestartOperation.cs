// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Fabric.Common;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Net;
    using System.Net.Sockets;
    using System.Linq;
    using Threading;

    internal class RestartOperation : DeploymentOperation
    {
        protected DeploymentParameters parameters;

        protected Infrastructure infrastructure;

        protected ClusterManifestType manifest;

        private static TimeSpan RetryInterval = TimeSpan.FromSeconds(5);

        private static int RetryCount = 6;

        protected FabricValidatorWrapper fabricValidator { get; set; }

        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            var isRunningAsAdmin = AccountHelper.IsAdminUser();
            if (!isRunningAsAdmin)
            {
                DeployerTrace.WriteWarning(StringResources.Warning_DeployerNotRunAsAdminSkipFirewallAndPerformanceCounter);
                return;
            }

            if (clusterManifest == null)
            {
                DeployerTrace.WriteError(StringResources.Error_FabricHostStartedWithoutConfiguringTheNode);
                throw new ArgumentException(StringResources.Error_FabricHostStartedWithoutConfiguringTheNode);
            }

            this.parameters = parameters;
            this.manifest = clusterManifest;
            this.infrastructure = infrastructure;
            this.fabricValidator = new FabricValidatorWrapper(parameters, manifest, infrastructure);
            fabricValidator.ValidateAndEnsureDefaultImageStore();

            if (!parameters.SkipFirewallConfiguration)
            {
                var securitySection = manifest.FabricSettings.FirstOrDefault(fabSetting => fabSetting.Name.Equals(Constants.SectionNames.Security, StringComparison.OrdinalIgnoreCase));

#if DotNetCoreClrLinux
                if (isRunningAsAdmin && clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux)
                {
                    var currentInfrastructure = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureLinux;
#else
                if (isRunningAsAdmin && clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
                {
                    var currentInfrastructure = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
#endif

                    var nodeSettings = GetNodeSettings();
                    var isScaleMin = currentInfrastructure.IsScaleMin;
                    FirewallManager.EnableFirewallSettings(nodeSettings, isScaleMin, securitySection, this is UpdateNodeStateOperation);
                    NetworkApiHelper.ReduceDynamicPortRange(nodeSettings, isScaleMin);
                }
                else if (isRunningAsAdmin)
                {
                    var nodeSettings = GetNodeSettings();
                    FirewallManager.EnableFirewallSettings(nodeSettings, false, securitySection, this is UpdateNodeStateOperation);
                    NetworkApiHelper.ReduceDynamicPortRange(nodeSettings, false);
                }
            }

#if !DotNetCoreClrIOT && !DotNetCoreClrLinux
            #region Container Network Reset
            // CreateOrUpdate operation inherits from RestartOperation.
            // This check will invoke network reset only in the restart case.
            // This is a work around to handle the case where the flat network was not usable after VM reboot.
            if (parameters.Operation == DeploymentOperations.None)
            {
                if (!parameters.SkipContainerNetworkResetOnReboot)
                {
                    var lastBootUpTimeFromRegistry = Utility.GetNodeLastBootUpTimeFromRegistry();
                    var lastBootUpTimeFromSystem = Utility.GetNodeLastBootUpTimeFromSystem();
                    DeployerTrace.WriteInfo("Last boot up time from registry:{0} from system:{1}", lastBootUpTimeFromRegistry, lastBootUpTimeFromSystem);

                    if (!string.Equals(lastBootUpTimeFromRegistry.ToString(), lastBootUpTimeFromSystem.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        DeployerTrace.WriteInfo("Invoking container network reset.");

                        // This check is needed to allow clean up on azure. This is symmetrical to the set up condition.
                        if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure ||
                            clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
                        {
                            var containerNetworkCleanupOperation = new ContainerNetworkCleanupOperation();
                            containerNetworkCleanupOperation.ExecuteOperation(parameters.ContainerNetworkName, parameters.Operation);
                        }

                        if (parameters.ContainerNetworkSetup)
                        {
                            // set up docker network.
                            var containerNetworkSetupOperation = new ContainerNetworkSetupOperation();
                            containerNetworkSetupOperation.ExecuteOperation(parameters, clusterManifest, infrastructure);
                        }

                        // Record last boot up time
                        Utility.SaveNodeLastBootUpTimeToRegistry(lastBootUpTimeFromSystem);
                    }
                }
                else
                {
                    DeployerTrace.WriteInfo("Skipping container network reset on reboot because SkipContainerNetworkResetOnReboot flag is enabled.");
                }
            }
            #endregion
#endif

#if !DotNetCoreClrIOT
            if (parameters.ContainerDnsSetup == ContainerDnsSetup.Allow || parameters.ContainerDnsSetup == ContainerDnsSetup.Require)
            {
                try
                {
                    string currentNodeIPAddressOrFQDN = string.Empty;
                    if ((infrastructure != null) && (infrastructure.InfrastructureNodes != null))
                    {
                        foreach (var infraNode in infrastructure.InfrastructureNodes)
                        {
                            DeployerTrace.WriteInfo("Infra node <{0}> params.NodeName <{1}>", infraNode.NodeName, parameters.NodeName);
                            if (NetworkApiHelper.IsAddressForThisMachine(infraNode.IPAddressOrFQDN))
                            {
                                currentNodeIPAddressOrFQDN = infraNode.IPAddressOrFQDN;
                                DeployerTrace.WriteInfo("Found node IPAddressOrFQDN <{0}>", currentNodeIPAddressOrFQDN);
                                break;
                            }
                        }
                    }

                    new DockerDnsHelper(parameters, currentNodeIPAddressOrFQDN).SetupAsync().GetAwaiter().GetResult();
                }
                catch (Exception ex)
                {
                    if (parameters.ContainerDnsSetup == ContainerDnsSetup.Require)
                    {
                        DeployerTrace.WriteError(
                            StringResources.Error_FabricDeployer_DockerDnsSetup_ErrorNotContinuing,
                            Constants.ParameterNames.ContainerDnsSetup,
                            parameters.ContainerDnsSetup,
                            ex);
                        throw;
                    }

                    DeployerTrace.WriteWarning(
                        StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorContinuing,
                        Constants.ParameterNames.ContainerDnsSetup,
                        parameters.ContainerDnsSetup,
                        ex);
                }
            }
            else if (parameters.ContainerDnsSetup == ContainerDnsSetup.Disallow)
            {
                // cleanupasync catches all exceptions
                new DockerDnsHelper(parameters, string.Empty).CleanupAsync().GetAwaiter().GetResult();
            }
#endif

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            if (!PerformanceCounters.StartCollection(clusterManifest.FabricSettings, parameters.DeploymentSpecification))
            {
                DeployerTrace.WriteWarning(StringResources.Error_FabricDeployer_StartCounterCollectionFailed_Formatted);
            }

            if (fabricValidator.ShouldRegisterSpnForMachineAccount)
            {
                if (!SpnManager.EnsureSpn())
                {
                    throw new InvalidDeploymentException(StringResources.Error_FabricDeployer_FailedToRegisterSpn_Formatted);
                }
            }
#endif
        }

        public List<NodeSettings> GetNodeSettings()
        {
            var versions = new Versions(
                manifest.Version,
                parameters.InstanceId,
                parameters.TargetVersion,
                parameters.CurrrentVersion ?? Utility.GetCurrentCodeVersion(null));

            List<NodeSettings> nodeSettings = new List<NodeSettings>();

            for (int i = 0; i < RestartOperation.RetryCount; i++)
            {
                foreach (var infrastructureNode in infrastructure.InfrastructureNodes)
                {
                    bool isNodeForThisMachine = NetworkApiHelper.IsNodeForThisMachine(infrastructureNode);
                    if (isNodeForThisMachine)
                    {
                        ClusterManifestTypeNodeType nodeType = manifest.NodeTypes.FirstOrDefault(nodeTypeVar => nodeTypeVar.Name.Equals(infrastructureNode.NodeTypeRef, StringComparison.OrdinalIgnoreCase));

                        // Ensuring literal IPv6 address
                        IPAddress ipAddress;
                        bool isIPv6Address = IPAddress.TryParse(infrastructureNode.IPAddressOrFQDN, out ipAddress) &&
                            (ipAddress.AddressFamily == AddressFamily.InterNetworkV6);
                        if (isIPv6Address)
                        {
                            infrastructureNode.IPAddressOrFQDN = string.Format(CultureInfo.InvariantCulture, "[{0}]", ipAddress.ToString());
                        }

                        nodeSettings.Add(new NodeSettings(
                            new FabricNodeType()
                            {
                                FaultDomain = infrastructureNode.FaultDomain,
                                IPAddressOrFQDN = infrastructureNode.IPAddressOrFQDN,
                                IsSeedNode = infrastructureNode.IsSeedNode,
                                NodeName = infrastructureNode.NodeName,
                                NodeTypeRef = infrastructureNode.NodeTypeRef,
                                UpgradeDomain = infrastructureNode.UpgradeDomain
                            },
                            nodeType,
                            new DeploymentFolders(
                                parameters.FabricBinRoot,
                                manifest.FabricSettings,
                                parameters.DeploymentSpecification,
                                infrastructureNode.NodeName,
                                versions,
                                infrastructure.IsScaleMin),
                            infrastructure.IsScaleMin,
                            infrastructureNode.Endpoints));
                    }
                }

                if (nodeSettings.Count != 0)
                {
                    break;
                }

                Thread.Sleep(RestartOperation.RetryInterval);
                NetworkApiHelper.CurrentMachineAddresses = NetworkApiHelper.GetCurrentMachineAddresses();
            }

            return nodeSettings;
        }
    }
}