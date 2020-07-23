// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Description
{
    using Microsoft.ServiceFabric.Preview.Client;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes a compose deployment to be created by using 
    /// System.Fabric.FabricClient.ComposeDeploymentClient.CreateComposeDeploymentAsync(Microsoft.ServiceFabric.Preview.Client.Description.ComposeDeploymentDescription) .</para>
    /// </summary>
    public sealed class ComposeDeploymentDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="ComposeDeploymentDescription" /> with the application instance name, and the path to the compose files.</para>
        /// </summary>
        /// <param name="deploymentName">
        /// <para>Name of compose deployment.</para>
        /// </param>
        /// <param name="composeFilePath">
        /// <para>Path to the compose file.</para>
        /// </param>
        public ComposeDeploymentDescription(string deploymentName, string composeFilePath)
        {
            this.DeploymentName = deploymentName;
            this.ComposeFilePath = composeFilePath;
            this.ContainerRegistryUserName = null;
            this.ContainerRegistryPassword = null;
            this.IsRegistryPasswordEncrypted = false;
        }

        /// <summary>
        /// <para>Gets the name of the compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the compose deployment.</para>
        /// </value>
        public string DeploymentName { get; private set; }

        /// <summary>
        /// <para>Gets the path to the compose file.</para>
        /// </summary>
        /// <value>
        /// <para>The compose file path.</para>
        /// </value>
        public string ComposeFilePath { get; private set; }

        /// <summary>
        /// <para>Gets the container registry user name.</para>
        /// </summary>
        /// <value>
        /// <para>The container registry user name.</para>
        /// </value>
        public string ContainerRegistryUserName { get; set; }

        /// <summary>
        /// <para>Gets the container registry password.</para>
        /// </summary>
        /// <value>
        /// <para>The container registry user password.</para>
        /// </value>
        public string ContainerRegistryPassword { get; set; }

        /// <summary>
        /// <para>Gets if the registry password specified is encrypted or not.</para>
        /// </summary>
        /// <value>
        /// <para>True if the registry password is encrypted. False otherwise.</para>
        /// </value>
        public bool IsRegistryPasswordEncrypted { get; set; }

        internal System.Fabric.Description.ComposeDeploymentDescriptionWrapper ToWrapper()
        {
            return new System.Fabric.Description.ComposeDeploymentDescriptionWrapper(
                this.DeploymentName,
                this.ComposeFilePath)
            {
                ContainerRepositoryUserName = this.ContainerRegistryUserName,
                ContainerRepositoryPassword = this.ContainerRegistryPassword,
                IsRepositoryPasswordEncrypted = this.IsRegistryPasswordEncrypted
            };
        }
    }
}