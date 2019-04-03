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
    public class ApplicationEntity : BaseEntity
    {
        /// <summary>
        /// System Application Name for system services
        /// </summary>
        public static readonly Uri SystemServiceApplicationName = new Uri("fabric:/System");

        /// <summary>
        /// System Application Type Name for system services
        /// </summary>
        public static readonly string SystemServiceApplicationTypeName = "System Service Application TypeName";

        /// <summary>
        /// System App Name for processes which are part of Service fabric but are not deployed as App (e.g. FabricDCA.exe)
        /// </summary>
        public static readonly Uri DefaultAppForSystemProcesses = new Uri("fabric:/Default App For SystemProcesses");

        /// <summary>
        /// System Type App Name
        /// </summary>
        public static readonly string DefaultAppTypeNameForSystemProcesses = "Default App TypeName For SystemProcesses";

        /// <summary>
        /// Place holder application Name
        /// </summary>
        public static readonly Uri PlaceholderApplicationName = new Uri("fabric:/Placeholder Application");

        /// <summary>
        /// Place holder application type name.
        /// </summary>
        public static readonly string PlaceholderApplicationTypeName = "Placeholder Application Type";

        public ApplicationEntity(Application application)
        {
            Assert.IsNotNull(application, "application object can't be null");
            this.ApplicationName = application.ApplicationName;
            this.ApplicationTypeName = application.ApplicationTypeName;
            this.ApplicationTypeVersion = application.ApplicationTypeVersion;
        }

        internal ApplicationEntity()
        {
        }

        /// <summary>
        /// App Name
        /// </summary>
        [DataMember(IsRequired = true)]
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// App type name
        /// </summary>
        [DataMember(IsRequired = true)]
        public string ApplicationTypeName { get; internal set; }

        /// <summary>
        /// App type version
        /// </summary>
        [DataMember(IsRequired = true)]
        public string ApplicationTypeVersion { get; internal set; }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            var otherObj = other as ApplicationEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.ApplicationTypeName == otherObj.ApplicationTypeName && this.ApplicationName == otherObj.ApplicationName &&
                   this.ApplicationTypeVersion == otherObj.ApplicationTypeVersion;
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
                hash = (hash * 23) + this.ApplicationTypeName.GetHashCode();
                hash = (hash * 23) + this.ApplicationTypeVersion.GetHashCode();
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
            return string.Format(
                CultureInfo.InvariantCulture,
                "AppName: {0}, Type: {1}, Version: {2}",
                this.ApplicationName,
                this.ApplicationTypeName,
                this.ApplicationTypeVersion);
        }
    }
}