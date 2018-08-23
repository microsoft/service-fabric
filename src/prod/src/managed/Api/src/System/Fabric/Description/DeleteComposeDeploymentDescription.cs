// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes an application to be deleted by using 
    /// <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.DeleteComposeDeploymentWrapperAsync(DeleteComposeDeploymentDescriptionWrapper, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    internal sealed class DeleteComposeDeploymentDescriptionWrapper
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.DeleteComposeDeploymentDescriptionWrapper" />. </para>
        /// </summary>
        /// <param name="deploymentName">
        /// <para>The name of compose deployment.</para>
        /// </param>
        public DeleteComposeDeploymentDescriptionWrapper(string deploymentName)
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
            var desc = new NativeTypes.FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION();
            desc.DeploymentName = pin.AddObject(DeploymentName);
            desc.Reserved = IntPtr.Zero;

            return pin.AddBlittable(desc);
        }
    }
}