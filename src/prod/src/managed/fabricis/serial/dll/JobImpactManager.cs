// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Linq;
    using Threading.Tasks;

    /// <summary>
    /// This class is associated with feature - MR Learning mode
    /// In recent times, there have been a sudden rise in the number of Platform Update jobs in Azure. 
    /// Most of these jobs have minimal or no impact on the node (e.g. a configuration update that does 
    /// not need any VM or host restart). Customers like SQL Azure etc. are facing a pain since a restart 
    /// is very expensive in their scenarios. To process these jobs faster and to reduce customer impact, 
    /// we would want to avoid restarting the Service Fabric node unless required.
    /// MR learning mode is a feature aimed to learn from previous job impact and take corresponding action 
    /// so that we minimize the impact(like restarts, repaves etc.) on the node.
    ///
    /// This implementation stores the previous impact in memory which isn't persisted if IS fails over. 
    /// This currently handles only DeploymentUpdateJob and PlatformUpdateJob types.
    /// DeploymentUpdateJob is also handled since it is much easier to test this than PlatformUpdateJob, even
    /// though this job type isn't a current production scenario.
    /// </summary>
    /// <remarks>
    /// a. To test a restart-impact scenario on an SF cluster running on Azure PaaS, from the old Azure portal,
    ///    Update the Operating System Version
    /// b. To test a no-impact scenario, 
    ///    Update a setting like 'WindowsFabric.TextLogTraceLevel' to 'Verbose' or 'Info'
    /// </remarks>
    /// <see cref="IJobImpactManager"/>
    internal sealed class JobImpactManager : IJobImpactManager
    {
        /// <summary>
        /// marked internal for unit testing
        /// </summary>
        internal const string ConfigLearningModeValidityTimeSpanKeyName = "WindowsAzure.LearningMode.ValidityTimeSpan";

        private readonly TimeSpan DefaultLearningModeValidityTimeSpan = TimeSpan.FromHours(1);

        
        private readonly TraceType traceType = new TraceType(typeof(JobImpactManager).Name);
        private readonly IConfigSection configSection;

        private JobImpactData currentJobImpactData;

        public JobImpactManager(
            IConfigSection configSection,
            IQueryClient queryClient)
        {
            this.configSection = configSection.Validate("configSection");
            this.QueryClient = queryClient.Validate("queryClient");
        }

        internal IQueryClient QueryClient { get; private set; }

        public async Task<JobImpactTranslationMode> EvaluateJobImpactAsync(IManagementNotificationContext notification)
        {
            notification.Validate("notification");

            if (notification.NotificationType != NotificationType.StartJobStep)
            {
                // this is a coding error
                throw new ArgumentException("Notification not relevant. Notification: {0}".ToString(notification.ToShortDisplayString()));
            }

            if (DoesAssessmentMatchNotification(notification))
            {
                return currentJobImpactData.AssessedImpact;
            }

            traceType.WriteInfo(
                "JobImpactData doesn't match, starting new evaluation. Notification: {0}, Current JobImpactData: {1}",
                notification.ToShortDisplayString(), currentJobImpactData != null ? currentJobImpactData.ToString() : "<null>");

            var now = DateTimeOffset.UtcNow;

            // There is already retry built into the QueryClient wrapper. If it goes beyond retry boundaries,
            // we'll let the caller handle this
            IList<INode> nodeList = await QueryClient.GetNodeListAsync(now).ConfigureAwait(false);
            var queriedNodes = nodeList.ToDictionary(e => e.NodeName, StringComparer.OrdinalIgnoreCase);

            Dictionary <string, INode> nodesToBeImpacted = GetNodesInNotification(notification, queriedNodes);

            var newJobImpactData = new JobImpactData
            {
                JobId = notification.ActiveJobId,
                JobType = notification.ActiveJobType,
                UD = notification.ActiveJobStepTargetUD,
                AssessedNodes = nodesToBeImpacted,
                Timestamp = now,
                AssessedImpact = JobImpactTranslationMode.Default,
            };

            // no previous data
            if (currentJobImpactData == null)
            {
                currentJobImpactData = newJobImpactData;

                traceType.WriteInfo(
                    "New assessed job impact stored. Returning {0}, JobImpactData: {1}, Notification: {2}",
                    currentJobImpactData.AssessedImpact,
                    currentJobImpactData,
                    notification.ToShortDisplayString());

                return currentJobImpactData.AssessedImpact;
            }
            
            // has too much time passed after assessment?
            bool expired = HasPreviousEvaluationExpired(now, currentJobImpactData);
            if (expired)
            {
                currentJobImpactData = newJobImpactData;

                traceType.WriteWarning(
                    "New assessed job impact stored. Time since last assessment is either invalid or has exceeded expiration time. Returning {0}. JobImpactData: {1}, Notification: {2}",
                    currentJobImpactData.AssessedImpact,
                    currentJobImpactData,
                    notification.ToShortDisplayString());

                return currentJobImpactData.AssessedImpact;
            }
            
            bool? restarted = DidPreviouslyAssessedNodesRestart(queriedNodes, currentJobImpactData);
            if (restarted == null)
            {
                traceType.WriteInfo(
                    "Unable to assess job impact, continuing to use previous assessment. Returning {0}, JobImpactData: {1}, Notification: {2}",
                    currentJobImpactData.AssessedImpact,
                    currentJobImpactData,
                    notification.ToShortDisplayString());

                return currentJobImpactData.AssessedImpact;
            }

            currentJobImpactData = newJobImpactData;

            currentJobImpactData.AssessedImpact = restarted.Value
                ? JobImpactTranslationMode.Default
                : JobImpactTranslationMode.Optimized;

            traceType.WriteInfo(
                "New assessed job impact stored. Returning {0}, JobImpactData: {1}, Notification: {2}",
                currentJobImpactData.AssessedImpact,
                currentJobImpactData,
                notification.ToShortDisplayString());

            return currentJobImpactData.AssessedImpact;
        }

        private bool DoesAssessmentMatchNotification(IManagementNotificationContext notification)
        {
            if (currentJobImpactData == null)
            {
                return false;
            }

            bool equals =
                string.Equals(currentJobImpactData.JobId, notification.ActiveJobId, StringComparison.OrdinalIgnoreCase) &&
                currentJobImpactData.UD == notification.ActiveJobStepTargetUD;

            return equals;
        }

        public void Reset()
        {
            currentJobImpactData = null;
        }

        private bool HasPreviousEvaluationExpired(DateTimeOffset now, JobImpactData previousJobImpactData)
        {
            TimeSpan timeElapsedSinceLastAssessment = now - previousJobImpactData.Timestamp;
            TimeSpan validityTimeInterval = GetValidityTimeInterval();

            bool expired = timeElapsedSinceLastAssessment > validityTimeInterval;

            if (expired)
            {
                traceType.WriteInfo(
                    "HasPreviousEvaluationExpired returning {0}. Time elapsed since last assessment: {1}, Validity time interval: {2}, Previous JobImpactData: {3}",
                    expired,
                    timeElapsedSinceLastAssessment,
                    validityTimeInterval,
                    previousJobImpactData);
            }

            return expired;
        }

        private TimeSpan GetValidityTimeInterval()
        {
            var validityTimeInterval = configSection.ReadConfigValue(ConfigLearningModeValidityTimeSpanKeyName, DefaultLearningModeValidityTimeSpan);
            if (validityTimeInterval <= TimeSpan.Zero)
            {
                validityTimeInterval = DefaultLearningModeValidityTimeSpan;
            }

            return validityTimeInterval;
        }

        /// <summary>
        /// Determines if the nodes assessed previously (i.e. in the previous UD or the previous job) restarted.
        /// Returns <c>null</c> if inconclusive.
        /// </summary>
        private bool? DidPreviouslyAssessedNodesRestart(Dictionary<string, INode> queriedNodes, JobImpactData previousJobImpactData)
        {
            var impactedNodes = new Dictionary<string, INode>(StringComparer.OrdinalIgnoreCase);
            var nodesMissingAfterPreviousAssessment = new List<string>();

            foreach (var nodeName in previousJobImpactData.AssessedNodes.Keys)
            {
                if (!queriedNodes.ContainsKey(nodeName))
                {
                    nodesMissingAfterPreviousAssessment.Add(nodeName);
                    continue;
                }
                
                impactedNodes[nodeName] = queriedNodes[nodeName];
            }

            if (nodesMissingAfterPreviousAssessment.Count != 0)
            {
                traceType.WriteInfo(
                    "DidPreviouslyAssessedNodesRestart returning {0}. The following nodes are not detected anymore. They may have been removed. Previous JobImpactData: {1}, Nodes missing: {2}",
                    true,
                    previousJobImpactData,
                    string.Join("; ", nodesMissingAfterPreviousAssessment));

                return true;
            }

            var restartedNodes = new List<string>();
            var notRestartedNodes = new List<string>();
            var unassessableNodes = new List<string>();

            //    Case                                                      Impact
            //    ----                                                      ------
            // 1. node was down during previous assessment itself           
            //    a. All nodes of UD were down during previous assessment   Inconclusive, so don't update current assessment (continue to use previous assessment)
            //    b. Some nodes of UD were down during previous assessment  NoRestart (if other impacted nodes don't need restart)
            // 2. node went down after previous assessment                  Restart
            // 3. node is down currently                                    Restart
            // 4. node has been up all the time                             NoRestart
            
            foreach (var impactedNode in impactedNodes.Values)
            {
                if (previousJobImpactData.AssessedNodes[impactedNode.NodeName].NodeUpTimestamp == null)
                {
                    // the node was down during the previous assessment itself
                    unassessableNodes.Add(impactedNode.NodeName);
                    continue;
                }

                if (impactedNode.NodeUpTimestamp == null ||
                    impactedNode.NodeUpTimestamp.Value > previousJobImpactData.Timestamp)
                {
                    // the node is down (it may have been down before the previous UD was walked, but that
                    // doesn't matter. for safety, we'll consider it as true)
                    restartedNodes.Add(impactedNode.NodeName);
                    continue;
                }

                notRestartedNodes.Add(impactedNode.NodeName);
            }

            bool? restart = restartedNodes.Count > 0;

            if (!restart.Value && impactedNodes.Count > 0 && unassessableNodes.Count == impactedNodes.Count)
            {
                restart = null;
                traceType.WriteInfo(
                    "All nodes were down during previous assessment itself. Impact cannot be evaluated for these nodes. Undetermined nodes: {0}",
                    string.Join("; ", unassessableNodes));
            }

            traceType.WriteInfo(
                "DidPreviouslyAssessedNodesRestart returning {0}. Previous JobImpactData: {1}{2}, Restarted nodes: {3}, Not-restarted nodes: {4}",
                restart.HasValue ? restart.Value.ToString() : "<null>",
                previousJobImpactData,
                Environment.NewLine,
                string.Join("; ", restartedNodes),
                string.Join("; ", notRestartedNodes));

            return restart;
        }

        /// <summary>
        /// Gets the nodes involved in the current notification
        /// </summary>
        private static Dictionary<string, INode> GetNodesInNotification(IManagementNotificationContext notification, Dictionary<string, INode> queriedNodes)
        {
            var impactedNodeMap = new Dictionary<string, INode>(StringComparer.OrdinalIgnoreCase);

            foreach (var instance in notification.ImpactedInstances)
            {
                var name = instance.Id.TranslateRoleInstanceToNodeName();

                if (queriedNodes.ContainsKey(name))
                {
                    impactedNodeMap[name] = queriedNodes[name];
                }                
            }

            return impactedNodeMap;
        }
    }
}