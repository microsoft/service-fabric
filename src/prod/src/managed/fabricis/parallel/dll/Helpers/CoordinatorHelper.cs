// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
#if DotNetCoreClr
    // NetStandard and Newtonsoft.Json has a common type
	// (System.Runtime.Serialization.Formatters.FormatterAssemblyStyle)
	// defined in both of them. Adding this alias guarantess linking with
    // Newtonsoft.Json.	
    extern alias ActualSystemRuntimeSerializationFormatters;
	using ActualSystemRuntimeSerializationFormatters::System.Runtime.Serialization.Formatters;
    using ActualSystemRuntimeSerializationFormatters::Newtonsoft.Json;
    using ActualSystemRuntimeSerializationFormatters::Newtonsoft.Json.Converters;
#else
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
	using System.Runtime.Serialization.Formatters;
#endif
	
    using Collections.Generic;
    using Globalization;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Repair;

    /// <summary>
    /// Contains helper methods used mainly by the coordinator.
    /// </summary>
    internal static class CoordinatorHelper
    {
        private static JsonSerializerSettings internalSerializationSettings = CreateInternalSerializationSettings();
        private static JsonSerializerSettings publicSerializationSettings = CreatePublicSerializationSettings();
        private static JsonSerializerSettings publicCompactSerializationSettings = CreatePublicCompactSerializationSettings();

        // TODO we can get this with bit-shifts. but okay for now.
        // This also doesn't handle additional states being added in RM
        private static readonly Dictionary<RepairTaskState, int> repairTaskStateIndexMap = new Dictionary<RepairTaskState, int>
            {
                { RepairTaskState.Invalid, 0 },
                { RepairTaskState.Created, 1 },
                { RepairTaskState.Claimed, 2 },
                { RepairTaskState.Preparing, 3 },
                { RepairTaskState.Approved, 4 },
                { RepairTaskState.Executing, 5 },
                { RepairTaskState.Restoring, 6 },
                { RepairTaskState.Completed, 7 },
            };

        /// <summary>
        /// Gets the time spent in the current state of a repair task.
        /// </summary>
        public static TimeSpan GetTimeInCurrentState(this IRepairTask repairTask, DateTimeOffset now)
        {
            repairTask.Validate("repairTask");

            var timestamps = new List<DateTime?>
            {
                null, repairTask.CreatedTimestamp, repairTask.ClaimedTimestamp, repairTask.PreparingTimestamp,
                repairTask.ApprovedTimestamp, repairTask.ExecutingTimestamp, repairTask.RestoringTimestamp, repairTask.CompletedTimestamp,
            };

            var index = repairTaskStateIndexMap[repairTask.State];
            var stateTime = timestamps[index];

            if (!stateTime.HasValue)
            {
                return TimeSpan.Zero;
            }

            var duration = now - stateTime.Value;
            return duration;
        }

        public static IList<RoleInstance> GetRoleInstances(this IPolicyAgentDocumentForTenant doc)
        {
            var roleInstances = new List<RoleInstance>();

            if (doc == null || doc.RoleInstanceHealthInfos == null)
            {
                Constants.TraceType.WriteWarning("Unexpected null received for {0}", doc == null ? "doc" : "doc.RoleInstanceHealthInfos");
                return roleInstances;
            }

            // Using DateTime instead of DateTimeOffset since RoleInstance class used DateTime (because it
            // was initially a wrapper around MR SDK ManagementRoleInstance object that had a DateTime.
            // The string is in RFC1123 format created by 'r'. e.g. time.ToString("r");            
            DateTime timestamp;
            bool success = DateTime.TryParse(
                doc.RoleInstanceHealthInfoTimestamp, 
                CultureInfo.InvariantCulture, 
                DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out timestamp);
            if (!success)
            {
                timestamp = DateTime.UtcNow;
            }

            foreach (RoleInstanceHealthInfo roleInstanceHealthInfo in doc.RoleInstanceHealthInfos)
            {                
                var roleInstance = new RoleInstance(
                    roleInstanceHealthInfo.RoleInstanceName,
                    roleInstanceHealthInfo.Health.ToRoleInstanceState(),
                    timestamp);

                roleInstances.Add(roleInstance);
            }

            return roleInstances;
        }

        private static RoleInstanceState ToRoleInstanceState(this RoleStateEnum roleState)
        {
            RoleInstanceState state;

            bool success = Enum.TryParse(roleState.ToString(), out state);
            if (success)
            {
                return state;
            }

            string message = "Unable to convert {0}.{1} to {2}".ToString(
                typeof(RoleStateEnum).Name,
                roleState,
                typeof(RoleInstanceState));

            Constants.TraceType.WriteError("{0}", message);
            throw new ArgumentException(message);
        }

        /// <summary>
        /// Tenant job summary for display purposes
        /// </summary>
        public static JobSummary GetSummary(this IMappedTenantJob mappedTenantJob)
        {
            if (mappedTenantJob == null)
            {
                return null;
            }

            var jobId = mappedTenantJob.Id;
            var tenantJob = mappedTenantJob.TenantJob;

            var summary = new JobSummary
            {
                Id = jobId,
                ContextId = tenantJob.ContextStringGivenByTenant,
                JobStatus = tenantJob.JobStatus.ToString(),
                ImpactAction = tenantJob.GetImpactAction().ToString(),
                RoleInstancesToBeImpacted = tenantJob.RoleInstancesToBeImpacted ?? new List<string>(),
                CurrentUD = tenantJob.GetJobUD(),
                RepairTasks = mappedTenantJob.MatchedTasks,
                CorrelationId = tenantJob.CorrelationId,
            };

            if (tenantJob.JobStep == null || tenantJob.JobStep.CurrentlyImpactedRoleInstances == null)
            {
                return summary;
            }

            summary.AcknowledgementStatus = tenantJob.JobStep.AcknowledgementStatus.ToString();
            summary.DeadlineForResponse = tenantJob.JobStep.DeadlineForResponse;
            summary.ActionStatus = tenantJob.JobStep.ActionStatus.ToString();
            summary.ImpactStep = tenantJob.JobStep.ImpactStep.ToString();

            foreach (var roleInstance in tenantJob.JobStep.CurrentlyImpactedRoleInstances)
            {
                var impactedRoleInstance = new ImpactedRoleInstance
                {
                    Name = roleInstance.RoleInstanceName,
                    UD = roleInstance.UpdateDomain.ToString(),
                };

                if (roleInstance.ExpectedImpact != null)
                {
                    var affectedResourceImpact = roleInstance.ExpectedImpact;
                    if (affectedResourceImpact != null && affectedResourceImpact.ListOfImpactTypes != null)
                    {
                        impactedRoleInstance.ImpactTypes =
                            affectedResourceImpact
                                .ListOfImpactTypes.Select(e => e.ToString())
                                .ToList();
                    }
                }

                summary.CurrentlyImpactedRoleInstances.Add(impactedRoleInstance);
            }

            summary.PendingActions = mappedTenantJob.PendingActions.ToString();
            summary.AllowedActions = mappedTenantJob.AllowedActions.ToString();
            summary.IsActive = mappedTenantJob.IsActive;

            if (mappedTenantJob.ImpactedNodeCount >= 0)
            {
                summary.ImpactedNodeCount = mappedTenantJob.ImpactedNodeCount;
            }

            return summary;
        }

        /// <summary>
        /// Is a job newly created by FC and not "seen" by RM yet
        /// </summary>
        public static bool IsWaitingForImpactStartAck(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            if (tenantJob.JobStep == null)
            {
                // we have noticed that some jobs (probably when they are created initially) have no job step                
                return false;
            }

            bool isWaiting =
                tenantJob.JobStep.AcknowledgementStatus == AcknowledgementStatusEnum.WaitingForAcknowledgement &&
                tenantJob.JobStep.ImpactStep == ImpactStepEnum.ImpactStart &&
                tenantJob.JobStatus == JobStatusEnum.Executing;

            return isWaiting;
        }

        public static bool IsWaitingForImpactEndAck(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            if (tenantJob.JobStep == null)
            {
                // we have noticed that some jobs (probably when they are created initially have no job step                
                return false;
            }

            bool ready =
                tenantJob.JobStep.AcknowledgementStatus == AcknowledgementStatusEnum.WaitingForAcknowledgement &&
                tenantJob.JobStep.ImpactStep == ImpactStepEnum.ImpactEnd;

            return ready;
        }

        public static bool IsImpactStartAcked(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            if (tenantJob.JobStep == null)
            {
                // we have noticed that some jobs (probably when they are created initially have no job step                
                return false;
            }

            // TODO consider adding JobStatus == JobStatusEnum.Executing check or deleting this comment once MR team 
            // fixes Icm 24737274
            bool acked =
                (tenantJob.JobStep.AcknowledgementStatus == AcknowledgementStatusEnum.Acknowledged || tenantJob.JobStep.AcknowledgementStatus == AcknowledgementStatusEnum.Timedout) &&
                tenantJob.JobStep.ImpactStep == ImpactStepEnum.ImpactStart /* &&
                tenantJob.JobStatus == JobStatusEnum.Executing */; 

            return acked;
        }

        /// <summary>
        /// Gets the job key which is a JobId/UD combination.
        /// A job key forms a 1:1 mapping with a repair task. A single job may walk multiple UDs. With each UD, it 
        /// creates a new repair task. The description field of the repair task is associated with this job key.
        /// </summary>
        /// <remarks>
        /// The job key can be null when the jobstep is null (probably because the job has just been created, 
        /// or has completed) and there is no UD information.
        /// </remarks>
        private static string GetJobKey(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            var id = tenantJob.Id;
            var stepId = tenantJob.GetJobStepId();

            if (stepId == null)
            {
                return null;
            }

            var key = Constants.JobKeyFormat.ToString(id, stepId);
            return key;
        }

        /// <summary>
        /// Gets the job key which is a Azure JobId/UD combination corresponding to the repair task.
        /// </summary>
        private static string GetJobKey(this IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");
            var id = repairTask.GetJobId();
            var stepId = repairTask.GetJobStepId();

            if (string.IsNullOrWhiteSpace(id) || stepId == null)
            {
                return null;
            }

            var key = Constants.JobKeyFormat.ToString(id, stepId);
            return key;
        }

        public static RepairTaskExecutorData GetExecutorData(this IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            if (string.IsNullOrWhiteSpace(repairTask.ExecutorData))
            {
                return null;
            }

            try
            {
                var executorData = repairTask.ExecutorData.FromJson<RepairTaskExecutorData>();
                return executorData;
            }
            catch (Exception ex)
            {
                Constants.TraceType.WriteWarning(
                    "Error while getting executor data for repair task. Returning null. Repair task: {0}{1}Exception: {2}",
                    repairTask.ToJson(),
                    Environment.NewLine,
                    ex);

                return null;
            }
        }

        private static string GetJobStepId(this IRepairTask repairTask)
        {
            var executorData = repairTask.GetExecutorData();
            if (executorData == null)
            {
                return null;
            }

            return executorData.StepId;
        }

        public static string GetJobId(this IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            var executorData = repairTask.GetExecutorData();
            if (executorData == null)
            {
                return null;
            }

            Guid jobId;

            // just a validation step here to return only guids
            bool success = Guid.TryParse(executorData.JobId, out jobId);
            if (!success)
            {
                Constants.TraceType.WriteWarning("Job Id could not be parsed from repair task: {0}", repairTask.ToJson());
                return null;
            }

            return executorData.JobId;
        }

        public static IList<string> GetRoleInstanceNames(this List<RoleInstanceHealthInfo> roleInstanceHealthInfos)
        {
            var roleInstanceNames = new List<string>();

            if (roleInstanceHealthInfos != null)
            {
                foreach (var rih in roleInstanceHealthInfos)
                {
                    roleInstanceNames.Add(rih.RoleInstanceName);
                }
            }
            else
            {
                Constants.TraceType.WriteWarning("Unexpected null received for roleInstanceHealthInfos");
            }

            return roleInstanceNames;
        }

        public static bool AllImpactsEqual(AffectedResourceImpact resourceImpact, Impact expectedImpact)
        {
            return
                resourceImpact.ApplicationConfigImpact == expectedImpact &&
                resourceImpact.ComputeImpact == expectedImpact &&
                resourceImpact.DiskImpact == expectedImpact &&
                resourceImpact.NetworkImpact == expectedImpact &&
                resourceImpact.OSImpact == expectedImpact;
        }

        public static bool IsVendorRepairBegin(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");
            return (tenantJob.IsRepairJobType() && AllImpactsEqual(tenantJob.ImpactDetail.ImpactedResources, Impact.Wipe));
        }

        public static bool IsVendorRepairEnd(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");
            return (tenantJob.IsRepairJobType() && AllImpactsEqual(tenantJob.ImpactDetail.ImpactedResources, Impact.None));
        }

        public static string FormatDocumentForTracing(IPolicyAgentDocumentForTenant doc)
        {
            string docString = doc.ToJson();

            int maxChars = 30000; // to allow for a 64KB ETW event size limit
            if (docString.Length > maxChars)
            {
                docString = doc.ToCompactJson();
                if (docString.Length > maxChars)
                {
                    docString = docString.Substring(0, maxChars) + "[TRUNCATED]";
                }
            }

            return docString;
        }

        /// <summary>
        /// Converts an object to its Json representation.
        /// </summary>
        public static string ToJson<T>(this T value)
        {
            string json = JsonConvert.SerializeObject(value, typeof(T), publicSerializationSettings);

            return json;
        }

        /// <summary>
        /// Converts an object to a more compact Json representation.
        /// </summary>
        public static string ToCompactJson<T>(this T value)
        {
            string json = JsonConvert.SerializeObject(value, typeof(T), publicCompactSerializationSettings);

            return json;
        }

        /// <remarks>
        /// Converts a Json string to an object.
        /// Note: This method does not deserialize types containing interfaces easily! So, you
        /// may have to play around with different serialization settings (and use it for
        /// both serialization and deserialization).
        /// </remarks>
        public static T FromJson<T>(this string value)
        {
            T @object = JsonConvert.DeserializeObject<T>(value, internalSerializationSettings);

            return @object;
        }

        public static bool IsRepairJobType(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            var action = tenantJob.GetImpactAction();
            return action == ImpactActionEnum.PlatformMaintenance || action == ImpactActionEnum.TenantMaintenance;
        }

        public static bool IsUpdateJobType(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            var action = tenantJob.GetImpactAction();
            return action == ImpactActionEnum.PlatformUpdate || action == ImpactActionEnum.TenantUpdate;
        }

        public static bool IsTenantUpdateJobType(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            var action = tenantJob.GetImpactAction();
            return action == ImpactActionEnum.TenantUpdate;
        }

        public static bool IsPlatformUpdateJobType(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            var action = tenantJob.GetImpactAction();
            return action == ImpactActionEnum.PlatformUpdate;
        }

        /// <summary>
        /// Gets the job type. E.g. is this a platform update, a tenant maintenance etc.
        /// </summary>
        public static ImpactActionEnum GetImpactAction(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");
            return tenantJob.ImpactDetail != null ? tenantJob.ImpactDetail.ImpactAction : ImpactActionEnum.Unknown;
        }

        public static string GetTenantJobContext(this IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            return Constants.TenantJobContextIdFormat.ToString(repairTask.TaskId, repairTask.CreatedTimestamp);
        }

        public static bool IsJobIdMatch(this IRepairTask repairTask, ITenantJob tenantJob)
        {
            repairTask.Validate("repairTask");
            tenantJob.Validate("tenantJob");

            var jobId = repairTask.GetJobId();

            bool isMatch = (jobId != null) && string.Equals(jobId, tenantJob.Id.ToString());
            return isMatch;
        }

        public static bool IsJobKeyMatch(this IRepairTask repairTask, ITenantJob tenantJob)
        {
            repairTask.Validate("repairTask");
            tenantJob.Validate("tenantJob");

            var jobId = repairTask.GetJobId();
            var jobKey = repairTask.GetJobKey();

            bool jobKeyMatch = jobId != null && jobKey != null && string.Equals(jobKey, tenantJob.GetJobKey());
            return jobKeyMatch;
        }

        public static bool IsContextIdMatch(this IRepairTask repairTask, ITenantJob tenantJob)
        {
            repairTask.Validate("repairTask");
            tenantJob.Validate("tenantJob");

            var contextId = repairTask.GetTenantJobContext();
            bool contextIdMatch = string.Equals(contextId, tenantJob.ContextStringGivenByTenant, StringComparison.OrdinalIgnoreCase);

            return contextIdMatch;
        }

        // Does repairTask represent a VendorRepair, via action/target match?
        public static bool MatchesVendorRepairJobByActionAndTarget(this IRepairTask repairTask, RepairActionProvider repairActionProvider, IConfigSection configSection, ITenantJob tenantJob)
        {
            repairTask.Validate("repairTask");
            tenantJob.Validate("tenantJob");

            // 1. check action
            RepairActionTypeEnum? actionFromRepairTask = repairActionProvider.GetRepairAction(repairTask.Action);
            if (!actionFromRepairTask.HasValue || RepairActionTypeEnum.Heal != actionFromRepairTask.Value)
            {
                return false;
            }

            // 2. check targets (nodes)
            // TODO should this be RoleInstancesToBeImpacted or JobStep.CurrentlyImpactedRoleInstances?
            if (tenantJob.RoleInstancesToBeImpacted == null || tenantJob.RoleInstancesToBeImpacted.Count != 1)
            {
                Constants.TraceType.WriteInfo(
                    "Task '{0}' does not match VendorRepair job {1} since RoleInstancesToBeImpacted does not have exactly one instance: {2}",
                    repairTask.TaskId,
                    tenantJob.Id,
                    tenantJob.ToJson());
                return false;
            }

            var nodeNames = repairTask.GetTargetNodeNames();
            if (nodeNames == null || nodeNames.Count != 1)
            {
                Constants.TraceType.WriteInfo(
                    "Task '{0}' does not match VendorRepair job {1} since task target does not have exactly one node: {2}",
                    repairTask.TaskId,
                    tenantJob.Id,
                    nodeNames.ToJson());
                return false;
            }

            var repairTaskTargetRoleInstance = nodeNames[0].TranslateNodeNameToRoleInstance();
            var jobRoleInstance = tenantJob.RoleInstancesToBeImpacted[0];

            bool targetsMatch = string.Equals(repairTaskTargetRoleInstance, jobRoleInstance, StringComparison.OrdinalIgnoreCase);

            return targetsMatch;
        }

        public static bool IsVendorRepair(this IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            var executorData = repairTask.GetExecutorData();
            if (executorData == null)
            {
                return false;
            }

            return executorData.HasFlag(RepairTaskExecutorData.VendorRepairFlag);
        }

        // Does repairTask represent a VendorRepair, via action/impact match?
        // This is used to check if a VRE matches a particular repair task
        public static bool MatchesVendorRepairEndJob(this IRepairTask repairTask, ITenantJob tenantJob)
        {
            repairTask.Validate("repairTask");
            tenantJob.Validate("tenantJob");

            if (!repairTask.IsVendorRepair())
            {
                return false;
            }

            // 2. check impact
            if (repairTask.Impact == null || repairTask.Impact.Kind != RepairImpactKind.Node)
            {
                return false;
            }

            var impactedNodesFromRepairTask = ((NodeRepairImpactDescription)repairTask.Impact).ImpactedNodes;

            var impactedNodesFromTenantJob = new HashSet<string>(
                tenantJob.JobStep.CurrentlyImpactedRoleInstances.Select(i => i.RoleInstanceName.TranslateRoleInstanceToNodeName()),
                StringComparer.OrdinalIgnoreCase);

            // If all impacted nodes in this VendorRepair task are covered by the job, it's a match
            foreach (var nodeFromRepairTask in impactedNodesFromRepairTask)
            {
                if (!impactedNodesFromTenantJob.Contains(nodeFromRepairTask.NodeName))
                {
                    return false;
                }
            }

            return true;
        }

        public static string GetJobStepId(this ITenantJob tenantJob)
        {
            string jobStepId = null;

            if (tenantJob.JobStep != null)
            {
                // An empty impacted role instance list will result in a job step ID with the following string
                jobStepId = "-";

                if (tenantJob.JobStep.CurrentlyImpactedRoleInstances != null)
                {
                    var canonicalRoleInstance = tenantJob.JobStep.CurrentlyImpactedRoleInstances.OrderBy(i => i.RoleInstanceName, StringComparer.Ordinal).FirstOrDefault();
                    if (canonicalRoleInstance != null)
                    {
                        jobStepId = canonicalRoleInstance.RoleInstanceName;
                    }
                }
            }

            return jobStepId;
        }

        public static uint? GetJobUD(this ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            if (tenantJob.JobStep == null || tenantJob.JobStep.CurrentlyImpactedRoleInstances == null)
            {
                return null;
            }

            var uds = tenantJob.JobStep.CurrentlyImpactedRoleInstances.Select(e => e.UpdateDomain).Distinct().ToList();
            if (uds.Count != 1)
            {
                // There is not a single consistent UD for this job; perhaps it is an FD-based maintenance job
                return null;
            }

            return uds[0];
        }

        /// <summary>
        /// Determines if the passed in exception is retry-able. This is called by an 
        /// instance <see cref="IRetryPolicy"/>.
        /// </summary>
        /// <param name="e">The exception</param>
        /// <returns><c>true</c> if retry-able; <c>false</c> otherwise.</returns>
        public static bool IsRetriableException(this Exception e)
        {
            e.Validate("e");

            if (e is FabricTransientException || e is OperationCanceledException || e is TimeoutException)
            {
                return true;
            }

            if (e is AggregateException)
            {
                bool retriable = ((AggregateException)e).Flatten().InnerExceptions.All(ex => ex.IsRetriableException());
                return retriable;
            }

            return false;
        }

        /// <summary>
        /// Gets the target node name for repair tasks.
        /// </summary>
        /// <param name="repairTask"></param>
        /// <returns>The target node name or <c>null</c> if the repair task didn't match the expected format.</returns>
        public static IList<string> GetTargetNodeNames(this IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            if (repairTask.Target == null || repairTask.Target.Kind != RepairTargetKind.Node)
            {
                Constants.TraceType.WriteInfo("Repair task {0}: ignoring unknown target type {1}", repairTask.TaskId, repairTask.Target);
                return null;
            }

            NodeRepairTargetDescription targetDescription = (NodeRepairTargetDescription)repairTask.Target;
            return targetDescription.Nodes;
        }

        public static IEnumerable<string> GetTargetRoleInstances(this IRepairTask repairTask)
        {
            var nodeNames = repairTask.GetTargetNodeNames();
            if (nodeNames == null)
            {
                return null;
            }

            return nodeNames.Select(e => e.TranslateNodeNameToRoleInstance());
        }

        private static bool DoesJobRequireHealthCheck(ITenantJob tenantJob, IConfigSection configSection, string keyFormat, bool defaultValue)
        {
            tenantJob.Validate("tenantJob");
            configSection.Validate("configSection");

            var impactAction = tenantJob.GetImpactAction();
            var key = keyFormat.ToString(impactAction);

            return configSection.ReadConfigValue(key, defaultValue);
        }

        public static bool DoesJobRequirePreparingHealthCheck(this ITenantJob tenantJob, IConfigSection configSection)
        {
            return DoesJobRequireHealthCheck(
                tenantJob,
                configSection,
                Constants.ConfigKeys.EnablePreparingHealthCheckFormat,
                false);
        }

        public static bool DoesJobRequireRestoringHealthCheck(this ITenantJob tenantJob, IConfigSection configSection)
        {
            return DoesJobRequireHealthCheck(
                tenantJob,
                configSection,
                Constants.ConfigKeys.EnableRestoringHealthCheckFormat,
                tenantJob.IsUpdateJobType());
        }

        /// <summary>
        /// This is to be used when we need to do a FromJson() to read from a .json file.
        /// This is required for deserializing interface types. Otherwise, Newtonsoft doesn't
        /// know what concrete type to assign the interface to.
        /// </summary>
        /// <example>
        /// For <see cref="RepairTaskExecutorData"/> here is the serialization detail.
        /// with publicSerializationSettings:   json string = "{\r\n  \"JobId\": \"id1\",\r\n  \"JobUD\": 3\r\n}"
        /// with internalSerializationSettings: json string = "{\r\n  \"$type\": \"System.Fabric.InfrastructureService.Parallel.RepairTaskExecutorData, FabricIS.parallel\",\r\n  \"JobId\": \"id1\",\r\n  \"JobUD\": 3\r\n}"
        /// </example>
        private static JsonSerializerSettings CreateInternalSerializationSettings()
        {
            var settings = new JsonSerializerSettings
            {
                Formatting = Formatting.Indented,
                NullValueHandling = NullValueHandling.Ignore,
                ReferenceLoopHandling = ReferenceLoopHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Auto,
                TypeNameAssemblyFormat = FormatterAssemblyStyle.Simple,
                ConstructorHandling = ConstructorHandling.Default,
                DateFormatHandling = DateFormatHandling.IsoDateFormat,
                DateFormatString = "o",
            };

            // for seeing enums as strings not as integer values
            settings.Converters.Add(new StringEnumConverter());

            return settings;
        }

        private static JsonSerializerSettings CreatePublicSerializationSettings()
        {
            var settings = new JsonSerializerSettings
            {
                Formatting = Formatting.Indented,
                NullValueHandling = NullValueHandling.Ignore,
                ReferenceLoopHandling = ReferenceLoopHandling.Ignore,
                ConstructorHandling = ConstructorHandling.Default,
                DateFormatHandling = DateFormatHandling.IsoDateFormat,
                DateFormatString = "o",
            };

            // for seeing enums as strings not as integer values
            settings.Converters.Add(new StringEnumConverter());

            return settings;
        }

        private static JsonSerializerSettings CreatePublicCompactSerializationSettings()
        {
            var settings = CreatePublicSerializationSettings();
            settings.Formatting = Formatting.None;
            return settings;
        }

        public static RepairTaskResult ConvertToRepairTaskResult(JobStatusEnum jobStatus)
        {
            switch (jobStatus)
            {
                case JobStatusEnum.Completed:
                    return RepairTaskResult.Succeeded;

                case JobStatusEnum.Cancelled:
                    return RepairTaskResult.Cancelled;

                case JobStatusEnum.Suspended:
                    return RepairTaskResult.Interrupted;

                case JobStatusEnum.Alerted:
                case JobStatusEnum.Failed:
                default:
                    return RepairTaskResult.Failed;
            }
        }

        public static RepairTaskResult ConvertToRepairTaskResult(ImpactActionStatus actionStatus)
        {
            switch (actionStatus)
            {
                case ImpactActionStatus.Executed:
                    return RepairTaskResult.Succeeded;

                case ImpactActionStatus.NotExecuted:
                    return RepairTaskResult.Interrupted;

                case ImpactActionStatus.Failed:
                default:
                    return RepairTaskResult.Failed;
            }
        }
    }
}