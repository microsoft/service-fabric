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
    public class DeployedApplicationEntity : BaseEntity
    {
        public DeployedApplicationEntity(DeployedApplication deployedApplication)
        {
            Assert.IsNotNull(deployedApplication);
            this.ApplicationName = deployedApplication.ApplicationName;
            this.ApplicationTypeName = deployedApplication.ApplicationTypeName;
        }

        internal DeployedApplicationEntity()
        {
        }

        /// <summary>
        /// App Name
        /// </summary>
        [DataMember(IsRequired = true)]
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// Application Type Name
        /// </summary>
        [DataMember(IsRequired = true)]
        public string ApplicationTypeName { get; internal set; }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            Assert.IsNotNull(other);
            var otherObj = other as DeployedApplicationEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.ApplicationName == otherObj.ApplicationName && this.ApplicationTypeName == otherObj.ApplicationTypeName;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.ApplicationName.GetHashCode();
                hash = (hash * 23) + this.ApplicationTypeName.GetHashCode();
                return hash;
            }
        }

        /// <inheritdoc />
        public override int GetUniqueIdentity()
        {
            return this.GetHashCode();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "AppName: '{0}', AppTypeName: '{1}'", this.ApplicationName, this.ApplicationTypeName);
        }
    }
}