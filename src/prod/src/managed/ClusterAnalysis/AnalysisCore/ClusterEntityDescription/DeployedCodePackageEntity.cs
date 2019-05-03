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
    public class DeployedCodePackageEntity : BaseEntity
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

        public DeployedCodePackageEntity()
        {
        }

        public DeployedCodePackageEntity(DeployedCodePackage dcp, string nodeName, ApplicationEntity applicationEntity)
        {
            Assert.IsNotNull(dcp);
            Assert.IsNotEmptyOrNull(nodeName, "Node Name can't be null/empty");
            Assert.IsNotNull(applicationEntity);
            this.CodePackageName = dcp.CodePackageName;
            this.CodePackageVersion = dcp.CodePackageVersion;
            this.ServiceManifestName = dcp.ServiceManifestName;
            this.NodeName = nodeName;
            this.EntryPointLocation = dcp.EntryPoint.EntryPointLocation;
            this.SetupPointLocation = dcp.SetupEntryPoint != null ? dcp.SetupEntryPoint.EntryPointLocation : null;
            this.Application = applicationEntity;
        }

        [DataMember(IsRequired = true)]
        public ApplicationEntity Application { get; internal set; }

        [DataMember]
        public string CodePackageName { get; internal set; }

        [DataMember]
        public string CodePackageVersion { get; internal set; }

        [DataMember]
        public string ServiceManifestName { get; internal set; }

        [DataMember]
        public string NodeName { get; internal set; }

        [DataMember]
        public string EntryPointLocation { get; internal set; }

        [DataMember]
        public string SetupPointLocation { get; internal set; }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            Assert.IsNotNull(other);
            var otherObj = other as DeployedCodePackageEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.Application.ApplicationName == otherObj.Application.ApplicationName && this.CodePackageName == otherObj.CodePackageName &&
                   this.CodePackageVersion == otherObj.CodePackageVersion && this.NodeName == otherObj.NodeName &&
                   this.EntryPointLocation == otherObj.EntryPointLocation;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "AppName: '{0}', CPName: '{1}', CPVersion: '{2}', ServiceManifestName: '{3}', NodeName: '{4}', EntryPointLocation: '{5}'",
                this.Application.ApplicationName,
                this.CodePackageName,
                this.CodePackageVersion,
                this.ServiceManifestName,
                this.NodeName,
                this.EntryPointLocation);
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
                hash = (hash * 23) + this.Application.ApplicationName.GetHashCode();
                hash = (hash * 23) + this.ServiceManifestName.GetHashCode();
                hash = (hash * 23) + this.CodePackageName.GetHashCode();
                hash = (hash * 23) + this.CodePackageVersion.GetHashCode();
                hash = (hash * 23) + this.EntryPointLocation.GetHashCode();
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