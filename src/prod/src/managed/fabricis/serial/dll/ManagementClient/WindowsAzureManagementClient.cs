// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using Common;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;
    using IImpactDetail = Microsoft.WindowsAzure.ServiceRuntime.Management.Public.IImpactDetail;

    /// <summary>
    /// Client for interacting with Windows Azure management protocol (MR).
    /// </summary>
    internal class WindowsAzureManagementClient : IManagementClient
    {
        #region NotificationContext

        /// <summary>
        /// Wrapper for an MRCP notification object.
        /// </summary>
        private class NotificationContext : IManagementNotificationContext
        {
            private static readonly TraceType TraceType = new TraceType("NotificationContext");

            /// <summary>
            /// A 1:1 mapping layer with the intent of mocking out the MR SDK. 
            /// </summary>
            private static readonly Dictionary<Type, NotificationType> notificationTypeMap = new Dictionary<Type, NotificationType>()
                {
                    { typeof(StartJobStepNotification), NotificationType.StartJobStep },
                    { typeof(CompleteJobStepNotification), NotificationType.CompleteJobStep },
                };

            private readonly ManagementNotification notification;
            private readonly IEnumerable<ImpactedInstance> impactList;
            private readonly bool isTopologyChange;
            
            public NotificationContext(ManagementNotification notification)
            {
                if (notification == null)
                    throw new ArgumentNullException("notification");

                this.notification = notification;
                this.impactList = this.BuildImpactList();
                this.isTopologyChange = IsTopologyChange(this.notification.SourceJobStep);                
            }

            #region properties

            public string NotificationId
            {
                get { return this.notification.Id; }
            }

            public string ActiveJobStepId
            {
                get { return this.notification.SourceJobStep.Id; }
            }

            public string ActiveJobId
            {
                get { return this.notification.SourceJobStep.SourceJob.Id; }
            }

            public NotificationType NotificationType
            {
                get
                {
                    NotificationType type;
                    if (!notificationTypeMap.TryGetValue(this.notification.GetType(), out type))
                    {
                        type = NotificationType.Unknown;
                    }
                    return type;
                }
            }

            public DateTime NotificationDeadline
            {
                get { return this.notification.Deadline; }
            }

            public ManagementNotificationStatus NotificationStatus
            {
                get { return this.notification.Status; }
            }

            public int ActiveJobStepTargetUD
            {
                get { return this.notification.SourceJobStep.TargetUpdateDomain; }
            }

            public JobType ActiveJobType
            {
                get
                {
                    JobType type = this.notification.SourceJobStep.SourceJob.GetJobType();
                    return type;
                }
            }

            public IEnumerable<ImpactedInstance> ImpactedInstances
            {
                get { return this.impactList; }
            }

            public bool ActiveJobIncludesTopologyChange
            {
                get { return this.isTopologyChange; }
            }

            public ulong Incarnation
            {
                get { return notification.Incarnation; }
            }

            public ManagementJobDetailedStatus ActiveJobDetailedStatus
            {
                get { return this.notification.SourceJobStep.SourceJob.DetailedStatus; }
            }

            public string ActiveJobContextId
            {
                get { return this.notification.SourceJobStep.SourceJob.ContextId; }
            }

            public ManagementJobStepStatus ActiveJobStepStatus
            {
                get { return this.notification.JobStepStatus; }
            }

            #endregion properties

            public void SignalReady()
            {
                try
                {
                    this.notification.SignalReady();
                }
                catch (Exception ex)
                {
                    string message = string.Format(
                        CultureInfo.InvariantCulture,
                        "Error invoking management protocol's SignalReady for notification Id: {0}, job step status: {1}, source job step: {2}, status: {3}, incarnation: {4}, deadline: {5}",
                        notification.Id,
                        notification.JobStepStatus,
                        notification.SourceJobStep,
                        notification.Status,
                        notification.Incarnation,
                        notification.Deadline);

                    Trace.WriteWarning(TraceType, message);

                    throw new ManagementException(message, ex);                    
                }                
            }

            public void SignalError(string errorDescription)
            {
                try
                {
                    this.notification.SignalError(errorDescription);
                }
                catch (Exception ex)
                {
                    string message = string.Format(
                        CultureInfo.InvariantCulture,
                        "Error invoking management protocol's SignalError for notification Id: {0}, error description: {1}, job step status: {2}, source job step: {3}, status: {4}, incarnation: {5}, deadline: {6}",
                        notification.Id,
                        errorDescription,
                        notification.JobStepStatus,
                        notification.SourceJobStep,
                        notification.Status,
                        notification.Incarnation,
                        notification.Deadline);

                    Trace.WriteWarning(TraceType, message);

                    throw new ManagementException(message, ex);
                }                
            }

            private IEnumerable<ImpactedInstance> BuildImpactList()
            {
                ManagementJobStep jobStep = this.notification.SourceJobStep;

                IDictionary<string, ImpactedRoleInstance> jobImpactMap;
                IDictionary<string, ImpactedRoleInstance> jobStepImpactMap;
                
                try
                {
                    jobImpactMap = jobStep.SourceJob.ListImpactedRoleInstances();
                    jobStepImpactMap = jobStep.ListImpactedRoleInstances();
                }
                catch (Exception ex)
                {
                    string message = string.Format(
                        CultureInfo.InvariantCulture,
                        "Error getting impacted role instances from management protocol while building impact list for notification Id: {0}, source job step: {1}, source job: {2}, target UD: {3}",
                        notification.Id,
                        jobStep,
                        jobStep.SourceJob,
                        jobStep.TargetUpdateDomain);

                    Trace.WriteWarning(TraceType, message);

                    throw new ManagementException(message, ex);
                }

                List<ImpactedInstance> jobStepImpactList = new List<ImpactedInstance>(jobStepImpactMap.Count);

                // Extract impact list from the job step, potentially augmented with impact from the job
                foreach (var impactedInstance in jobStepImpactMap.Values)
                {
                    string instanceId = impactedInstance.Id;
                    IEnumerable<ImpactReason> jobStepImpact = impactedInstance.ImpactReasons;
                    IImpactDetail jobStepImpactDetail = impactedInstance.ImpactStatement;

                    // Workaround for RDBug#783846: MR platform repair job notifications present no impact reasons for impacted role instances
                    //
                    // If this is a PlatformRepairJob and the job *step* provides no impact reasons,
                    // then merge in the impact reasons given for that role instance in the job.
                    if (this.ActiveJobType == JobType.PlatformRepairJob && !jobStepImpact.Any())
                    {
                        ImpactedRoleInstance jobImpactedInstance;
                        if (jobImpactMap.TryGetValue(instanceId, out jobImpactedInstance))
                        {
                            Trace.WriteWarning(TraceType, "Job step impact for {0} is empty; using impact from job ({1})",
                                instanceId,
                                string.Join(",", jobImpactedInstance.ImpactReasons));

                            jobStepImpact = jobStepImpact.Union(jobImpactedInstance.ImpactReasons);
                        }
                        else
                        {
                            Trace.WriteWarning(TraceType, "Job step impact for {0} is empty, and job provides no impact", instanceId);
                        }
                    }

                    ImpactedInstance element = new ImpactedInstance(
                        instanceId,
                        jobStepImpact.ToArray(),
                        jobStepImpactDetail);

                    jobStepImpactList.Add(element);
                }

                return jobStepImpactList;
            }

            private static bool IsTopologyChange(ManagementJobStep jobStep)
            {
                bool isTopologyChange = false;

                isTopologyChange |= IsTopologyChange(jobStep.ListImpactedRoleInstances().Values);
                isTopologyChange |= IsTopologyChange(jobStep.SourceJob.ListImpactedRoleInstances().Values);

                return isTopologyChange;
            }

            private static bool IsTopologyChange(IEnumerable<ImpactedRoleInstance> impactList)
            {
                return impactList.Any(i => i.ImpactReasons.Any(IsTopologyChange));
            }

            private static bool IsTopologyChange(ImpactReason reason)
            {
                switch (reason)
                {
                    case ImpactReason.AddInstance:
                    case ImpactReason.Relocation:
                    case ImpactReason.RemoveInstance:
                        return true;
                }

                return false;
            }
        }

        #endregion NotificationContext

        private static readonly TraceType TraceType = new TraceType("WindowsAzureManagementClient");

        private readonly DeploymentManagementClient client;

        public WindowsAzureManagementClient()
        {
            this.client = DeploymentManagementClient.CreateInstance();            
        }

        public IManagementNotificationContext GetCurrentNotification()
        {
            IManagementNotificationContext context = null;

            ManagementNotification realNotification;

            try
            {
                realNotification = this.client.GetCurrentNotification();
            }
            catch (Exception ex)
            {
                string message = string.Format(CultureInfo.InvariantCulture, "Error getting current notification from management protocol. Exception: {0}", ex);
                Trace.WriteWarning(TraceType, message);

                throw new ManagementException(message, ex);
            }
            
            if (realNotification != null)
            {
                context = new NotificationContext(realNotification);
            }

            return context;
        }

        public IList<RoleInstance> GetRoleInstances()
        {
            try
            {
                List<RoleInstance> roleInstances = this.client.ListRoleInstances()
                    .Values
                    .Select(instance => new RoleInstance(instance.Id, instance.Status.ToRoleInstanceState(), instance.LastUpdateTime))
                    .ToList();

                return roleInstances.AsReadOnly();
            }
            catch (Exception ex)
            {
                string message = string.Format(CultureInfo.InvariantCulture, "Error getting role instances from management protocol. Exception: {0}", ex);
                Trace.WriteWarning(TraceType, message);

                throw new ManagementException(message, ex);
            }
        }

        /// <summary>
        /// Requests a maintenance action to Azure through the management protocol (MR).
        /// </summary>
        /// <param name="roleInstanceName">Name of the role instance.</param>
        /// <param name="action">The action.</param>
        /// <param name="reason">The reason.</param>
        /// <param name="contextId">An optional context string that will be attached to the MR job resulting from this request.</param>
        /// <exception cref="KeyNotFoundException">
        /// If the role instance for the specified <c>roleInstanceName</c> is not found.
        /// </exception>
        /// <exception cref="ManagementException">
        /// If there was an error from MR while requesting maintenance.
        /// </exception>
        /// <exception cref="InvalidOperationException">If <c>action</c> cannot map to a valid MR maintenance request.</exception>
        public void RequestMaintenance(string roleInstanceName, InfrastructureServiceMaintenanceAction action, string reason, string contextId)
        {
            ManagementRoleInstance roleInstance;

            IDictionary<string, ManagementRoleInstance> roleInstances;

            try
            {
                roleInstances = this.client.ListRoleInstances();
            }
            catch (Exception ex)
            {
                string message = string.Format(
                    CultureInfo.InvariantCulture,
                    "Error getting role instances. Roleinstance name: {0}, action: {1}, reason: {2}, exception: {3}",
                    roleInstanceName,
                    action,
                    reason,
                    ex);

                Trace.WriteWarning(TraceType, message);

                throw new ManagementException(message, ex);
            }

            // null check not necessary
            if (!roleInstances.TryGetValue(roleInstanceName, out roleInstance))
            {
                // not the best exception type to throw.. but suffices
                throw new KeyNotFoundException(
                    string.Format(CultureInfo.InvariantCulture, "Unable to get role instance info for role instance name: {0}", roleInstanceName));
            }

            // Truncate reason, API does not accept reasons over 255 characters
            if (reason != null && reason.Length > 255)
            {
                reason = reason.Substring(0, 255);
            }

            // ContextId max length = 512, according to MR SDK. Do not silently truncate
            // because it is used as a key to match jobs to repair tasks.
            RequestMaintenance(roleInstance, action, string.IsNullOrWhiteSpace(reason) ? "<null>" : reason, contextId);
        }

        /// <summary>
        /// Maps the action object from <see cref="InfrastructureServiceMaintenanceAction" /> to <see cref="MaintenanceAction" />
        /// and invokes the MR API to request maintenance by Azure of the role instance.
        /// </summary>
        /// <param name="roleInstance">The role instance.</param>
        /// <param name="action">The action.</param>
        /// <param name="reason">The reason for requesting the action.</param>
        /// <param name="contextId">An optional context string that will be attached to the MR job resulting from this request.</param>
        /// <exception cref="System.InvalidOperationException">If <c>action</c> cannot map to a valid MR maintenance request.</exception>
        /// <exception cref="ManagementException">If there was an error from MR while requesting maintenance.</exception>
        private static void RequestMaintenance(
            ManagementRoleInstance roleInstance, 
            InfrastructureServiceMaintenanceAction action,
            string reason,
            string contextId)
        {
            Trace.WriteInfo(TraceType, "Requesting maintenance. Roleinstance Id: {0}, action: {1}, reason: {2}, context: {3}", roleInstance.Id, action, reason, contextId);

            switch (action)
            {
                case InfrastructureServiceMaintenanceAction.Exclude:
                case InfrastructureServiceMaintenanceAction.Reboot:
                case InfrastructureServiceMaintenanceAction.ReimageOS:
                case InfrastructureServiceMaintenanceAction.RepaveData:
                case InfrastructureServiceMaintenanceAction.HostReboot:
                case InfrastructureServiceMaintenanceAction.HostRepaveData:
                case InfrastructureServiceMaintenanceAction.Heal:
                    try
                    {
                        roleInstance.RequestMaintenance(action.ToString(), reason, false, contextId);

                        Trace.WriteInfo(
                            TraceType,
                            "Successfully requested maintenance. Roleinstance Id: {0}, action: {1}, reason: {2}, context = {3}",
                            roleInstance.Id,
                            action,
                            reason,
                            contextId);

                    }
                    catch (Exception ex)
                    {
                        string message = string.Format(
                            CultureInfo.InvariantCulture, 
                            "Error while requesting maintenance. Roleinstance Id: {0}, action: {1}, reason: {2}, context: {3}, exception: {4}",
                            roleInstance.Id,
                            action,
                            reason,
                            contextId,
                            ex);

                        Trace.WriteWarning(TraceType, message);

                        throw new ManagementException(message, ex);
                    }

                    break;

                default:
                    throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Unexpected InfrastructureServiceMaintenanceAction: {0}", action));
            }            
        }

        /// <summary>
        /// Gets all the jobs from the MR.
        /// </summary>
        /// <returns>A list of jobs.</returns>
        public IList<IManagementJobWrapper> GetAllJobs()
        {
            try
            {
                var list = client.ListJobs().Values.Select(job => new WindowsAzureManagementJobWrapper(job)).Cast<IManagementJobWrapper>().ToList();

                return list.AsReadOnly();
            }
            catch (Exception ex)
            {
                string message = string.Format(CultureInfo.InvariantCulture, "Error getting jobs from management protocol. Exception: {0}", ex);
                Trace.WriteWarning(TraceType, message);

                throw new ManagementException(message, ex);
            }
        }

        public ulong? GetJobListIncarnation()
        {
            return client.JobListIncarnation;
        }

        public ulong? GetServiceHealthIncarnation()
        {
            return client.ServiceHealthIncarnation;
        }
    }
}