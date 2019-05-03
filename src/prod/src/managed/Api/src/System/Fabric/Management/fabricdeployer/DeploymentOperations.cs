// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    internal enum DeploymentOperations
    {
        Configure, 
        Create,
        None,
        Remove,
        RemoveNodeConfig,
        Rollback,
        Update,
        UpdateNodeState,
        Validate,        
        ValidateClusterManifest,
        UpdateInstanceId,

        /// <summary>
        /// <see cref="DockerDnsHelper"/> class. This enum is used only for mock/simulation purpose.
        /// </summary>
        DockerDnsSetup,

        /// <summary>
        /// <see cref="DockerDnsHelper"/> class. This enum is used only for mock/simulation purpose.
        /// </summary>
        DockerDnsCleanup,

        /// <summary>
        /// Sets up docker network used for flat networking
        /// </summary>
        ContainerNetworkSetup,

        /// <summary>
        /// Cleans up docker network used for flat networking
        /// </summary>
        ContainerNetworkCleanup,

        /// <summary>
        /// Sets up the isolated network.
        /// </summary>
        IsolatedNetworkSetup,

        /// <summary>
        /// Cleans up the isolated network.
        /// </summary>
        IsolatedNetworkCleanup,
    }
}