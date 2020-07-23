// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Xml;
    using System.Xml.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    /// The "output" to be consumed by the VMs which describes how the cluster
    /// should be configured and the binary version.
    /// </summary>
    public class ClusterExternalState
    {
        [JsonConstructor]
        public ClusterExternalState(ClusterManifestType clusterManifest, string msiVersion)
        {
            clusterManifest.MustNotBeNull("clusterManifest");
            msiVersion.MustNotBeNull("msiVersion");

            this.ClusterManifest = clusterManifest;
            this.ClusterManifestBase64 = GetBase64ClusterManifest(clusterManifest);
            this.MsiVersion = msiVersion;
        }

        public ClusterManifestType ClusterManifest { get; set; }

        [JsonIgnore]
        public string ClusterManifestBase64 { get; private set; }

        public string MsiVersion { get; set; }

        public bool Equals(string clusterManifestVersion, string msiVersion)
        {
            return this.ClusterManifest.Version == clusterManifestVersion && this.MsiVersion == msiVersion;
        }

        public override bool Equals(object obj)
        {
            var other = obj as ClusterExternalState;

            if (other == null || this.MsiVersion != other.MsiVersion ||
                this.ClusterManifest.Version != other.ClusterManifest.Version)
            {
                return false;
            }

            return true;
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.MsiVersion.GetHashCode();
            hash = (13 * hash) + this.ClusterManifest.GetHashCode();
            return hash;
        }

        private static string GetBase64ClusterManifest(ClusterManifestType clusterManifestRoot)
        {
            var manifestSerializer = new XmlSerializer(typeof(ClusterManifestType));

            using (var memoryStream = new MemoryStream())
            {
                using (var writer = XmlWriter.Create(memoryStream))
                {
                    manifestSerializer.Serialize(writer, clusterManifestRoot);
                }

                return Convert.ToBase64String(memoryStream.GetBuffer(), 0, (int)memoryStream.Length);
            }
        }
    }
}