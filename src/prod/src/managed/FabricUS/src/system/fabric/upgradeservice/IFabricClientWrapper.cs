// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Collections.Generic;
    using Health;
    using Query;
    using Threading;
    using Threading.Tasks;
    using WRP.Model;

    internal interface IFabricClientWrapper
    {
        Task<ClusterHealth> GetClusterHealthAsync(TimeSpan timeout, CancellationToken token);

        Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName, TimeSpan timeout,
            CancellationToken token);

        Task<ServiceList> GetServicesAsync(Uri applicationName, TimeSpan timeout, CancellationToken token);

        Task<NodeList> GetNodeListAsync(TimeSpan timeout, CancellationToken token);

        Task<Dictionary<string, ServiceRuntimeDescription>> GetSystemServiceRuntimeDescriptionsAsync(TimeSpan timeout, CancellationToken token);

        Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<FabricUpgradeProgress> GetCurrentRunningUpgradeTaskAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<Tuple<string, string>> GetCurrentCodeAndConfigVersionAsync(string targetConfigVersion);

        Task<string> GetCurrentClusterManifestFile();

        Task UpdateSystemServicesDescriptionAsync(
            Dictionary<string, ServiceRuntimeDescription> systemServiceDescritpions,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task UpdateServiceDescriptionAsync(Uri serviceName, ServiceRuntimeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task DisableNodesAsync(List<PaasNodeStatusInfo> nodesToDisable, TimeSpan timeout, CancellationToken cancellationToken);

        Task EnableNodesAsync(List<PaasNodeStatusInfo> nodesToEnable, TimeSpan timeout, CancellationToken cancellationToken);

        Task ClusterUpgradeAsync(ClusterUpgradeCommandParameter commandParameter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken);        

        Task CopyClusterPackageAsync(
                string imageStoreConnectionString,
                string clusterManifestPath,
                string clusterManifestPathInImageStore,
                string codePackagePath,
                string codePackagePathInImageStore,
                TimeSpan timeout,
                CancellationToken cancellationToken);

        Task RemoveClusterPackageAsync(
                string imageStoreConnectionString,
                string clusterManifestPathInImageStore,
                string codePackagePathInImageStore,
                TimeSpan timeout,
                CancellationToken cancellationToken);

        Task ProvisionFabricAsync(string codePathInImageStore, string configPathInImageStore, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpgradeFabricAsync(
           CommandProcessorClusterUpgradeDescription commandDescription,
           string targetCodeVersion,
           string targetConfigVersion,
           TimeSpan timeout,
           CancellationToken cancellationToken);
    }
}