// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Scenario
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Chaos.RandomActionGenerator;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// The ChaosTestScenario is a long running scenario which keeps inducing failover and faults into the cluster up until the TimetoRun has expired.
    /// </summary>
    /// <remarks>
    /// The test will induce up to 'maxConcurrentFaults' every iteration and then will validate the health and availability of all the services in the cluster before
    /// moving on to the next iteration of faults. If at any point the services are not available or healthy by 'maxClusterStabilizationTimeout' the test will fail with a 
    /// FabricValidationException. For every iteration the concurrent faults that are induced into the system ensure safety such that the faults together will not cause any service to
    /// become unavailable or lose any data. This assumes no faults induced from the outside or any unexpected failures which if they happen (concurrent with the chaos test faults)
    /// can cause data availability loss. This is a very good test to run against your test or staging clusters which test workloads are running to ensure that faults in the system do not 
    /// result in any sort of availability loss or other unexpected service issues.
    /// </remarks>
    [Obsolete("This class is deprecated. To manage Chaos, please use StartChaosAsync, StopChaosAsync, and GetChaosReportAsync APIs from FabricClient.TestManager instead.")]
    public sealed class ChaosTestScenario : TestScenario
    {
        private static readonly string TraceType = "ChaosTestScenario";

        private ChaosTestScenarioParameters chaosScenarioParameters;
        private RootActionGenerator rootActionGenerator;
        private ActionTranslator actionTranslator;

        private Random random;

        internal ChaosTestScenario(FabricClient fabricClient, string testScenarioName, ChaosTestScenarioParameters chaosScenarioParameters)
            : base(fabricClient, chaosScenarioParameters)
        {
            this.chaosScenarioParameters = chaosScenarioParameters;
            this.actionTranslator = new ActionTranslator(this.ReportProgress);
            this.ScenarioInstanceName = testScenarioName;
            this.random = new Random((int)DateTime.Now.Ticks);
            this.rootActionGenerator = new RootActionGenerator(chaosScenarioParameters.ActionGeneratorParameters, this.random);
        }

        /// <summary>
        /// Constructor for the ChaosTestScenario.
        /// </summary>
        /// <param name="fabricClient">FabricClient object which will be used to connect to the cluster and induce the faults.</param>
        /// <param name="chaosScenarioParameters">ChaosTestScenarioParameters which define the configuration for the chaos test.</param>
        public ChaosTestScenario(FabricClient fabricClient, ChaosTestScenarioParameters chaosScenarioParameters)
            : this(fabricClient, TraceType + DateTime.Now.ToString(CultureInfo.CurrentCulture.DateTimeFormat.ShortTimePattern), chaosScenarioParameters)
        {
        }

        internal string ScenarioInstanceName { get; private set; }

        internal int IterationsCompleted { get; set; }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="token">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <returns></returns>
        protected override async Task OnExecuteAsync(CancellationToken token)
        {
            await this.ExecuteIterationsWithPauseAsync(token);
        }

        private async Task ExecuteIterationsWithPauseAsync(CancellationToken token)
        {
            while (!token.IsCancellationRequested && this.SessionNotDone())
            {
                if (await this.IsClusterReadyForFaultsAsync(token))
                {
                    this.ReportProgress("Ensuring cluster is healthy before proceeding with faults");

                    await this.ValidateClusterAsync(token);

                    this.ReportProgress("Running iteration {0}. Elapsed time so far: {1}", this.IterationsCompleted, this.GetElapsedTime());

                    await this.ExecuteFaultIterationAsync(token);

                    Log.WriteInfo(TraceType, "Pausing for '{0}' before executing next iteration.", this.chaosScenarioParameters.WaitTimeBetweenIterations);
                    await Task.Delay(this.chaosScenarioParameters.WaitTimeBetweenIterations);
                    this.IterationsCompleted++;
                }
            }

            Log.WriteInfo(TraceType, "Session has completed. \nTotal iterations: {0}. Total elapsed time: {1}", this.IterationsCompleted, this.GetElapsedTime());
        }

        /// <summary>
        /// Get test actions for the iteration.
        /// Execute each action and wait for a random wait duration.
        /// </summary>
        private async Task ExecuteFaultIterationAsync(CancellationToken token)
        {
            IList<FabricTestAction> testActionList = await this.GetGeneratedActionsAsync(token);
            ReleaseAssert.AssertIf(testActionList == null, "testActionList should not be null");

            if (testActionList.Count > 0)
            {
                List<Task> tasks = new List<Task>(); ;
                foreach (FabricTestAction testAction in testActionList)
                {
                    // Run one task
                    tasks.Add(this.ExecuteTestActionAsync(testAction, token));
                }

                await Task.WhenAll(tasks);
            }
            else
            {
                // No actions generated we can skip the validation
            }
        }

        private async Task ExecuteTestActionAsync(FabricTestAction testAction, CancellationToken token)
        {
            ReleaseAssert.AssertIf(testAction == null, "testAction should not be null");
            string testActionId = testActionId = testAction.ToString() + testAction.GetHashCode();
            string actionName = testAction.GetType().Name;
            // wait for random time duration.
            int pauseMilliseconds = (int)(this.random.NextDouble() * this.chaosScenarioParameters.WaitTimeBetweenFaults.TotalMilliseconds);
            Log.WriteInfoWithId(TraceType, testActionId, "Pausing for {0} milliseconds before executing next action", pauseMilliseconds);
            await Task.Delay((int)pauseMilliseconds);

            // create an id for action for tracing.
            Log.WriteInfoWithId(TraceType, testActionId, "Executing action: {0}", actionName);

            // Execute this action linked token.
            Task testActionTask = this.TestContext.ActionExecutor.RunAsync(testAction, token).ContinueWith(t =>
            {
                this.HandleTaskComplete(t, testActionId, actionName);
            });

            await testActionTask;

            Log.WriteInfoWithId(TraceType, testActionId, "Action {0} completed.", testAction.GetType().Name);
        }

        private async Task<IList<FabricTestAction>> GetGeneratedActionsAsync(CancellationToken token)
        {
            List<FabricTestAction> testActions = new List<FabricTestAction>();

            Log.WriteNoise(TraceType, "GetGeneratedActions: Getting latest StateSnapshot.");
            ClusterStateSnapshot taskSnapshot = await this.GetStateSnapshotAsync(token);
            if (taskSnapshot != null)
            {
                StringBuilder upgradingApplications = new StringBuilder();
                foreach(var application in taskSnapshot.Applications)
                {
                    if(application.Application.ApplicationStatus == Query.ApplicationStatus.Upgrading)
                    {
                        upgradingApplications.Append(application.Application.ApplicationName + " ");
                    }
                }

                if (upgradingApplications.Length > 0)
                {
                    this.ReportProgress("Skipping faults for following applications due to on going upgrades:\n\t\t{0}", upgradingApplications);
                }

                Log.WriteNoise(TraceType, "GetGeneratedActions: Getting actions from action generator.");
                IList<StateTransitionAction> ragActions = this.rootActionGenerator.GetNextActions(taskSnapshot);

                Log.WriteNoise(TraceType, "GetGeneratedActions: Translating state transition actions to TestabilityAction.");
                var commandMap = this.actionTranslator.GetCommandsMap(ragActions);
                testActions = commandMap.Keys.ToList();
            }
            else
            {
                // GetStateSnapshot must have failed hence we need to retry the entire iteration. 
            }

            return testActions;
        }

        // Gather latest information to create StateSnapshot
        private async Task<ClusterStateSnapshot> GetStateSnapshotAsync(CancellationToken token)
        {
            Task<ClusterStateSnapshot> clusterStateSnapshotTask = this.GetClusterStateSnapshotAsync(token).ContinueWith<ClusterStateSnapshot>(t =>
            {
                this.HandleTaskComplete(t, "GetClusterStateSnapshot", "GetClusterStateSnapshot");
                if (t.Exception == null)
                {
                    return t.Result;
                }
                else
                {
                    return null;
                }
            });

            var snapshot = await clusterStateSnapshotTask;
            return snapshot;
        }

        // Get latest active application info.
        private async Task<ClusterStateSnapshot> GetClusterStateSnapshotAsync(CancellationToken token)
        {
            Log.WriteNoise(TraceType, "GetClusterStateSnapshotAction(): Getting latest application and node information.");
            GetClusterStateSnapshotAction getClusterStateSnapshotAction = new GetClusterStateSnapshotAction(0.05, false, int.MaxValue, null);
            await this.TestContext.ActionExecutor.RunAsync(getClusterStateSnapshotAction, token);

            return getClusterStateSnapshotAction.Result;
        }

        private async Task<bool> IsClusterReadyForFaultsAsync(CancellationToken token)
        {
            IsClusterUpgradingAction isUpgradingAction = new IsClusterUpgradingAction();
            isUpgradingAction.ActionTimeout = this.chaosScenarioParameters.OperationTimeout;
            isUpgradingAction.RequestTimeout = this.chaosScenarioParameters.RequestTimeout;

            var clusterUpgradeCheckTask = this.TestContext.ActionExecutor.RunAsync(isUpgradingAction, token);

            bool checkFailed = false;
            await clusterUpgradeCheckTask.ContinueWith(t =>
            {
                this.HandleTaskComplete(t, "IsClusterUpgrading", "IsClusterUpgrading");
                if (t.Exception != null)
                {
                    checkFailed = true;
                }
            });

            if (checkFailed)
            {
                // Need to retry
                return false;
            }
            else if (isUpgradingAction.Result)
            {
                // Need to retry after a wait
                this.ReportProgress("Cluster is under going an upgrade waiting 30 seconds before next check");
                await Task.Delay(TimeSpan.FromSeconds(30));
                return false;
            }

            return true;
        }

        private async Task ValidateClusterAsync(CancellationToken token)
        {
            var clusterVerifyAction = new ValidateClusterAction(this.chaosScenarioParameters.MaxClusterStabilizationTimeout)
                {
                    ActionTimeout = this.chaosScenarioParameters.OperationTimeout,
                    RequestTimeout = this.chaosScenarioParameters.RequestTimeout
                };

            await this.TestContext.ActionExecutor.RunAsync(clusterVerifyAction, token).ContinueWith(t =>
            {
                this.HandleTaskComplete(t, "ValidateClusterAction", "ValidateClusterAction");
            });
        }

        // Check whether scenario should timeout or continue.
        private bool SessionNotDone()
        {
            // True if timer is null (i.e. no time limit) or remaining time is enough for one iteration.
            bool result = this.chaosScenarioParameters.TimeToRun - this.GetElapsedTime() > TimeSpan.Zero;
            return result;
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="disposing">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected override void OnDispose(bool disposing)
        {
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="token">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <returns></returns>
        protected override async Task ValidateScenarioAtExitAsync(CancellationToken token)
        {
            await this.ValidateClusterAsync(token);
        }
    }
}