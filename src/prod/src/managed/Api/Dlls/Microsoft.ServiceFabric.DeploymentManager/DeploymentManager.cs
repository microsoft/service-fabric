// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.ServiceFabric.DeploymentManager.BPA;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    /// <summary>
    /// Contains methods to create and remove Service Fabric clusters
    /// </summary>
    public static class DeploymentManager
    {
        /// <summary>
        /// Gets all available Runtime versions from the Goal State File.
        /// </summary>
        /// <param name="DownloadableRuntimeVersionsReturnSet">Specifies the kind of Runtime version to be returned - All, Supported or Latest</param>
        public static List<RuntimePackageDetails> GetDownloadableRuntimeVersions(
            DownloadableRuntimeVersionReturnSet returnSet = DownloadableRuntimeVersionReturnSet.Supported)
        {            
            return DeploymentManagerInternal.GetDownloadableRuntimeVersions(
                returnSet);
        }

        /// <summary>
        /// Gets all upgrade compatible runtime versions for the provided base version from the goal state file .
        /// </summary>
        /// <param name="baseVersion">Specifies the base version to retrieve all compatible versions.</param>
        public static List<RuntimePackageDetails> GetUpgradeableRuntimeVersions(
            string baseVersion)
        {
            return DeploymentManagerInternal.GetUpgradeableRuntimeVersions(
                baseVersion);
        }

        /// <summary>
        /// Creates a Service Fabric cluster based on an input JSON configuration file and Service Fabric runtime package CAB file.
        /// </summary>
        /// <param name="clusterConfigPath">Specifies the path to the JSON cluster configuration file where you define the cluster nodes & configuration settings. Templates are provided in the Standalone deployment package.</param>
        /// <param name="fabricRuntimePackagePath">Specifies the path to Service Fabric runtime package CAB file.</param>
        /// <param name="creationOptions">Options for cluster creation.</param>
        /// <param name="maxPercentFailedNodes">Maximum percentage of nodes allowed to fail during cluster creation. If more than this percentage of nodes fail, the cluster creation will fail and roll back. Default value is 0 percent.</param>
        /// <param name="timeout">Timeout for cluster creation. Default value null means there is no timeout for the creation process.</param>
        public static async Task CreateClusterAsync(
            string clusterConfigPath,
            string fabricRuntimePackagePath,
            ClusterCreationOptions creationOptions = ClusterCreationOptions.None,
            int maxPercentFailedNodes = 0,
            TimeSpan? timeout = null)
        {
            await DeploymentManagerInternal.CreateClusterAsyncInternal(
                clusterConfigPath,
                null,
                null,
                true,
                FabricPackageType.XCopyPackage,
                fabricRuntimePackagePath,
                null,
                creationOptions,
                maxPercentFailedNodes,
                timeout).ConfigureAwait(false);
        }

        /// <summary>
        /// Removes a Service Fabric cluster based on input JSON cluster configuration file.
        /// </summary>
        /// <param name="clusterConfigPath">Specifies the path to the JSON cluster configuration file where you define the cluster nodes & configuration settings. Templates are provided in the Standalone deployment package.</param>
        /// <param name="deleteLog">Determines whether operation cleans up Fabric logs as part of removal.</param>
        public static async Task RemoveClusterAsync(string clusterConfigPath, bool deleteLog)
        {
            await DeploymentManagerInternal.RemoveClusterAsyncInternal(clusterConfigPath, deleteLog, true, FabricPackageType.XCopyPackage)
            .ConfigureAwait(false);
        }

        /// <summary>
        /// This method adds a new node to an existing multi-machine cluster, to be based on the current machine.
        /// The FabricClient is used to connect to the cluster in order to configure the new node.
        /// </summary>
        /// <param name="nodeName">Name of the node that will be added. NodeName must be unique.</param>
        /// <param name="nodeType">Type of the node that will be added. NodeType must already exist in the configuration.</param>
        /// <param name="ipAddressOrFqdn">IP address or fully-qualified domain name of the node being added. This FQDN must be reachable from every other node.</param>
        /// <param name="upgradeDomain">The new node's upgrade domain.</param>
        /// <param name="faultDomain">The new node's fault domain.</param>
        /// <param name="fabricRuntimePackagePath">Specifies the path to Service Fabric runtime package CAB file.</param>
        /// <param name="fabricClient">Required to connect to the cluster.</param>
        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for ipAddressOrFQDN.")]
        public static async Task AddNodeAsync(
            string nodeName,
            string nodeType,
            string ipAddressOrFqdn,
            string upgradeDomain,
            string faultDomain,
            string fabricRuntimePackagePath,
            FabricClient fabricClient)
        {
            await DeploymentManagerInternal.AddNodeAsyncInternal(
                nodeName,
                nodeType,
                ipAddressOrFqdn,
                upgradeDomain,
                faultDomain,
                fabricRuntimePackagePath,
                fabricClient).ConfigureAwait(false);
        }

        /// <summary>
        /// This method safely removes the current machine's node from an existing cluster.
        /// The FabricClient is used to communicate DeactivateNode to the cluster, before removing Service Fabric from the node.
        /// </summary>
        /// <param name="fabricClient">Required to connect to the cluster.</param>
        /// <returns></returns>
        [Obsolete("This API is deprecated. Use Start-ServiceFabricClusterConfigurationUpgrade to remove nodes from the cluster.", false)]
        public static async Task RemoveNodeAsync(FabricClient fabricClient)
        {
            await DeploymentManagerInternal.RemoveNodeAsyncInternal(fabricClient).ConfigureAwait(false);
        }       
    }
}