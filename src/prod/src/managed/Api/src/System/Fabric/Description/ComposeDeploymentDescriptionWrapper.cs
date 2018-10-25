// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes a compose application to be created by using 
    /// <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.CreateComposeDeploymentAsync(ComposeDeploymentDescriptionWrapper)" />.</para>
    /// </summary>
    internal sealed class ComposeDeploymentDescriptionWrapper
    {
        public ComposeDeploymentDescriptionWrapper(
            string deploymentName,
            string composeFilePath)
        {
            this.DeploymentName = deploymentName;
            this.ComposeFilePath = composeFilePath;
            this.ContainerRepositoryUserName = null;
            this.ContainerRepositoryPassword = null;
            this.IsRepositoryPasswordEncrypted = false;
        }

       public string DeploymentName { get; private set; }

       public string ComposeFilePath { get; private set; }

       public string ContainerRepositoryUserName { get; set; }

       public string ContainerRepositoryPassword { get; set; }

       public bool IsRepositoryPasswordEncrypted { get; set; }


        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION();

            nativeDescription.DeploymentName = pinCollection.AddObject(this.DeploymentName);
            nativeDescription.ComposeFilePath = pinCollection.AddObject(this.ComposeFilePath);
            nativeDescription.OverridesFilePath = IntPtr.Zero;

            if (this.ContainerRepositoryUserName != null)
            {
                nativeDescription.ContainerRepositoryUserName = pinCollection.AddObject(this.ContainerRepositoryUserName);
            }
            else
            {
                nativeDescription.ContainerRepositoryUserName = IntPtr.Zero;
            }

            if (this.ContainerRepositoryPassword != null)
            {
                nativeDescription.ContainerRepositoryPassword = pinCollection.AddObject(this.ContainerRepositoryPassword);
            }
            else
            {
                nativeDescription.ContainerRepositoryPassword = IntPtr.Zero;
            }

            nativeDescription.IsRepositoryPasswordEncrypted = NativeTypes.ToBOOLEAN(this.IsRepositoryPasswordEncrypted);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}