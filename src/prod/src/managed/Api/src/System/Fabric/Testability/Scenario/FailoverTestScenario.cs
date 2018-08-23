// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Scenario
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// The FailoverTestScenario is a test which runs a series of faults against a specific partition defined by the PartitionSelector in the FailoverTestScenarioParameters.
    /// </summary>
    /// <remarks>
    /// The faults induced put the partition through some specific failover scenarios to ensure those paths are tested and exercised. Running your workload against the service
    /// at the same time as the test being run will increase the chances of inducing and discovering bugs with the service. The faults induced for the
    /// Primary, Secondaries and stateless instances are RestartReplica(only persisted), RemoveReplica, ResartDeployedCodePackage, MovePrimary (only stateful),
    /// MoveSecondary (Only stateful), RestartPartition (no data loss).
    /// </remarks>
    [Obsolete("This class is deprecated.  Please use Chaos instead https://docs.microsoft.com/azure/service-fabric/service-fabric-controlled-chaos")]
    public sealed class FailoverTestScenario : TestScenario
    {
        private FailoverTestScenarioParameters failoverTestScenarioParameters;
        private ServiceDescription serviceDescription;

        private const string TraceType = "FailoverTestScenario";

        /// <summary>
        /// Constructor for the FailoverTestScenario.
        /// </summary>
        /// <param name="fabricClient">FabricClient object which will be used to connect to the cluster and induce the faults.</param>
        /// <param name="testScenarioParameters">FailoverTestScenarioParameters which define the configuration for the failover test.</param>
        public FailoverTestScenario(FabricClient fabricClient, FailoverTestScenarioParameters testScenarioParameters)
            : base(fabricClient, testScenarioParameters)
        {
            this.failoverTestScenarioParameters = testScenarioParameters;
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="token">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <returns></returns>
        protected override async Task OnExecuteAsync(CancellationToken token)
        {
            this.serviceDescription = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                    this.failoverTestScenarioParameters.PartitionSelector.ServiceName, 
                    this.failoverTestScenarioParameters.RequestTimeout,
                    token), 
                this.failoverTestScenarioParameters.OperationTimeout,
                token).ConfigureAwait(false); 

            bool hasPersistedState = false;
            if (this.serviceDescription.IsStateful())
            {
                StatefulServiceDescription statefulDescription = this.serviceDescription as StatefulServiceDescription;
                ReleaseAssert.AssertIf(statefulDescription == null, "Stateful service description is not WinFabricStatefulServiceDescription");
                hasPersistedState = statefulDescription.HasPersistedState;
            }

            Log.WriteInfo(TraceType, "Validating Service health and availability");
            await this.FabricClient.TestManager.ValidateServiceAsync(
                this.failoverTestScenarioParameters.PartitionSelector.ServiceName,
                this.failoverTestScenarioParameters.MaxServiceStabilizationTimeout,
                token);

            Log.WriteInfo(TraceType, "Getting Selected Partition");
            var getPartitionStateAction = new GetSelectedPartitionStateAction(this.failoverTestScenarioParameters.PartitionSelector)
            {
                RequestTimeout = this.failoverTestScenarioParameters.RequestTimeout,
                ActionTimeout = this.failoverTestScenarioParameters.OperationTimeout
            };

            await this.TestContext.ActionExecutor.RunAsync(getPartitionStateAction, token);
            Guid selectedPartitionId = getPartitionStateAction.Result.PartitionId;
            Log.WriteInfo(TraceType, "Running test for partition {0}", selectedPartitionId);

            this.ReportProgress("Selected partition {0} for testing failover", selectedPartitionId);

            PartitionSelector selectedPartition = PartitionSelector.PartitionIdOf(this.failoverTestScenarioParameters.PartitionSelector.ServiceName, selectedPartitionId);

            while (this.failoverTestScenarioParameters.TimeToRun - this.GetElapsedTime() > TimeSpan.Zero && !token.IsCancellationRequested)
            {
                if (this.serviceDescription.IsStateful())
                {
                    ReplicaSelector primaryReplicaSelector = ReplicaSelector.PrimaryOf(selectedPartition);
                    ReplicaSelector secondaryReplicaSelector = ReplicaSelector.RandomSecondaryOf(selectedPartition);

                    // Make Primary go through RemoveReplica, RestartReplica and RestartCodePackage

                    await this.TestReplicaFaultsAsync(primaryReplicaSelector, "Primary", hasPersistedState, token);

                    // Make Secondary go through RemoveReplica, RestartReplica and RestartCodePackage

                    await this.TestReplicaFaultsAsync(secondaryReplicaSelector, "Secondary", hasPersistedState, token);
                }
                else
                {
                    ReplicaSelector randomInstanceSelector = ReplicaSelector.RandomOf(selectedPartition);

                    // Make Stateless Instance go through RemoveReplica, RestartReplica and RestartCodePackage

                    await this.TestReplicaFaultsAsync(randomInstanceSelector, "Stateless Instance", hasPersistedState, token);
                }

                if (this.serviceDescription.IsStateful())
                {
                    // Restart all secondary replicas and make sure the replica set recovers

                    await this.InvokeAndValidateFaultAsync(
                        "Restarting all the secondary replicas",
                        () =>
                        {
#pragma warning disable 618
                            return this.FabricClient.TestManager.RestartPartitionAsync(
                                selectedPartition,
                                RestartPartitionMode.OnlyActiveSecondaries,
                                this.failoverTestScenarioParameters.OperationTimeout,
                                token);
#pragma warning restore 618
                        }, token);

                    // Restart all replicas if service is persisted

                    if (hasPersistedState)
                    {
                        await this.InvokeAndValidateFaultAsync(
                            "Restarting all replicas including Primary",
                            () =>
                            {
#pragma warning disable 618
                                return this.FabricClient.TestManager.RestartPartitionAsync(
                                    selectedPartition,
                                    RestartPartitionMode.AllReplicasOrInstances,
                                    this.failoverTestScenarioParameters.OperationTimeout,
                                    token);
#pragma warning restore 618
                            }, token);
                    }

                    // Induce move and swap primary a few times

                    await this.InvokeAndValidateFaultAsync(
                        "Move Primary to a different node",
                        () =>
                        {
                            return this.FabricClient.FaultManager.MovePrimaryAsync(
                                string.Empty,
                                selectedPartition,
                                true,
                                this.failoverTestScenarioParameters.OperationTimeout,
                                token);
                        }, token);

                    // Induce move secondary a few times

                    await this.InvokeAndValidateFaultAsync(
                        "Move Secondary to a different node",
                        () =>
                        {
                            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                                string.Empty,
                                string.Empty,
                                selectedPartition,
                                true,
                                this.failoverTestScenarioParameters.OperationTimeout,
                                token);
                        }, token);
                }
                else
                {
                    // Restart all stateless instances

                    await this.InvokeAndValidateFaultAsync(
                        "Restarting all stateless instances for partition",
                        () =>
                        {
#pragma warning disable 618
                            return this.FabricClient.TestManager.RestartPartitionAsync(
                                selectedPartition,
                                RestartPartitionMode.AllReplicasOrInstances,
                                this.failoverTestScenarioParameters.OperationTimeout,
                                token);
#pragma warning restore 618
                        }, token);
                }
            }
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="disposing">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected override void OnDispose(bool disposing)
        {
            // Do nothing
        }

        private async Task TestReplicaFaultsAsync(
            ReplicaSelector replicaSelector,
            string replicaRole,
            bool hasPersistedState,
            CancellationToken token)
        {
            await this.InvokeAndValidateFaultAsync(
                StringHelper.Format("Removing replica state for {0}", replicaRole),
                () =>
                {
                    return this.FabricClient.FaultManager.RemoveReplicaAsync(
                        replicaSelector,
                        CompletionMode.Verify,
                        false,
                        this.failoverTestScenarioParameters.OperationTimeout,
                        token);
                }, token);

            if (hasPersistedState)
            {
                await this.InvokeAndValidateFaultAsync(
                    StringHelper.Format("Restarting replica state for {0}", replicaRole),
                    () =>
                    {
                        return this.FabricClient.FaultManager.RestartReplicaAsync(
                            replicaSelector,
                            CompletionMode.Verify,
                            this.failoverTestScenarioParameters.OperationTimeout,
                            token);
                    }, token);
            }

            await this.InvokeAndValidateFaultAsync(
                StringHelper.Format("Restarting code package for {0} replica", replicaRole),
                () =>
                {
                    return this.FabricClient.FaultManager.RestartDeployedCodePackageAsync(
                        this.serviceDescription.ApplicationName,
                        replicaSelector,
                        CompletionMode.Verify,
                        this.failoverTestScenarioParameters.OperationTimeout,
                        token);
                }, token);
        }

        private async Task InvokeAndValidateFaultAsync(
            string actionText,
            Func<Task> action,
            CancellationToken token)
        {
            for (int i = 0; i < 5; i++)
            {
                if (this.failoverTestScenarioParameters.TimeToRun - this.GetElapsedTime() > TimeSpan.Zero && !token.IsCancellationRequested)
                {
                    this.ReportProgress("Executing FailoverScenario fault - '{0}'", actionText);

                    Log.WriteInfo(TraceType, "Testing Failover Scenario by '{0}'. Iteration {1}", actionText, i);
                    Task testActionTask = action().ContinueWith(t =>
                    {
                        this.HandleTaskComplete(t, "FaultAction", actionText);
                    });

                    await testActionTask;

                    Log.WriteInfo(TraceType, "Validating Service health and availability after '{0}'", actionText);
                    await this.FabricClient.TestManager.ValidateServiceAsync(
                        this.failoverTestScenarioParameters.PartitionSelector.ServiceName,
                        this.failoverTestScenarioParameters.MaxServiceStabilizationTimeout,
                        token).ContinueWith(t =>
                        {
                            this.HandleTaskComplete(t, "ValidateServiceAsync", actionText);
                        });

                    Log.WriteInfo(TraceType, "Waiting {0} seconds before next fault", this.failoverTestScenarioParameters.WaitTimeBetweenFaults.TotalSeconds);
                    await AsyncWaiter.WaitAsync(this.failoverTestScenarioParameters.WaitTimeBetweenFaults, token);
                }
            }
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="token">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <returns></returns>
        protected override async Task ValidateScenarioAtExitAsync(CancellationToken token)
        {
            await this.FabricClient.TestManager.ValidateServiceAsync(
                this.failoverTestScenarioParameters.PartitionSelector.ServiceName,
                this.failoverTestScenarioParameters.MaxServiceStabilizationTimeout,
                token).ContinueWith(t =>
                {
                    this.HandleTaskComplete(t, "ValidateServiceAsync", "ValidateScenarioAtExitAsync");
                });
        }
    }
}