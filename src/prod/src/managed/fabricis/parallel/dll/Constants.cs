// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    // namespace used just for comments/documentation references.
    using RD.Fabric.PolicyAgent;

    internal static class Constants
    {
        public const string TraceTypeName = "Azure";

        public static readonly TraceType TraceType = new TraceType(TraceTypeName);

        public const int MaxRetryAttempts = 5;

        public const int BackoffPeriodInMilliseconds = 5000;

        // <JobID>/<StepID>
        public const string JobKeyFormat = "{0}/{1}";

        public const string RepairActionFormat = "System.Azure.{0}";

        /// <summary>
        /// The context Id of a tenant job that was created by IS from a repair task
        /// i.e. the scenario where a repair task created the tenant job
        /// Format is: RepairTaskId/CreationTimestamp
        /// </summary>
        public const string TenantJobContextIdFormat = "{0}/{1:O}";

        /// <summary>
        /// Different from the one created by powershell which has FabricClient/{0}
        /// Format: Azure/(JobType)/(JobId)/(UD)/(JobDocIncarnation)
        /// The combination of JobId, UD and JobDocIncarnation stamp uniquely identify the repair task.
        /// </summary>
        /// <remarks>JobDocIncarnation also helps us understand how far ago the tenant job associated with this repair task
        /// was 'seen' by IS.</remarks>
        public const string DefaultRepairTaskIdFormat = "Azure/{0}/{1}/{2}/{3}";

        public static class ConfigKeys
        {
            public const string ConfigKeyPrefix = "Azure.";

            public const string JobPollingIntervalInSeconds = ConfigKeyPrefix + "JobPollingIntervalInSeconds";

            /// <summary>
            /// The config key for adjusting the expiration time of a health report.
            /// expiry time = factor * polling period
            /// </summary>
            public const string HealthReportExpiryFactor = ConfigKeyPrefix + "HealthReportExpiryFactor";

            /// <summary>
            /// Config key that defines the policy on whether to apply a reboot maintenance action on the Host or on the VM
            /// <seealso cref="RepairActionTypeEnum"/>.
            /// The valid values this key can take are <see cref="RepairActionTypeEnum.Reboot"/> (default) and 
            /// <see cref="RepairActionTypeEnum.HostReboot"/>. E.g. "HostReboot"    
            /// </summary>
            /// <remarks>The word 'maintenance action' is used to be compatible with config keys of serial jobs IS.</remarks>
            public const string RebootMaintenanceAction = ConfigKeyPrefix + "RebootMaintenanceAction";

            /// <summary>
            /// Config key that defines the policy on whether to apply a repave data maintenance action on the Host or on the VM.
            /// The valid values this key can take are <see cref="RepairActionTypeEnum.RepaveData"/> (default) and 
            /// <see cref="RepairActionTypeEnum.HostRepaveData"/>. E.g. "HostRepaveData"         
            /// </summary>
            public const string FullReimageMaintenanceAction = ConfigKeyPrefix + "FullReimageMaintenanceAction";

            /// <summary>
            /// Config key format that maps the <see cref="ImpactTypeEnum"/> to a corresponding <see cref="RepairActionTypeEnum"/>.
            /// This mapping is provided by the user (in the service configuration)
            /// </summary>
            public const string RepairActionTypeForImpactTypeFormat = ConfigKeyPrefix + "RepairActionTypeForImpactType.{0}";

            public const string MaxParallelJobCountKeyPrefix = ConfigKeyPrefix + "MaxParallelJobCount.";

            /// <summary>
            /// The config key specifying the max. number of parallel jobs that are allowed.
            /// Specify a high value like <see cref="int.MaxValue"/> to allow unlimited jobs.
            /// TODO, consolidate all IS keys into a separate class (like MR Extension source code)
            /// </summary>
            public const string MaxParallelJobCountTotal = MaxParallelJobCountKeyPrefix + "Total";

            /// <summary>
            /// The config key specifying the max. number of parallel update jobs that are allowed.
            /// </summary>
            public const string MaxParallelJobCountUpdate = MaxParallelJobCountKeyPrefix + "Update";

            /// <summary>
            /// Perform health checks on the created repair tasks in the Preparing stage for the corresponding <see cref="ImpactActionEnum"/>.
            /// </summary>
            /// <example>EnablePreparingHealthCheck.PlatformMaintenance = true</example>
            public const string EnablePreparingHealthCheckFormat = ConfigKeyPrefix + "EnablePreparingHealthCheck.{0}";

            /// <summary>
            /// Perform health checks on the created repair tasks in the Restoring stage for the corresponding <see cref="ImpactActionEnum"/>.
            /// </summary>
            public const string EnableRestoringHealthCheckFormat = ConfigKeyPrefix + "EnableRestoringHealthCheck.{0}";

            /// <summary>
            /// Config key that determines whether the coordinator needs to wait for a change in the incarnation
            /// number after it becomes active, so that it can be more confident that the MR channel is functioning properly.
            /// A 'change' in incarnation number indicates that the entire channel from host->VM->coordinator is okay.
            /// </summary>
            public const string WaitForIncarnationChangeOnStartup = ConfigKeyPrefix + "WaitForIncarnationChangeOnStartup";

            /// <summary>
            /// Config key that determines how long the coordinator should wait for the incarnation
            /// number to change before concluding that the MR channel is down. Exceeding this time
            /// will cause the IS to exit in order to trigger a failover.
            /// </summary>
            public const string MaxIncarnationUpdateWaitTime = ConfigKeyPrefix + "MaxIncarnationUpdateWaitTimeInSeconds";

            /// <summary>
            /// If a claimed repair task doesn't have a corresponding tenant job within a certain time, then cancel
            /// the repair task. This config specifies the time to wait before cancel.
            /// </summary>
            public const string CancelThresholdForUnmatchedRepairTask = ConfigKeyPrefix + "CancelThresholdForUnmatchedRepairTask";

            /// <summary>
            /// Handle jobs of type VendorRepairBegin (bool)
            /// </summary>
            public const string EnableVendorRepairBeginHandling = ConfigKeyPrefix + "EnableVendorRepairBeginHandling";

            /// <summary>
            /// Handle jobs of type VendorRepairEnd (bool)
            /// </summary>
            public const string EnableVendorRepairEndHandling = ConfigKeyPrefix + "EnableVendorRepairEndHandling";

            /// <summary>
            /// Handle jobs whose phase is unknown (bool)
            /// </summary>
            public const string EnableUnknownJobHandling = ConfigKeyPrefix + "EnableUnknownJobHandling";

            /// <summary>
            /// Enables or disables calling NodeStateRemoved as appropriate during the state change to Restoring (bool)
            /// </summary>
            public const string ProcessRemovedNodes = ConfigKeyPrefix + "ProcessRemovedNodes";

            /// <summary>
            /// The amount of time to observe coordinator errors before reporting a health warning.
            /// <seealso cref="CoordinatorFailureRetryDuration"/>
            /// </summary>
            public const string CoordinatorFailureWarningThreshold = ConfigKeyPrefix + "CoordinatorFailureWarningThresholdInSeconds";
            
            /// <summary>
            /// Max. amount of time to retry on coordinator errors beyond which the service fails over.
            /// </summary>
            public const string CoordinatorFailureRetryDuration = ConfigKeyPrefix + "CoordinatorFailureRetryDurationInSeconds";

            /// <summary>
            /// Enables or disables execution of user-requested repairs (bool)
            /// </summary>
            public const string EnableRepairExecution = ConfigKeyPrefix + "EnableRepairExecution";

            /// <summary>
            /// Maximum age of Completed repair tasks to process
            /// </summary>
            public const string CompletedTaskAgeThreshold = ConfigKeyPrefix + "CompletedTaskAgeThresholdInSeconds";

            /// <summary>
            /// Version of the job classification logic to use (int)
            /// </summary>
            /// <example>
            /// Azure.JobClassifier.Version=2
            /// </example>
            public const string JobClassifierVersion = ConfigKeyPrefix + "JobClassifier.Version";

            /// <summary>
            /// <see cref="JobPhase"/> to use when an ImpactStart job step has AcknowledgementStatus = Alerted
            /// </summary>
            /// <example>
            /// Azure.JobClassifier.ImpactStartAcknowledgementStatusAlerted=Unknown
            /// </example>
            public const string JobClassifierImpactStartAcknowledgementStatusAlerted = ConfigKeyPrefix + "JobClassifier.ImpactStartAcknowledgementStatusAlerted";

            /// <summary>
            /// Determines how to interpret a particular job status, as active/inactive/unknown.  Specifically,
            /// sets the <see cref="JobClassifier.JobStatusCategory"/> to use for the given <see cref="JobStatusEnum"/>
            /// </summary>
            /// <example>
            /// Azure.JobClassifier.JobStatusCategory.Alerted=Inactive
            /// </example>
            public const string JobClassifierJobStatusCategoryFormat = ConfigKeyPrefix + "JobClassifier.JobStatusCategory.{0}";

            /// <summary>
            /// Controls whether Execute/Restore actions are automatically handled per job type
            /// </summary>
            /// <example>
            /// Azure.AutoExecute.TenantUpdate=false
            /// </example>
            public const string AutoActionTypeHandlingFormat = ConfigKeyPrefix + "Auto{0}.{1}";

            /// <summary>
            /// The Uri for the mrzerosdk endpoint.        
            /// </summary>
            /// <remarks>This is used on SF Linux cluster VMs created via SFRP since there is no registry to query in Linux.</remarks>
            /// <example>http://169.254.169.254:80/mrzerosdk</example>
            public const string MRZeroSdkUri = "MRZeroSdkUri";
        }
    }
}