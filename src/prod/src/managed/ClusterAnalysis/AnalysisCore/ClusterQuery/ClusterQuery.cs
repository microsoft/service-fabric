// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Cluster.ClusterQuery
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.Helpers;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Runtime;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Default implementation of cluster query
    /// </summary>
    public class ClusterQuery : IClusterQuery
    {
        private const string ExeExtension = ".exe";

        private static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);

        private FabricClient fabricClient;

        private ILogger logger;

        private HealthQueryHelper healthQuery;

        private ILogProvider logProvider;

        private ClusterQuery(IInsightRuntime runtime)
        {
            this.fabricClient = runtime.GetService(typeof(FabricClient)) as FabricClient;
            Assert.IsNotNull(this.fabricClient, "Runtime needs to supply non-null Fabric client");
            this.logger = runtime.GetLogProvider().CreateLoggerInstance("ClusterQuery.log");
            this.logProvider = runtime.GetLogProvider();
        }

        /// <summary>
        /// Get singleton instance of Cluster Query
        /// </summary>
        /// <param name="runtime"></param>
        /// <returns></returns>
        public static IClusterQuery CreateClusterQueryInstance(IInsightRuntime runtime)
        {
            Assert.IsNotNull(runtime, "Runtime can't be null");

            return new ClusterQuery(runtime);
        }

        /// <inheritdoc />
        public async Task<NodeEntity> GetNodeAsync(string nodeName, CancellationToken token)
        {
            if (string.IsNullOrEmpty(nodeName))
            {
                throw new ArgumentException("Value cannot be null or empty.", nameof(nodeName));
            }

            return new NodeEntity(
                (await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetNodeListAsync(nodeName, DefaultTimeout, token), token)
                    .ConfigureAwait(false)).Single());
        }

        /// <inheritdoc />
        public async Task<IEnumerable<NodeEntity>> GetNodeListAsync(CancellationToken token)
        {
            return (await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetNodeListAsync(), token)
                .ConfigureAwait(false)).Select(node => new NodeEntity(node));
        }

        /// <inheritdoc />
        public async Task<IEnumerable<ReplicaEntity>> GetReplicasInPartitionAsync(Guid partitionIdGuid, CancellationToken token)
        {
            // The cluster api doesn't provide overload with just timeout and token... so skipping passing token for now.
            return (await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetReplicaListAsync(partitionIdGuid), token).ConfigureAwait(false))
                .Select(replica => new ReplicaEntity(replica, partitionIdGuid));
        }

        /// <exception cref="Exception"></exception>
        /// <inheritdoc />
        public async Task<IEnumerable<string>> GetPartitionDeployedExecutableNamesAsync(
            string nodeName,
            Uri appName,
            Guid partitionGuid,
            CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(nodeName);
            Assert.IsNotNull(appName);

            // Again the cluster API doesn't provide an easy overload to pass just timeout and token... really a pain.
            var allReplicaList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.QueryManager.GetDeployedReplicaListAsync(nodeName, appName, partitionGuid),
                token).ConfigureAwait(false);

            if (allReplicaList.Count() != 1)
            {
                throw new Exception(
                    string.Format(
                        "With Partition ID '{0}', node Name '{1}', appName '{2}', Expected 1 Replica, But seeing '{3}'",
                        partitionGuid,
                        nodeName,
                        appName,
                        allReplicaList.Count()));
            }

            var allDeployedCodePkg = await FabricClientRetryHelper
                .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetDeployedCodePackageListAsync(nodeName, appName), token)
                .ConfigureAwait(false);

            var procNameList = new List<string>();

            foreach (var deployedPkg in allDeployedCodePkg)
            {
                procNameList.Add(Path.GetFileName(deployedPkg.EntryPoint.EntryPointLocation));
                if (deployedPkg.SetupEntryPoint == null || string.IsNullOrEmpty(deployedPkg.SetupEntryPoint.EntryPointLocation))
                {
                    continue;
                }

                var fileName = Path.GetFileName(deployedPkg.SetupEntryPoint.EntryPointLocation);
                if (ExeExtension.Equals(Path.GetExtension(fileName), StringComparison.InvariantCultureIgnoreCase))
                {
                    procNameList.Add(fileName);
                }
            }

            return procNameList;
        }

        /// <inheritdoc />
        public async Task<ServiceEntity> GetServiceAsync(Uri applicationName, Uri serviceName, CancellationToken token)
        {
            Assert.IsNotNull(applicationName);
            Assert.IsNotNull(serviceName);
            var service = new ServiceEntity(
                (await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetServiceListAsync(applicationName, serviceName), token)
                    .ConfigureAwait(false)).Single());

            return service;
        }

        /// <inheritdoc />
        public async Task<PartitionEntity> GetPartitionAsync(Guid partitionGuid, CancellationToken token)
        {
            var partition = new PartitionEntity(
                (await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetPartitionAsync(partitionGuid), token)
                    .ConfigureAwait(false)).Single());

            if (partition.PartitionStatus == ServicePartitionStatus.Deleting)
            {
                return partition;
            }

            partition.Application = await this.GetPartitionApplicationAsync(partitionGuid, token).ConfigureAwait(false);
            partition.Service = await this.GetPartitionServiceAsync(partitionGuid, token).ConfigureAwait(false);
            return partition;
        }

        /// <inheritdoc />
        public async Task<ReplicaEntity> GetReplicaAsync(Guid partitionIdGuid, long replicaId, CancellationToken token)
        {
            // The cluster api doesn't provide overload with just timeout and token... so skipping passing token for now.
            var allReplicas = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetReplicaListAsync(partitionIdGuid, replicaId), token).ConfigureAwait(false);

            var replica = allReplicas.SingleOrDefault(item => item.Id != replicaId);
            if (replica == null)
            {
                throw new FabricElementNotFoundException();
            }

            return new ReplicaEntity(replica, partitionIdGuid);
        }

        /// <inheritdoc />
        public async Task<IEnumerable<Guid>> GetAllPartitionIdsAsync(CancellationToken token)
        {
            // No easy way to pass token easily so not using token.
            var partitionIds = new List<Guid>();
            var applicationList = await FabricClientRetryHelper
                .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetApplicationListAsync(), token).ConfigureAwait(false);

            foreach (var application in applicationList)
            {
                var servicesInAppList = await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetServiceListAsync(application.ApplicationName), token)
                    .ConfigureAwait(false);

                foreach (var service in servicesInAppList)
                {
                    var partitionInServiceList = await FabricClientRetryHelper
                        .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetPartitionListAsync(service.ServiceName), token)
                        .ConfigureAwait(false);

                    partitionIds.AddRange(partitionInServiceList.Select(partition => partition.PartitionInformation.Id));
                }
            }

            return partitionIds;
        }

        /// <inheritdoc />
        public async Task<ApplicationEntity> GetPartitionApplicationAsync(Guid partitionGuid, CancellationToken token)
        {
            var applicationList = await this.GetApplicationsAsync(token).ConfigureAwait(false);
            foreach (var application in applicationList)
            {
                var servicesInAppList = await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetServiceListAsync(application.ApplicationName), token)
                    .ConfigureAwait(false);

                foreach (var service in servicesInAppList)
                {
                    var partitionInServiceList = await FabricClientRetryHelper
                        .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetPartitionListAsync(service.ServiceName), token)
                        .ConfigureAwait(false);

                    if (partitionInServiceList.Any(partition => partition.PartitionInformation.Id == partitionGuid))
                    {
                        return application;
                    }
                }
            }

            throw new Exception(string.Format(CultureInfo.InvariantCulture, "Couldn't find Application for partition '{0}'", partitionGuid));
        }

        /// <inheritdoc />
        /// <remarks>
        /// TODO: Remove duplication from application name function
        /// </remarks>
        public async Task<ServiceEntity> GetPartitionServiceAsync(Guid partitionGuid, CancellationToken token)
        {
            // TODO Reduce duplicate logic in GetService/GetApp functions.
            var applicationList = await this.GetApplicationsAsync(token).ConfigureAwait(false);

            foreach (var application in applicationList)
            {
                var servicesInAppList = await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetServiceListAsync(application.ApplicationName), token)
                    .ConfigureAwait(false);

                foreach (var service in servicesInAppList)
                {
                    var partitionInServiceList = await FabricClientRetryHelper
                        .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetPartitionListAsync(service.ServiceName), token)
                        .ConfigureAwait(false);

                    if (partitionInServiceList.Any(partition => partition.PartitionInformation.Id == partitionGuid))
                    {
                        return new ServiceEntity(service);
                    }
                }
            }

            throw new Exception(string.Format(CultureInfo.InvariantCulture, "Couldn't find Service Information for partition '{0}'", partitionGuid));
        }

        /// <inheritdoc />
        public async Task<IEnumerable<ApplicationEntity>> GetApplicationsAsync(CancellationToken token)
        {
            var allApps = (await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetApplicationListAsync(), token).ConfigureAwait(false))
                .Select(app => new ApplicationEntity(app)).ToList();

            // This is required since query manager doesn't report System Application. In Type version, we can add Cluster Code Version in future.
            allApps.Add(
                new ApplicationEntity()
                {
                    ApplicationName = new Uri("fabric:/System"),
                    ApplicationTypeName = "SystemServices",
                    ApplicationTypeVersion = "Dummy Version"
                });

            return allApps;
        }

        /// <inheritdoc />
        public async Task<ApplicationEntity> GetApplicationAsync(Uri applicationName, CancellationToken token)
        {
            Assert.IsNotNull(applicationName);

            // TODO: Clean up these multiple Ifs.
            if (applicationName == ApplicationEntity.SystemServiceApplicationName)
            {
                var singleNode = (await this.GetNodeListAsync(token).ConfigureAwait(false)).First();
                return new ApplicationEntity
                {
                    ApplicationName = ApplicationEntity.SystemServiceApplicationName,
                    ApplicationTypeName = ApplicationEntity.SystemServiceApplicationTypeName,
                    ApplicationTypeVersion = singleNode.CodeVersion
                };
            }

            if (applicationName == ApplicationEntity.DefaultAppForSystemProcesses)
            {
                var singleNode = (await this.GetNodeListAsync(token).ConfigureAwait(false)).First();
                return new ApplicationEntity
                {
                    ApplicationName = ApplicationEntity.DefaultAppForSystemProcesses,
                    ApplicationTypeName = ApplicationEntity.DefaultAppTypeNameForSystemProcesses,
                    ApplicationTypeVersion = singleNode.CodeVersion
                };
            }

            if (applicationName == ApplicationEntity.PlaceholderApplicationName)
            {
                var singleNode = (await this.GetNodeListAsync(token).ConfigureAwait(false)).First();
                return new ApplicationEntity
                {
                    ApplicationName = ApplicationEntity.PlaceholderApplicationName,
                    ApplicationTypeName = ApplicationEntity.PlaceholderApplicationTypeName,
                    ApplicationTypeVersion = singleNode.CodeVersion
                };
            }

            var appList = await FabricClientRetryHelper
                .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetApplicationListAsync(applicationName), token).ConfigureAwait(false);

            return new ApplicationEntity(appList.Single());
        }

        /// <inheritdoc />
        public async Task<DeployedApplicationEntity> GetDeployedApplicationsAsync(Uri applicationName, string nodeName, CancellationToken token)
        {
            Assert.IsNotNull(applicationName, "App Name can't be null");
            Assert.IsNotNull(nodeName);
            return (await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.QueryManager.GetDeployedApplicationListAsync(nodeName, applicationName),
                token).ConfigureAwait(false)).Select(deployedApplication => new DeployedApplicationEntity(deployedApplication)).Single();
        }

        /// <inheritdoc />
        public async Task<IEnumerable<DeployedApplicationEntity>> GetDeployedApplicationsAsync(string nodeName, CancellationToken token)
        {
            Assert.IsNotNull(nodeName);
            return (await FabricClientRetryHelper
                    .ExecuteFabricActionWithRetryAsync(() => this.fabricClient.QueryManager.GetDeployedApplicationListAsync(nodeName), token)
                    .ConfigureAwait(false))
                .Select(deployedApplication => new DeployedApplicationEntity(deployedApplication));
        }

        /// <inheritdoc />
        public async Task<IEnumerable<DeployedCodePackageEntity>> GetDeployedCodePackageAsync(string nodeName, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(nodeName, "Node name can't be null/empty");
            var allApplications = await this.GetDeployedApplicationsAsync(nodeName, token).ConfigureAwait(false);

            var cps = new List<DeployedCodePackageEntity>();
            foreach (var app in allApplications)
            {
                IEnumerable<DeployedCodePackageEntity> oneSetOfCps;
                try
                {
                    oneSetOfCps = await this.GetDeployedCodePackageAsync(nodeName, app.ApplicationName, token).ConfigureAwait(false);
                }
                catch (FabricElementNotFoundException)
                {
                    this.logger.LogMessage("GetDeployedCodePackageAsync:: Ignoreing FabricElementNotFound for NodeName: {0}, App: {1}", nodeName, app.ApplicationName);
                    continue;
                }

                cps.AddRange(oneSetOfCps);
            }

            // Check if any code packages are deployed for System Application. If not, it will throw and we will ignore it.
            // There is really no good way (that I know of) that can give a boolean result if a CP for an app is deployed on a node or not.
            try
            {
                cps.AddRange(await this.GetDeployedCodePackageAsync(nodeName, ApplicationEntity.SystemServiceApplicationName, token).ConfigureAwait(false));
            }
            catch (Exception)
            {
                // Ignore
            }

            return cps;
        }

        /// <inheritdoc />
        public async Task<IEnumerable<DeployedCodePackageEntity>> GetDeployedCodePackageAsync(string nodeName, Uri applicationName, CancellationToken token)
        {
            Assert.IsNotEmptyOrNull(nodeName, "Node name can't be null/empty");
            Assert.IsNotNull(applicationName, "App Name can't be null/Empty");
            IEnumerable<DeployedCodePackageEntity> deployedCodePackages = null;

            try
            {
                var application = await this.GetApplicationAsync(applicationName, token).ConfigureAwait(false);

                deployedCodePackages =
                (await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.fabricClient.QueryManager.GetDeployedCodePackageListAsync(nodeName, applicationName),
                    token).ConfigureAwait(false)).Select(deployedCp => new DeployedCodePackageEntity(deployedCp, nodeName, application));
            }
            catch (FabricInvalidAddressException)
            {
                this.logger.LogWarning("GetDeployedCodePackageAsync:: Node: {0}, App: {1}, Encountered FabricInvalidAddressException", nodeName, applicationName);
                var node = this.GetNodeAsync(nodeName, token).GetAwaiter().GetResult();
                if (node.NodeStatus == NodeStatus.Up)
                {
                    this.logger.LogWarning("GetDeployedCodePackageAsync:: Node : {0}, is up. Investigate why Invalid Address Exception", nodeName);
                    throw;
                }

                this.logger.LogWarning("GetDeployedCodePackageAsync:: Node : {0}, Status: {1}", nodeName, node.NodeStatus);
                throw new FabricElementNotFoundException();
            }
            catch (Exception exp)
            {
                // For System service where this exception is expected in some cases, its a noisy log so we don't log it.
                if (!ApplicationEntity.SystemServiceApplicationName.Equals(applicationName))
                {
                    this.logger.LogWarning("GetDeployedCodePackageAsync:: Node: {0}, App: {1}, Exception Encountered: {2}", nodeName, applicationName, exp);
                    throw;
                }
            }

            return deployedCodePackages;
        }

        public Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsAsync(CancellationToken token)
        {
            if (this.healthQuery == null)
            {
                this.healthQuery = new HealthQueryHelper(this, this.fabricClient, this.logProvider);
            }

            return this.healthQuery.GetErrorAndWarningHealthEventsAsync(token);
        }
    }
}