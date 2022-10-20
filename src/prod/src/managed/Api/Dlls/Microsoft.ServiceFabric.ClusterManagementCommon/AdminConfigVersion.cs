// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using Newtonsoft.Json;

    public class AdminConfigVersion
    {
        [JsonConstructor]
        public AdminConfigVersion(string msiVersion, string clusterSettingsVersion)
        {
            msiVersion.MustNotBeNull("msiVersion");
            clusterSettingsVersion.MustNotBeNull("clusterSettingsVersion");

            this.MsiVersion = msiVersion;
            this.ClusterSettingsVersion = clusterSettingsVersion;
        }

        public AdminConfigVersion(AdminConfigVersion clusterWrpConfigVersion)
        {
            clusterWrpConfigVersion.MustNotBeNull("clusterWrpConfigVersion");

            this.MsiVersion = clusterWrpConfigVersion.MsiVersion;
            this.ClusterSettingsVersion = clusterWrpConfigVersion.ClusterSettingsVersion;
        }

        public string MsiVersion { get; set; }

        public string ClusterSettingsVersion { get; set; }

        public override bool Equals(object other)
        {
            var otherVersion = other as AdminConfigVersion;
            if (otherVersion == null)
            {
                return false;
            }

            return this.MsiVersion.Equals(otherVersion.MsiVersion) &&
                   this.ClusterSettingsVersion.Equals(otherVersion.ClusterSettingsVersion);
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.MsiVersion.ToUpperInvariant().GetHashCode();
            hash = (13 * hash) + this.ClusterSettingsVersion.ToUpperInvariant().GetHashCode();
            return hash;
        }

        public override string ToString()
        {
            return string.Format("{0}:{1}", this.MsiVersion, this.ClusterSettingsVersion);
        }
    }
}