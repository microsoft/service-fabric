// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Description
{
    using Microsoft.ServiceFabric.Preview.Client;
    using System;
    using System.Fabric.Description;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes an application to be deleted by using 
    /// <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.DeleteComposeDeploymentWrapperAsync(DeleteComposeDeploymentDescriptionWrapper, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class DeleteComposeDeploymentDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="Microsoft.ServiceFabric.Preview.Client.Description.DeleteComposeDeploymentDescription" />. </para>
        /// </summary>
        /// <param name="deploymentName">
        /// <para>The name of compose deployment.</para>
        /// </param>
        public DeleteComposeDeploymentDescription(string deploymentName)
        {
            this.DeploymentName = deploymentName;
        }

        /// <summary>
        /// <para>Gets the name of the compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the compose deployment.</para>
        /// </value>
        public string DeploymentName { get; set; }

        internal System.Fabric.Description.DeleteComposeDeploymentDescriptionWrapper ToWrapper()
        {
            return new System.Fabric.Description.DeleteComposeDeploymentDescriptionWrapper(
               this.DeploymentName);
        }
    }
}