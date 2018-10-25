// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Strings;
    using System.Fabric.Health;
    using System.Text;

    internal class ValidateClusterAction : ValidateAction
    {
        public ValidateClusterAction(TimeSpan maximumStabilizationTimeout)
        {
            this.MaximumStabilizationTimeout = maximumStabilizationTimeout;
            this.CheckFlag = ValidationCheckFlag.All;
        }

        internal override Type ActionHandlerType
        {
            get { return typeof(ValidateClusterActionHandler); }
        }

        private class ValidateClusterActionHandler : FabricTestActionHandler<ValidateClusterAction>
        {
            private const string TraceSource = "ValidateClusterActionHandler";

            private static readonly TimeSpan RetryWaitTimeout = TimeSpan.FromSeconds(5d);

            // Throws exception if validation was unsuccessful.
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, ValidateClusterAction action, CancellationToken token)
            {
                List<Task> tasks = new List<Task>();

                var clusterHealthValidationTask = this.ValidateClusterHealth(testContext, action, token);

                var validateAllServices = new ValidateAllServicesAction(action.MaximumStabilizationTimeout)
                {
                    ActionTimeout = action.ActionTimeout,
                    RequestTimeout = action.RequestTimeout,
                    CheckFlag = action.CheckFlag
                };

                var validateAllServicesTask = testContext.ActionExecutor.RunAsync(validateAllServices, token);

                tasks.Add(clusterHealthValidationTask);
                tasks.Add(validateAllServicesTask);

                await Task.WhenAll(tasks).ConfigureAwait(false);

                ResultTraceString = "ValidateClusterActionHandler completed for all services";
            }

            private async Task ValidateClusterHealth(FabricTestContext testContext, ValidateClusterAction action, CancellationToken token)
            {
                TimeoutHelper timer = new TimeoutHelper(action.MaximumStabilizationTimeout);

                bool success = false;
                StringBuilder healthinfo = new StringBuilder();
                while (!success && timer.GetRemainingTime() > TimeSpan.Zero)
                {
                    healthinfo.Clear();

                    ClusterHealthPolicy healthPolicy = new ClusterHealthPolicy();
                    var clusterHealthResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () =>
                        testContext.FabricClient.HealthManager.GetClusterHealthAsync(
                            healthPolicy,
                            action.RequestTimeout,
                            token),
                        FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                        timer.GetRemainingTime(),
                        token).ConfigureAwait(false);

                    bool checkError = (action.CheckFlag & ValidationCheckFlag.CheckError) != 0;
                    bool checkWarning = (action.CheckFlag & ValidationCheckFlag.CheckWarning) != 0;

                    if ((checkError && clusterHealthResult.AggregatedHealthState == HealthState.Error) ||
                    (checkWarning && clusterHealthResult.AggregatedHealthState == HealthState.Warning) ||
                    clusterHealthResult.AggregatedHealthState == HealthState.Invalid ||
                    clusterHealthResult.AggregatedHealthState == HealthState.Unknown)
                    {
                        AppTrace.TraceSource.WriteInfo(TraceSource, "Cluster health state is {0}. Will Retry check", clusterHealthResult.AggregatedHealthState);
                        foreach (HealthEvent healthEvent in clusterHealthResult.HealthEvents)
                        {
                            healthinfo.AppendLine(string.Format(
                                "Cluster health state is '{0}' with property '{1}', sourceId '{2}' and description '{3}'",
                                healthEvent.HealthInformation.HealthState,
                                healthEvent.HealthInformation.Property,
                                healthEvent.HealthInformation.SourceId,
                                healthEvent.HealthInformation.Description));
                        }

                        AppTrace.TraceSource.WriteInfo(TraceSource, healthinfo.ToString());
                    }
                    else
                    {
                        success = true;
                    }

                    if (!success)
                    {
                        // Delay before querying again so we allow some time for state to change - don't spam the node
                        await AsyncWaiter.WaitAsync(RetryWaitTimeout, token).ConfigureAwait(false);
                    }
                }

                if (!success)
                {
                    throw new FabricValidationException(StringHelper.Format(StringResources.Error_ServiceNotHealthy, "Cluster", action.MaximumStabilizationTimeout, healthinfo));
                }
            }
        }
    }
}