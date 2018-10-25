// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.WRP.Model;
    using System.Linq;
    using System.Threading.Tasks;
    using Newtonsoft.Json.Linq;

    public class ApplicationClient : FabricClientApplicationWrapper
    {
        private readonly FabricClient fabricClient;

        public ApplicationClient(FabricClient fabricClient)
        {
            fabricClient.ThrowIfNull(nameof(fabricClient));
            this.fabricClient = fabricClient;
        }

        protected ApplicationClient()
        {
        }

        public override TraceType TraceType { get; protected set; } = new TraceType("ApplicationClient");

        public override async Task<IFabricOperationResult> GetAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "GetAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ApplicationOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appDescription = (ApplicationOperationDescription)description;
            Application clusterApp = (await this.fabricClient.QueryManager.GetApplicationListAsync(
                appDescription.ApplicationUri,
                context.ContinuationToken,
                context.GetRemainingTimeOrThrow(),
                context.CancellationToken)).FirstOrDefault();

            var result = new FabricOperationResult()
            {
                OperationStatus = null,
                QueryResult = new ApplicationFabricQueryResult(clusterApp)
            };

            Trace.WriteInfo(
                TraceType,
                null == clusterApp
                    ? "GetAsync: Application not found. Name: {0}."
                    : "GetAsync: Application exists. Name: {0}.",
                appDescription.ApplicationUri);

            if (null == clusterApp)
            {
                if (description.OperationType == OperationType.Delete)
                {
                    result.OperationStatus =
                        new ApplicationOperationStatus(appDescription)
                        {
                            Status = ResultStatus.Succeeded,
                            Progress = new JObject(),
                            ErrorDetails = new JObject()
                        };

                    return result;
                }

                return null;
            }

            var status = GetResultStatus(clusterApp.ApplicationStatus);
            var progress = new JObject();
            var errorDetails = new JObject();
            if ((clusterApp.ApplicationStatus == ApplicationStatus.Ready && !appDescription.TypeVersion.Equals(clusterApp.ApplicationTypeVersion)) ||
                clusterApp.ApplicationStatus == ApplicationStatus.Upgrading ||
                clusterApp.ApplicationStatus == ApplicationStatus.Failed)
            {
                // TODO: Can deletions end up in a failed state?
                ApplicationUpgradeProgress upgradingApp =
                    await this.fabricClient.ApplicationManager.GetApplicationUpgradeProgressAsync(
                        appDescription.ApplicationUri,
                        context.OperationTimeout,
                        context.CancellationToken);

                if (upgradingApp.UpgradeState == ApplicationUpgradeState.RollingBackCompleted)
                {
                    // clusterApp.ApplicationStatus == ApplicationStatus.Ready

                    status = ResultStatus.Failed;
                    errorDetails = JObject.FromObject(
                        new
                        {
                            FailureReason = upgradingApp.FailureReason.HasValue ? upgradingApp.FailureReason.ToString() : string.Empty,
                            Details = upgradingApp.UpgradeStatusDetails
                        });
                }
                else if (upgradingApp.UpgradeState == ApplicationUpgradeState.RollingForwardCompleted)
                {
                    // clusterApp.ApplicationStatus == ApplicationStatus.Ready

                    status = ResultStatus.Failed;
                    errorDetails = JObject.FromObject(new { Details = $"Deployment completed with {clusterApp.ApplicationTypeVersion} version. Deployment goal was overriden through SF native APIs." });
                }
                else if (upgradingApp.UpgradeState == ApplicationUpgradeState.RollingBackInProgress ||
                         upgradingApp.UpgradeState == ApplicationUpgradeState.RollingForwardInProgress ||
                         upgradingApp.UpgradeState == ApplicationUpgradeState.RollingForwardPending)
                {
                    // clusterApp.ApplicationStatus == ApplicationStatus.Upgrading

                    status = ResultStatus.InProgress;
                    progress = JObject.FromObject(upgradingApp.CurrentUpgradeDomainProgress);
                }
                else if (upgradingApp.UpgradeState == ApplicationUpgradeState.Failed)
                {
                    // clusterApp.ApplicationStatus == ApplicationStatus.Failed

                    status = ResultStatus.Failed;
                    errorDetails = JObject.FromObject(
                        new
                        {
                            FailureReason = upgradingApp.FailureReason.HasValue ? upgradingApp.FailureReason.ToString() : string.Empty,
                            Details = upgradingApp.UpgradeStatusDetails
                        });
                }
            }

            result.OperationStatus = 
                new ApplicationOperationStatus(appDescription)
                {
                    Status = status,
                    Progress = progress,
                    ErrorDetails = errorDetails
                };

            return result;
        }

        public override async Task CreateAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "CreateAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));
            
            string errorMessage;
            if (!this.ValidObjectType<ApplicationOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appDescription = (ApplicationOperationDescription)description;
            appDescription.ApplicationUri.ThrowIfNull(nameof(appDescription.ApplicationUri));
            appDescription.TypeName.ThrowIfNullOrWhiteSpace(nameof(appDescription.TypeName));
            appDescription.TypeVersion.ThrowIfNullOrWhiteSpace(nameof(appDescription.TypeVersion));

            var createDescription = new ApplicationDescription(
                appDescription.ApplicationUri,
                appDescription.TypeName,
                appDescription.TypeVersion,
                appDescription.Parameters?.ToNameValueCollection());
            if (appDescription.MinimumNodes.HasValue)
            {
                createDescription.MinimumNodes = appDescription.MinimumNodes.Value;
            }

            if (appDescription.MaximumNodes.HasValue)
            {
                createDescription.MaximumNodes = appDescription.MaximumNodes.Value;
            }

            if (appDescription.Metrics != null && appDescription.Metrics.Any())
            {
                createDescription.Metrics = new List<ApplicationMetricDescription>();
                foreach (var metric in appDescription.Metrics)
                {
                    createDescription.Metrics.Add(
                        new ApplicationMetricDescription()
                        {
                            Name = metric.Name,
                            MaximumNodeCapacity = metric.MaximumCapacity,
                            NodeReservationCapacity = metric.ReservationCapacity,
                            TotalApplicationCapacity = metric.TotalApplicationCapacity
                        });
                }
            }

            Trace.WriteInfo(
                TraceType,
                "CreateAsync: Creating application. Name: {0}. TypeName: {1}. TypeVersion: {2}. Timeout: {3}",
                appDescription.ApplicationUri,
                appDescription.TypeName,
                appDescription.TypeVersion,
                context.OperationTimeout);
            await this.fabricClient.ApplicationManager.CreateApplicationAsync(
                createDescription,
                context.OperationTimeout,
                context.CancellationToken);
            Trace.WriteInfo(TraceType, "CreateAsync: Create application call accepted");
        }

        public override async Task UpdateAsync(IOperationDescription description, IFabricOperationResult result, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "UpdateAsync called");
            description.ThrowIfNull(nameof(description));
            result.ThrowIfNull(nameof(result));
            result.OperationStatus.ThrowIfNull(nameof(result.OperationStatus));
            result.QueryResult.ThrowIfNull(nameof(result.QueryResult));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ApplicationOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            if (!this.ValidObjectType<ApplicationFabricQueryResult>(result.QueryResult, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appDescription = (ApplicationOperationDescription)description;
            appDescription.TypeVersion.ThrowIfNullOrWhiteSpace(nameof(appDescription.TypeVersion));
            appDescription.ApplicationUri.ThrowIfNull(nameof(appDescription.ApplicationUri));
            var appResult = (ApplicationFabricQueryResult)result.QueryResult;

            var healthPolicy = new ApplicationHealthPolicy();
            var defaultServiceTypeHealthPolicy = new ServiceTypeHealthPolicy();
            if (appDescription.UpgradePolicy?.ApplicationHealthPolicy != null)
            {
                healthPolicy.ConsiderWarningAsError =
                    appDescription.UpgradePolicy.ApplicationHealthPolicy.ConsiderWarningAsError;
                healthPolicy.MaxPercentUnhealthyDeployedApplications =
                    appDescription.UpgradePolicy.ApplicationHealthPolicy.MaxPercentUnhealthyDeployedApplications;

                if (appDescription.UpgradePolicy.ApplicationHealthPolicy.DefaultServiceTypeHealthPolicy != null)
                {
                    defaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService =
                        appDescription.UpgradePolicy.ApplicationHealthPolicy.DefaultServiceTypeHealthPolicy
                            .MaxPercentUnhealthyPartitionsPerService;
                    defaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition =
                        appDescription.UpgradePolicy.ApplicationHealthPolicy.DefaultServiceTypeHealthPolicy
                            .MaxPercentUnhealthyReplicasPerPartition;
                    defaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices =
                        appDescription.UpgradePolicy.ApplicationHealthPolicy.DefaultServiceTypeHealthPolicy
                            .MaxPercentUnhealthyServices;
                    healthPolicy.DefaultServiceTypeHealthPolicy = defaultServiceTypeHealthPolicy;
                }
            }

            var monitoringPolicy = new RollingUpgradeMonitoringPolicy();
            if (appDescription.UpgradePolicy?.RollingUpgradeMonitoringPolicy != null)
            {
                monitoringPolicy.FailureAction =
                    (UpgradeFailureAction) Enum.Parse(
                        typeof (UpgradeFailureAction),
                        appDescription.UpgradePolicy.RollingUpgradeMonitoringPolicy.FailureAction.ToString(),
                        true);
                monitoringPolicy.HealthCheckRetryTimeout =
                    appDescription.UpgradePolicy.RollingUpgradeMonitoringPolicy.HealthCheckRetryTimeout;
                monitoringPolicy.HealthCheckStableDuration =
                    appDescription.UpgradePolicy.RollingUpgradeMonitoringPolicy.HealthCheckStableDuration;
                monitoringPolicy.HealthCheckWaitDuration =
                    appDescription.UpgradePolicy.RollingUpgradeMonitoringPolicy.HealthCheckWaitDuration;
                monitoringPolicy.UpgradeDomainTimeout =
                    appDescription.UpgradePolicy.RollingUpgradeMonitoringPolicy.UpgradeDomainTimeout;
                monitoringPolicy.UpgradeTimeout = appDescription.UpgradePolicy.RollingUpgradeMonitoringPolicy.UpgradeTimeout;
            }

            var policyDescription = new MonitoredRollingApplicationUpgradePolicyDescription()
            {
                UpgradeMode = RollingUpgradeMode.Monitored,
                HealthPolicy = healthPolicy,
                MonitoringPolicy = monitoringPolicy
            };

            if (appDescription.UpgradePolicy != null)
            {
                if (appDescription.UpgradePolicy.ForceRestart.HasValue)
                {
                    policyDescription.ForceRestart = appDescription.UpgradePolicy.ForceRestart.Value;
                }

                if (appDescription.UpgradePolicy.UpgradeReplicaSetCheckTimeout.HasValue)
                {
                    policyDescription.UpgradeReplicaSetCheckTimeout = appDescription.UpgradePolicy.UpgradeReplicaSetCheckTimeout.Value;
                }
            }

            // Update
            var updateDescription = new ApplicationUpdateDescription(appDescription.ApplicationUri)
            {
                RemoveApplicationCapacity = appDescription.RemoveApplicationCapacity
            };

            if (appDescription.MinimumNodes.HasValue)
            {
                updateDescription.MinimumNodes = appDescription.MinimumNodes.Value;
            }

            if (appDescription.MaximumNodes.HasValue)
            {
                updateDescription.MaximumNodes = appDescription.MaximumNodes.Value;
            }

            if (appDescription.Metrics != null && appDescription.Metrics.Any())
            {
                updateDescription.Metrics = new List<ApplicationMetricDescription>();
                foreach (var metric in appDescription.Metrics)
                {
                    updateDescription.Metrics.Add(
                        new ApplicationMetricDescription()
                        {
                            Name = metric.Name,
                            MaximumNodeCapacity = metric.MaximumCapacity,
                            NodeReservationCapacity = metric.ReservationCapacity,
                            TotalApplicationCapacity = metric.TotalApplicationCapacity
                        });
                }
            }

            Trace.WriteInfo(
                TraceType,
                "UpdateAsync: Updating application. Name: {0}. Timeout: {1}",
                updateDescription.ApplicationName,
                context.OperationTimeout);
            await this.fabricClient.ApplicationManager.UpdateApplicationAsync(
                updateDescription,
                context.OperationTimeout,
                context.CancellationToken);
            Trace.WriteInfo(TraceType, "UpdateAsync: Application update call accepted");

            // Upgrade
            var upgradeDescription = new ApplicationUpgradeDescription()
            {
                ApplicationName = appDescription.ApplicationUri,
                TargetApplicationTypeVersion = appDescription.TypeVersion,
                UpgradePolicyDescription = policyDescription
            };

            if (appDescription.Parameters != null)
            {
                upgradeDescription.ApplicationParameters.Add(appDescription.Parameters.ToNameValueCollection());
            }

            try
            {
                Trace.WriteInfo(
                    TraceType,
                    "UpdateAsync: Upgrading application. Name: {0}. Version: {1}. TargetVersion: {2}. Timeout: {3}",
                    upgradeDescription.ApplicationName,
                    appResult.Application.ApplicationTypeVersion,
                    upgradeDescription.TargetApplicationTypeVersion,
                    context.OperationTimeout);
                await this.fabricClient.ApplicationManager.UpgradeApplicationAsync(
                    upgradeDescription,
                    context.OperationTimeout,
                    context.CancellationToken);
                Trace.WriteInfo(TraceType, "UpdateAsync: Application upgrade call accepted");
            }
            catch (FabricException fe)
            {
                if (fe.ErrorCode == FabricErrorCode.ApplicationAlreadyInTargetVersion)
                {
                    Trace.WriteInfo(TraceType, "UpdateAsync: Application already in target version: {0}.", upgradeDescription.TargetApplicationTypeVersion);
                    return;
                }

                if (fe.ErrorCode == FabricErrorCode.ApplicationUpgradeInProgress)
                {
                    Trace.WriteInfo(TraceType, "UpdateAsync: Application upgrade in progress with same version: {0}", upgradeDescription.TargetApplicationTypeVersion);                 
                }
                else
                {
                    Trace.WriteError(TraceType, "UpdateAsync: Application upgrade encountered an exception: {0}", fe);
                    throw;
                }                
            }
            catch (Exception e)
            {
                Trace.WriteInfo(TraceType, "UpdateAsync: Application upgrade encountered an exception: {0}", e);
                throw;
            }

            // UpgradeUpdate
            ApplicationUpgradeProgress upgradeProgress = null;
            try
            {
                upgradeProgress = await this.fabricClient.ApplicationManager.GetApplicationUpgradeProgressAsync(
                        appDescription.ApplicationUri,
                        context.OperationTimeout,
                        context.CancellationToken);
            }
            catch (Exception)
            {
                Trace.WriteWarning(
                    TraceType,
                    "UpdateAsync: Failed to get the application upgrade progress to determine the parameters for upgrade update. Need to retry the operation. Application Name: {0}",
                    appDescription.ApplicationUri);

                throw new FabricTransientException();
            }
                       
            ApplicationUpgradeUpdateDescription upgradeUpdateDescription;
            if (upgradeProgress.UpgradeState == ApplicationUpgradeState.RollingBackInProgress)
            {
                // During rollback we can only change below properties
                upgradeUpdateDescription = new ApplicationUpgradeUpdateDescription()
                {
                    ApplicationName = appDescription.ApplicationUri,
                    UpgradeReplicaSetCheckTimeout = appDescription.UpgradePolicy.UpgradeReplicaSetCheckTimeout,
                    ForceRestart = appDescription.UpgradePolicy.ForceRestart
                };
            }
            else
            {
                upgradeUpdateDescription = new ApplicationUpgradeUpdateDescription()
                {
                    ApplicationName = appDescription.ApplicationUri,
                    UpgradeDomainTimeout = monitoringPolicy.UpgradeDomainTimeout,
                    UpgradeTimeout = monitoringPolicy.UpgradeTimeout,
                    HealthCheckRetryTimeout = monitoringPolicy.HealthCheckRetryTimeout,
                    HealthCheckWaitDuration = monitoringPolicy.HealthCheckWaitDuration,
                    HealthCheckStableDuration = monitoringPolicy.HealthCheckStableDuration,
                    UpgradeReplicaSetCheckTimeout = policyDescription.UpgradeReplicaSetCheckTimeout,
                    ForceRestart = policyDescription.ForceRestart,
                    FailureAction = monitoringPolicy.FailureAction,
                    HealthPolicy = healthPolicy
                };
            }

            try
            {               

                Trace.WriteInfo(
                    TraceType,
                    "UpdateAsync: Updating application upgrade in progress. Name: {0}. Timeout: {1}",
                    upgradeUpdateDescription.ApplicationName,
                    context.OperationTimeout);
                await this.fabricClient.ApplicationManager.UpdateApplicationUpgradeAsync(
                    upgradeUpdateDescription,
                    context.OperationTimeout,
                    context.CancellationToken);
                Trace.WriteInfo(TraceType, "UpdateAsync: Update application upgrade call accepted");
            }
            catch (FabricException fe)
            {
                if (fe.ErrorCode == FabricErrorCode.ApplicationNotUpgrading)
                {
                    Trace.WriteInfo(TraceType, "UpdateAsync: No application upgrade in progress");
                    return;
                }
            }
            catch (Exception e)
            {
                Trace.WriteInfo(TraceType, "UpdateAsync: Update application upgrade encountered an exception: {0}", e);
                throw;
            }
        }

        public override async Task DeleteAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "DeleteAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));
            
            string errorMessage;
            if (!this.ValidObjectType<ApplicationOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var appDescription = (ApplicationOperationDescription)description;
            try
            {
                Trace.WriteInfo(
                    TraceType,
                    "DeleteAsync: Deleting application. Name: {0}. Timeout: {1}",
                    appDescription.ApplicationUri,
                    context.OperationTimeout);
                await this.fabricClient.ApplicationManager.DeleteApplicationAsync(
                    new DeleteApplicationDescription(appDescription.ApplicationUri),
                    context.OperationTimeout,
                    context.CancellationToken);
                Trace.WriteInfo(TraceType, "DeleteAsync: Delete application call accepted");
            }
            catch (FabricElementNotFoundException nfe)
            {
                if (nfe.ErrorCode == FabricErrorCode.ApplicationNotFound)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "DeleteAsync: Application not found. Name: {0}",
                        appDescription.ApplicationUri);
                    return;
                }

                throw;
            }
        }

        private ResultStatus GetResultStatus(ApplicationStatus status)
        {
            switch (status)
            {
                case ApplicationStatus.Ready:
                    return ResultStatus.Succeeded;
                case ApplicationStatus.Failed:
                    return ResultStatus.Failed;
                case ApplicationStatus.Creating:
                case ApplicationStatus.Deleting:
                case ApplicationStatus.Upgrading:
                    return ResultStatus.InProgress;
            }

            Trace.WriteWarning(this.TraceType, "Invalid ApplicationStatus: {0}", status);
            return ResultStatus.InProgress;
        }
    }

    public class ApplicationFabricQueryResult : IFabricQueryResult
    {
        public ApplicationFabricQueryResult(Application application)
        {
            this.Application = application;
        }

        public Application Application { get; private set; }
    }
}