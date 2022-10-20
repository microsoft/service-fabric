// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterQuery
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.Helpers;
    using ClusterAnalysis.Common.Log;

    internal class HealthQueryHelper
    {
        private const HealthStateFilter ErrorAndWarningFilter = HealthStateFilter.Error | HealthStateFilter.Warning;

        private IClusterQuery clusterQuery;

        private FabricClient fabricClient;

        private ILogger logger;

        public HealthQueryHelper(IClusterQuery query, FabricClient fabricClient, ILogProvider logProvider)
        {
            this.logger = logProvider.CreateLoggerInstance("HealthQuery");
            this.clusterQuery = query;
            this.fabricClient = fabricClient;
        }

        public async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsAsync(CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();

            // Get cluster health Data
            var clusterHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.HealthManager.GetClusterHealthAsync(
                    new ClusterHealthQueryDescription
                    {
                        EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                        ApplicationsFilter = new ApplicationHealthStatesFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                        NodesFilter = new NodeHealthStatesFilter { HealthStateFilterValue = ErrorAndWarningFilter }
                    }),
                token).ConfigureAwait(false);

            // If there are any health Events at Cluster Level, add them to our list
            if (clusterHealth.HealthEvents.Any())
            {
                outputResult.Add(
                    new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                       new ClusterEntity("Cluster Name TODO"),
                       clusterHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
            }

            // Query all the health Events at the Application (and down from there) level and add them.
            outputResult.AddRange(await this.GetErrorAndWarningHealthEventsOnApplicationAsync(clusterHealth.ApplicationHealthStates, token).ConfigureAwait(false));

            // Query all the Health Events at different node levels and add them.
            outputResult.AddRange(await this.GetErrorAndWarningHealthEventsOnNodeAsync(clusterHealth.NodeHealthStates, token).ConfigureAwait(false));

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnNodeAsync(
            IEnumerable<NodeHealthState> nodeHealthStates,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();

            foreach (var oneNodeHealthState in nodeHealthStates)
            {
                NodeHealth nodeHealth;
                try
                {
                    nodeHealth = await this.fabricClient.HealthManager
                        .GetNodeHealthAsync(
                            new NodeHealthQueryDescription(oneNodeHealthState.NodeName)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter }
                            }).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage("GetErrorAndWarningHealthEventsOnNodeAsync:: Failed for Node: {0}. Exp: {1}", oneNodeHealthState.NodeName, e);
                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (nodeHealth.HealthEvents.Any())
                {
                    // TODO: Certain Optimization can be done at this level maybe? For example, can we not query and just use a cached value for a certain amount of time?
                    outputResult.Add(
                        new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                            await this.clusterQuery.GetNodeAsync(oneNodeHealthState.NodeName, token).ConfigureAwait(false),
                            nodeHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
                }
            }

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnApplicationAsync(
            IEnumerable<ApplicationHealthState> applicationHealthState,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();

            foreach (var oneAppHealthState in applicationHealthState)
            {
                ApplicationHealth appHealth;
                try
                {
                    appHealth = await this.fabricClient.HealthManager
                        .GetApplicationHealthAsync(
                            new ApplicationHealthQueryDescription(oneAppHealthState.ApplicationName)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                                DeployedApplicationsFilter = new DeployedApplicationHealthStatesFilter() { HealthStateFilterValue = ErrorAndWarningFilter }
                            }).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage("GetErrorAndWarningHealthEventsOnApplicationAsync:: Failed for App: {0}. Exp: {1}", oneAppHealthState.ApplicationName, e);
                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (appHealth.HealthEvents.Any())
                {
                    outputResult.Add(
                        new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                            await this.clusterQuery.GetApplicationAsync(oneAppHealthState.ApplicationName, token).ConfigureAwait(false),
                            appHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
                }

                this.logger.LogMessage("TODO: AppHealthQuery: App Name: {0}, State: {1}", oneAppHealthState.ApplicationName, oneAppHealthState.AggregatedHealthState);
                outputResult.AddRange(
                    await this.GetErrorAndWarningHealthEventsOnDeployedApplicationAsync(appHealth.DeployedApplicationHealthStates, token)
                        .ConfigureAwait(false));

                outputResult.AddRange(
                    await this.GetErrorAndWarningHealthEventsOnServiceAsync(oneAppHealthState.ApplicationName, appHealth.ServiceHealthStates, token)
                        .ConfigureAwait(false));
            }

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnDeployedApplicationAsync(
            IEnumerable<DeployedApplicationHealthState> deployedAppState,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();
            foreach (var oneDeployedAppState in deployedAppState)
            {
                DeployedApplicationHealth appHealth;
                try
                {
                    appHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.fabricClient.HealthManager.GetDeployedApplicationHealthAsync(
                            new DeployedApplicationHealthQueryDescription(oneDeployedAppState.ApplicationName, oneDeployedAppState.NodeName)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                                DeployedServicePackagesFilter = new DeployedServicePackageHealthStatesFilter { HealthStateFilterValue = ErrorAndWarningFilter }
                            }),
                        token).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage(
                        "GetErrorAndWarningHealthEventsOnDeployedApplicationAsync:: Failed for DeployedApp: {0}, Node: {1}",
                        oneDeployedAppState.ApplicationName,
                        oneDeployedAppState.NodeName);

                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (appHealth.HealthEvents.Any())
                {
                    DeployedApplicationEntity deployedAppEntity;
                    try
                    {
                        deployedAppEntity = await this.clusterQuery.GetDeployedApplicationsAsync(oneDeployedAppState.ApplicationName, oneDeployedAppState.NodeName, token)
                                .ConfigureAwait(false);
                    }
                    catch (FabricElementNotFoundException exp)
                    {
                        this.logger.LogWarning(
                            "Failed to Deployed App Entity AppName: {0}, NodeName: {1}. Exception: {2}",
                            oneDeployedAppState.ApplicationName,
                            oneDeployedAppState.NodeName,
                            exp.Message);

                        continue;
                    }

                    outputResult.Add(
                        new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                            deployedAppEntity,
                            appHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
                }

                outputResult.AddRange(
                    await this.GetErrorAndWarningHealthEventsOnDeployedServicePackageAsync(appHealth.DeployedServicePackageHealthStates, token)
                        .ConfigureAwait(false));
            }

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnDeployedServicePackageAsync(
            IEnumerable<DeployedServicePackageHealthState> deployedSvcPkgHealthStates,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();
            foreach (var oneDeployedSvcPkgHealthState in deployedSvcPkgHealthStates)
            {
                DeployedServicePackageHealth deployedSvcPkgHealth;
                try
                {
                    deployedSvcPkgHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.fabricClient.HealthManager.GetDeployedServicePackageHealthAsync(
                            new DeployedServicePackageHealthQueryDescription(
                                oneDeployedSvcPkgHealthState.ApplicationName,
                                oneDeployedSvcPkgHealthState.NodeName,
                                oneDeployedSvcPkgHealthState.ServiceManifestName)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                            }),
                        token).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage(
                        "GetErrorAndWarningHealthEventsOnDeployedServicePackageAsync:: Failed. App: {0}, Node: {1}, SvcManifest: {2}. Excp: {3}",
                        oneDeployedSvcPkgHealthState.ApplicationName,
                        oneDeployedSvcPkgHealthState.NodeName,
                        oneDeployedSvcPkgHealthState.ServiceManifestName,
                        e);

                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (!deployedSvcPkgHealth.HealthEvents.Any())
                {
                    continue;
                }

                DeployedCodePackageEntity dcp;
                try
                {
                    dcp = (await this.clusterQuery.GetDeployedCodePackageAsync(deployedSvcPkgHealth.NodeName, deployedSvcPkgHealth.ApplicationName, token)
                        .ConfigureAwait(false)).SingleOrDefault(item => item.ServiceManifestName == deployedSvcPkgHealth.ServiceManifestName);
                }
                catch (FabricElementNotFoundException exp)
                {
                    this.logger.LogWarning(
                        "Failed to Find Dcp with NodeName: {0}, AppName: {1}, SvcManifestName: {2}. Exception: {3}",
                        deployedSvcPkgHealth.NodeName,
                        deployedSvcPkgHealth.ApplicationName,
                        deployedSvcPkgHealth.ServiceManifestName,
                        exp.Message);

                    continue;
                }

                // This can happen if the package got deleted. In such a case we ignore and continue.
                if (dcp == null)
                {
                    this.logger.LogWarning(
                        "Failed to Find Dcp with NodeName: {0}, AppName: {1}, SvcManifestName: {2}",
                        deployedSvcPkgHealth.NodeName,
                        deployedSvcPkgHealth.ApplicationName,
                        deployedSvcPkgHealth.ServiceManifestName);

                    continue;
                }

                outputResult.Add(
                    new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                        dcp,
                        deployedSvcPkgHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
            }

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnServiceAsync(
            Uri applicationName,
            IEnumerable<ServiceHealthState> serviceHealthStates,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();
            foreach (var oneServiceHealthState in serviceHealthStates)
            {
                ServiceHealth serviceHealth;
                try
                {
                    serviceHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.fabricClient.HealthManager.GetServiceHealthAsync(
                            new ServiceHealthQueryDescription(oneServiceHealthState.ServiceName)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                                PartitionsFilter = new PartitionHealthStatesFilter { HealthStateFilterValue = ErrorAndWarningFilter }
                            }),
                        token).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage("GetErrorAndWarningHealthEventsOnServiceAsync:: Failed. SvcName: {0}. Exp: {1}", oneServiceHealthState.ServiceName, e);
                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (serviceHealth.HealthEvents.Any())
                {
                    ServiceEntity svcEntity;
                    try
                    {
                        svcEntity = await this.clusterQuery.GetServiceAsync(applicationName, oneServiceHealthState.ServiceName, token).ConfigureAwait(false);
                    }
                    catch (FabricElementNotFoundException exp)
                    {
                        this.logger.LogWarning(
                            "Failed to Svc Entity AppName: {0}, Svc Name: {1}. Exception: {2}",
                            applicationName,
                            oneServiceHealthState.ServiceName,
                            exp.Message);

                        continue;
                    }

                    outputResult.Add(
                        new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                            svcEntity,
                            serviceHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
                }

                outputResult.AddRange(
                    await this.GetErrorAndWarningHealthEventsOnPartitionAsync(serviceHealth.PartitionHealthStates, token).ConfigureAwait(false));
            }

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnPartitionAsync(
            IEnumerable<PartitionHealthState> partitionHealthStates,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();
            foreach (var onePartitionHealthState in partitionHealthStates)
            {
                PartitionHealth partitionHealth;
                try
                {
                    partitionHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.fabricClient.HealthManager.GetPartitionHealthAsync(
                            new PartitionHealthQueryDescription(onePartitionHealthState.PartitionId)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter },
                                ReplicasFilter = new ReplicaHealthStatesFilter { HealthStateFilterValue = ErrorAndWarningFilter }
                            }),
                        token).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage("GetErrorAndWarningHealthEventsOnPartitionAsync:: Failed. Partition: {0}. Exp: {1}", onePartitionHealthState.PartitionId, e);
                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (partitionHealth.HealthEvents.Any())
                {
                    PartitionEntity partitionEntity;
                    try
                    {
                        partitionEntity = await this.clusterQuery.GetPartitionAsync(onePartitionHealthState.PartitionId, token).ConfigureAwait(false);
                    }
                    catch (FabricElementNotFoundException exp)
                    {
                        this.logger.LogWarning(
                            "Failed to Get Partition: {0}, Exception: {1}",
                            onePartitionHealthState.PartitionId,
                            exp.Message);

                        continue;
                    }

                    outputResult.Add(
                        new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                            partitionEntity,
                            partitionHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
                }

                outputResult.AddRange(
                    await this.GetErrorAndWarningHealthEventsOnReplicaAsync(partitionHealth.ReplicaHealthStates, token).ConfigureAwait(false));
            }

            return outputResult;
        }

        private async Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsOnReplicaAsync(
            IEnumerable<ReplicaHealthState> replicaHealthStates,
            CancellationToken token)
        {
            var outputResult = new List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>();
            foreach (var oneReplicaHealthState in replicaHealthStates)
            {
                ReplicaHealth replicaHealth;
                try
                {
                    replicaHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => this.fabricClient.HealthManager.GetReplicaHealthAsync(
                            new ReplicaHealthQueryDescription(oneReplicaHealthState.PartitionId, oneReplicaHealthState.Id)
                            {
                                EventsFilter = new HealthEventsFilter { HealthStateFilterValue = ErrorAndWarningFilter }
                            }),
                        token).ConfigureAwait(false);
                }
                catch (FabricException e)
                {
                    this.logger.LogMessage(
                        "GetErrorAndWarningHealthEventsOnReplicaAsync:: Failed. Partition: {0}, Replica: {1}, Exp: {2}",
                        oneReplicaHealthState.PartitionId,
                        oneReplicaHealthState.Id,
                        e);

                    if (!this.IsFabricElementNotFound(e))
                    {
                        throw;
                    }

                    continue;
                }

                if (!replicaHealth.HealthEvents.Any())
                {
                    continue;
                }

                ReplicaEntity replicaEntity;
                try
                {
                    replicaEntity = await this.clusterQuery.GetReplicaAsync(oneReplicaHealthState.PartitionId, oneReplicaHealthState.Id, token).ConfigureAwait(false);
                }
                catch (FabricElementNotFoundException exp)
                {
                    this.logger.LogWarning(
                        "Failed to Get replica Entity: Partition: {0}, ReplicaId: {1}, Exception: {2}",
                        oneReplicaHealthState.PartitionId,
                        oneReplicaHealthState.Id,
                        exp.Message);

                    continue;
                }

                outputResult.Add(
                    new KeyValuePair<BaseEntity, IList<HealthEventEntity>>(
                        replicaEntity,
                        replicaHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList()));
            }

            return outputResult;
        }

        private bool IsFabricElementNotFound(FabricException exp)
        {
            return exp.ErrorCode == FabricErrorCode.FabricHealthEntityNotFound;
        }
    }
}
