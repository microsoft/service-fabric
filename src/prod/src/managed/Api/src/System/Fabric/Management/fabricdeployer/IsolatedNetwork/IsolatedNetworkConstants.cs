// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{ 
    internal class IsolatedNetworkConstants
    {
        #region Network Constants

        /// <summary>
        /// Network name used to set up the isolated network
        /// </summary>
        public static readonly string NetworkName = "isolated_servicefabric_network";

        /// <summary>
        /// Isolated network plugin name with extension
        /// </summary>
        public static readonly string NetworkPlugin = "sf_cns.exe";

        /// <summary>
        /// Isolated network plugin process name
        /// </summary>
        public static readonly string NetworkPluginProcessName = "sf_cns";

        /// <summary>
        /// Address prefix used by the isolated network
        /// </summary>
        public static readonly string DefaultAddressPrefix = "1.0.0.0/24";

        /// <summary>
        /// Gateway address used by the isolated network
        /// </summary>
        public static readonly string DefaultGatewayAddress = "1.0.0.1";

        /// <summary>
        /// Default vxlan id used by the isolated network
        /// </summary>
        public static readonly int DefaultVxlanId = 4444;

        /// <summary>
        /// Url used for retrieving isolated network details
        /// </summary>
        public static readonly string NetworkGetUrl = "http://localhost:10090/sf/networks";

        /// <summary>
        /// Url used for creating the isolated network
        /// </summary>
        public static readonly string NetworkCreateUrl = "http://localhost:10090/sf/network/create";

        /// <summary>
        /// Url used to delete the isolated network
        /// </summary>
        public static readonly string NetworkDeleteUrl = "http://localhost:10090/sf/network/delete";

        /// <summary>
        /// Network type used by the isolated network
        /// </summary>
        public static readonly string NetworkType = "Overlay";

        /// <summary>
        /// Network policy type used by the isolated network
        /// </summary>
        public static readonly string NetworkPolicyType = "VSID";

        /// <summary>
        /// Registry value to persist the network interface name used to set up the isolated network.
        /// This is used by the hosting subsystem when attaching the Docker container to the isolated network.
        /// </summary>
        public static readonly string IsolatedNetworkInterfaceNameRegValue = "FabricIsolatedNetworkInterfaceName";

        #endregion
    }
}
