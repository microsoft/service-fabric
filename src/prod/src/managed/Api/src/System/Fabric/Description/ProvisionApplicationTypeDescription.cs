// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Describes a provision application type operation which uses an application package copied to a relative path in the image store.
    /// The application type can be provisioned using
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.ProvisionApplicationAsync(System.Fabric.Description.ProvisionApplicationTypeDescriptionBase, System.TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class ProvisionApplicationTypeDescription : ProvisionApplicationTypeDescriptionBase
    {
        /// <summary>
        /// <para>Creates an instance of <see cref="System.Fabric.Description.ProvisionApplicationTypeDescription" />
        /// using the relative path to the application package in the image store.</para>
        /// </summary>
        /// <param name="buildPath">
        /// <para>The relative path to the application package in the image store specified during
        /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, System.TimeSpan)"/>.</para>
        /// </param>
        /// <remarks>
        /// <para>
        /// The pending provision operation can be interrupted using <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UnprovisionApplicationAsync(System.Fabric.Description.UnprovisionApplicationTypeDescription)" />.
        /// </para>
        /// </remarks>
        public ProvisionApplicationTypeDescription(string buildPath)
            : base(ProvisionApplicationTypeKind.ImageStorePath)
        {
            Requires.Argument<string>("buildPath", buildPath).NotNullOrWhiteSpace();
            this.BuildPath = buildPath;
        }

        /// <summary>
        /// <para>Gets the relative path to the application package in the image store specified during
        /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, System.TimeSpan)"/>.</para>
        /// </summary>
        /// <value>
        /// <para>The application package build path.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ApplicationTypeBuildPath)]
        public string BuildPath { get; private set; }

        /// <summary>
        /// <para>Gets or sets the policy indicating the application package cleanup policy </para>
        /// </summary>
        /// <value>
        /// <para>The policy indicating whether or not the cleanup of the application package is determined by service fabric runtime or determined by the cluster wide setting (CleanupApplicationPackageOnProvisionSuccess).
        /// By default the value is to use cluster wide setting.</para>
        /// </value>
        /// <remarks>
        /// <para>If the value is <see cref="System.Fabric.Description.ApplicationPackageCleanupPolicy.Automatic"/>, then the service fabric runtime determines when to perform the application package cleanup.By default, cleanup is done after a successful provision.
        /// If the value is <see cref="System.Fabric.Description.ApplicationPackageCleanupPolicy.Manual"/>, then user has to explictly clean up the application package by using <see cref="System.Fabric.FabricClient.ApplicationManagementClient.RemoveApplicationPackage(string, string)"/>.
        /// If the value is <see cref="System.Fabric.Description.ApplicationPackageCleanupPolicy.Default"/>, then cluster configuration setting "CleanupApplicationPackageOnProvisionSuccess" determines how application package cleanup should occur.
        /// The application package that is referred to was previously uploaded to the incoming location using 
        /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, System.TimeSpan)"/>.
        /// </para>
        /// </remarks>
        public ApplicationPackageCleanupPolicy ApplicationPackageCleanupPolicy { get; set; }

        /// <summary>
        /// Gets a string representation of the provision application type operation.
        /// </summary>
        /// <returns>A string representation of the provision application type operation.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "BuildPath: {0}, async: {1}", this.BuildPath, this.Async);
        }

        internal override IntPtr ToNativeValue(PinCollection pin)
        {
            var desc = new NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION()
            {
                BuildPath = pin.AddBlittable(this.BuildPath),
                Async = NativeTypes.ToBOOLEAN(this.Async)
            };

            var ex = new NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_EX1();
            ex.ApplicationPackageCleanupPolicy = (NativeTypes.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY)(this.ApplicationPackageCleanupPolicy);
            ex.Reserved = IntPtr.Zero;
            desc.Reserved = pin.AddBlittable(ex);

            return pin.AddBlittable(desc);
        }
    }
}
