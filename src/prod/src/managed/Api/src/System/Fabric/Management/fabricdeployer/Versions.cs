// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Globalization;

    internal class Versions
    {
        #region Public properties
        public const string ConfigVersionPattern = "{0}.{1}";

        public const string InstanceVersionPattern = "{0}:{1}:{2}";

        public string ClusterManifestVersion { get; private set; }

        public string InstanceId { get; private set; }

        public string FabricInstanceVersion { get; private set; }

        public string ConfigVersion { get; private set; }
        #endregion

        #region Constructors
        public Versions(
            string clusterManifestVersion,
            string instanceId,
            string targetVersion,
            string currentVersion)
        {
            string codeVersion = currentVersion;

            // targetVersion indicates the intended version of the fabric binaries after a Fabric Upgrade 
            // If the targetVersion is equal to the current code version, all is good.
            // Otherwise, we set the instance id to 0. This will trigger another fabric upgrade once the node is UP
            // This model is followed in the hope that we will eventually reach the target version via a series of fabric upgrade operations
            if (targetVersion.StartsWith(currentVersion, StringComparison.OrdinalIgnoreCase))
            {
                FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Using input instance ID as the code version matched the target version");
                this.InstanceId = instanceId;
            }
            else
            {
                FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Using default instance ID as the code version {0} does not match the target version {1}", currentVersion, targetVersion);
                this.InstanceId = FabricValidatorConstants.FabricUpgradeDefaultInstanceId;
            }
            
            this.ClusterManifestVersion = clusterManifestVersion;
            this.FabricInstanceVersion = string.Format(
                CultureInfo.InvariantCulture,
                InstanceVersionPattern,
                codeVersion,
                ClusterManifestVersion,
                InstanceId);
            this.ConfigVersion = string.Format(
                CultureInfo.InvariantCulture,
                ConfigVersionPattern,
                ClusterManifestVersion,
                InstanceId);
        }
        #endregion

    }
}