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
    /// <para>Describes a provision application type operation which uses an application package from an external store,
    /// as opposed to a package uploaded to the Service Fabric image store.
    /// The application type can be provisioned using
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.ProvisionApplicationAsync(System.Fabric.Description.ProvisionApplicationTypeDescriptionBase, System.TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class ExternalStoreProvisionApplicationTypeDescription : ProvisionApplicationTypeDescriptionBase
    {
        /// <summary>
        /// <para>
        /// The extension for a Service Fabric application package compressed into a single file.
        /// Service Fabric recognizes files with this extension as application packages
        /// and can provision the application type contained within.
        /// </para>
        /// </summary>
        public const string SfpkgExtension = "sfpkg";

        /// <summary>
        /// <para>Creates an instance of <see cref="System.Fabric.Description.ExternalStoreProvisionApplicationTypeDescription" />
        /// using the download URI of the 'sfpkg' application package
        /// and the application type information.</para>
        /// </summary>
        /// <param name="applicationPackageDownloadUri">
        /// <para>
        /// The path to the '.sfpkg' application package from where the application package can be downloaded using HTTP or HTTPS protocols.
        /// </para>
        /// </param>
        /// <param name="applicationTypeName"><para>The application type name, defined in the application manifest.</para></param>
        /// <param name="applicationTypeVersion"><para>The application type version, defined in the application manifest.</para></param>
        /// <remarks>
        /// <para>
        /// The application package can be stored in an external store that provides GET operation to download the file.
        /// Supported protocols are HTTP and HTTPS, and the path must allow READ access.
        /// </para>
        /// <para>
        /// The download of the application package may take a long time, depending on its size and the network speed.
        /// We recommend you set the async parameter to <languageKeyword>true</languageKeyword>.
        /// In this case, the provision operation returns when the request is accepted by the system and the provision operation continues without any timeout limit.
        /// By default, async is set to false.
        /// </para>
        /// <para>
        /// You can query the provision operation state
        /// using <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypeListAsync()" />.
        /// </para>
        /// <para>
        /// The pending provision operation can be interrupted using <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UnprovisionApplicationAsync(System.Fabric.Description.UnprovisionApplicationTypeDescription)" />.
        /// </para>
        /// </remarks>
        public ExternalStoreProvisionApplicationTypeDescription(
            Uri applicationPackageDownloadUri,
            string applicationTypeName,
            string applicationTypeVersion)
            : base(ProvisionApplicationTypeKind.ExternalStore)
        {
            Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrWhiteSpace();
            Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNullOrWhiteSpace();
            CheckIfValidApplicationPackageDownloadUri(applicationPackageDownloadUri);

            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ApplicationPackageDownloadUri = applicationPackageDownloadUri;
        }

        /// <summary>
        /// <para>Gets the application type name.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The application type name.</para>
        /// </value>
        /// <remarks><para>
        /// The application type name represents the name of the application type found in the application manifest.
        /// </para></remarks>
        public string ApplicationTypeName { get; private set; }

        /// <summary>
        /// <para>Gets the application type version.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The application type version.</para>
        /// </value>
        /// <remarks><para>
        /// The application type version represents the version of the application type found in the application manifest.
        /// </para></remarks>
        public string ApplicationTypeVersion { get; private set; }

        /// <summary>
        /// <para>
        /// Gets the path to the '.sfpkg' application package from where the application package can be downloaded using HTTP or HTTPS protocols.
        /// </para>
        /// </summary>
        /// <value>
        /// The path to the '.sfpkg' application package from where the application package can be downloaded using HTTP or HTTPS protocols.
        /// </value>
        public Uri ApplicationPackageDownloadUri { get; private set; }

        /// <summary>
        /// Gets a string representation of the provision application type operation.
        /// </summary>
        /// <returns>A string representation of the provision application type operation.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "DownloadUri: {0}, ApplicationTypeName: {1}, ApplicationTypeVersion: {2}, async: {3}",
                this.ApplicationPackageDownloadUri,
                this.ApplicationTypeName,
                this.ApplicationTypeVersion,
                this.Async);
        }

        internal static void CheckIfValidApplicationPackageDownloadUri(Uri applicationPackageDownloadUri)
        {
            Requires.Argument<Uri>("applicationPackageDownloadUri", applicationPackageDownloadUri).NotNullOrEmpty();

            var scheme = applicationPackageDownloadUri.Scheme.ToLower();
            if (scheme != "http" && scheme != "https")
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidApplicationPackageDownloadUri_Protocol, applicationPackageDownloadUri, scheme));
            }

            var segments = applicationPackageDownloadUri.Segments;
            if (segments.Length == 0)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidApplicationPackageDownloadUri, applicationPackageDownloadUri));
            }

            var fileName = segments[segments.Length - 1];
            if (!fileName.EndsWith(ExternalStoreProvisionApplicationTypeDescription.SfpkgExtension))
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidApplicationPackageDownloadUri_FileName, applicationPackageDownloadUri, fileName));
            }
        }

        internal override IntPtr ToNativeValue(PinCollection pin)
        {          
            var desc = new NativeTypes.FABRIC_EXTERNAL_STORE_PROVISION_APPLICATION_TYPE_DESCRIPTION();

            desc.Async = NativeTypes.ToBOOLEAN(this.Async);
            desc.ApplicationPackageDownloadUri = pin.AddObject(this.ApplicationPackageDownloadUri);
            desc.ApplicationTypeName = pin.AddObject(this.ApplicationTypeName);
            desc.ApplicationTypeVersion = pin.AddObject(this.ApplicationTypeVersion);
            desc.Reserved = IntPtr.Zero;

            return pin.AddBlittable(desc);
        }
    }
}
