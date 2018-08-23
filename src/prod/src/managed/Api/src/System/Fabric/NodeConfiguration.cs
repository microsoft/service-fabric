// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Common;
    using System.IO;
    using System.Fabric.Interop;
    using System.Linq;
    using System.Xml.Serialization;

    /// <summary>
    /// <para>
    /// This class contains APIs for managing the configuration of the node.
    /// </para>
    /// </summary>
#if !DotNetCoreClr
    [CLSCompliant(false)]
#endif
    public static class NodeConfiguration
    {
        #region API
        /// <summary>
        /// <para>
        /// Gets the cluster manifest configured on that node.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>Gets the cluster manifest used to configure the node. Returns null if the node has never been configured.</para>
        /// </returns>
        public static string GetNodeConfiguration()
        {
            return GetNodeConfiguration(null);
        }

        internal static string GetNodeConfiguration(string machineName)
        {
            string dataRoot = FabricEnvironment.GetDataRoot(machineName);
            if (!Directory.Exists(dataRoot))
            {
                return null;
            }

            dataRoot = machineName == null || Helpers.IsLocalIpAddress(machineName)
                ? dataRoot
                : Helpers.GetRemotePath(dataRoot, machineName);
            if (!Directory.Exists(dataRoot))
            {
                return null;
            }

            var stringwriter = new System.IO.StringWriter();
            var serializer = new XmlSerializer(typeof(ClusterManifestType));
            if (File.Exists(Path.Combine(dataRoot, Constants.TargetInformationFileName)))
            {
                var targetInformation = Utility.ReadXml<TargetInformationType>(Path.Combine(dataRoot, Constants.TargetInformationFileName), SchemaLocation.GetWindowsFabricSchemaLocation());
                if (!string.IsNullOrEmpty(targetInformation.TargetInstallation.ClusterManifestLocation))
                {
                    var clusterManifestLocation = machineName == null ?
                        targetInformation.TargetInstallation.ClusterManifestLocation
                        : Helpers.GetRemotePath(targetInformation.TargetInstallation.ClusterManifestLocation, machineName);
                    serializer.Serialize(stringwriter, Utility.ReadXml<ClusterManifestType>(clusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation()));
                    return stringwriter.ToString();
                }
            }

            string clusterManifestFile = Helpers.GetCurrentClusterManifestPath(dataRoot);
            if (clusterManifestFile != null)
            {
                serializer.Serialize(stringwriter, Utility.ReadXml<ClusterManifestType>(clusterManifestFile, SchemaLocation.GetWindowsFabricSchemaLocation()));
                return stringwriter.ToString();
            }

            return null;
        }
        #endregion
    }
}