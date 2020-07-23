// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Repair;
    using Threading.Tasks;
   
    /// <summary>
    /// Class that contains the core logic of observing tenant jobs and repair tasks,
    /// matching them with each other and setting the actions to be executed on them.
    /// </summary>
    internal sealed class Reconciler
    {
        private readonly CoordinatorEnvironment environment;
        private readonly ActionFactory actionFactory;
        private readonly RepairActionProvider repairActionProvider;
        private readonly TraceType traceType;
        private readonly JobClassifier jobClassifier;

        private readonly Dictionary<Tuple<JobCategory, JobPhase>, Action<CoordinatorContext, IMappedTenantJob>> jobReconcileHandlerMap =
            new Dictionary<Tuple<JobCategory, JobPhase>, Action<CoordinatorContext, IMappedTenantJob>>();

        public Reconciler(CoordinatorEnvironment environment, IPolicyAgentClient policyAgentClient, IRepairManager repairManager, RepairActionProvider repairActionProvider)
        {
            this.environment = environment.Validate("environment");
            this.actionFactory = new ActionFactory(environment, policyAgentClient, repairManager, repairActionProvider);
            this.repairActionProvider = repairActionProvider.Validate("repairActionProvider");
            this.traceType = environment.CreateTraceType("Reconciler");
            this.jobClassifier = new JobClassifier(environment);

            if (this.environment.Config.ReadConfigValue(Constants.ConfigKeys.EnableVendorRepairBeginHandling, false))
            {
                traceType.WriteInfo("VendorRepairBegin handling is enabled");
                AddHandler(JobCategory.VendorRepairBegin, JobPhase.ImpactStartWaitingForAck, this.ReconcileJobVRBImpactStartWaitingForAck);
                AddHandler(JobCategory.VendorRepairBegin, JobPhase.ImpactStartAcked, this.ReconcileJobVRBImpactStartAcked);
                AddHandler(JobCategory.VendorRepairBegin, JobPhase.ImpactEndWaitingForAck, this.ReconcileJobVRBImpactEndWaitingForAck);
                AddHandler(JobCategory.VendorRepairBegin, JobPhase.Inactive, this.ReconcileJobVRBInactive);
            }

            if (this.environment.Config.ReadConfigValue(Constants.ConfigKeys.EnableVendorRepairEndHandling, false))
            {
                traceType.WriteInfo("VendorRepairEnd handling is enabled");
                AddHandler(JobCategory.VendorRepairEnd, JobPhase.ImpactStartWaitingForAck, this.ReconcileJobVREImpactStartWaitingForAck);
                AddHandler(JobCategory.VendorRepairEnd, JobPhase.ImpactStartAcked, this.ReconcileJobVREImpactStartAcked);
                AddHandler(JobCategory.VendorRepairEnd, JobPhase.ImpactEndWaitingForAck, this.ReconcileJobVREImpactEndWaitingForAck);
                AddHandler(JobCategory.VendorRepairEnd, JobPhase.Inactive, this.ReconcileJobVREInactive);
            }

            AddHandler(JobCategory.Normal, JobPhase.ImpactStartWaitingForAck, this.ReconcileJobNormalImpactStartWaitingForAck);
            AddHandler(JobCategory.Normal, JobPhase.ImpactStartAcked, this.ReconcileJobNormalImpactStartAcked);
            AddHandler(JobCategory.Normal, JobPhase.ImpactEndWaitingForAck, this.ReconcileJobNormalImpactEndWaitingForAck);
            AddHandler(JobCategory.Normal, JobPhase.Inactive, this.ReconcileJobNormalInactive);

            if (this.environment.Config.ReadConfigValue(Constants.ConfigKeys.EnableUnknownJobHandling, true))
            {
                traceType.WriteInfo("Unknown job handling is enabled");
                AddHandler(JobCategory.Normal, JobPhase.Unknown, this.ReconcileJobNormalUnknown);
            }
        }

        private void AddHandler(JobCategory category, JobPhase phase, Action<CoordinatorContext, IMappedTenantJob> handler)
        {
            jobReconcileHandlerMap.Add(
                Tuple.Create(category, phase),
                handler);
        }

        public Task ReconcileAsync(Guid activityId, CoordinatorContext coordinatorContext)
        {
            coordinatorContext.Validate("coordinatorContext");

            ReconcileMatchingTenantJobs(coordinatorContext);
            ReconcileUnmatchedRepairTasks(coordinatorContext);
            return Task.FromResult(0);
        }

        public IReadOnlyDictionary<Guid, JobStepResponseEnum> GetJobStepResponses()
        {
            return this.actionFactory.GetJobStepResponses();
        }

        /// <summary>
        /// Reconciles tenant jobs that have one or more matching repair tasks.
        /// </summary>
        private void ReconcileMatchingTenantJobs(CoordinatorContext coordinatorContext)
        {
            foreach (var mappedTenantJob in coordinatorContext.MappedTenantJobs.Values)
            {
                ReconcileTenantJob(coordinatorContext, mappedTenantJob);
            }
        }

        private void ReconcileTenantJob(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;

            var jobCategory = this.jobClassifier.GetJobCategory(job);
            var jobPhase = this.jobClassifier.GetJobPhase(job);

            Action<CoordinatorContext, IMappedTenantJob> handler;
            if (this.jobReconcileHandlerMap.TryGetValue(Tuple.Create(jobCategory, jobPhase), out handler))
            {
                traceType.WriteInfo(
                    "Reconciling job {0}: {1} {2} {3} ({4}/{5}/{6}/{7})",
                    mappedTenantJob.Id,
                    jobCategory,
                    jobPhase,
                    job.GetImpactAction(),
                    job.JobStatus,
                    job.JobStep == null ? "-" : job.JobStep.ImpactStep.ToString(),
                    job.JobStep == null ? "-" : job.JobStep.AcknowledgementStatus.ToString(),
                    job.JobStep == null ? "-" : job.JobStep.DeadlineForResponse);

                handler(coordinatorContext, mappedTenantJob);
            }
            else
            {
                traceType.WriteWarning(
                    "No handler to reconcile job {0}: {1} {2} {3} ({4}/{5}/{6}/{7})",
                    mappedTenantJob.Id,
                    jobCategory,
                    jobPhase,
                    job.GetImpactAction(),
                    job.JobStatus,
                    job.JobStep == null ? "-" : job.JobStep.ImpactStep.ToString(),
                    job.JobStep == null ? "-" : job.JobStep.AcknowledgementStatus.ToString(),
                    job.JobStep == null ? "-" : job.JobStep.DeadlineForResponse);
            }
        }

        private void RecordNormalMatch(IMappedTenantJob job, IRepairTask task)
        {
            traceType.WriteInfo("Matched repair task '{0}' in expected state {1}", task.TaskId, task.State);
            AddMatchingTaskToJob(job, task);
        }

        private void RecordAbnormalMatch(IMappedTenantJob job, IRepairTask task)
        {
            traceType.WriteWarning("Matched repair task '{0}' in unexpected state {1}", task.TaskId, task.State);
            AddMatchingTaskToJob(job, task);
        }

        private void AddMatchingTaskToJob(IMappedTenantJob job, IRepairTask repairTask)
        {
            var taskSummary = new RepairTaskSummary()
            {
                TaskId = repairTask.TaskId,
                State = repairTask.State,
            };

            job.MatchedTasks.Add(taskSummary);
        }

        #region normal jobs

        private void ReconcileJobNormalImpactStartWaitingForAck(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();
            bool foundCancelledTask = false;

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.IsContextIdMatch(job))
                        {
                            // Normal scenario

                            var args = RepairTaskPrepareArgs.FromTenantJob(
                                job,
                                coordinatorContext.Doc.JobDocumentIncarnation,
                                this.environment,
                                isVendorRepair: false,
                                restoringHealthCheckOnly: false);

                            if (args != null)
                            {
                                mappedTenantJob.Actions.Add(actionFactory.NewMoveToPreparingAction(repairTask, job, args));
                                mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                            }

                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (repairTask.IsJobKeyMatch(job))
                        {
                            // Normal scenario
                            mappedTenantJob.IsActive = true;
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (repairTask.IsJobKeyMatch(job))
                        {
                            // Normal scenario
                            mappedTenantJob.IsActive = true;
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToExecutingAction(repairTask));
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (repairTask.IsJobKeyMatch(job))
                        {
                            // Normal scenario
                            mappedTenantJob.IsActive = true;
                            mappedTenantJob.Actions.Add(actionFactory.NewExecuteJobAction(job, repairTask));
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Restoring:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            // Prevent creation of a new task until this one finishes
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        if (repairTask.IsContextIdMatch(job) &&
                            ((repairTask.Flags & RepairTaskFlags.CancelRequested) == RepairTaskFlags.CancelRequested) &&
                            (repairTask.ResultStatus == RepairTaskResult.Cancelled))
                        {
                            // Abnormal scenario
                            traceType.WriteWarning("Cancelled task '{0}' matches job {1} by context ID", repairTask.TaskId, job.Id);
                            foundCancelledTask = true;
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                        }
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);

            if (matchingRepairTaskIds.Count == 0)
            {
                if (foundCancelledTask && job.GetImpactAction() == ImpactActionEnum.TenantMaintenance)
                {
                    // Cancel tenant-initiated repair jobs with a corresponding task that was cancelled in the RM before executing (e.g. while it was in Claimed)
                    traceType.WriteWarning(
                        "Cancelling {0} job {1} due to cancelled task which matched by context ID",
                        job.GetImpactAction(),
                        job.Id);

                    mappedTenantJob.Actions.Add(actionFactory.NewCancelJobAction(job));
                }
                else
                {
                    if (foundCancelledTask)
                    {
                        traceType.WriteWarning(
                            "Jobs of type {0} cannot be cancelled from Repair Manager; creating new repair task for job {1}",
                            job.GetImpactAction(),
                            job.Id);
                    }

                    // Normal scenario (e.g. tenant update, platform update)

                    var args = RepairTaskPrepareArgs.FromTenantJob(
                        job,
                        coordinatorContext.Doc.JobDocumentIncarnation,
                        this.environment,
                        isVendorRepair: false,
                        restoringHealthCheckOnly: false,
                        description: job.ContextStringGivenByTenant);

                    if (args != null)
                    {
                        traceType.WriteInfo(
                            "Creating new repair task '{0}' for job {1}",
                            args.TaskId,
                            job.Id);

                        mappedTenantJob.Actions.Add(actionFactory.NewCreateInPreparingAction(job, args));
                        mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                    }
                }
            }
        }

        private void ReconcileJobNormalImpactStartAcked(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            mappedTenantJob.IsActive = true;
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.IsContextIdMatch(job))
                        {
                            // Abnormal scenario

                            var args = RepairTaskPrepareArgs.FromTenantJob(
                                job,
                                coordinatorContext.Doc.JobDocumentIncarnation,
                                this.environment,
                                isVendorRepair: false,
                                restoringHealthCheckOnly: false);

                            if (args != null)
                            {
                                mappedTenantJob.Actions.Add(actionFactory.NewMoveToPreparingAction(repairTask, job, args, surpriseJob: true));
                                mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                            }

                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                    case RepairTaskState.Restoring:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToExecutingAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Normal scenario (no-op)
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);

            if (matchingRepairTaskIds.Count == 0)
            {
                // Abnormal scenario

                var args = RepairTaskPrepareArgs.FromTenantJob(
                    job,
                    coordinatorContext.Doc.JobDocumentIncarnation,
                    this.environment,
                    isVendorRepair: false,
                    restoringHealthCheckOnly: false,
                    description: job.ContextStringGivenByTenant);

                if (args != null)
                {
                    traceType.WriteInfo(
                        "Creating new repair task '{0}' for job {1}",
                        args.TaskId,
                        job.Id);

                    mappedTenantJob.Actions.Add(actionFactory.NewCreateInPreparingAction(job, args, surpriseJob: true));
                    mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                }
            }
        }
        
        private void ReconcileJobNormalImpactEndWaitingForAck(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            // Mark this job as active even though work may no longer be active in Service Fabric
            // (e.g. the matching repair task is in Completed) to discourage selection of another job
            // of a different type from starting while this job step is finishing up and the job is about
            // to present another step of this job, or another job of the same type/priority.
            mappedTenantJob.IsActive = true;

            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();
            var completedRepairTasks = new List<IRepairTask>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.IsContextIdMatch(job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            var resultStatus = CoordinatorHelper.ConvertToRepairTaskResult(job.JobStep.ActionStatus);
                            string details = "Job step completed with status {0}".ToString(job.JobStep.ActionStatus);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, resultStatus, details, surpriseJob: true, processRemovedNodes: true));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Normal scenario
                            var resultStatus = CoordinatorHelper.ConvertToRepairTaskResult(job.JobStep.ActionStatus);
                            string details = "Job step completed with status {0}".ToString(job.JobStep.ActionStatus);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, resultStatus, details, processRemovedNodes: true));
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Restoring:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Normal scenario
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Normal scenario
                            // TODO prefer tasks with jobKeyMatch?
                            traceType.WriteInfo("Job {0} matched repair task '{1}' completed at {2:O}",
                                job.Id,
                                repairTask.TaskId,
                                repairTask.CompletedTimestamp);
                            completedRepairTasks.Add(repairTask);
                        }
                        break;
                }
            }

            if (matchingRepairTaskIds.Count == 0)
            {
                // No active tasks matched this job. The most recently completed task, if any, can provide justification to ack
                // as long as its health check succeeded or was skipped.
                var mostRecentlyCompletedRepairTask =
                    completedRepairTasks.OrderByDescending(e => e.CompletedTimestamp).FirstOrDefault();

                if (mostRecentlyCompletedRepairTask != null)
                {
                    if (mostRecentlyCompletedRepairTask.PreparingHealthCheckState != RepairTaskHealthCheckState.TimedOut &&
                        mostRecentlyCompletedRepairTask.RestoringHealthCheckState != RepairTaskHealthCheckState.TimedOut)
                    {
                        traceType.WriteInfo(
                            "Job {0} matched completed task '{1}' with successful health check (preparing = {2}, restoring = {3}); acknowledging job",
                            job.Id,
                            mostRecentlyCompletedRepairTask.TaskId,
                            mostRecentlyCompletedRepairTask.PreparingHealthCheckState,
                            mostRecentlyCompletedRepairTask.RestoringHealthCheckState);

                        // Normal scenario: successful or skipped health check => acknowledge the ImpactEnd
                        mappedTenantJob.Actions.Add(actionFactory.NewAckJobAction(job));
                        matchingRepairTaskIds.Add(mostRecentlyCompletedRepairTask.TaskId);
                        RecordNormalMatch(mappedTenantJob, mostRecentlyCompletedRepairTask);
                    }
                    else
                    {
                        // Abnormal scenario: health check timed out; blind ack or retry will happen below
                        traceType.WriteWarning(
                            "Job {0} matched completed task '{1}' with unsuccessful health check (preparing = {2}, restoring = {3})",
                            job.Id,
                            mostRecentlyCompletedRepairTask.TaskId,
                            mostRecentlyCompletedRepairTask.PreparingHealthCheckState,
                            mostRecentlyCompletedRepairTask.RestoringHealthCheckState);
                    }
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);

            if (matchingRepairTaskIds.Count == 0)
            {
                // Abnormal scenario
                if (job.DoesJobRequireRestoringHealthCheck(this.environment.Config))
                {
                    traceType.WriteWarning(
                        "No completed task with successful health check found for job {0}, but health check is required; creating new task for health check",
                        job.Id);

                    // Restoring health check required for this job type, so create a new zero-impact task to perform one.
                    var args = RepairTaskPrepareArgs.FromTenantJob(
                        job,
                        coordinatorContext.Doc.JobDocumentIncarnation,
                        this.environment,
                        isVendorRepair: false,
                        restoringHealthCheckOnly: true,
                        description: job.ContextStringGivenByTenant);

                    if (args != null)
                    {
                        mappedTenantJob.Actions.Add(actionFactory.NewCreateInPreparingAction(job, args));
                    }
                }
                else
                {
                    traceType.WriteWarning(
                        "No completed task with successful health check found for job {0}, but health check is not required; acknowledging job",
                        job.Id);

                    // Restoring health check not required, so just ack the ImpactEnd.
                    mappedTenantJob.Actions.Add(actionFactory.NewAckJobAction(job));
                }
            }
        }

        /// <summary>
        /// This is useful later on to determine the unmatched repair tasks and account for them.
        /// </summary>
        private static void AddTenantJobInfoToMatchingRepairTasks(
            IDictionary<string, IMappedRepairTask> mappedRepairTasks,
            IList<string> matchingRepairTaskIds,
            ITenantJob tenantJob)
        {
            foreach (var id in matchingRepairTaskIds)
            {
                mappedRepairTasks[id].TenantJob = tenantJob;
            }
        }

        private void ReconcileJobNormalInactive(
            CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.IsContextIdMatch(job))
                        {
                            // Abnormal scenario
                            if (job.JobStatus != JobStatusEnum.Pending)
                            {
                                // In case Pending is considered inactive by the
                                // job classifier, avoid cancelling the repair task
                                // that requested the job, when the job may be just
                                // about to start.
                                mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            }

                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            mappedTenantJob.IsActive = true;
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            var resultStatus = CoordinatorHelper.ConvertToRepairTaskResult(job.JobStatus);
                            string details = "Job ended with status {0}".ToString(job.JobStatus);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, resultStatus, details, surpriseJob: true, processRemovedNodes: true));
                            mappedTenantJob.IsActive = true;
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            var resultStatus = CoordinatorHelper.ConvertToRepairTaskResult(job.JobStatus);
                            string details = "Job ended with status {0}".ToString(job.JobStatus);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, resultStatus, details, processRemovedNodes: true));
                            mappedTenantJob.IsActive = true;
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }

                        break;

                    case RepairTaskState.Restoring:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Abnormal scenario
                            //
                            // Do not mark as active, since the job is not active and the task is already restoring.
                            // This avoids having this job count against the concurrency throttle if the job has
                            // timed out but the restoring health check is still running.
                            //
                            // TODO consider cancelling restoring health check, if necessary
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        // Normal scenario, job is completed on both sides
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);
        }

        private void ReconcileJobNormalUnknown(
            CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.IsContextIdMatch(job))
                        {
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                    case RepairTaskState.Approved:
                    case RepairTaskState.Executing:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            mappedTenantJob.IsActive = true;
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }

                        break;

                    case RepairTaskState.Restoring:
                        if (repairTask.IsJobIdMatch(job))
                        {
                            // Do not mark as active, since the job is not active and the task is already restoring.
                            // This avoids having this job count against the concurrency throttle if the job has
                            // timed out but the restoring health check is still running.
                            //
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        // Nothing to do; ignore completed repair tasks
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);
        }

        #endregion normal jobs

        #region VRB (vendor repair begin) jobs 

        private void ReconcileJobVRBImpactStartWaitingForAck(
            CoordinatorContext coordinatorContext,            
            IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();
            var completedRepairTasks = new List<IRepairTask>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;
                bool jobKeyMatch = repairTask.IsJobKeyMatch(job);

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.MatchesVendorRepairJobByActionAndTarget(repairActionProvider, environment.Config, job))
                        {
                            // Normal scenario

                            var args = RepairTaskPrepareArgs.FromTenantJob(
                                job,
                                coordinatorContext.Doc.JobDocumentIncarnation,
                                this.environment,
                                isVendorRepair: true,
                                restoringHealthCheckOnly: false);

                            if (args != null)
                            {
                                mappedTenantJob.Actions.Add(actionFactory.NewMoveToPreparingAction(repairTask, job, args));
                                mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                            }

                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (jobKeyMatch)
                        {
                            // Normal scenario
                            mappedTenantJob.IsActive = true;
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (jobKeyMatch)
                        {
                            // Normal scenario
                            mappedTenantJob.IsActive = true;
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToExecutingAction(repairTask));
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (jobKeyMatch)
                        {
                            // Normal scenario
                            mappedTenantJob.IsActive = true;
                            mappedTenantJob.Actions.Add(actionFactory.NewExecuteJobAction(job, repairTask));
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Restoring:
                        if (jobKeyMatch)
                        {
                            // Abnormal scenario
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);

            if (matchingRepairTaskIds.Count == 0)
            {
                // Normal scenario

                var args = RepairTaskPrepareArgs.FromTenantJob(
                    job,
                    coordinatorContext.Doc.JobDocumentIncarnation,
                    this.environment,
                    isVendorRepair: true,
                    restoringHealthCheckOnly: false,
                    description: job.ContextStringGivenByTenant);

                if (args != null)
                {
                    traceType.WriteInfo(
                        "Creating new repair task '{0}' for VendorRepairBegin job {1}",
                        args.TaskId,
                        job.Id);

                    mappedTenantJob.Actions.Add(actionFactory.NewCreateInPreparingAction(job, args));
                    mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                }
            }
        }

        private void ReconcileJobVRBImpactStartAcked(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            mappedTenantJob.IsActive = true;
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;
                bool jobKeyMatch = repairTask.IsJobKeyMatch(job);

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.MatchesVendorRepairJobByActionAndTarget(repairActionProvider, environment.Config, job))
                        {
                            // Abnormal scenario

                            var args = RepairTaskPrepareArgs.FromTenantJob(
                                job,
                                coordinatorContext.Doc.JobDocumentIncarnation,
                                this.environment,
                                isVendorRepair: true,
                                restoringHealthCheckOnly: false);

                            if (args != null)
                            {
                                mappedTenantJob.Actions.Add(actionFactory.NewMoveToPreparingAction(repairTask, job, args, surpriseJob: true));
                                mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                            }

                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                    case RepairTaskState.Restoring:
                        if (jobKeyMatch)
                        {
                            // Abnormal scenario
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (jobKeyMatch)
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToExecutingAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (jobKeyMatch)
                        {
                            // Normal scenario
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Completed:
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);

            if (matchingRepairTaskIds.Count == 0)
            {
                // Abnormal scenario

                var args = RepairTaskPrepareArgs.FromTenantJob(
                    job,
                    coordinatorContext.Doc.JobDocumentIncarnation,
                    this.environment,
                    isVendorRepair: true,
                    restoringHealthCheckOnly: false,
                    description: job.ContextStringGivenByTenant);

                if (args != null)
                {
                    traceType.WriteInfo(
                        "Creating new repair task '{0}' for VendorRepairBegin job {1}",
                        args.TaskId,
                        job.Id);

                    mappedTenantJob.Actions.Add(actionFactory.NewCreateInPreparingAction(job, args, surpriseJob: true));
                    mappedTenantJob.ImpactedNodeCount = Math.Max(mappedTenantJob.ImpactedNodeCount, args.Impact.ImpactedNodes.Count);
                }
            }
        }

        private void ReconcileJobVRBImpactEndWaitingForAck(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            mappedTenantJob.IsActive = true;
            var job = mappedTenantJob.TenantJob;
            mappedTenantJob.Actions.Add(actionFactory.NewAckJobAction(job));
        }

        private void ReconcileJobVRBInactive(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;

            switch (job.JobStatus)
            {
                case JobStatusEnum.Pending:
                case JobStatusEnum.Completed:
                    break;

                default:
                    traceType.WriteWarning(
                        "Vendor repair job is not in normal state. Consider inspecting impact of this job (e.g. check node, cluster health etc.). Job: {0}",
                        job.ToJson());
                    break;
            }
        }

        #endregion VRB (vendor repair begin) jobs 

        #region VRE (vendor repair end) jobs 

        private void ReconcileJobVREImpactStartWaitingForAck(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            mappedTenantJob.IsActive = true;
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;
                bool isVendorRepair = repairTask.IsVendorRepair();

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.MatchesVendorRepairJobByActionAndTarget(repairActionProvider, environment.Config, job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (repairTask.MatchesVendorRepairEndJob(job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (repairTask.MatchesVendorRepairEndJob(job))
                        {
                            // Abnormal scenario
                            string details = "Completed by VendorRepairEnd job {0}".ToString(job.Id);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, RepairTaskResult.Succeeded, details, surpriseJob: true));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (repairTask.MatchesVendorRepairEndJob(job))
                        {
                            // Normal scenario
                            string details = "Completed by VendorRepairEnd job {0}".ToString(job.Id);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, RepairTaskResult.Succeeded, details));
                            RecordNormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Restoring:
                    case RepairTaskState.Completed:
                        // Normal scenario (will ack below)
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);

            if (matchingRepairTaskIds.Count == 0)
            {
                mappedTenantJob.Actions.Add(actionFactory.NewAckJobAction(job));
            }
        }

        private void ReconcileJobVREImpactStartAcked(
            CoordinatorContext coordinatorContext,
            IMappedTenantJob mappedTenantJob)
        {
            mappedTenantJob.IsActive = true;
            var job = mappedTenantJob.TenantJob;
            var matchingRepairTaskIds = new List<string>();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                var repairTask = mappedRepairTask.RepairTask;
                bool isVendorRepair = repairTask.IsVendorRepair();

                switch (repairTask.State)
                {
                    case RepairTaskState.Claimed:
                        if (repairTask.MatchesVendorRepairJobByActionAndTarget(repairActionProvider, environment.Config, job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (repairTask.MatchesVendorRepairEndJob(job))
                        {
                            // Abnormal scenario
                            mappedTenantJob.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Approved:
                        if (repairTask.MatchesVendorRepairEndJob(job))
                        {
                            // Abnormal scenario
                            string details = "Completed by VendorRepairEnd job {0}".ToString(job.Id);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, RepairTaskResult.Succeeded, details, surpriseJob: true));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:
                        if (repairTask.MatchesVendorRepairEndJob(job))
                        {
                            // Abnormal scenario
                            string details = "Completed by VendorRepairEnd job {0}".ToString(job.Id);
                            mappedTenantJob.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, RepairTaskResult.Succeeded, details));
                            RecordAbnormalMatch(mappedTenantJob, repairTask);
                            matchingRepairTaskIds.Add(repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Restoring:
                    case RepairTaskState.Completed:
                        // Normal scenario (no-op)
                        break;
                }
            }

            AddTenantJobInfoToMatchingRepairTasks(coordinatorContext.MappedRepairTasks, matchingRepairTaskIds, job);
        }

        private void ReconcileJobVREImpactEndWaitingForAck(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            mappedTenantJob.IsActive = true;
            var job = mappedTenantJob.TenantJob;
            mappedTenantJob.Actions.Add(actionFactory.NewAckJobAction(job));
        }

        private void ReconcileJobVREInactive(CoordinatorContext coordinatorContext, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;

            switch (job.JobStatus)
            {
                case JobStatusEnum.Pending:
                case JobStatusEnum.Completed:
                    break;

                default:
                    traceType.WriteWarning(
                        "Vendor repair job is not in normal state. Consider inspecting impact of this job (e.g. check node, cluster health etc.). Job: {0}",
                        job.ToJson());
                    break;
            }
        }

        #endregion VRE (vendor repair end) jobs 

        private TimeSpan GetRepairRequestTimeout()
        {
            return TimeSpan.FromSeconds(this.environment.Config.ReadConfigValue(
                Constants.ConfigKeys.CancelThresholdForUnmatchedRepairTask,
                5 * 60));
        }

        private bool IsRepairExecutionEnabled()
        {
            // While this is disabled by default in serial jobs since it was added after the initial release,
            // the parallel coordinator is already opt-in so repair execution can be enable by default.
            return this.environment.Config.ReadConfigValue(Constants.ConfigKeys.EnableRepairExecution, true);
        }

        private void ReconcileUnmatchedRepairTasks(CoordinatorContext coordinatorContext)
        {
            var roleInstanceNames = coordinatorContext.Doc.RoleInstanceHealthInfos.GetRoleInstanceNames();

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values.Where(e => e.TenantJob == null))
            {
                var repairTask = mappedRepairTask.RepairTask;

                if (repairTask.State == RepairTaskState.Completed)
                {
                    // Don't even bother logging these; no work to do
                    continue;
                }

                traceType.WriteInfo("Reconciling unmatched repair task: {0} ({1})", repairTask.TaskId, repairTask.State);

                // Repair tasks created for VendorRepair should not be cancelled due to being unmatched, since the VendorRepairBegin
                // job would have disappeared but the repair task sticks around until the VendorRepairEnd job appears.
                bool canCancelUnmatchedTask = !repairTask.IsVendorRepair();

                switch (repairTask.State)
                {
                    case RepairTaskState.Created:
                        if (this.IsRepairExecutionEnabled())
                        {
                            mappedRepairTask.Actions.Add(actionFactory.NewClaimedRepairTaskAction(repairTask, roleInstanceNames));
                        }
                        break;

                    case RepairTaskState.Claimed:
                        if (!repairTask.ClaimedTimestamp.HasValue || 
                            DateTime.UtcNow - repairTask.ClaimedTimestamp.Value > this.GetRepairRequestTimeout())
                        {
                            traceType.WriteWarning(
                                "Repair task '{0}' has been in Claimed since {1:O} and no matching job exists; cancelling repair task",
                                repairTask.TaskId,
                                repairTask.ClaimedTimestamp); // no issue with display of a null here

                            mappedRepairTask.Actions.Add(
                                actionFactory.NewMoveToCompletedAction(
                                    repairTask,
                                    RepairTaskResult.Failed,
                                    "Timed out waiting for an Azure job matching this repair request",
                                    false));
                        }
                        else if (this.IsRepairExecutionEnabled())
                        {
                            mappedRepairTask.Actions.Add(actionFactory.NewRequestRepairAction(repairTask));
                        }
                        break;

                    case RepairTaskState.Preparing:
                        if (canCancelUnmatchedTask)
                        {
                            mappedRepairTask.Actions.Add(actionFactory.NewCancelRepairTaskAction(repairTask));
                        }
                        break;

                    case RepairTaskState.Approved:
                    case RepairTaskState.Executing:
                        if (canCancelUnmatchedTask)
                        {
                            string details = "Completed because the matching Azure job is no longer active";
                            mappedRepairTask.Actions.Add(actionFactory.NewMoveToRestoringAction(repairTask, RepairTaskResult.Interrupted, details));
                        }
                        break;

                    case RepairTaskState.Restoring:
                        // TODO, Cancel restoring health check (we need an IS config key). For now, we'll not do this.
                        break;
                }
            }
        }        
    }
}