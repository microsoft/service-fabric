// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Interop;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;
    internal interface ICoordinatorCommandProcessor
    {
        CoordinatorDiagnosticInfo CoordinatorDiagnosticInfo { get; set; }

        CoordinatorContext CoordinatorContext { get; set; }

        Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout,
            CancellationToken cancellationToken);

        void Reset();
    }

    /// <summary>
    /// Helper class that just moves the command processing functionality out of <see cref="AzureParallelInfrastructureCoordinator"/>.
    /// </summary>
    internal sealed class CoordinatorCommandProcessor : ICoordinatorCommandProcessor
    {
        private static readonly char[] CommandTokenDelimiter = { ':' };

        private readonly CoordinatorEnvironment environment;
        private readonly TraceType traceType;

        private readonly CommandHandler commandHandler;

        private readonly IPolicyAgentClient policyAgentClient;

        private readonly IJobBlockingPolicyManager jobBlockingPolicyManager;

        private readonly IAllowActionMap allowActionMap;

        private readonly object locker = new object();

        private CoordinatorDiagnosticInfo coordinatorDiagnosticInfo;

        private CoordinatorContext coordinatorContext;

        public CoordinatorCommandProcessor(
            CoordinatorEnvironment environment,
            IPolicyAgentClient policyAgentClient,
            IJobBlockingPolicyManager jobBlockingPolicyManager,
            IAllowActionMap allowActionMap)
        {
            this.environment = environment.Validate("environment");
            this.policyAgentClient = policyAgentClient.Validate("policyAgentClient");
            this.jobBlockingPolicyManager = jobBlockingPolicyManager.Validate("jobBlockingPolicyManager");
            this.allowActionMap = allowActionMap.Validate("allowActionMap");

            this.traceType = environment.CreateTraceType("CommandProcessor");
            this.commandHandler = new CommandHandler(this.traceType);

            RegisterCommandHandlers();
        }

        /// <summary>
        /// Remember to update this property before calling <see cref="HandleCommandGetCurrentStateAsync"/>
        /// </summary>
        public CoordinatorDiagnosticInfo CoordinatorDiagnosticInfo
        {
            get
            {
                lock (locker)
                {
                    return coordinatorDiagnosticInfo;
                }
            }
            set
            {
                lock (locker)
                {
                    coordinatorDiagnosticInfo = value;
                }
            }
        }

        public CoordinatorContext CoordinatorContext
        {
            get
            {
                lock (locker)
                {
                    return coordinatorContext;
                }
            }
            set
            {
                lock (locker)
                {
                    coordinatorContext = value;
                }
            }
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return commandHandler.ExecuteCommandAsync(command, isAdminCommand);
        }

        public void Reset()
        {
            allowActionMap.Clear();
        }

        #region command handling

        // TODO, handle exceptions thrown by the handlers and return a descriptive string
        private void RegisterCommandHandlers()
        {
            // Admin commands
            RegisterCommand(
                "AcknowledgeJob",
                HandleCommandAckJobsAsync,
                "Acknowledges the job with the given ID, allowing it to proceed.\n" +
                "Usage: AcknowledgeJob:<JobDocumentIncarnation>:<JobID>[,<JobID>[,...]]\n" +
                "The last known current job document incarnation number is returned from the GetCurrentState command. " +
                "This is used as an optimistic concurrency check, and the AcknowledgeJob command will only " +
                "take effect if the provided incarnation number matches the current incarnation number.\n" +
                "Acknowledging jobs via this command bypasses automated safety and health checks, and may cause availability loss or data loss.",
                isAdminCommand: true);

            RegisterCommand(
                "AlertJob",
                HandleCommandAlertJobsAsync,
                "Alerts the job with the given ID.\n" +
                "Usage: AlertJob:<JobDocumentIncarnation>:<JobID>[,<JobID>[,...]]\n" +
                "The last known current job document incarnation number is returned from the GetCurrentState command. " +
                "This is used as an optimistic concurrency check, and the AcknowledgeJob command will only " +
                "take effect if the provided incarnation number matches the current incarnation number.",
                isAdminCommand: true,
                documented: false);

            RegisterCommand(
                "FailFast",
                HandleCommandFailFastAsync,
                "Causes the primary InfrastructureService replica to exit by calling Environment.FailFast().",
                isAdminCommand: true,
                documented: false);

            // this command is provided only for debug purposes since the proper mechanism is now via a new repair task
            RegisterCommand(
                "RequestMaintenance",
                HandleCommandRequestMaintenanceAsync,
                "Requests maintenance to Azure. Command: RequestMaintenance:<role instance>:<maintenance action>:<optional reason>. E.g. RequestMaintenance:ServiceFabricRole_IN_1:Reboot:testing_1",
                isAdminCommand: true,
                documented: false);

            RegisterCommand(
                "RequestSuspend",
                HandleCommandRequestSuspendAsync,
                "Requests Azure infrastructure to suspend a PlatformUpdate job. Command: RequestSuspend:<Job Id>",
                isAdminCommand: true,
                documented: false); // privileged tenants only

            RegisterCommand(
                "RequestResume",
                HandleCommandRequestResumeAsync,
                "Requests Azure infrastructure to resume a suspended PlatformUpdate job. Command: RequestResume:<Job Id>",
                isAdminCommand: true,
                documented: false); // privileged tenants only

            RegisterCommand(
                "RequestCancel",
                HandleCommandRequestCancelAsync,
                "Requests Azure infrastructure to cancel a job. Command: RequestCancel:<Job Id>",
                isAdminCommand: true,
                documented: false);

            RegisterCommand(
                "RequestRollback",
                HandleCommandRequestRollbackAsync,
                "Requests Azure infrastructure to rollback a job. Command: RequestSuspend:<Job Id>:<UD incarnation>",
                isAdminCommand: true,
                documented: false);

            RegisterCommand("Help", HandleCommandHelpAsync, "Displays help.");

            RegisterCommand(
                "GetDoc",
                HandleCommandGetDocAsync,
                "Gets the policy agent document for the tenant",
                isAdminCommand: false,
                documented: false);

            RegisterCommand(
                "GetDocRaw",
                HandleCommandGetDocRawAsync,
                "Gets the policy agent document for the tenant in its original binary form",
                isAdminCommand: false,
                documented: false);

            // User commands
            RegisterCommand(
                "GetCurrentState",
                HandleCommandGetCurrentStateAsync,
                "Gets the current state of the Infrastructure Service.");

            RegisterCommand(
                "GetJobs",
                HandleCommandGetJobsAsync,
                "Gets the Azure infrastructure jobs that act on this Azure tenant",
                documented: false);

            RegisterCommand(
                "GetRoleInstances",
                HandleCommandGetRoleInstancesAsync,
                "Gets Azure role instances (or VM scale set instances)",
                documented: false);

            RegisterCommand(
                "GetRepairTasks",
                HandleCommandGetRepairTasksAsync,
                "Gets the Service Fabric repair tasks",
                documented: false);

            // skip the Invalid option
            var jobBlockingPolicies = string.Join(", ", Enum.GetNames(typeof (JobBlockingPolicy)).Skip(1));

            RegisterCommand(
                "UpdateJobBlockingPolicy",
                HandleUpdateJobBlockingPolicyAsync,
                "Updates the job blocking policy. This allows blocking certain types of Azure jobs without performing a configuration upgrade.\n" +
                "The valid options are: " + jobBlockingPolicies + "\n" +
                "Usage: UpdateJobBlockingPolicy:<Policy>",
                isAdminCommand: true);

            var actionTypes = string.Join(", ", Enum.GetNames(typeof(ActionType)));

            RegisterCommand(
                "AllowAction",
                HandleCommandAllowActionAsync,
                "Allows the action on a certain Azure infrastructure job Id/UD. " +
                "This is used for explicitly overriding the decisions made by the Infrastructure Service on the job. " +
                "Before using this, ensure that the appropriate JobBlockingPolicy is set. (Please see command: UpdateJobBlockingPolicy). " +
                "Command: AllowAction:<JobId>:<UD>:<ActionTypeFlag1,ActionTypeFlag2> " +
                "The valid ActionType options are: " + actionTypes +
                ". E.g. AllowAction:88ca0d52-d04d-4cbd-b70c-e71ced31a05c:0:Prepare",
                isAdminCommand: true,
                documented: false);

            RegisterCommand(
                "FinishInfrastructureTask",
                HandleCommandFinishInfrastructureTask,
                "(Reserved)",
                isAdminCommand: true,
                documented: false);
        }

        /// <example>Help</example>
        /// <example>Help:All</example>
        private Task<string> HandleCommandHelpAsync(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            bool allCommands = false;

            if (tokens.Length == 1)
            {
                if (string.Equals(tokens[0], "All", StringComparison.OrdinalIgnoreCase))
                {
                    allCommands = true;
                }
            }

            var json = commandHandler
                .Commands
                .Values
                .Where(e => allCommands || e.Documented)
                .Select(e =>
                    new
                    {
                        e.Command,
                        e.Description,
                        e.IsAdminCommand,
                    })
                .ToJson();

            return Task.FromResult(json);
        }

        /// <summary>
        /// Undocumented helper method since the PowerShell Get-ServiceFabricRepairTask returns timestamps without second/millisecond info.
        /// Not intended for automation use. Use Get-ServiceFabricRepairTask instead.
        /// </summary>
        private Task<string> HandleCommandGetRepairTasksAsync(string args)
        {
            List<IRepairTask> repairTaskList;

            CoordinatorContext cc = CoordinatorContext;
            if (cc == null)
            {
                throw Trace.CreateException(
                    traceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                    "Job information is not available");
            }

            repairTaskList = cc.MappedRepairTasks.Values.Select(e => e.RepairTask).ToList();

            var json = repairTaskList.ToJson();
            return Task.FromResult(json);
        }

        /// <summary>
        /// RequestMaintenance:[RoleInstanceName]:[RepairActionTypeEnum][:optional Reason] 
        /// E.g.
        /// RequestMaintenance:ServiceFabricRole_IN_2:Reboot:mitigating_incident_123456
        /// </summary>
        private async Task<string> HandleCommandRequestMaintenanceAsync(string args)
        {
            traceType.WriteInfo("Requesting maintenance. Input: {0}", args);

            string[] tokens = args.Split(CommandTokenDelimiter, 3);
            if (tokens.Length < 2)
            {
                throw new ArgumentException("Invalid command arguments");
            }

            string roleInstanceName = tokens[0];

            RepairActionTypeEnum action;
            if (!Enum.TryParse(tokens[1], out action))
            {
                throw new ArgumentException("Invalid command arguments");
            }

            var activityId = Guid.NewGuid();

            // Bond doesn't like a null value for RepairRequest.Reason
            string reason = tokens.Length >= 3 ? reason = tokens[2] : string.Empty;

            try
            {
                var repairRequest = new RepairRequest
                {
                    RoleInstanceId = roleInstanceName,
                    ContextId = activityId.ToString(),
                    Action = action,
                    Reason = reason
                };

                string requestJson = repairRequest.ToJson();

                traceType.WriteInfo("Requesting maintenance. {0}", requestJson);
                await policyAgentClient.RequestRepairAsync(activityId, repairRequest, CancellationToken.None).ConfigureAwait(false);

                traceType.WriteInfo("Successfully requested maintenance. {0}", requestJson);
                return requestJson;
            }
            catch (ManagementException)
            {
                traceType.WriteWarning(
                    "Error in management protocol while requesting maintenance of role instance: {0} with action: {1} and reason: {2}",
                    roleInstanceName,
                    action,
                    reason);

                throw;
            }
        }

        private async Task<string> HandleCommandGetRoleInstancesAsync(string args)
        {
            traceType.WriteInfo("GetRoleInstances requested");

            var activityId = Guid.NewGuid();

            IPolicyAgentDocumentForTenant doc = await policyAgentClient.GetDocumentAsync(activityId, CancellationToken.None).ConfigureAwait(false);

            string json = new RoleInstanceCollectionData
            {
                RoleInstances = doc.GetRoleInstances(),
            }.ToJson();

            return json;
        }

        private async Task<string> HandleCommandGetCurrentStateAsync(string args)
        {
            traceType.WriteInfo("GetCurrentState requested");

            CoordinatorContext cc = CoordinatorContext;
            
            var jobBlockingPolicy = await jobBlockingPolicyManager.GetPolicyAsync().ConfigureAwait(false);

            //var actionsToAllow = allowActionMap.Get().Cast<AllowActionRecord>().ToList();

            var coordinatorStateData = new CoordinatorStateData
            {
                JobBlockingPolicy = jobBlockingPolicy.ToString(),
                //JobSetsRequiringApproval = cc.Doc.JobSetsRequiringApproval,
                CoordinatorStartTime = CoordinatorDiagnosticInfo.CoordinatorStartTime,
                AzureTenantId = CoordinatorDiagnosticInfo.AzureTenantId,
                AssemblyFileVersion = CoordinatorDiagnosticInfo.AssemblyFileVersion,
                //OperatorAllowedActions = actionsToAllow,                               
            };

            if (cc != null)
            {
                var jobs = new List<JobSummary>();

                foreach (var j in cc.MappedTenantJobs.Values)
                {
                    jobs.Add(j.GetSummary());
                }

                coordinatorStateData.Jobs = jobs;
                coordinatorStateData.JobDocumentIncarnation = cc.Doc.JobDocumentIncarnation.ToString();
                coordinatorStateData.LastRunStartTime = cc.RunStartTime.ToString("O");
                coordinatorStateData.LastRunFinishTime = cc.RunFinishTime.ToString("O");
            }

            return coordinatorStateData.ToJson();
        }

        /// <example>
        /// AllowAction:a2e1bace-512a-4bbf-ba3b-6aa2cd9c10a2:2:Execute
        /// AllowAction:a2e1bace-512a-4bbf-ba3b-6aa2cd9c10a2:*:Restore
        /// </example>>
        private Task<string> HandleCommandAllowActionAsync(string args)
        {
            traceType.WriteInfo("AllowAction requested. Input: {0}", args);

            string[] tokens = args.Split(CommandTokenDelimiter, 3);
            if (tokens.Length < 3)
            {
                throw new ArgumentException("Invalid command arguments");
            }

            Guid jobId;
            if (!Guid.TryParse(tokens[0], out jobId))
            {
                throw new ArgumentException("Invalid command argument for JobId: {0}".ToString(tokens[0]));
            }

            string udString = tokens[1];
            uint? ud;
            if (udString == "*")
            {
                ud = null;
            }
            else
            {
                uint parsedUD;
                if (!uint.TryParse(udString, out parsedUD))
                {
                    throw new ArgumentException("Invalid command argument for UD: {0}".ToString(udString));
                }
                ud = parsedUD;
            }

            // A flags enum can be parsed from a string if the string is like e.g. "Prepare, Execute" or "Prepare,Execute" or " Prepare , Execute ".
            ActionType actionType;
            if (!Enum.TryParse(tokens[2], true, out actionType))
            {
                throw new ArgumentException("Invalid command argument for ActionType: {0}".ToString(tokens[2]));
            }

            var record = new AllowActionRecord(jobId, ud, actionType);

            allowActionMap.Set(record);

            // TODO clean/scrub this map periodically
            var now = DateTimeOffset.UtcNow;
            var removedItems = new List<IAllowActionRecord>();

            foreach (var aa in allowActionMap.Get().Where(e => e.CreationTime < now - TimeSpan.FromHours(1))) // TODO expiry as config
            {
                if (!CoordinatorContext.MappedTenantJobs.ContainsKey(aa.JobId))
                {
                    allowActionMap.Remove(aa.JobId);
                    removedItems.Add(aa);
                }  
            }

            if (removedItems.Count > 0)
            {
                traceType.WriteInfo(
                    "Removing the following AllowAction entries since they aren't relevant anymore.{0}{1}", Environment.NewLine, removedItems.ToJson());
            }

            return Task.FromResult(record.ToJson());
        }

        private async Task<string> HandleCommandGetDocAsync(string args)
        {
            traceType.WriteInfo("GetDoc requested");

            var activityId = Guid.NewGuid();

            IPolicyAgentDocumentForTenant doc = await policyAgentClient.GetDocumentAsync(activityId, CancellationToken.None).ConfigureAwait(false);

            return doc.ToJson();
        }

        private async Task<string> HandleCommandGetDocRawAsync(string args)
        {
            traceType.WriteInfo("GetDocRaw requested");

            var activityId = Guid.NewGuid();

            byte[] docBytes = await policyAgentClient.GetDocumentRawAsync(activityId, CancellationToken.None).ConfigureAwait(false);

            return new
            {
                DocumentBase64 = Convert.ToBase64String(docBytes),
            }.ToJson();
        }

        private Task<string> HandleCommandGetJobsAsync(string args)
        {
            traceType.WriteInfo("GetJobs requested");

            CoordinatorContext cc = CoordinatorContext;
            if (cc == null)
            {
                throw Trace.CreateException(
                    traceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                    "Job information is not available");
            }

            // for a full view, GetDoc could be used
            var jobs = new List<JobSummary>();
            foreach (var j in cc.MappedTenantJobs.Values)
            {
                jobs.Add(j.GetSummary());
            }

            return Task.FromResult(jobs.ToJson());
        }

        /// <summary>
        /// Approves jobs that are waiting for acknowledgment.
        /// AcknowledgeJob:[job document incarnation number]:[job id1;job id2]
        /// </summary>
        private Task<string> HandleCommandAckJobsAsync(string args)
        {
            return SendJobResponseAsync(args, JobStepResponseEnum.Acknowledged);
        }

        private Task<string> HandleCommandAlertJobsAsync(string args)
        {
            return SendJobResponseAsync(args, JobStepResponseEnum.Alerted);
        }

        /// <summary>
        /// Helper method for approving or alerting jobs.
        /// </summary>
        private async Task<string> SendJobResponseAsync(string args, JobStepResponseEnum responseType)
        {
            traceType.WriteInfo("Sending {0} job response. Input: {1}", responseType, args);

            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 2)
                throw new ArgumentException("Invalid command arguments");

            // TODO make this optional
            int incarnationNumber;
            bool success = int.TryParse(tokens[0], out incarnationNumber);
            if (!success)
            {
                throw new ArgumentException("Invalid command arguments");
            }

            var activityId = Guid.NewGuid();

            string[] jobIds = tokens[1].Split(new[] { ';', ',' }, StringSplitOptions.RemoveEmptyEntries);

            var jobResponseMap = jobIds.ToDictionary(
                i => Guid.Parse(i),
                _ => responseType);

            try
            {
                await policyAgentClient.SendJobResponseAsync(
                    activityId,
                    incarnationNumber,
                    jobResponseMap,
                    "{0} by operator command".ToString(responseType),
                    CancellationToken.None).ConfigureAwait(false);

                traceType.WriteInfo("{0} job response sent successfully", responseType);

                return string.Empty;
            }
            catch (ManagementException)
            {
                traceType.WriteWarning("Error in management protocol while sending job response");
                throw;
            }
        }

        /// <summary>
        /// Helper method for requesting suspend, resume, cancel and rollback of jobs.
        /// </summary>
        private async Task<string> RequestActionAsync<T>(
            string args,
            T request,
            Func<Guid, T, CancellationToken, Task> func,
            string actionName)
        {
            traceType.WriteInfo("Action {0} requested. Input: {1}", actionName, args);

            var activityId = Guid.NewGuid();

            try
            {
                await func(activityId, request, CancellationToken.None).ConfigureAwait(false);

                traceType.WriteInfo("{0} successfully initiated.", actionName);

                return string.Empty;
            }
            catch (ManagementException)
            {
                traceType.WriteWarning(
                    "Error in management protocol in {0}", actionName);

                throw;
            }
        }

        private Task<string> HandleCommandRequestSuspendAsync(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 1)
                throw new ArgumentException("Invalid command arguments");

            var jobSuspendRequest = new JobSuspendRequest
            {
                JobId = tokens[0].Trim().ToGUID(),
            };

            return RequestActionAsync(args, jobSuspendRequest, policyAgentClient.RequestSuspendAsync, "RequestSuspend");
        }

        private Task<string> HandleCommandRequestResumeAsync(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 1)
                throw new ArgumentException("Invalid command arguments");

            var jobResumeRequest = new JobResumeRequest
            {
                JobId = tokens[0].Trim().ToGUID(),
            };

            return RequestActionAsync(args, jobResumeRequest, policyAgentClient.RequestResumeAsync, "RequestResume");
        }

        private Task<string> HandleCommandRequestCancelAsync(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 1)
                throw new ArgumentException("Invalid command arguments");

            var jobCancelRequest = new JobCancelRequest
            {
                JobId = tokens[0].Trim().ToGUID(),
            };

            return RequestActionAsync(args, jobCancelRequest, policyAgentClient.RequestCancelAsync, "RequestCancel");
        }

        private Task<string> HandleCommandRequestRollbackAsync(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 2)
                throw new ArgumentException("Invalid command arguments");

            var updateRollbackRequest = new UpdateRollbackRequest
            {
                JobId = tokens[0].Trim().ToGUID(),
                UdIncarnation = ulong.Parse(tokens[1].Trim()),
            };

            return RequestActionAsync(args, updateRollbackRequest, policyAgentClient.RequestRollbackAsync, "RequestRollback");
        }

        private async Task<string> HandleUpdateJobBlockingPolicyAsync(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 1)
                throw new ArgumentException("Invalid command arguments");

            JobBlockingPolicy newBlockingPolicy;

            if (!Enum.TryParse(tokens[0], out newBlockingPolicy))
            {
                throw new ArgumentException("Invalid command arguments");
            }

            traceType.WriteInfo("UpdateJobBlockingPolicy. New policy {0} requested", newBlockingPolicy);

            await jobBlockingPolicyManager.UpdatePolicyAsync(newBlockingPolicy).ConfigureAwait(false);

            traceType.WriteInfo("Successfully updated job blocking policy. New policy: {0}", newBlockingPolicy);

            return string.Empty;
        }

        private Task<string> HandleCommandFailFastAsync(string args)
        {
            // Delay slightly to give the process some time to acknowledge the client request
            TimeSpan delay = TimeSpan.FromSeconds(1);
            traceType.WriteWarning("Received FailFast command; terminating process after {0} delay", delay);
            Task.Delay(delay).ContinueWith(_ => FailFast());

            return Task.FromResult(string.Empty);
        }

        private void FailFast()
        {
            traceType.WriteWarning("Terminating process due to FailFast client command");
            Environment.FailFast("FailFast requested by client command");
        }

        private async Task<string> HandleCommandFinishInfrastructureTask(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);

            if (tokens.Length != 2)
                throw new ArgumentException("Invalid command arguments");

            string taskId = tokens[0];

            long instanceId;
            if (!long.TryParse(tokens[1], out instanceId))
            {
                throw new ArgumentException("Invalid command arguments");
            }

            traceType.WriteInfo("Calling InfrastructureAgent.FinishInfrastructureTaskAsync('{0}', 0x{1:X})", taskId, instanceId);

            await this.environment.InfrastructureAgent.FinishInfrastructureTaskAsync(taskId, instanceId, FabricClient.DefaultTimeout, CancellationToken.None);

            traceType.WriteInfo("Successfully called InfrastructureAgent.FinishInfrastructureTaskAsync('{0}', 0x{1:X})", taskId, instanceId);

            return string.Empty;
        }

        private void RegisterCommand(string commandName, Func<string, Task<string>> commandFuncAsync, string description, bool isAdminCommand = false, bool documented = true)
        {
            var entry = new CommandHandlerEntry(commandName, commandFuncAsync, description, isAdminCommand, documented);
            this.commandHandler.Commands.Add(commandName, entry);
        }

        #endregion command handling

        /// <summary>
        /// Class just for Json serialization since it is not so straight forward to deserialize a list in Json.
        /// TODO - the above comment may not be true. investigate. Service Fabric Explorer scenario shouldn't be broken.
        /// </summary>
        private sealed class RoleInstanceCollectionData
        {
            public IList<RoleInstance> RoleInstances { get; set; }
        }      
    }
}