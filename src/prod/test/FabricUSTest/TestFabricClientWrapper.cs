// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricUS.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.UpgradeService;
    using System.Fabric.WRP.Model;
    using System.Threading;
    using System.Threading.Tasks;

    class TestFabricClientWrapper : IFabricClientWrapper
    {

        public ServiceList GetServicesResult { get; set; }

        public Task ClusterUpgradeAsync(ClusterUpgradeCommandParameter commandParameter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task CopyClusterPackageAsync(string imageStoreConnectionString, string clusterManifestPath, string clusterManifestPathInImageStore, string codePackagePath, string codePackagePathInImageStore, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task DisableNodesAsync(List<PaasNodeStatusInfo> nodesToDisable, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task EnableNodesAsync(List<PaasNodeStatusInfo> nodesToEnable, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName, TimeSpan timeout, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<ClusterHealth> GetClusterHealthAsync(TimeSpan timeout, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<string> GetCurrentClusterManifestFile()
        {
            throw new NotImplementedException();
        }

        public Task<Tuple<string, string>> GetCurrentCodeAndConfigVersionAsync(string targetConfigVersion)
        {
            throw new NotImplementedException();
        }

        public Task<FabricUpgradeProgress> GetCurrentRunningUpgradeTaskAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<NodeList> GetNodeListAsync(TimeSpan timeout, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ServiceList> GetServicesAsync(Uri applicationName, TimeSpan timeout, CancellationToken token)
        {
            return Task.FromResult(this.GetServicesResult);
        }

        public Task<Dictionary<string, ServiceRuntimeDescription>> GetSystemServiceRuntimeDescriptionsAsync(TimeSpan timeout, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task ProvisionFabricAsync(string codePathInImageStore, string configPathInImageStore, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task RemoveClusterPackageAsync(string imageStoreConnectionString, string clusterManifestPathInImageStore, string codePackagePathInImageStore, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task UpdateServiceDescriptionAsync(Uri serviceName, ServiceRuntimeDescription replicaSetSize, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task UpdateSystemServicesDescriptionAsync(Dictionary<string, ServiceRuntimeDescription> systemServicesSize, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task UpgradeFabricAsync(CommandProcessorClusterUpgradeDescription commandDescription, string targetCodeVersion, string targetConfigVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }
    }
}