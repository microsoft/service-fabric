// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Text;

    /// <summary>
    /// <para>Information about the name and version of service/application with which a image store file/folder is associated.</para>
    /// </summary>
    internal class ImageStoreVersionInfo
    {
        private List<string> applicationVersions;
        private List<string> serviceManifestVersions;

        /// <summary>
        /// <para>Gets the name of service manifest..</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest name.</para>
        /// </value>
        public string ServiceManifestName
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Get the version of service manifest.</para>
        /// </summary>
        public string ServiceManifestVersion
        {
            get
            { 
                return this.serviceManifestVersions != null && this.applicationVersions.Count > 0
                ? string.Join("|", this.serviceManifestVersions.ToArray())
                : string.Empty;
            }
        }

        /// <summary>
        /// <para>Get the version of the applicaiton.</para>
        /// </summary>
        public string ApplicationVersion
        {
            get
            {
                return this.applicationVersions != null && this.applicationVersions.Count > 0
                ? string.Join("|", this.applicationVersions.ToArray())
                : string.Empty;
            }
        }

        /// <summary>
        /// Add the version of application if it doesn't exist.
        /// </summary>
        /// <param name="applicationVersion">The version of the application</param>
        private void ConfigApplicationVersion(string applicationVersion)
        {
            if (!string.IsNullOrEmpty(applicationVersion))
            {
                if (this.applicationVersions == null)
                {
                    this.applicationVersions = new List<string>() { applicationVersion };
                }
                else if (!this.applicationVersions.Exists(version => string.Compare(version, applicationVersion, StringComparison.OrdinalIgnoreCase) == 0))
                {
                    this.applicationVersions.Add(applicationVersion);
                }
            }
        }

        /// <summary>
        /// <para>Setup the version/name of service and application which is associated with a image store file/folder.</para>
        /// </summary>
        /// <param name="serviceManifestName">The name of service manifest</param>
        /// <param name="serviceManifestVerison">The version of service manifest</param>
        /// <param name="applicationVersion">The version of the applicaiton</param>
        public void ConfigVersionInfo(string serviceManifestName, string serviceManifestVerison, string applicationVersion)
        {
            if (!string.IsNullOrEmpty(serviceManifestName) && !string.IsNullOrEmpty(serviceManifestVerison))
            {
                if (string.IsNullOrEmpty(this.ServiceManifestName))
                {
                    this.ServiceManifestName = serviceManifestName;
                }
                else if (string.Compare(this.ServiceManifestName, serviceManifestName, StringComparison.OrdinalIgnoreCase) != 0)
                {
                    throw new InvalidOperationException("Service manifest name mismatch");
                }

                if (this.serviceManifestVersions == null)
                {
                    this.serviceManifestVersions = new List<string>() { serviceManifestVerison };
                }
                else if (!this.serviceManifestVersions.Exists(version => string.Compare(version, serviceManifestVerison, StringComparison.OrdinalIgnoreCase) == 0))
                {
                    this.serviceManifestVersions.Add(serviceManifestVerison);
                }
            }

            this.ConfigApplicationVersion(applicationVersion);
        }

    }
}