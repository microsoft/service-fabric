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

    [DataContract]
    public class ServiceEntity : BaseEntity
    {
        public ServiceEntity(Service application)
        {
            this.ServiceName = application.ServiceName;
            this.ServiceTypeName = application.ServiceTypeName;
            this.ServiceManifestVersion = application.ServiceManifestVersion;
        }

        internal ServiceEntity()
        {
        }

        /// <summary>
        /// Service Name
        /// </summary>
        [DataMember(IsRequired = true)]
        public Uri ServiceName { get; internal set; }

        /// <summary>
        /// Service type name
        /// </summary>
        [DataMember(IsRequired = true)]
        public string ServiceTypeName { get; internal set; }

        /// <summary>
        /// Service type version
        /// </summary>
        [DataMember(IsRequired = true)]
        public string ServiceManifestVersion { get; internal set; }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            var otherObj = other as ServiceEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.ServiceTypeName == otherObj.ServiceTypeName && this.ServiceName == otherObj.ServiceName &&
                   this.ServiceManifestVersion == otherObj.ServiceManifestVersion;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "ServiceName: {0}, Type: {1}, ManifestVersion: {2}",
                this.ServiceName,
                this.ServiceTypeName,
                this.ServiceManifestVersion);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.ServiceName.GetHashCode();
                hash = (hash * 23) + this.ServiceTypeName.GetHashCode();
                hash = (hash * 23) + this.ServiceManifestVersion.GetHashCode();
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