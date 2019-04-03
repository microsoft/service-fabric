// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Description
{
    using System;
    using System.Fabric.Description;

    /// <summary>
    /// <para>Describes a rollback of compose deployment upgrade by using 
    /// <see cref="Microsoft.ServiceFabric.Preview.Client.Extensions.RollbackComposeDeploymentUpgradeAsync(System.Fabric.FabricClient.ComposeDeploymentClient, ComposeDeploymentRollbackDescription, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class ComposeDeploymentRollbackDescription
    {
        /// <summary>
        /// <para>Creates an instance of <see cref="Microsoft.ServiceFabric.Preview.Client.Description.ComposeDeploymentRollbackDescription" />. </para>
        /// </summary>
        /// <param name="deploymentName">
        /// <para>The name of compose deployment.</para>
        /// </param>
        public ComposeDeploymentRollbackDescription(string deploymentName)
        {
            this.DeploymentName = deploymentName;
        }

        /// <summary>
        /// <para>Gets the name of the compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the compose deployment.</para>
        /// </value>
        public string DeploymentName { get; private set; }

        internal ComposeDeploymentRollbackDescriptionWrapper ToWrapper()
        {
            return new ComposeDeploymentRollbackDescriptionWrapper(
               this.DeploymentName);
        }
    }
}