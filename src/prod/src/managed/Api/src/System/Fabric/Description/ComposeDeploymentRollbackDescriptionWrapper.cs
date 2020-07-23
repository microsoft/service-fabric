// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes a rollback of compose deployment upgrade by using
    /// <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.RollbackComposeDeploymentUpgradeAsync(System.Fabric.Description.ComposeDeploymentRollbackDescriptionWrapper, System.TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    internal sealed class ComposeDeploymentRollbackDescriptionWrapper
    {
        /// <summary>
        /// <para>Creates an instance of <see cref="System.Fabric.Description.ComposeDeploymentRollbackDescriptionWrapper" />. </para>
        /// </summary>
        /// <param name="deploymentName">
        /// <para>The name of compose deployment.</para>
        /// </param>
        public ComposeDeploymentRollbackDescriptionWrapper(string deploymentName)
        {
            Requires.Argument<string>("deploymentName", deploymentName).NotNull();
            this.DeploymentName = deploymentName;
        }

        /// <summary>
        /// <para>Gets the name of the compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the compose deployment.</para>
        /// </value>
        public string DeploymentName { get; private set; }

        internal IntPtr ToNative(PinCollection pin)
        {
            var desc = new NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_ROLLBACK_DESCRIPTION();
            desc.DeploymentName = pin.AddObject(DeploymentName);
            desc.Reserved = IntPtr.Zero;

            return pin.AddBlittable(desc);
        }
    }
}