// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{ 
    internal class FlatNetworkConstants
    {
        #region Firewall Constants
#if !DotNetCoreClrLinux
        /// <summary>
        /// Firewall policy name
        /// </summary>
        public static readonly string FirewallPolicy2TypeProgID = "HNetCfg.FwPolicy2";

        /// <summary>
        /// Group name used when creating firewall rule
        /// </summary>
        public static readonly string FirewallGroupName = "ServiceFabric";

        /// <summary>
        /// Name used when creating firewall rule
        /// </summary>
        public static readonly string FirewallRuleName = FirewallGroupName + ".ContainerFlatNetwork.v1";

        /// <summary>
        /// Description used when creating firewall rule
        /// </summary>
        public static readonly string FirewallRuleDescription = "Inbound rule for ServiceFabric Container Flat Networking";
#endif
        #endregion

        #region Container Service Constants

        /// <summary>
        /// Docker provider service name
        /// </summary>
        public static readonly string ContainerProviderProcessName = "dockerd";

        /// <summary>
        /// Docker daemon pid file directory name
        /// </summary>
        public static readonly string DockerProcessIdFileDirectory = "_sf_docker_pid";

        /// <summary>
        /// Docker daemon pid file name
        /// </summary>
        public static readonly string DockerProcessFile = "sfdocker.pid";

#if !DotNetCoreClrLinux
        /// <summary>
        /// Docker service name
        /// </summary>
        public static readonly string ContainerServiceName = "Docker";

        /// <summary>
        /// Docker provider service name with extension
        /// </summary>
        public static readonly string ContainerProviderProcessNameWithExtension = "dockerd.exe";

        /// <summary>
        /// Argument passed in to dockerd to start up in debug mode.
        /// </summary>
        public static readonly string ContainerProviderServiceDebugModeArg = "-D";

        /// <summary>
        /// Docker daemon arguments
        /// </summary>
        public static readonly string ContainerServiceArguments = "-H localhost:2375 -H npipe://";
#else
        /// <summary>
        /// Azure network plugin used to set up flat network
        /// </summary>
        public static readonly string NetworkDriverPlugin = "azure-cns";

        /// <summary>
        /// Azure vnet sock file path
        /// </summary>
        public static readonly string AzureVnetPluginSockPath = "/run/docker/plugins/azure-vnet.sock";

        /// <summary>
        /// Docker daemon arguments
        /// </summary>
        public static readonly string ContainerServiceArguments = "-H localhost:2375 -H unix:///var/run/docker.sock";
#endif
        #endregion

        #region Network Constants

        /// <summary>
        /// Nm agent network interface url
        /// </summary>
        public static readonly string NetworkInterfaceUrl = "http://169.254.169.254/machine/plugins?comp=nmagent&type=getinterfaceinfov1";

        /// <summary>
        /// Network name used when setting up network
        /// </summary>
        public static readonly string NetworkName = "servicefabric_network";

        /// <summary>
        /// Url used for retrieving docker network details
        /// </summary>
        public static readonly string NetworkGetByTypeRequestPath = "networks?filters={{\"driver\":[\"{0}\"]}}";

        /// <summary>
        /// Url used for deleting docker network
        /// </summary>
        public static readonly string NetworkDeleteRequestPath = "networks/{0}";

        /// <summary>
        /// Port on which Dns service is listening
        /// </summary>
        public static readonly int PortNumber = 53;

        /// <summary>
        /// Character separating ip and mask in CIDR format
        /// </summary>
        public static readonly string SubnetMaskSeparator = "/";

#if !DotNetCoreClrLinux
        /// <summary>
        /// Network driver used to set up flat network
        /// </summary>
        public static readonly string NetworkDriver = "l2tunnel";

        /// <summary>
        /// Url used for creating docker network
        /// </summary>
        public static readonly string NetworkCreateRequestPath = "networks/create";

        /// <summary>
        /// Windows HNS parameter name to take the network interface to be used, to set up the flat network.
        /// </summary>
        public static string NetworkInterfaceParameterName = "com.docker.network.windowsshim.interface";
#else
        /// <summary>
        /// Network driver used to set up flat network
        /// </summary>
        public static readonly string NetworkDriver = "azure-vnet";

        /// <summary>
        /// Url used for creating docker network
        /// </summary>
        public static readonly string NetworkCreateUrl = "http://localhost:10090/network/create";

        /// <summary>
        /// Url used for initializing a CNS network environment
        /// </summary>
        public static readonly string NetworkEnvInitializationUrl = "http://localhost:10090/network/environment";

        /// <summary>
        /// Indicates that networks should be created with the type Overlay
        /// </summary>
        public static readonly string OverlayNetworkType = "Overlay";

        /// <summary>
        /// Indicates that networks should be created with the type Underlay
        /// </summary>
        public static readonly string UnderlayNetworkType = "Underlay";

        /// <summary>
        /// Networks are provisioned in Azure
        /// </summary>
        public static readonly string NetworkEnvAzure = "Azure";

        /// <summary>
        /// Networks are provisioned in an on-premise environment, i.e. not Azure
        /// </summary>
        public static readonly string NetworkEnvOnPrem = "StandAlone";

        /// <summary>
        /// Iptable accept target action
        /// </summary>
        public static readonly string AcceptAction = "ACCEPT";

        /// <summary>
        /// Iptable all protocols supported
        /// </summary>
        public static readonly string AllProtocols = "all";

        /// <summary>
        /// Iptable any ip for source or destination
        /// </summary>
        public static readonly string AnyIp = "anywhere";

        /// <summary>
        /// Iptable no options specified
        /// </summary>
        public static readonly string NoOptions = "--";

        /// <summary>
        /// Ebtables package name
        /// </summary>
        public static readonly string EbtablesPackageName = "ebtables";

        /// <summary>
        /// Dpkg installed status
        /// </summary>
        public static readonly string DpkgInstallStatus = "install";

        /// <summary>
        /// Dpkg uninstalled status
        /// </summary>
        public static readonly string DpkgUninstallStatus = "deinstall";
#endif
        #endregion
    }
}