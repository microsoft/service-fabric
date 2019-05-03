// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common.Util;

    [DataContract]
    public class DeployedServicePackageEntity : BaseEntity
    {
        /// <summary>
        /// System Code Package Name
        /// </summary>
        public static readonly string SystemCodePackageName = "SystemCodePackageName";

        /// <summary>
        /// System Service Manifest Name
        /// </summary>
        public static readonly string SystemServiceManifestName = "SystemServiceManifestName";

        /// <summary>
        /// Placeholder Code Package Name
        /// </summary>
        public static readonly string PlaceholderCodePackageName = "PlaceholderCodePackageName";

        /// <summary>
        /// Placeholder Service Manifest Name
        /// </summary>
        public static readonly string PlaceholderServiceManifestName = "PlaceholderServiceManifestName";

        public DeployedServicePackageEntity(DeployedServicePackage dcp, string nodeName, Uri applicationName)
        {
            Assert.IsNotNull(dcp);
            Assert.IsNotEmptyOrNull(nodeName, "Node Name can't be null/empty");
            Assert.IsNotNull(applicationName);
            this.ServiceManifestName = dcp.ServiceManifestName;
            this.NodeName = nodeName;
            this.ServiceManifestVersion = dcp.ServiceManifestVersion;
            this.ApplicationName = applicationName;
        }

        [DataMember]
        public Uri ApplicationName { get; internal set; }

        [DataMember]
        public string ServiceManifestName { get; internal set; }

        [DataMember]
        public string ServiceManifestVersion { get; internal set; }

        [DataMember]
        public string NodeName { get; internal set; }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            Assert.IsNotNull(other);
            var otherObj = other as DeployedServicePackageEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.ApplicationName == otherObj.ApplicationName && this.ServiceManifestVersion == otherObj.ServiceManifestVersion &&
                   this.NodeName == otherObj.NodeName;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "AppName: '{0}', ServiceManifestName: '{1}', NodeName: '{2}', ServiceManifestVersion: '{3}'",
                this.ApplicationName,
                this.ServiceManifestName,
                this.NodeName,
                this.ServiceManifestVersion);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return this.Equals(obj as BaseEntity);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.ApplicationName.GetHashCode();
                hash = (hash * 23) + this.ServiceManifestName.GetHashCode();
                hash = (hash * 23) + this.ServiceManifestVersion.GetHashCode();
                hash = (hash * 23) + this.NodeName.GetHashCode();
                return hash;
            }
        }

        /// <inheritdoc />
        public override int GetUniqueIdentity()
        {
            return this.GetHashCode();
        }
    }
}