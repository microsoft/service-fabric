// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Specifies the status of the compose deployment.
    /// </summary>
    public enum ComposeDeploymentStatus
    {
        /// <summary>
        /// Invalid.
        /// </summary>
        Invalid = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_INVALID,

        /// <summary>
        /// The application type is being provisioned at background.
        /// </summary>
        Provisioning = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_PROVISIONING,

        /// <summary>
        /// The application is being created at background.
        /// </summary>
        Creating = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_CREATING,

        /// <summary>
        /// The docker compose deployment creation or upgrade is complete.
        /// </summary>
        Ready = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_READY,

        /// <summary>
        /// The application type is being unprovisioned at background.
        /// </summary>
        Unprovisioning = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_UNPROVISIONING,

        /// <summary>
        /// The application is being deleted at background.
        /// </summary>
        Deleting = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_DELETING,

        /// <summary>
        /// Creation or deletion was terminated due to persistent failures.
        /// </summary>
        Failed = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_FAILED,

        /// <summary>
        /// The deployment is being upgraded.
        /// </summary>
        Upgrading = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS.FABRIC_COMPOSE_DEPLOYMENT_STATUS_UPGRADING
    }
}
