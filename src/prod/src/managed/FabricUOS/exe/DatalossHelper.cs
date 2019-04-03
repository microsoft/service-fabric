// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.UpgradeService;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.DeploymentManager.Common;

    internal static class DatalossHelper
    {
        public static async Task<bool> IsInDatalossStateAsync(CancellationToken cancellationToken)
        {
            try
            {
                FabricClient fc = new FabricClient();
                var healthQuery = new ClusterHealthQueryDescription()
                {
                    EventsFilter = new HealthEventsFilter() { HealthStateFilterValue = HealthStateFilter.Error }
                };

                var clusterHealth = await fc.HealthManager.GetClusterHealthAsync(healthQuery, Constants.FabricQueryTimeoutInMinutes, cancellationToken);
                foreach (var healthEvent in clusterHealth.HealthEvents)
                {
                    if (healthEvent != null &&
                        healthEvent.HealthInformation != null &&
                        string.Equals(healthEvent.HealthInformation.SourceId, Constants.UpgradeOrchestrationHealthSourceId, StringComparison.OrdinalIgnoreCase) &&
                        string.Equals(healthEvent.HealthInformation.Property, Constants.DatalossHealthProperty, StringComparison.OrdinalIgnoreCase))
                    {
                        return true;
                    }
                }

                return false;
            }
            catch (Exception ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(FabricUpgradeOrchestrationService.TraceType, "IsInDatalossStateAsync failed with {0}... Continuing assuming no data loss has occured.", ex);
                return false;
            }
        }

        public static void DryRunConfigUpgrade(StandAloneCluster cluster)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(FabricUpgradeOrchestrationService.TraceType, "In data loss state. Starting dry run config upgrade in order to recover FUOS state");

            // 1-step baseline upgrade with non-baseline config
            if (cluster.RunStateMachine())
            {
                cluster.ClusterUpgradeCompleted();
                cluster.Reset();
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(FabricUpgradeOrchestrationService.TraceType, "Dry run config upgrade completed");
            }
            else
            {
                UpgradeOrchestrationTrace.TraceSource.WriteError(FabricUpgradeOrchestrationService.TraceType, "Unexpected. Dry run config upgrade is skipped");
            }
        }

        public static async Task UpdateHeathStateAsync(bool isHealthy)
        {
            try
            {
                FabricClient fc = new FabricClient();
                HealthInformation healthInfo;

                if (!isHealthy)
                {
                    healthInfo = new HealthInformation(
                        Constants.UpgradeOrchestrationHealthSourceId,
                        Constants.DatalossHealthProperty,
                        HealthState.Error)
                    {
                        Description = "Data loss has occurred to Fabric Upgrade Orchestration Service. Run Start-ServiceFabricClusterConfigurationUpgrade to recover the service state.",
                        TimeToLive = TimeSpan.MaxValue,
                        RemoveWhenExpired = true
                    };
                }
                else
                {
                    healthInfo = new HealthInformation(
                        Constants.UpgradeOrchestrationHealthSourceId,
                        Constants.DatalossHealthProperty,
                        HealthState.Ok)
                    {
                        TimeToLive = TimeSpan.FromMilliseconds(1),
                        RemoveWhenExpired = true
                    };
                }

                fc.HealthManager.ReportHealth(new ClusterHealthReport(healthInfo), new HealthReportSendOptions() { Immediate = true });

                // HM takes a bit to process the health report. Wait for it to complete before returning or else the report will not be processed since fabricClient will be killed beforehand
                while (isHealthy == await DatalossHelper.IsInDatalossStateAsync(CancellationToken.None))
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(FabricUpgradeOrchestrationService.TraceType, "Waiting for HM to process health report..");
                    await Task.Delay(TimeSpan.FromSeconds(5), CancellationToken.None).ConfigureAwait(false);
                }
            }
            catch (Exception ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteError(FabricUpgradeOrchestrationService.TraceType, "Fail to report health warning for data loss due to error: {0}", ex);
            }
        }
    }
}