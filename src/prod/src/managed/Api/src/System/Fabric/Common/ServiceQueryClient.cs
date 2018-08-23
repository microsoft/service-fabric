// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ServiceQueryClient
    {
        private const string TraceSource = "ServiceQueryClient";

        internal static readonly TimeSpan RequestTimeoutDefault = TimeSpan.FromMinutes(1);
        internal static readonly TimeSpan RetryWaitTimeoutDefault = TimeSpan.FromSeconds(5d);
        private const string SystemApplicationName = "fabric:/System";

        private readonly Uri serviceName;
        private readonly bool isSystemService;
        private readonly ValidationCheckFlag checkFlags;
        private readonly TimeSpan operationTimeout;
        private readonly TimeSpan requestTimeout;

        // lock
        private readonly SemaphoreSlim asyncMutex;

        // These information will be loaded using query.
        private readonly SynchronizedModifyOnce<bool> isStateful;
        private readonly SynchronizedModifyOnce<int> partitionCount;
        private readonly SynchronizedModifyOnce<int> targetReplicaSetSize;

        public ServiceQueryClient(
            Uri serviceName,
            FabricTestContext testContext,
            ValidationCheckFlag checkFlags,
            TimeSpan operationTimeout,
            TimeSpan requestTimeout)
        {
            ThrowIf.Null(serviceName, "serviceName");
            ThrowIf.Null(testContext, "testContext");

            // initialize lock and synchronized fields.
            this.asyncMutex = new SemaphoreSlim(1); // only 1 thread can access CS.
            this.isStateful = new SynchronizedModifyOnce<bool>();
            this.partitionCount = new SynchronizedModifyOnce<int>();
            this.targetReplicaSetSize = new SynchronizedModifyOnce<int>();

            this.TestContext = testContext;
            this.serviceName = serviceName;
            this.isSystemService = serviceName.OriginalString.StartsWith(SystemApplicationName);
            this.checkFlags = checkFlags;
            this.operationTimeout = operationTimeout;
            this.requestTimeout = requestTimeout;
        }

        private FabricTestContext TestContext
        {
            get;
            set;
        }

        public async Task<ServicePartitionList> GetPartitionsAsync(CancellationToken ct)
        {
            ReleaseAssert.AssertIfNull(FabricClientRetryErrors.GetPartitionListFabricErrors.Value, "partition list error code");

            var retryableErrors = new FabricClientRetryErrors();
            retryableErrors.RetryableFabricErrorCodes.AddRangeNullSafe(FabricClientRetryErrors.GetPartitionListFabricErrors.Value.RetryableFabricErrorCodes);
            retryableErrors.RetryableExceptions.AddRangeNullSafe(FabricClientRetryErrors.GetPartitionListFabricErrors.Value.RetryableExceptions);

            retryableErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.PartitionNotFound);

            ServicePartitionList servicePartitionList = new ServicePartitionList();
            string continuationToken = null;

            do
            {
                ServicePartitionList queryResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                        () =>
                                                        this.TestContext.FabricClient.QueryManager.GetPartitionListAsync(
                                                            this.serviceName,
                                                            null,
                                                            continuationToken,
                                                            this.requestTimeout,
                                                            ct),
                                                        retryableErrors,
                                                        this.operationTimeout,
                                                        ct).ConfigureAwait(false);

                servicePartitionList.AddRangeNullSafe(queryResult);
                continuationToken = queryResult.ContinuationToken;

            } while ( !string.IsNullOrEmpty(continuationToken) );

            return servicePartitionList;
        }

        public async Task<ServiceReplicaList> GetReplicasAsync(Guid partitionId, CancellationToken ct)
        {
            ServiceReplicaList serviceReplicaList = new ServiceReplicaList();
            string continuationToken = null;

            do
            {
                ServiceReplicaList queryResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                                        () =>
                                                        this.TestContext.FabricClient.QueryManager.GetReplicaListAsync(
                                                            partitionId,
                                                            0,
                                                            ServiceReplicaStatusFilter.Default,
                                                            continuationToken,
                                                            this.requestTimeout,
                                                            ct),
                                                        this.operationTimeout,
                                                        ct).ConfigureAwait(false);

                serviceReplicaList.AddRangeNullSafe(queryResult);
                continuationToken = queryResult.ContinuationToken;

            } while ( !string.IsNullOrEmpty(continuationToken) );

            return serviceReplicaList;
        }

        public async Task ValidateHealthAsync(TimeSpan maximumStabilizationTimeout, CancellationToken ct)
        {
            await this.ValidateHealthAsync(maximumStabilizationTimeout, RetryWaitTimeoutDefault, ct);
        }

        public async Task ValidateHealthAsync(TimeSpan maximumStabilizationTimeout, TimeSpan retryWait, CancellationToken ct)
        {
            var report = await this.ValidateHealthWithReportAsync(
                                                        maximumStabilizationTimeout,
                                                        retryWait,
                                                        ct).ConfigureAwait(false);

            if (report.ValidationFailed)
            {
                throw new FabricValidationException(StringHelper.Format(
                                                                        StringResources.Error_ServiceNotHealthy,
                                                                        this.serviceName,
                                                                        maximumStabilizationTimeout,
                                                                        report.FailureReason));
            }
        }

        public async Task<ValidationReport> ValidateHealthWithReportAsync(TimeSpan maximumStabilizationTimeout, TimeSpan retryWait, CancellationToken ct)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Validating that '{0}' is healthy with timeout '{1}'.", this.serviceName, maximumStabilizationTimeout);

            TimeoutHelper timer = new TimeoutHelper(maximumStabilizationTimeout);
            bool success = false;
            string healthinfo = string.Empty;
            int retryCount = 1;
            while (!success && timer.GetRemainingTime() > TimeSpan.Zero)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "ValidateHealthWithReportAsync(): retryCount='{0}', timer.GetRemainingTime()='{1}'", retryCount, timer.GetRemainingTime());

                healthinfo = string.Empty;

                if (this.TestContext == null)
                {
                    Console.WriteLine("testcontext is null");
                }

                ReleaseAssert.AssertIfNull(this.TestContext, "test context");
                ReleaseAssert.AssertIfNull(this.serviceName, "serviceName");
                ReleaseAssert.AssertIfNull(FabricClientRetryErrors.GetEntityHealthFabricErrors.Value, "health error code");

                ApplicationHealthPolicy healthPolicy = new ApplicationHealthPolicy();

                var serviceHealthResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => 
                    this.TestContext.FabricClient.HealthManager.GetServiceHealthAsync(
                        this.serviceName, 
                        healthPolicy, 
                        this.requestTimeout, 
                        ct),
                    FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                    timer.GetRemainingTime(),
                    ct).ConfigureAwait(false);

                bool checkError = (this.checkFlags & ValidationCheckFlag.CheckError) != 0;
                bool checkWarning = (this.checkFlags & ValidationCheckFlag.CheckWarning) != 0;

                if ((checkError && serviceHealthResult.AggregatedHealthState == HealthState.Error) ||
                    (checkWarning && serviceHealthResult.AggregatedHealthState == HealthState.Warning) ||
                    serviceHealthResult.AggregatedHealthState == HealthState.Invalid ||
                    serviceHealthResult.AggregatedHealthState == HealthState.Unknown)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "{0} is health state is {1}. Will Retry check", this.serviceName, serviceHealthResult.AggregatedHealthState);
                    healthinfo = await this.GetUnhealthyItemsAsync(serviceHealthResult, timer, ct).ConfigureAwait(false);
                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, healthinfo);
                }
                else
                {
                    success = true;
                }

                if (!success)
                {
                    if (retryCount % 10 == 0)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceSource, "Service {0} health validation failed due to issues below, will retry: \n{1}", this.serviceName, healthinfo);
                    }

                    // Delay before querying again so we allow some time for state to change - don't spam the node
                    await AsyncWaiter.WaitAsync(retryWait);
                }

                retryCount++;
            }

            if (!success)
            {
                return new ValidationReport(true, StringHelper.Format(StringResources.Error_ServiceNotHealthy, serviceName, maximumStabilizationTimeout, healthinfo));
            }
            else
            {
                return ValidationReport.Default;
            }
        }

        private async Task<string> GetUnhealthyItemsAsync(ServiceHealth serviceHealth, TimeoutHelper timer, CancellationToken ct)
        {
            StringBuilder healthinfo = new StringBuilder();
            foreach (var serviceHealthEvent in serviceHealth.HealthEvents)
            {
                if (serviceHealthEvent.HealthInformation.HealthState == HealthState.Ok)
                {
                    continue;
                }

                healthinfo.AppendLine(StringHelper.Format(
                    "Service {0} health state is '{1}' with property '{2}', sourceId '{3}' and description '{4}'",
                    this.serviceName,
                    serviceHealthEvent.HealthInformation.HealthState,
                    serviceHealthEvent.HealthInformation.Property,
                    serviceHealthEvent.HealthInformation.SourceId,
                    serviceHealthEvent.HealthInformation.Description));
            }

            foreach (var partitionHealthState in serviceHealth.PartitionHealthStates)
            {
                if (partitionHealthState.AggregatedHealthState == HealthState.Ok)
                {
                    continue;
                }

                var partitionHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () =>
                    this.TestContext.FabricClient.HealthManager.GetPartitionHealthAsync(
                        partitionHealthState.PartitionId,
                        this.requestTimeout,
                        ct),
                    FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                    timer.GetRemainingTime(),
                    ct).ConfigureAwait(false);

                foreach (var partitionHealthEvent in partitionHealth.HealthEvents)
                {
                    if (partitionHealthEvent.HealthInformation.HealthState == HealthState.Ok)
                    {
                        continue;
                    }

                    healthinfo.AppendLine(StringHelper.Format(
                        "Service {0}:{1} health state is '{2}' with property '{3}', sourceId '{4}' and description '{5}'",
                        this.serviceName,
                        partitionHealth.PartitionId,
                        partitionHealthEvent.HealthInformation.HealthState,
                        partitionHealthEvent.HealthInformation.Property,
                        partitionHealthEvent.HealthInformation.SourceId,
                        partitionHealthEvent.HealthInformation.Description));
                }

                foreach (var replicaHealthState in partitionHealth.ReplicaHealthStates)
                {
                    if (replicaHealthState.AggregatedHealthState == HealthState.Ok)
                    {
                        continue;
                    }

                    var replicaHealth = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () =>
                        this.TestContext.FabricClient.HealthManager.GetReplicaHealthAsync(
                            replicaHealthState.PartitionId,
                            replicaHealthState.Id,
                            this.requestTimeout,
                            ct),
                        FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                        timer.GetRemainingTime(),
                        ct).ConfigureAwait(false);

                    foreach (var replicaHealthEvent in replicaHealth.HealthEvents)
                    {
                        if (replicaHealthEvent.HealthInformation.HealthState == HealthState.Ok)
                        {
                            continue;
                        }

                        healthinfo.AppendLine(StringHelper.Format(
                            "Service {0}:{1}:{2} health state is '{3}' with property '{4}', sourceId '{5}' and description '{6}'",
                            this.serviceName,
                            replicaHealth.PartitionId,
                            replicaHealth.Id,
                            replicaHealthEvent.HealthInformation.HealthState,
                            replicaHealthEvent.HealthInformation.Property,
                            replicaHealthEvent.HealthInformation.SourceId,
                            replicaHealthEvent.HealthInformation.Description));
                    }
                }
            }

            return healthinfo.ToString();
        }

        public async Task EnsureStabilityAsync(TimeSpan maximumStabilizationTimeout, CancellationToken ct)
        {
            await this.EnsureStabilityAsync(maximumStabilizationTimeout, RetryWaitTimeoutDefault, ct);
        }

        public async Task EnsureStabilityAsync(
            TimeSpan maximumStabilizationTimeout,
            TimeSpan retryWait,
            CancellationToken ct)
        {
            var report = await this.EnsureStabilityWithReportAsync(
                                                        maximumStabilizationTimeout,
                                                        retryWait,
                                                        ct).ConfigureAwait(false);

            if (report.ValidationFailed)
            {
                throw new FabricValidationException(StringHelper.Format(
                                                                        StringResources.Error_ServiceNotStable,
                                                                            this.serviceName,
                                                                            maximumStabilizationTimeout,
                                                                            report.FailureReason));
            }
        }

        public async Task<ValidationReport> EnsureStabilityWithReportAsync(TimeSpan maximumStabilizationTimeout, TimeSpan retryWait, CancellationToken ct)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Ensuring that '{0}' is online with timeout '{1}'.", this.serviceName, maximumStabilizationTimeout);

            bool checkQuorumLoss = (this.checkFlags & ValidationCheckFlag.CheckQuorumLoss) != 0;

            // Load basic information about this service.
            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "Querying basic information for {0}.", this.serviceName);
            await this.LoadPartitionAndReplicaCountAsync(ct);

            DateTime startTime = DateTime.Now;
            TimeoutHelper timer = new TimeoutHelper(maximumStabilizationTimeout);
            bool success = false;
            
            List<Guid> partitionsInQuorumLoss = new List<Guid>();
            StringBuilder errorString = new StringBuilder();
            int retryCount = 1;
            while (!success && timer.GetRemainingTime() > TimeSpan.Zero)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "EnsureStabilityWithReportAsync(): retryCount='{0}', timer.GetRemainingTime()='{1}'", retryCount, timer.GetRemainingTime());

                var nodes = await this.TestContext.FabricCluster.GetLatestNodeInfoAsync(this.requestTimeout, this.operationTimeout, ct);

                // Empty error string and list of partitions in quorum loss
                partitionsInQuorumLoss.Clear();
                errorString.Clear();
                
                success = true;
                int totalPartitionsFound = 0;

                bool stateful;
                ReleaseAssert.AssertIfNot(this.isStateful.TryGetValue(out stateful), "isStateful flag is not available");
                bool checkTarget = (this.checkFlags & ValidationCheckFlag.CheckTargetReplicaSetSize) != 0;
                bool checkInBuild = (this.checkFlags & ValidationCheckFlag.CheckInBuildReplica) != 0;

                if (stateful)
                {
                    var partitionDictionary = await this.QueryPartitionAndReplicaResultAsyncStateful(ct);
                    totalPartitionsFound = partitionDictionary.Count();

                    foreach (KeyValuePair<Partition, StatefulServiceReplica[]> partition in partitionDictionary)
                    {
                        bool partitionIsReady = partition.Key.PartitionStatus == ServicePartitionStatus.Ready;
                        if (!partitionIsReady)
                        {
                            var message = StringHelper.Format("Partition '{0}' is not Ready", partition.Key.PartitionId());
                            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "{0}", message);
                            errorString.AppendLine(message);
                        }

                        if (partition.Key.PartitionStatus != ServicePartitionStatus.InQuorumLoss)
                        {
                            int validCount = 0;
                            int inBuildReplicas = 0;
                            foreach (StatefulServiceReplica replica in partition.Value)
                            {
                                if (replica.ReplicaStatus == ServiceReplicaStatus.Ready 
                                    && (replica.ReplicaRole == ReplicaRole.Primary || replica.ReplicaRole == ReplicaRole.ActiveSecondary))
                                {
                                    ++validCount;
                                }

                                if (replica.ReplicaStatus == ServiceReplicaStatus.InBuild)
                                {
                                    ++inBuildReplicas;
                                    var message = StringHelper.Format("Replica {0} for partition '{1}' is InBuild", replica.Id, partition.Key.PartitionId());
                                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "{0}", message);
                                    errorString.AppendLine(message);
                                }
                            }

                            bool targetAchieved = this.CheckReplicaSetSize(partition.Key.PartitionInformation.Id, validCount, startTime, nodes, errorString);
                            if (!partitionIsReady ||
                                (checkInBuild && inBuildReplicas > 0) ||
                                (checkTarget && !targetAchieved))
                            {
                                success = false;
                            }
                        }
                        else
                        {
                            partitionsInQuorumLoss.Add(partition.Key.PartitionInformation.Id);
                        }
                    }
                }
                else
                {
                    int targetInstanceCount = 0;
                    ReleaseAssert.AssertIf(!this.targetReplicaSetSize.TryGetValue(out targetInstanceCount), "targetReplicaSetSize for service: {0} should have been populated at this point.", this.serviceName);

                    bool placementConstraintsDefined = false;
                    try
                    {
                        // Get the service description to find out if there are placement constraints on the service
                        ServiceDescription result = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.TestContext.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                                this.serviceName,
                                this.requestTimeout,
                                ct),
                            this.operationTimeout,
                            ct).ConfigureAwait(false);

                        ThrowIf.IsTrue(result == null, "A description must be associated with the service: {0}", this.serviceName);

                        placementConstraintsDefined = !string.IsNullOrEmpty(result.PlacementConstraints);
                    }
                    catch (UnauthorizedAccessException)
                    {
                        ServiceGroupDescription groupDescription = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => this.TestContext.FabricClient.ServiceGroupManager.GetServiceGroupDescriptionAsync(
                                this.serviceName,
                                this.requestTimeout,
                                ct),
                            this.operationTimeout,
                            ct).ConfigureAwait(false);

                        ThrowIf.IsTrue(groupDescription == null, "A description must be associated with the service group: {0}", this.serviceName);

                        placementConstraintsDefined = !string.IsNullOrEmpty(groupDescription.ServiceDescription.PlacementConstraints);
                    }

                    // If a stateless service has instance count == -1 and it has placement constraints such
                    // that the possible number of instances cannot match the total number of nodes,
                    // we need to find out the number of eligible nodes for the service which is tracked by RDBug 8993319.
                    // Until RDBug 8993319 is fixed, we take the presence of placement constraints into consideration to make the
                    // validation more accurate.
                    if (targetInstanceCount == -1 && placementConstraintsDefined)
                    {
                        checkTarget = false;
                    }

                    var partitionDictionary = await this.QueryPartitionAndReplicaResultAsyncStateless(timer.GetRemainingTime(), ct);
                    totalPartitionsFound = partitionDictionary.Count();

                    foreach (KeyValuePair<Partition, StatelessServiceInstance[]> partition in partitionDictionary)
                    {
                        bool partitionIsReady = partition.Key.PartitionStatus == ServicePartitionStatus.Ready;
                        if (!partitionIsReady)
                        {
                            var message = StringHelper.Format("Partition '{0}' is not Ready", partition.Key.PartitionId());
                            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "{0}", message);
                            errorString.AppendLine(message);
                        }

                        int validCount = 0;
                        foreach (StatelessServiceInstance instance in partition.Value)
                        {
                            if (instance.ReplicaStatus == ServiceReplicaStatus.Ready)
                            {
                                ++validCount;
                            }
                        }

                        bool targetAchieved = this.CheckReplicaSetSize(partition.Key.PartitionInformation.Id, validCount, startTime, nodes, errorString);
                        if (!partitionIsReady ||
                            (checkTarget && !targetAchieved))
                        {
                            success = false;
                        }
                    }
                }

                if (!this.ValidatePartitionCount(totalPartitionsFound))
                {
                    success = false;
                }

                if (partitionsInQuorumLoss.Count > 0 && checkQuorumLoss)
                {
                    string paritionIds = string.Join(",", partitionsInQuorumLoss.ToArray());
                    var message = StringHelper.Format("Partitions '{0}' in quorum loss for service {1}", paritionIds, this.serviceName);
                    TestabilityTrace.TraceSource.WriteInfo(TraceSource, "{0}", message);
                    errorString.AppendLine(message);
                    success = false;
                }

                if (!success)
                {
                    if(retryCount % 10 == 0)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceSource, "Service {0} validation failed due to issues below, will retry: \n{1}", this.serviceName, errorString);
                    }

                    // Delay before querying again so we allow some time for state to change - don't spam the node
                    await AsyncWaiter.WaitAsync(retryWait, ct).ConfigureAwait(false);
                }

                retryCount++;
            }

            if (partitionsInQuorumLoss.Count > 0)
            {
                string partitionIds = string.Join(",", partitionsInQuorumLoss.ToArray());
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Partitions in quorum loss for service {0} are '{1}'", this.serviceName, partitionIds);

                if (checkQuorumLoss)
                {
                    throw new FabricValidationException(StringHelper.Format(StringResources.Error_PartitionsInQuorumLoss, partitionIds, this.serviceName));
                }
            }

            if (!success)
            {
                return new ValidationReport(
                                            true,
                                            StringHelper.Format(StringResources.Error_ServiceNotStable, this.serviceName, maximumStabilizationTimeout, errorString));
            }
            else
            {
                return ValidationReport.Default;
            }
        }

        public async Task<Dictionary<Partition, StatefulServiceReplica[]>> QueryPartitionAndReplicaResultAsyncStateful(CancellationToken ct)
        {
            var servicePartitionMap = await this.QueryLocationsAsync(ct).ConfigureAwait(false);

            var allServiceReplicas =
                new Dictionary<Partition, StatefulServiceReplica[]>();
            foreach (var partition in servicePartitionMap)
            {
                List<StatefulServiceReplica> statefulReplicas = new List<StatefulServiceReplica>();
                foreach (Replica replica in partition.Value)
                {
                    StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                    ReleaseAssert.AssertIf(statefulReplica == null, "Replica {0} should be of type stateful for Partition {1}", replica.Id, partition.Key.PartitionId());

                    statefulReplicas.Add(statefulReplica);
                }

                allServiceReplicas.Add(partition.Key, statefulReplicas.ToArray());
            }

            return allServiceReplicas;
        }

        public async Task<Dictionary<Partition, StatelessServiceInstance[]>> QueryPartitionAndReplicaResultAsyncStateless(TimeSpan timeout, CancellationToken ct)
        {
            Dictionary<Partition, Replica[]> instancesMap = await this.QueryLocationsAsync(ct).ConfigureAwait(false);

            var allServiceInstances =
                new Dictionary<Partition, StatelessServiceInstance[]>();
            foreach (var partition in instancesMap)
            {
                var statelessInstances = new List<StatelessServiceInstance>();
                foreach (Replica instance in partition.Value)
                {
                    StatelessServiceInstance statelessInstance = instance as StatelessServiceInstance;
                    ReleaseAssert.AssertIf(statelessInstance == null, "Instance {0} should be of type stateless for Partition {1}", instance.Id, partition.Key.PartitionId());
                    statelessInstances.Add(statelessInstance);
                }

                allServiceInstances.Add(partition.Key, statelessInstances.ToArray());
            }

            return allServiceInstances;
        }

        public async Task<Dictionary<Partition, Replica[]>> QueryLocationsAsync(CancellationToken ct)
        {
            ServicePartitionList servicePartitionsResult = await this.GetPartitionsAsync(ct).ConfigureAwait(false);

            var allServiceReplicas =
                new Dictionary<Partition, Replica[]>();

            foreach (var partition in servicePartitionsResult)
            {
                List<Replica> serviceReplicas = new List<Replica>();
                ServiceReplicaList serviceReplicaResult = await this.GetReplicasAsync(partition.PartitionInformation.Id, ct).ConfigureAwait(false);

                serviceReplicas.AddRange(serviceReplicaResult);
                allServiceReplicas.Add(partition, serviceReplicas.ToArray());
            }

            return allServiceReplicas;
        }

        private int GetExpectedReplicaSetSize(IEnumerable<NodeInfo> nodes)
        {
            int replicaCount;
            // Assert
            ReleaseAssert.AssertIfNot(this.targetReplicaSetSize.TryGetValue(out replicaCount), "TargetReplicaSetSize is null");

            int upNodeCount = nodes.Count(n => n.IsNodeUp);
            if (replicaCount == -1)
            {
                // This can only be the case for stateless service and this means we want to place on all nodes
                replicaCount = upNodeCount;
            }

            // Return min of nodes or replica count i.e. if we have 3 nodes and replica count of 5 we will only be able 
            // to place 3/5 replicas and the check below will handle this case
            return nodes.Any() ? Math.Min(upNodeCount, replicaCount) : replicaCount;
        }

        private bool CheckReplicaSetSize(Guid partitionId, int actual, DateTime startTime, IEnumerable<NodeInfo> nodes, StringBuilder errorString)
        {
            int expectedReplicaSetSize = this.GetExpectedReplicaSetSize(nodes);
            bool success = actual >= expectedReplicaSetSize;
            if (!success)
            {
                var message = StringHelper.Format("{0}:{1} is not yet stable since it has {2}/{3} replicas online", this.serviceName, partitionId, actual, expectedReplicaSetSize);
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "{0}", message);
                errorString.AppendLine(message);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Validated that {0}:{1} has {2} online replicas. Took '{3}' to stabilize", this.serviceName, partitionId, actual, DateTime.Now - startTime);
            }

            return success;
        }

        private bool ValidatePartitionCount(int totalPartitionsFound)
        {
            int expectedPartitionCount;

            // Assert
            ReleaseAssert.AssertIfNot(this.partitionCount.TryGetValue(out expectedPartitionCount), "ParitionCount is null.");
            
            if (expectedPartitionCount != totalPartitionsFound)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Found only {0}/{1} Partitions for service {2}", totalPartitionsFound, expectedPartitionCount, this.serviceName);
                return false;
            }

            // Success
            TestabilityTrace.TraceSource.WriteInfo(TraceSource, "Validated that service '{0}' has {1} stable partitions.", this.serviceName, totalPartitionsFound);
            return true;
        }

        // Running multiple times should be safe.
        private async Task LoadPartitionAndReplicaCountAsync(CancellationToken ct)
        {
            // Optimization. No need for more than one thread to enter this method.
            await this.asyncMutex.WaitAsync();
            try
            {
                if (this.isStateful.HasValue && this.partitionCount.HasValue && this.targetReplicaSetSize.HasValue)
                {
                    // values already loaded
                    return;
                }

                ServicePartitionList servicePartitions = await this.GetPartitionsAsync(ct).ConfigureAwait(false);

                // Make sure servicePartitions has at least one item.
                ThrowIf.NullOrEmpty(servicePartitions, "servicePartitions");

                // set PartitionCount
                ReleaseAssert.AssertIfNot(this.partitionCount.TrySetValue(servicePartitions.Count), "partitionCount has already been set to a different value");

                // set isStateful field
                Partition partition = servicePartitions[0];
                bool stateful = partition is StatefulServicePartition;
                ReleaseAssert.AssertIfNot(this.isStateful.TrySetValue(stateful), "isStateful has already been set to a different value");

                // retrieve replicaCount
                if (stateful)
                {
                    var statefulServicePartition = partition as StatefulServicePartition;
                    ReleaseAssert.AssertIfNot(this.targetReplicaSetSize.TrySetValue((int)statefulServicePartition.TargetReplicaSetSize), "targetReplicaSetSize has already been set to a different value");
                }
                else
                {
                    var statelessServicePartition = partition as StatelessServicePartition;
                    ReleaseAssert.AssertIfNot(this.targetReplicaSetSize.TrySetValue((int)statelessServicePartition.InstanceCount), "targetReplicaSetSize has already been set to a different value");
                }
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceSource, "Error while getting partitions for service {0}. Exception: {1}", this.serviceName, e.Message);
                throw;
            }
            finally
            {
                this.asyncMutex.Release();
            }
        }

        /// Synchronization wrapper around struct type 'T'. 
        /// Read/Write access are serialized.
        /// Stored value can be "set" (changed) only once.
        private class SynchronizedModifyOnce<T> where T : struct, IEquatable<T>
        {
            private T storedValue;
            private bool isSet;
            private readonly object accessLock;

            public SynchronizedModifyOnce()
            {
                this.accessLock = new object();
                this.isSet = false;
            }

            public bool HasValue
            {
                get
                {
                    return this.IsValueSet();
                }
            }

            // Returns true if value was ever set.
            private bool IsValueSet()
            {
                bool success;
                lock (this.accessLock)
                {
                    success = this.isSet;
                }

                return success;
            }

            // get storedValue via out parameter.
            // Returns true if value was ever set.
            public bool TryGetValue(out T outVal)
            {
                bool success;
                lock (this.accessLock)
                {
                    outVal = storedValue;
                    success = this.isSet;
                }

                return success;
            }

            // set storedValue to newVal iff it was not set.
            // Returns true iff "storedValue" is equals to parameter "newVal" by the end of this method.
            private bool TrySetValue(T newVal, out T oldVal)
            {
                bool success;
                lock (this.accessLock)
                {
                    oldVal = this.storedValue;

                    if (!this.isSet)
                    {
                        // Set the value and set "isSet" to true
                        this.storedValue = newVal;
                        this.isSet = true;
                    }

                    success = this.storedValue.Equals(newVal);
                }

                return success;
            }

            // Returns true iff "storedValue" is equals to input argument "newVal" by the end of this method.
            public bool TrySetValue(T newVal)
            {
                T temp;
                return this.TrySetValue(newVal, out temp);
            }
        }
    }
}