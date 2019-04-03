// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Repair;
    using System.Globalization;
    using System.Linq;
    using Common;
    using ManagementJobStepStatus = Microsoft.WindowsAzure.ServiceRuntime.Management.ManagementJobStepStatus;

    internal sealed class WindowsAzureRepairExecutor
    {
        /// <summary>
        /// Results of CompareRepairToNotification
        /// </summary>
        private enum MatchResult
        {
            None,
            InProgress,
            Complete,
        }

        private const string ConfigRepairExecutionTimeoutInSecondsKeyName = "WindowsAzure.RepairExecutionTimeoutInSeconds";

        /// <summary>
        /// Config key that defines the policy on whether to apply a reboot maintenance action on the Host or on the VM
        /// <seealso cref="InfrastructureServiceMaintenanceAction"/>.
        /// The valid values this key can take are <see cref="InfrastructureServiceMaintenanceAction.Reboot"/> (default) and 
        /// <see cref="InfrastructureServiceMaintenanceAction.HostReboot"/>. E.g. "HostReboot"    
        /// </summary>
        private const string ConfigRebootMaintenanceActionKeyName = "WindowsAzure.RebootMaintenanceAction";
        
        /// <summary>
        /// Config key that defines the policy on whether to apply a repave data maintenance action on the Host or on the VM.
        /// The valid values this key can take are <see cref="InfrastructureServiceMaintenanceAction.RepaveData"/> (default) and 
        /// <see cref="InfrastructureServiceMaintenanceAction.HostRepaveData"/>. E.g. "HostRepaveData"         
        /// </summary>
        private const string ConfigFullReimageMaintenanceActionKeyName = "WindowsAzure.FullReimageMaintenanceAction";

        /// <summary>
        /// The abstract reboot maintenance action string that gets mapped to either HostReboot or Reboot (VM reboot).
        /// </summary>
        private const string AbstractRebootMaintenanceAction = "Reboot";

        /// <summary>
        /// The abstract reimage maintenance action string that gets mapped to either HostRepaveData or RepaveData.
        /// </summary>
        private const string AbstractFullReimageMaintenanceAction = "FullReimage";

        /// <summary>
        /// The format for passing in a command for a system action. E.g. System.Azure.Heal, System.Azure.HostReboot etc.
        /// </summary>
        private const string SystemActionFormat = "System.{0}";

        /// <summary>
        /// Indicates that the action is towards Azure. E.g. MR related actions etc.
        /// </summary>
        private const string ActionNamespaceAzure = "Azure";

        private static readonly TraceType TraceType = new TraceType("WindowsAzureRepairExecutor");
        private static readonly TimeSpan DefaultRepairExecutionTimeout = TimeSpan.FromHours(2);

        private static readonly Uri FabricSystemApplicationUri = new Uri("fabric:/System");
        private static readonly Uri FabricRepairManagerServiceUri = new Uri("fabric:/System/RepairManagerService");

        private readonly Dictionary<string, InfrastructureServiceMaintenanceAction> maintenanceActionMap =
            new Dictionary<string, InfrastructureServiceMaintenanceAction>();

        private readonly FabricClient fabricClient;
        private readonly FabricClient.RepairManagementClient repairManager;

        private readonly WindowsAzureInfrastructureCoordinator coordinator;
        private readonly IManagementClient managementClient;
        private readonly string executorName;

        private IList<RoleInstance> allRoleInstances;
        private bool repairManagerServiceExists;
        
        public WindowsAzureRepairExecutor(
            WindowsAzureInfrastructureCoordinator coordinator,
            string internalServiceName,            
            IManagementClient managementClient)
        {
            this.fabricClient = new FabricClient();
            this.repairManager = fabricClient.RepairManager;
            this.coordinator = coordinator;
            this.managementClient = managementClient;            
            this.executorName = "fabric:/System/" + internalServiceName;            

            this.BuildMaintenanceActionMap();
        }

        private void BuildMaintenanceActionMap()
        {            

            // if users specify a generic command. E.g. Reboot, then apply the config setting policy and determine
            // whether to apply it on the VM or on the Host
            InfrastructureServiceMaintenanceAction reboot = 
                GetMappedMaintenanceAction(ConfigRebootMaintenanceActionKeyName, InfrastructureServiceMaintenanceAction.Reboot);
            InfrastructureServiceMaintenanceAction repaveData = 
                GetMappedMaintenanceAction(ConfigFullReimageMaintenanceActionKeyName, InfrastructureServiceMaintenanceAction.RepaveData);

            AddMaintenanceAction(reboot, AbstractRebootMaintenanceAction);
            AddMaintenanceAction(repaveData, AbstractFullReimageMaintenanceAction);

            // There is no Host/VM target for ReimageOS currently. It is just applicable to VMs
            AddMaintenanceAction(InfrastructureServiceMaintenanceAction.ReimageOS);
            
            // if users specify the command explicitly. E.g. System.Azure.Reboot, then apply the foll. irrespective of the 
            // *MaintenanceAction config setting
            // Note: Unlike ReimageOS above, we haven't added a generic 'Heal' action. Instead users have to specify this explicitly
            AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction.Reboot);
            AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction.ReimageOS);
            AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction.RepaveData);
            AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction.HostReboot);
            AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction.HostRepaveData);            
            AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction.Heal);
        }

        /// <summary>
        /// This method is used to create an explicit maintenance action. E.g. System.Azure.HostReboot.
        /// </summary>
        /// <param name="action">The action.</param>
        private void AddMaintenanceActionWithNamespace(InfrastructureServiceMaintenanceAction action)
        {
            string command = string.Format(CultureInfo.InvariantCulture, "{0}.{1}", ActionNamespaceAzure, action);
            
            AddMaintenanceAction(action, command);
        }

        /// <summary>
        /// This method adds a generic maintenance action to a map of known actions. 
        /// This allows users to specify a simpler maintenance action string e.g. 'Reboot' which then gets mapped to either
        /// System.Azure.Reboot or System.Azure.HostReboot based on configuration settings.
        /// </summary>
        /// <param name="action">The action.</param>
        /// <param name="command">The command.</param>
        private void AddMaintenanceAction(InfrastructureServiceMaintenanceAction action, string command = null)
        {
            string key = string.Format(CultureInfo.InvariantCulture, SystemActionFormat, command ?? action.ToString());
            maintenanceActionMap.Add(key, action);

            Trace.WriteInfo(TraceType, "Enabled execution of repair command: {0} with action: {1}", key, action);
        }

        /// <summary>
        /// Claims and executes requested repairs
        /// </summary>
        /// <param name="executeRepairs">true if this call should execute the next ready repair task, false otherwise</param>
        /// <param name="currentNotification">The current MR notification</param>
        public void ProcessRepairs(bool executeRepairs, IManagementNotificationContext currentNotification)
        {
            if (!this.RepairManagerServiceExists())
            {
                Trace.WriteInfo(TraceType, "RepairManager service not created; skipping repair execution");
                return;
            }

            try
            {
                this.ProcessNewTasks();
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "ProcessNewTasks exception: {0}", e);
            }

            try
            {
                this.ProcessReadyTasks(executeRepairs, currentNotification);
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "ProcessReadyTasks exception: {0}", e);
            }
        }

        /// <summary>
        /// Checks if the Repair Manager service has been created.
        /// Caches the result, so that querying stops after the service is detected once.
        /// </summary>
        private bool RepairManagerServiceExists()
        {
            if (!this.repairManagerServiceExists)
            {
                var result = this.fabricClient.QueryManager.GetServiceListAsync(
                    FabricSystemApplicationUri,
                    FabricRepairManagerServiceUri).Result;

                Trace.WriteInfo(TraceType, "GetServiceList('{0}','{1}'): result count = {2}",
                    FabricSystemApplicationUri,
                    FabricRepairManagerServiceUri,
                    result.Count);

                this.repairManagerServiceExists = (result.Count > 0);
            }

            return this.repairManagerServiceExists;
        }

        /// <summary>
        /// Claims any repairs in the Created state that this executor understands how to execute
        /// </summary>
        private void ProcessNewTasks()
        {
            // Refresh list of role instances
            this.allRoleInstances = this.managementClient.GetRoleInstances();

            var taskList = this.GetUnclaimedRepairTasks();
            int claimedCount = 0;

            foreach (var repairTask in taskList)
            {
                try
                {
                    if (this.ProcessUnclaimedTask(repairTask))
                    {
                        ++claimedCount;
                    }
                }
                catch (Exception e)
                {
                    Trace.WriteWarning(TraceType, "ProcessUnclaimedTask failed for {0}: {1}", repairTask.TaskId, e);
                }
            }

            Trace.WriteInfo(TraceType, "Claimed {0} of {1} unclaimed repair tasks", claimedCount, taskList.Count);
        }

        private static string GetTargetNodeName(RepairTask repairTask)
        {
            if (repairTask.Target == null || repairTask.Target.Kind != RepairTargetKind.Node)
            {
                Trace.WriteInfo(TraceType, "Repair task {0}: ignoring unknown target type {1}", repairTask.TaskId, repairTask.Target);
                return null;
            }

            NodeRepairTargetDescription targetDescription = (NodeRepairTargetDescription)repairTask.Target;
            if (targetDescription.Nodes.Count != 1)
            {
                Trace.WriteInfo(TraceType, "Repair task {0}: ignoring bad target count {1}", repairTask.TaskId, targetDescription.Nodes.Count);
                return null;
            }

            string targetNodeName = targetDescription.Nodes[0];

            return targetNodeName;
        }

        /// <summary>
        /// Processes a single unclaimed task, and claims or ignores it as appropriate
        /// </summary>
        private bool ProcessUnclaimedTask(RepairTask repairTask)
        {
            // Is this an action that this executor can process?
            InfrastructureServiceMaintenanceAction? action = GetMaintenanceAction(repairTask.Action);
            if (action == null)
            {
                Trace.WriteInfo(TraceType, "Repair task {0}: ignoring unknown action {1}", repairTask.TaskId, repairTask.Action);
                return false;
            }

            if (repairTask.Scope.Kind != RepairScopeIdentifierKind.Cluster)
            {
                Trace.WriteInfo(TraceType, "Repair task {0}: ignoring unknown scope {1}", repairTask.TaskId, repairTask.Scope);
                return false;
            }

            // Ignore requests for anything other than a single node
            string targetNodeName = GetTargetNodeName(repairTask);
            if (targetNodeName == null)
            {
                Trace.WriteInfo(TraceType, "Repair task {0}: ignoring bad target description {1}", repairTask.TaskId, repairTask.Target);
                return false;
            }

            // Map Windows Fabric node name to Windows Azure role instance ID
            string targetRoleInstance = targetNodeName.TranslateNodeNameToRoleInstance();                

            // Is the target a valid role instance in this tenant?
            if (!this.IsKnownRoleInstance(targetRoleInstance))
            {
                Trace.WriteInfo(TraceType, "Repair task {0}: ignoring unknown role instance {1}", repairTask.TaskId, targetRoleInstance);
                return false;
            }

            // All checks passed; this executor can execute this repair task
            repairTask.Executor = this.executorName;

            // Zero impact, because approval for actual execution will go through the
            // existing infrastructure task mechanism (MR job queue)
            repairTask.State = RepairTaskState.Preparing;
            repairTask.Impact = new NodeRepairImpactDescription();

            long commitVersion = this.repairManager.UpdateRepairExecutionStateAsync(repairTask).Result;
            Trace.WriteInfo(TraceType, "Repair task {0}: moved to Preparing (commit version = {1})", repairTask.TaskId, commitVersion);

            return true;
        }

        /// <summary>
        /// Performs work on any repairs owned by this executor that are in the Approved or Executing states
        /// </summary>
        private void ProcessReadyTasks(bool executeRepairs, IManagementNotificationContext currentNotification)
        {
            TimeSpan executionTimeout = this.GetRepairExecutionTimeout();
            List<RepairTask> approvedTasks = new List<RepairTask>();

            foreach (var repairTask in this.GetReadyRepairTasks())
            {
                try
                {
                    if (repairTask.State == RepairTaskState.Executing)
                    {
                        MatchResult match = CompareRepairToNotification(repairTask, currentNotification);

                        if (match == MatchResult.Complete)
                        {
                            string details = string.Format(
                                CultureInfo.InvariantCulture,
                                "Completed Windows Azure job {0} ({1})",
                                currentNotification.ActiveJobId,
                                currentNotification.ActiveJobStepStatus);

                            this.UpdateRepairToRestoring(
                                repairTask,
                                TranslateJobStepStatusToRepairTaskResult(currentNotification.ActiveJobStepStatus),
                                details);
                        }
                        else if (match == MatchResult.InProgress)
                        {
                            // Saw an executing task that is in progress, don't do any other work
                            executeRepairs = false;

                            Trace.WriteInfo(TraceType,
                                "Repair task {0}: currently executing (job = {1}, notif status = {2}, job step status = {3}, job detailed status = {4})",
                                repairTask.TaskId,
                                currentNotification.ActiveJobId,
                                currentNotification.NotificationStatus,
                                currentNotification.ActiveJobStepStatus,
                                currentNotification.ActiveJobDetailedStatus);

                            string details = string.Format(
                                CultureInfo.InvariantCulture,
                                "Windows Azure job {0} ({1}/{2}/{3})",
                                currentNotification.ActiveJobId,
                                currentNotification.NotificationStatus,
                                currentNotification.ActiveJobStepStatus,
                                currentNotification.ActiveJobDetailedStatus);

                            // Publish details on the repair task, to aid troubleshooting stuck repairs
                            if (!details.Equals(repairTask.ResultDetails))
                            {
                                repairTask.ResultDetails = details;
                                this.repairManager.UpdateRepairExecutionStateAsync(repairTask).Wait();
                            }
                        }
                        else if (DateTime.UtcNow - repairTask.ExecutingTimestamp > executionTimeout)
                        {
                            string message = string.Format(
                                CultureInfo.InvariantCulture,
                                "Repair timed out after being sent to Windows Azure for execution (timeout = {0})",
                                executionTimeout);

                            this.UpdateRepairToRestoring(
                                repairTask,
                                RepairTaskResult.Failed,
                                message);
                        }
                        else
                        {
                            // Saw an executing task that has not timed out, don't do any other work
                            executeRepairs = false;

                            Trace.WriteInfo(TraceType, "Repair task {0}: currently executing (unknown job)", repairTask.TaskId);
                        }
                    }
                    else if (repairTask.State == RepairTaskState.Approved)
                    {
                        if (repairTask.Flags.HasFlag(RepairTaskFlags.CancelRequested))
                        {
                            this.UpdateRepairToRestoring(
                                repairTask,
                                RepairTaskResult.Cancelled,
                                "Cancelled by user request");
                        }
                        else
                        {
                            approvedTasks.Add(repairTask);
                        }
                    }
                }
                catch (Exception e)
                {
                    Trace.WriteWarning(TraceType, "Failed to process ready task {0}: {1}", repairTask.TaskId, e);
                }
            }

            if (executeRepairs)
            {
                // Start executing the approved repair (if any) that has the oldest creation time
                var oldestApprovedTask = approvedTasks.OrderBy(t => t.CreatedTimestamp).FirstOrDefault();
                if (oldestApprovedTask == null)
                {
                    Trace.WriteInfo(TraceType, "No new repairs to send to Windows Azure");
                }
                else
                {
                    Trace.WriteInfo(TraceType, "Executing repair with oldest approved task Id: {0}", oldestApprovedTask.TaskId);
                    this.ExecuteRepair(oldestApprovedTask);
                }
            }
            else
            {
                Trace.WriteInfo(TraceType, "Not executing new repairs; approved count = {0}", approvedTasks.Count);
            }
        }

        private static RepairTaskResult TranslateJobStepStatusToRepairTaskResult(ManagementJobStepStatus jobStepStatus)
        {
            switch (jobStepStatus)
            {
                case ManagementJobStepStatus.Executed:
                    return RepairTaskResult.Succeeded;

                case ManagementJobStepStatus.NotExecuted:
                    return RepairTaskResult.Interrupted;

                default:
                    return RepairTaskResult.Failed;
            }
        }

        private static string GenerateJobContextId(RepairTask task)
        {
            return string.Format("{0}@{1:O}", task.TaskId, task.CreatedTimestamp);
        }

        private static bool NotificationMatchesRepairTask(RepairTask repairTask, IManagementNotificationContext notification)
        {
            if (notification == null)
            {
                return false;
            }

            if (notification.ActiveJobType != JobType.DeploymentMaintenanceJob)
            {
                return false;
            }

            // Exact match of the context ID provided when RequestMaintenance was called
            if (string.Equals(notification.ActiveJobContextId, GenerateJobContextId(repairTask)))
            {
                return true;
            }

            // TODO return false here if strict matching is enabled

            // No exact match; try to guess if the job is related to the repair task
            string repairTarget = GetTargetNodeName(repairTask);
            if (repairTarget == null)
            {
                return false;
            }

            string roleInstanceName = repairTarget.TranslateNodeNameToRoleInstance();

            if (!notification.ImpactedInstances.Any(i => i.Id == roleInstanceName))
            {
                return false;
            }

            // TODO compare impact reasons?

            return true;
        }

        /// <summary>
        /// Determines whether the given repair task matches the MR notification, and if so, whether it has completed.
        /// </summary>
        private static MatchResult CompareRepairToNotification(RepairTask repairTask, IManagementNotificationContext notification)
        {
            if (!NotificationMatchesRepairTask(repairTask, notification))
            {
                return MatchResult.None;
            }

            if (notification.NotificationType == NotificationType.CompleteJobStep)
            {
                return MatchResult.Complete;
            }
            else
            {
                return MatchResult.InProgress;
            }
        }

        /// <summary>
        /// Begins execution of a single repair by sending the request to Windows Azure
        /// </summary>
        /// <param name="repairTask">The repair task.</param>
        private void ExecuteRepair(RepairTask repairTask)
        {
            string targetRoleInstance = null;

            string targetNodeName = GetTargetNodeName(repairTask);
            if (targetNodeName != null)
            {
                targetRoleInstance = targetNodeName.TranslateNodeNameToRoleInstance();
            }

            InfrastructureServiceMaintenanceAction? action = GetMaintenanceAction(repairTask.Action);

            if (action == null)
            {
                // Reject due to bad target or action
                this.UpdateRepairToRestoring(
                    repairTask,
                    RepairTaskResult.Failed,
                    "Approved repair has invalid parameters");
            }
            else
            {
                // Ready to execute
                long commitVersion = this.UpdateRepairToExecuting(repairTask);

                Exception exception = null;

                try
                {
                    this.managementClient.RequestMaintenance(
                        targetRoleInstance,
                        action.Value,
                        repairTask.TaskId,
                        GenerateJobContextId(repairTask));

                    Trace.WriteInfo(
                        TraceType,
                        "Successfully requested maintenance in ExecuteRepair. Target role instance: {0}, action: {1}, repair task Id: {2}",
                        targetRoleInstance,
                        action.Value,
                        repairTask.TaskId);

                }
                catch (KeyNotFoundException ex)
                {
                    // Request failed; bad role instance ID.  Fail the repair task.
                    exception = ex;

                    Trace.WriteWarning(
                        TraceType,
                        "Request maintenance failed in ExecuteRepair due to role instance name not being found. Target role instance: {0}, action: {1}, repair task Id: {2}, exception: {3}",
                        targetRoleInstance,
                        action.Value,
                        repairTask.TaskId,
                        ex);

                }
                catch (ManagementException ex)
                {
                    // Request failed; due to MR. Fail the repair task.
                    exception = ex;
                    
                    Trace.WriteWarning(
                        TraceType,
                        "Request maintenance failed in ExecuteRepair due to management protocol. Target role instance: {0}, action: {1}, repair task Id: {2}, exception: {3}",
                        targetRoleInstance,
                        action.Value,
                        repairTask.TaskId,
                        ex);
                }

                if (exception != null)
                {
                    // Must specify commit version of the previous update.
                    repairTask.Version = commitVersion;

                    this.UpdateRepairToRestoring(
                        repairTask,
                        RepairTaskResult.Failed,
                        "RequestMaintenance failed; check target name");
                }
            }
        }

        private long UpdateRepairToExecuting(RepairTask task)
        {
            Trace.WriteInfo(TraceType, "Updating ready task {0} from {1} to {2}",
                task.TaskId,
                task.State,
                RepairTaskState.Executing);

            task.State = RepairTaskState.Executing;

            long commitVersion = this.repairManager.UpdateRepairExecutionStateAsync(task).Result;
            Trace.WriteInfo(TraceType, "Committed version {0}", commitVersion);

            return commitVersion;
        }

        private void UpdateRepairToRestoring(RepairTask task, RepairTaskResult result, string details)
        {
            string message = string.Format(
                CultureInfo.InvariantCulture,
                "Updating task {0} from {1} to {2}; result = {3}, details = {4}",
                task.TaskId,
                task.State,
                RepairTaskState.Restoring,
                result,
                details);

            if (result == RepairTaskResult.Succeeded || result == RepairTaskResult.Cancelled)
            {
                Trace.WriteInfo(TraceType, message);
            }
            else
            {
                Trace.WriteWarning(TraceType, message);
            }

            task.State = RepairTaskState.Restoring;
            task.ResultStatus = result;
            task.ResultDetails = details;

            long commitVersion = this.repairManager.UpdateRepairExecutionStateAsync(task).Result;
            Trace.WriteInfo(TraceType, "Committed version {0}", commitVersion);
        }

        private RepairTaskList GetUnclaimedRepairTasks()
        {
            var tasks = this.repairManager.GetRepairTaskListAsync(null, RepairTaskStateFilter.Created, null).Result;
            Trace.WriteInfo(TraceType, "Unclaimed repair tasks count: {0}", tasks.Count);

            return tasks;
        }

        private RepairTaskList GetReadyRepairTasks()
        {
            var tasks = this.repairManager.GetRepairTaskListAsync(null, RepairTaskStateFilter.ReadyToExecute, this.executorName).Result;
            Trace.WriteInfo(TraceType, "Ready repair tasks count: {0}", tasks.Count);

            return tasks;
        }

        private bool IsKnownRoleInstance(string roleInstanceName)
        {
            return this.allRoleInstances.Any(r => r.Id.Equals(roleInstanceName));
        }

        /// <summary>
        /// Gets the maintenance action from a string matching an <see cref="InfrastructureServiceMaintenanceAction"/>.        
        /// </summary>
        /// <param name="action">The action.</param>
        /// <returns>
        /// A <see cref="InfrastructureServiceMaintenanceAction"/> object or null if the action didn't match any of the allowed actions.
        /// </returns>
        private InfrastructureServiceMaintenanceAction? GetMaintenanceAction(string action)
        {
            InfrastructureServiceMaintenanceAction maintenanceAction;

            if (this.maintenanceActionMap.TryGetValue(action, out maintenanceAction))
            {
                return maintenanceAction;
            }

            return null;
        }

        private TimeSpan GetRepairExecutionTimeout()
        {
            double timeoutInSeconds = this.coordinator.ReadConfigValue(
                ConfigRepairExecutionTimeoutInSecondsKeyName, 
                DefaultRepairExecutionTimeout.TotalSeconds);

            return TimeSpan.FromSeconds(timeoutInSeconds);
        }

        /// <summary>
        /// Gets a config setting that governs on what target (currently supported are Host or VM) we execute the maintenance action.
        /// </summary>
        /// <param name="configKeyName">Name of the configuration key.</param>
        /// <param name="defaultValue">The default value to be used if we are unable to get it from the config setting.</param>
        /// <returns>
        /// The config setting value.
        /// </returns>
        private InfrastructureServiceMaintenanceAction GetMappedMaintenanceAction(
            string configKeyName, 
            InfrastructureServiceMaintenanceAction defaultValue)
        {
            var action = coordinator.ReadConfigValue(configKeyName, defaultValue);
            
            Trace.WriteInfo(TraceType, "Maintenance action for key name: {0} maps to: {1}", configKeyName, action);

            return action;
        }
    }
}