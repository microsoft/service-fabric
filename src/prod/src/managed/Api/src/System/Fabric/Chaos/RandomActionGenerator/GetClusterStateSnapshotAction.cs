// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.Common.Exceptions;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Description;
    using System.Globalization;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Text;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetClusterStateSnapshotAction : FabricTestAction<ClusterStateSnapshot>
    {
        private const string TraceType = "GetClusterStateSnapshotAction";
        private const string ExceptionDelimeter = ";";
        private const string Tab = "\t";
        private const string ReplicaViewFormat = "{0}:{1}:{2}";
        private const string ReplicaViewPrintFormat = "{0}PartitionID:NodeName:ReplicaStatus={1}";

        private static readonly Random UniformRandomNumberGenerator = new Random((int)DateTime.Now.Ticks);

        internal GetClusterStateSnapshotAction(
            double chaosSnapshotTelemetrySamplingProbability,
            bool shouldFaultSystem,
            int maximumNumberOfRetries,
            ChaosTargetFilter chaosTargetFilter)
        {
            this.TelemetrySamplingProbability = chaosSnapshotTelemetrySamplingProbability;
            this.ShouldFaultSystem = shouldFaultSystem;

            ThrowIf.IsTrue(maximumNumberOfRetries < 0, "Value must be non-negative.");

            this.MaximumNumberOfRetries = maximumNumberOfRetries;

            this.ChaosTargetFilter = chaosTargetFilter;
        }

        internal double TelemetrySamplingProbability { get; set; }

        internal bool ShouldFaultSystem { get; set; }

        internal int MaximumNumberOfRetries { get; set; }

        internal ChaosTargetFilter ChaosTargetFilter { get; set; }

        internal override Type ActionHandlerType => typeof(GetClusterStateSnapshotActionHandler);

        internal static int ServiceCount { get; set; }
        internal static int PartitionCount { get; set; }
        internal static int ReplicaCount { get; set; }

        private class GetClusterStateSnapshotActionHandler : FabricTestActionHandler<GetClusterStateSnapshotAction>
        {
            // Except for ApplicationName, other fields are dummy
            private static readonly Application SystemApplication = new Application
            {
                ApplicationName = new Uri(Constants.SystemApplicationName),
                ApplicationTypeName = "SystemApplication",
                ApplicationTypeVersion = "3.14",
                ApplicationStatus = ApplicationStatus.Ready,
                HealthState = HealthState.Ok
            };

            // Except for ApplicationName, other fields are dummy
            private static readonly DeployedApplication DeployedSystemApplication = new DeployedApplication(
                SystemApplication.ApplicationName,
                SystemApplication.ApplicationTypeName,
                DeploymentStatus.Active,
                string.Empty, // workDirectory
                string.Empty, // logDirectory
                string.Empty, // tempDirectory
                HealthState.Ok);

            private TimeSpan requestTimeOut = TimeSpan.FromSeconds(30);
            private TimeoutHelper timer;
            private FabricTestContext testContext;
            private Dictionary<NodeInfo, DeployedServiceReplicaList> deployedSystemReplicaMap;

            private static readonly DeployedServicePackageHealth DefaultDeployedServicePackageHealth =
                new DeployedServicePackageHealth
                    {
                        AggregatedHealthState = HealthState.Ok
                    };

            private HashSet<string> PartitionMapFromFM;
            private HashSet<string> PartitionMapFromNodes;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, GetClusterStateSnapshotAction action, CancellationToken cancellationToken)
            {
                Dictionary<string, int> ExceptionHistory = new Dictionary<string, int>();

                int retries = 0;

                GetClusterStateSnapshotAction.ServiceCount = 0;
                GetClusterStateSnapshotAction.PartitionCount = 0;
                GetClusterStateSnapshotAction.ReplicaCount = 0;

                Stopwatch stopWatch = Stopwatch.StartNew();

                ClusterStateSnapshot clusterSnapshot = null;

                do
                {
                    ++retries;

                    await Task.Delay(Constants.DefaultChaosSnapshotRecaptureBackoffInterval, cancellationToken).ConfigureAwait(false);

                    try
                    {
                        clusterSnapshot = await this.CaptureClusterStateSnapshotAndPopulateEntitiesAsync(
                                                                                                            testContext,
                                                                                                            action,
                                                                                                            cancellationToken).ConfigureAwait(false);
                    }
                    catch (Exception exception) when (exception is FabricException || exception is ChaosInconsistentClusterSnapshotException)
                    {
                        string exceptionString = exception.Message;

                        if (ExceptionHistory.ContainsKey(exceptionString))
                        {
                            ExceptionHistory[exceptionString]++;
                        }
                        else
                        {
                            ExceptionHistory[exceptionString] = 1;
                        }
                    }

                    string allExceptions = string.Join(ExceptionDelimeter, ExceptionHistory);

                    if (retries >= action.MaximumNumberOfRetries)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "While taking a consistent cluster snapshot, following exceptions occurred: {0}", allExceptions);
                    }

                    ChaosUtility.ThrowOrAssertIfTrue(
                        ChaosConstants.GetClusterSnapshotAction_MaximumNumberOfRetriesAchieved_TelemetryId,
                        retries >= action.MaximumNumberOfRetries,
                        string.Format(StringResources.ChaosEngineError_GetClusterSnapshotAction_MaximumNumberOfRetriesAchieved, action.MaximumNumberOfRetries, allExceptions));
                }
                while (clusterSnapshot == null);

                stopWatch.Stop();

                var elapsedInGatherSnapshot = stopWatch.Elapsed;

                stopWatch = Stopwatch.StartNew();

                clusterSnapshot.ApplyChaosTargetFilter(action.ChaosTargetFilter);

                clusterSnapshot.MarkAllUnsafeEntities();

                stopWatch.Stop();

                var elapsedInMarkAllUnsafe = stopWatch.Elapsed;

                if (UniformRandomNumberGenerator.NextDouble() < action.TelemetrySamplingProbability)
                {
                    FabricEvents.Events.ChaosSnapshot(
                        Guid.NewGuid().ToString(),
                        clusterSnapshot.Nodes.Count,
                        clusterSnapshot.Applications.Count,
                        GetClusterStateSnapshotAction.ServiceCount,
                        GetClusterStateSnapshotAction.PartitionCount,
                        GetClusterStateSnapshotAction.ReplicaCount,
                        elapsedInGatherSnapshot.TotalSeconds,
                        elapsedInMarkAllUnsafe.TotalSeconds,
                        retries);
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "For '{0}' nodes, '{1}' apps, '{2}' services, '{3}' partitions, '{4}' replicas, snapshot took '{5}', mark unsafe took '{6}', took '{7}' retries.",
                    clusterSnapshot.Nodes.Count,
                    clusterSnapshot.Applications.Count,
                    GetClusterStateSnapshotAction.ServiceCount,
                    GetClusterStateSnapshotAction.PartitionCount,
                    GetClusterStateSnapshotAction.ReplicaCount,
                    elapsedInGatherSnapshot,
                    elapsedInMarkAllUnsafe,
                    retries);

                action.Result = clusterSnapshot;
                ResultTraceString = "GetClusterStateSnapshotAction succeeded";
            }

            private async Task<ClusterStateSnapshot> CaptureClusterStateSnapshotAndPopulateEntitiesAsync(
                FabricTestContext testContext,
                GetClusterStateSnapshotAction action,
                CancellationToken cancellationToken)
            {
                this.PartitionMapFromFM = new HashSet<string>(StringComparer.InvariantCulture);
                this.PartitionMapFromNodes = new HashSet<string>(StringComparer.InvariantCulture);

                this.requestTimeOut = action.RequestTimeout;
                this.timer = new TimeoutHelper(action.ActionTimeout);
                this.testContext = testContext;
                this.deployedSystemReplicaMap = new Dictionary<NodeInfo, DeployedServiceReplicaList>();

                var nodes = await this.testContext.FabricCluster.GetLatestNodeInfoAsync(this.requestTimeOut, this.timer.GetRemainingTime(), cancellationToken).ConfigureAwait(false);

                var clusterSnapshot = new ClusterStateSnapshot(false, action.ShouldFaultSystem);
                var nodeInfos = nodes as IList<NodeInfo> ?? nodes.ToList();
                clusterSnapshot.Nodes.AddNodes(nodeInfos);
                clusterSnapshot.PopulateNodeMaps(nodes);

                // Get all current active applications
                var appListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.testContext.FabricClient.QueryManager.GetApplicationListAsync(
                        null,
                        string.Empty,
                        this.requestTimeOut,
                        cancellationToken),
                    this.timer.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (appListResult != null)
                {
                    foreach (var appResultItem in appListResult)
                    {
                        var applicationEntity = clusterSnapshot.Applications.AddApplication(appResultItem);
                        await this.PopulateApplicationEntityAsync(applicationEntity, cancellationToken).ConfigureAwait(false);
                    }

                    var systemApplicationEntity = clusterSnapshot.Applications.AddApplication(SystemApplication);
                    await this.PopulateApplicationEntityAsync(systemApplicationEntity, cancellationToken).ConfigureAwait(false);
                }

                foreach (var node in nodeInfos)
                {
                    var node1 = node.Clone();
                    if (node1.IsNodeUp)
                    {
                        var retryableErrorsForGetDeployedApplicationList = new FabricClientRetryErrors();
                        retryableErrorsForGetDeployedApplicationList.RetryableFabricErrorCodes.Add(FabricErrorCode.InvalidAddress);

                        var deployedApplicationList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                () => this.testContext.FabricClient.QueryManager.GetDeployedApplicationListAsync(
                                    node1.NodeName,
                                    null,
                                    this.requestTimeOut,
                                    cancellationToken),
                                retryableErrorsForGetDeployedApplicationList,
                                this.timer.GetRemainingTime(),
                                cancellationToken).ConfigureAwait(false);

                        // Add system app entity in the deployed application list
                        // so that we get the deployed replica list for the node
                        if (await this.HasDeployedSystemServiceAsync(node1, cancellationToken).ConfigureAwait(false))
                        {
                            if (deployedApplicationList == null)
                            {
                                deployedApplicationList = new DeployedApplicationList();
                            }

                            deployedApplicationList.Add(DeployedSystemApplication);
                        }

                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "Node: {0} has the following apps deployed...", node1);

                        foreach (var app in deployedApplicationList)
                        {
                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed app = {0}", app.ApplicationName.OriginalString);
                        }

                        foreach (var app in deployedApplicationList)
                        {
                            var application = app;
                            var applicationEntity = clusterSnapshot.Applications.FirstOrDefault(a => a.Application.ApplicationName == application.ApplicationName);
                            if (applicationEntity != null)
                            {
                                if (!await this.TryAssociateDeployedReplicaWithDeployedCodepackageAsync(
                                                                                                    node1,
                                                                                                    applicationEntity,
                                                                                                    cancellationToken).ConfigureAwait(false))
                                {
                                    return null;
                                }
                            }
                        } // iterate through the deployed apps
                    } // if a node is up
                } // iterate through the nodes

                // Information acquired through queries could go stale due to the cluster dynamism.
                // This happened while the cluster snapshot was being taken -- making the snapshot internally inconsistent.
                // The fix is to ignore the inconsistent snapshot and capture it again.
                //
                // If FailoverManager's point of view coincides with that of the Nodes, return the snapshot;
                // otherwise, throw FabricException to indicate that the snapshot should be captured afresh.
                //
                if (!this.PartitionMapFromFM.SetEquals(this.PartitionMapFromNodes))
                {
                    StringBuilder exceptionMessageBuilder = new StringBuilder();

                    var copyOfFmInfo = new HashSet<string>(this.PartitionMapFromFM);

                    this.PartitionMapFromFM.ExceptWith(this.PartitionMapFromNodes);

                    if (this.PartitionMapFromFM.Any())
                    {
                        exceptionMessageBuilder.AppendLine(string.Format(CultureInfo.InvariantCulture, "FM has the following extra information:"));

                        foreach (var pinfo in this.PartitionMapFromFM)
                        {
                            exceptionMessageBuilder.AppendLine(string.Format(CultureInfo.InvariantCulture, ReplicaViewPrintFormat, Tab, pinfo));
                        }
                    }

                    this.PartitionMapFromNodes.ExceptWith(copyOfFmInfo);

                    if (this.PartitionMapFromNodes.Any())
                    {
                        exceptionMessageBuilder.AppendLine(string.Format(CultureInfo.InvariantCulture, "Nodes has the following partitions deployed, which FM does not know about:"));

                        foreach (var pinfo in this.PartitionMapFromNodes)
                        {
                            exceptionMessageBuilder.AppendLine(string.Format(CultureInfo.InvariantCulture, ReplicaViewPrintFormat, Tab, pinfo));
                        }
                    }

                    TestabilityTrace.TraceSource.WriteWarning(TraceType, string.Format(CultureInfo.InvariantCulture, "{0}", exceptionMessageBuilder.ToString()));

                    throw new ChaosInconsistentClusterSnapshotException(exceptionMessageBuilder.ToString());
                }

                return clusterSnapshot;
            }

            private async Task<bool> TryAssociateDeployedReplicaWithDeployedCodepackageAsync(
                NodeInfo node1,
                ApplicationEntity applicationEntity,
                CancellationToken cancellationToken)
            {
                // Handle the case where application is deployed but code package is not
                var retryableErrorsForGetDeployedCodePackageList = new FabricClientRetryErrors();

                // RDBug 5408739: GetDeployedApplicationList found the application;
                // but, by the time GetDeployedCodePackageList was issued, the application is no longer deployed on the node
                retryableErrorsForGetDeployedCodePackageList.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.ApplicationNotFound);

                // RDBug 5420064 : GetDeployed(Application/CodePackage)ListRequest fails with FABRIC_E_INVALID_ADDRESS
                retryableErrorsForGetDeployedCodePackageList.RetryableFabricErrorCodes.Add(FabricErrorCode.InvalidAddress);

                var deployedCodePackageList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.testContext.FabricClient.QueryManager.GetDeployedCodePackageListAsync(
                        node1.NodeName,
                        applicationEntity.Application.ApplicationName,
                        string.Empty,
                        string.Empty,
                        this.requestTimeOut,
                        cancellationToken),
                    retryableErrorsForGetDeployedCodePackageList,
                    this.timer.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                // If FM, CM, or NS replicas are on the node, we add corresponding dummy codepackages in the deployedCodePackageList
                // This is required, because when we want to fault a node, we want to reach the deployed FM/CM/NS replicas through these
                // deployed dummy codepackages
                var listOfDeployedCodePackages = await this.GetSystemServiceDummyCodepackagesAsync(
                    node1,
                    applicationEntity,
                    cancellationToken).ConfigureAwait(false);

                listOfDeployedCodePackages.AddRangeNullSafe(deployedCodePackageList);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed codepacks for app entity: {0}...", applicationEntity.Application.ApplicationName.OriginalString);

                if (listOfDeployedCodePackages != null && listOfDeployedCodePackages.Any())
                {
                    foreach (var dcp in listOfDeployedCodePackages)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed codepack: {0}", dcp.CodePackageName + dcp.ServiceManifestName + dcp.CodePackageVersion);
                    }

                    var deployedReplicaList =
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () =>
                            this.testContext.FabricClient.QueryManager.GetDeployedReplicaListAsync(
                                node1.NodeName,
                                applicationEntity.Application.ApplicationName,
                                string.Empty,
                                Guid.Empty,
                                this.requestTimeOut,
                                cancellationToken),
                            FabricClientRetryErrors.GetDeployedClusterEntityErrors.Value,
                            this.timer.GetRemainingTime(),
                            cancellationToken).ConfigureAwait(false);

                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed replicas...");

                    foreach (var dr in deployedReplicaList)
                    {
                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed replica: {0}", dr.ServiceName.OriginalString + dr.Partitionid);
                    }

                    foreach (var dc in listOfDeployedCodePackages)
                    {
                        DeployedCodePackage deployedCodePackage = dc;
                        DeployedServicePackageHealth spHealth;
                        if (!string.IsNullOrEmpty(deployedCodePackage.ServiceManifestName))
                        {
                            DeployedServicePackageHealthQueryDescription desc = new DeployedServicePackageHealthQueryDescription(applicationEntity.Application.ApplicationName, node1.NodeName, deployedCodePackage.ServiceManifestName, deployedCodePackage.ServicePackageActivationId);

                            spHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<DeployedServicePackageHealth>(
                            () =>
                                this.testContext.FabricClient.HealthManager.GetDeployedServicePackageHealthAsync(desc, this.requestTimeOut, cancellationToken),
                            FabricClientRetryErrors.GetDeployedClusterEntityErrors.Value,
                            this.timer.GetRemainingTime(),
                            cancellationToken).ConfigureAwait(false);
                        }
                        else
                        {
                            spHealth = DefaultDeployedServicePackageHealth;
                        }

                        var codePackage = applicationEntity.AddCodePackage(
                                                                deployedCodePackage,
                                                                node1.NodeName,
                                                                spHealth);

                        foreach (var replica in deployedReplicaList)
                        {
                            string partitionIdentifier = string.Format(ReplicaViewFormat, replica.Partitionid, node1.NodeName, replica.ReplicaStatus);

                            this.PartitionMapFromNodes.Add(partitionIdentifier);

                            DeployedServiceReplica deployedReplica = replica;

                            var isSystemServiceReplicaWithoutCodepackage =
                                string.IsNullOrEmpty(deployedReplica.CodePackageName)
                                && string.IsNullOrEmpty(deployedReplica.ServiceManifestName);

                            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed replica: {0} isSystemServiceReplicaWithoutCodepackage={1}", deployedReplica.ServiceName.OriginalString + deployedReplica.Partitionid, isSystemServiceReplicaWithoutCodepackage);

                            // Handle FM/CM/NS replica separately, because these
                            // services do not have an actual codepackage
                            if ((isSystemServiceReplicaWithoutCodepackage && this.MatchFmCmNsReplicaWithDummyCodepackage(deployedReplica, deployedCodePackage))
                                || (!isSystemServiceReplicaWithoutCodepackage && this.MatchReplicaWithCodepackage(deployedReplica, deployedCodePackage)))
                            {
                                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Deployed replica:{0} is in codepack: {1} so need to add deployed partition to codepack", deployedReplica.ServiceName.OriginalString + deployedReplica.Partitionid, deployedCodePackage.CodePackageName + deployedCodePackage.ServiceManifestName + deployedCodePackage.CodePackageVersion);

                                this.PopulateCodepackageWithDeployedPartitions(deployedReplica, codePackage, applicationEntity);
                            }
                        } // iterate through deployed replicas
                    } // iterate through deployed codepacks
                } // if there are deployed codepacks

                return true;
            }

            private void PopulateCodepackageWithDeployedPartitions(
                DeployedServiceReplica deployedReplica,
                CodePackageEntity codePackage,
                ApplicationEntity applicationEntity)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Inside PopulateCodepackageWithDeployedPartitions");

                var serviceEntity = applicationEntity.ServiceList.FirstOrDefault(s => s.Service.ServiceName == deployedReplica.ServiceName);

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "ServiceEntity: {0}", serviceEntity);

                if (serviceEntity != null)
                {
                    var partitionEntity =
                        serviceEntity.PartitionList.FirstOrDefault(
                            p => p.Guid == deployedReplica.Partitionid);

                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "PartitionEntity: {0}", partitionEntity);

                    if (partitionEntity != null)
                    {
                        codePackage.DeployedPartitions.Add(partitionEntity);

                        TestabilityTrace.TraceSource.WriteInfo(TraceType, "Added partitionEntity: {0} in codepack: {1}", partitionEntity, codePackage);
                    }
                    else
                    {
                        // This would mean that the service for this partition has been deleted and recreated hence the old partition does not exist
                    }
                }
                else
                {
                    // This could mean that the service for this replica has been deleted but the replica is still alive
                }
            }

            private bool MatchReplicaWithCodepackage(
                DeployedServiceReplica deployedReplica,
                DeployedCodePackage deployedCodePackage)
            {
                string deployedCodePackageName = deployedCodePackage.CodePackageName;
                string deployedCodePackageVersion = deployedCodePackage.CodePackageVersion;
                string deployedServiceManifestName = deployedCodePackage.ServiceManifestName;

                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "Inside MatchReplicaWithCodepackage: {0} vs {1}",
                    deployedReplica.Address,
                    deployedCodePackageName + deployedCodePackageVersion + deployedServiceManifestName);

                return deployedReplica.CodePackageName.Equals(deployedCodePackageName, StringComparison.OrdinalIgnoreCase)
                       && deployedReplica.ServiceManifestName.Equals(deployedServiceManifestName, StringComparison.OrdinalIgnoreCase)
                       && MatchServicePackageActivationId(deployedReplica.ServicePackageActivationId, deployedCodePackage.ServicePackageActivationId);
            }

            private bool MatchServicePackageActivationId(string replicaActivationId, string codePackageActivationId)
            {
                if (string.IsNullOrEmpty(replicaActivationId) && string.IsNullOrEmpty(codePackageActivationId))
                {
                    return true;
                }
                else if (!string.IsNullOrEmpty(replicaActivationId) && !string.IsNullOrEmpty(codePackageActivationId))
                {
                    return replicaActivationId.Equals(codePackageActivationId, StringComparison.OrdinalIgnoreCase);
                }
                else
                {
                    return false;
                }
            }

            private bool MatchFmCmNsReplicaWithDummyCodepackage(
                DeployedServiceReplica deployedReplica,
                DeployedCodePackage codePackage)
            {
                var isFmReplica =
                    deployedReplica.ServiceName.OriginalString.Equals(Constants.FailoverManagerServiceName, StringComparison.OrdinalIgnoreCase)
                    && codePackage.CodePackageVersion.Equals(ChaosConstants.DummyFMCodePackageVersion, StringComparison.OrdinalIgnoreCase);

                var isCmReplica =
                    deployedReplica.ServiceName.OriginalString.Equals(Constants.ClusterManagerServiceName, StringComparison.OrdinalIgnoreCase)
                    && codePackage.CodePackageVersion.Equals(ChaosConstants.DummyCMCodePackageVersion, StringComparison.OrdinalIgnoreCase);

                var isNsReplica =
                    deployedReplica.ServiceName.OriginalString.Equals(Constants.NamingServiceName, StringComparison.OrdinalIgnoreCase)
                    && codePackage.CodePackageVersion.Equals(ChaosConstants.DummyNSCodePackageVersion, StringComparison.OrdinalIgnoreCase);

                return isFmReplica || isCmReplica || isNsReplica;
            }

            private async Task PopulateApplicationEntityAsync(
                ApplicationEntity applicationEntity,
                CancellationToken cancellationToken)
            {
                // Get all services under this application
                var svcListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.testContext.FabricClient.QueryManager.GetServiceListAsync(
                        applicationEntity.Application.ApplicationName,
                        null,
                        this.requestTimeOut,
                        cancellationToken),
                    this.timer.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (svcListResult != null)
                {
                    GetClusterStateSnapshotAction.ServiceCount += svcListResult.Count;

                    foreach (var svcResultItem in svcListResult)
                    {
                        var serviceEntity = applicationEntity.AddService(svcResultItem);
                        await this.PopulateServiceEntityAsync(serviceEntity, cancellationToken).ConfigureAwait(false);
                    }
                }
            }

            private async Task PopulateServiceEntityAsync(
                ServiceEntity serviceEntity,
                CancellationToken cancellationToken)
            {
                // Get all partitions in this service
                ServicePartitionList partitionListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.testContext.FabricClient.QueryManager.GetPartitionListAsync(
                        serviceEntity.Service.ServiceName,
                        null,
                        this.requestTimeOut,
                        cancellationToken),
                    FabricClientRetryErrors.GetPartitionListFabricErrors.Value,
                    this.timer.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (partitionListResult != null)
                {
                    GetClusterStateSnapshotAction.PartitionCount += partitionListResult.Count;

                    foreach (var partitionResultItem in partitionListResult)
                    {
                        var partitionEntity = serviceEntity.AddPartition(partitionResultItem);
                        await this.PopulatePartitionEntityAsync(partitionEntity, cancellationToken).ConfigureAwait(false);
                    }
                }
            }

            private async Task PopulatePartitionEntityAsync(
                PartitionEntity partitionEntity,
                CancellationToken cancellationToken)
            {
                // Get all current active applications
                var replicaListResult =
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () =>
                            this.testContext.FabricClient.QueryManager.GetReplicaListAsync(
                                partitionEntity.Guid,
                                0,
                                this.requestTimeOut,
                                cancellationToken),
                            this.timer.GetRemainingTime(),
                            cancellationToken).ConfigureAwait(false);

                if (replicaListResult != null)
                {
                    GetClusterStateSnapshotAction.ReplicaCount += replicaListResult.Count;

                    foreach (var replicaResultItem in replicaListResult)
                    {
                        string partitionIdentifier = string.Format(ReplicaViewFormat, partitionEntity.Partition.PartitionInformation.Id, replicaResultItem.NodeName, replicaResultItem.ReplicaStatus);

                        this.PartitionMapFromFM.Add(partitionIdentifier);

                        partitionEntity.AddReplica(replicaResultItem);
                    }
                }
            }

            private DeployedCodePackage GetDummyDeployedServiceCodepackage(string dummyCodePackageVersion)
            {
                var dummyEntryPoint = new CodePackageEntryPoint(
                    string.Empty, // entry-point location
                    0, // process id
                    string.Empty, // runas username
                    EntryPointStatus.Started,
                    DateTime.UtcNow, // next activation utc
                    null, // codepack statistics
                    0 /*codepack instance id*/);

                var dummyCodePack = new DeployedCodePackage(
                    string.Empty, // codepack name
                    dummyCodePackageVersion, // codepack version
                    dummyEntryPoint, // setup entrypoint
                    string.Empty, // servicemanifest name
                    string.Empty, // Default ServicePackageActivationId
                    0, // run frequency interval
                    HostType.ExeHost,
                    HostIsolationMode.None,
                    DeploymentStatus.Active,
                    dummyEntryPoint /*entrypoint*/);

                return dummyCodePack;
            }

            private async Task<DeployedCodePackageList> GetSystemServiceDummyCodepackagesAsync(
                NodeInfo node,
                ApplicationEntity applicationEntity,
                CancellationToken cancellationToken)
            {
                var presenceOfFmCmNs = await this.GetPresenceOfCodePackagelessSystemServiceAsync(
                                                                                                node,
                                                                                                applicationEntity,
                                                                                                cancellationToken).ConfigureAwait(false);

                var listOfDeployedDummyCodePacks = new DeployedCodePackageList();

                // FM present
                if (presenceOfFmCmNs.Item1)
                {
                    listOfDeployedDummyCodePacks.Add(this.GetDummyDeployedServiceCodepackage(ChaosConstants.DummyFMCodePackageVersion));
                }

                // CM present
                if (presenceOfFmCmNs.Item2)
                {
                    listOfDeployedDummyCodePacks.Add(this.GetDummyDeployedServiceCodepackage(ChaosConstants.DummyCMCodePackageVersion));
                }

                // NS present
                if (presenceOfFmCmNs.Item3)
                {
                    listOfDeployedDummyCodePacks.Add(this.GetDummyDeployedServiceCodepackage(ChaosConstants.DummyNSCodePackageVersion));
                }

                return listOfDeployedDummyCodePacks;
            }

            private async Task<bool> HasDeployedSystemServiceAsync(
                NodeInfo node,
                CancellationToken cancellationToken)
            {
                var deployedSystemReplicaList = await this.GetDeployedSystemServiceReplicaListAsync(node, cancellationToken).ConfigureAwait(false);

                return deployedSystemReplicaList.Any(ClusterStateSnapshot.IsSystemServiceReplica);
            }

            private async Task<Tuple<bool, bool, bool>> GetPresenceOfCodePackagelessSystemServiceAsync(
                NodeInfo node,
                ApplicationEntity application,
                CancellationToken cancellationToken)
            {
                var isSystemApplication = ClusterStateSnapshot.IsSystemApplication(application);

                if (!isSystemApplication)
                {
                    return new Tuple<bool, bool, bool>(false, false, false);
                }

                var deployedSystemReplicaList = await this.GetDeployedSystemServiceReplicaListAsync(node, cancellationToken).ConfigureAwait(false);

                bool isFmPresent = false, isCmPresent = false, isNsPresent = false;

                foreach (var systemReplica in deployedSystemReplicaList)
                {
                    if (!isFmPresent
                        && systemReplica.ServiceName.OriginalString.Equals(Constants.FailoverManagerServiceName, StringComparison.OrdinalIgnoreCase))
                    {
                        isFmPresent = true;
                    }
                    else if (!isCmPresent
                        && systemReplica.ServiceName.OriginalString.Equals(Constants.ClusterManagerServiceName, StringComparison.OrdinalIgnoreCase))
                    {
                        isCmPresent = true;
                    }
                    else if (!isNsPresent
                        && systemReplica.ServiceName.OriginalString.Equals(Constants.NamingServiceName, StringComparison.OrdinalIgnoreCase))
                    {
                        isNsPresent = true;
                    }
                }

                return new Tuple<bool, bool, bool>(isFmPresent, isCmPresent, isNsPresent);
            }

            private async Task<DeployedServiceReplicaList> GetDeployedSystemServiceReplicaListAsync(
                NodeInfo node,
                CancellationToken cancellationToken)
            {
                DeployedServiceReplicaList deployedReplicaList;
                if (this.deployedSystemReplicaMap.ContainsKey(node))
                {
                    deployedReplicaList = this.deployedSystemReplicaMap[node];
                }
                else
                {
                    deployedReplicaList =
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () =>
                            this.testContext.FabricClient.QueryManager.GetDeployedReplicaListAsync(
                                node.NodeName,
                                SystemApplication.ApplicationName,
                                string.Empty,
                                Guid.Empty,
                                this.requestTimeOut,
                                cancellationToken),
                            FabricClientRetryErrors.GetDeployedClusterEntityErrors.Value,
                            this.timer.GetRemainingTime(),
                            cancellationToken).ConfigureAwait(false);

                    this.deployedSystemReplicaMap[node] = deployedReplicaList;
                }

                return deployedReplicaList;
            }
        }
    }
}