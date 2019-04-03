// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;
    using Repair;
    using Threading;
    using Threading.Tasks;

    internal sealed class MoveToRestoringAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;
        private readonly RepairTaskResult repairTaskResult;
        private readonly string repairTaskResultDetails;
        private readonly bool surpriseJob;
        private readonly bool cancelRestoringHealthCheck;
        private readonly bool processRemovedNodes;

        public MoveToRestoringAction(
            CoordinatorEnvironment environment,
            IRepairManager repairManager, 
            IRepairTask repairTask, 
            RepairTaskResult repairTaskResult,
            string repairTaskResultDetails,
            bool surpriseJob,
            bool cancelRestoringHealthCheck,
            bool processRemovedNodes)
            : base(environment, ActionType.Restore)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
            this.repairTaskResult = repairTaskResult;
            this.repairTaskResultDetails = repairTaskResultDetails;
            this.surpriseJob = surpriseJob;
            this.cancelRestoringHealthCheck = cancelRestoringHealthCheck;
            this.processRemovedNodes = processRemovedNodes;
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            repairTask.State = RepairTaskState.Restoring;
            repairTask.ResultStatus = repairTaskResult;
            repairTask.ResultDetails = repairTaskResultDetails;

            if (surpriseJob)
            {
                // TODO, add surprise in executordata
                this.Environment.DefaultTraceType.WriteWarning("MoveToRestoring: job bypassed normal automated workflow for repair task '{0}'", repairTask.TaskId);
            }

            if (processRemovedNodes && this.Environment.Config.ReadConfigValue(Constants.ConfigKeys.ProcessRemovedNodes, true))
            {
                await ProcessRemovedNodesAsync(activityId);
            }

            if (cancelRestoringHealthCheck)
            {
                // TODO: this is not processed, consider calling UpdateRepairTaskHealthPolicyAsync instead
                repairTask.PerformRestoringHealthCheck = false;
            }

            await repairManager.UpdateRepairExecutionStateAsync(activityId, repairTask);
        }

        // Used to approximate when the first attempt to restore was made, so that
        // node state removal attempts can time out and the restore can move on.
        private bool IsTaskRecentlyExecuted()
        {
            var now = DateTime.UtcNow;
            var recentThreshold = TimeSpan.FromMinutes(60);

            if (this.repairTask.ExecutingTimestamp.HasValue)
            {
                return (now - this.repairTask.ExecutingTimestamp.Value < recentThreshold);
            }

            if (this.repairTask.ApprovedTimestamp.HasValue)
            {
                return (now - this.repairTask.ApprovedTimestamp.Value < recentThreshold);
            }

            // No relevant timestamps
            return false;
        }

        private async Task ProcessRemovedNodesAsync(Guid activityId)
        {
            if (this.repairTask.Impact == null || this.repairTask.Impact.Kind != RepairImpactKind.Node)
            {
                this.Environment.DefaultTraceType.WriteWarning(
                    "ProcessRemovedNodes: unknown impact in task {0}: {1}",
                    this.repairTask.TaskId,
                    this.repairTask.Impact);
                return;
            }

            // TODO: Consider checking PA doc role instance list (health info) to verify that node has been removed
            var impact = (NodeRepairImpactDescription)this.repairTask.Impact;
            var removedNodeNames = impact.ImpactedNodes.Where(n => n.ImpactLevel == NodeImpactLevel.RemoveNode).Select(n => n.NodeName).ToList();

            int retryCount = this.Environment.Config.ReadConfigValue(
                Constants.ConfigKeys.ConfigKeyPrefix + "ProcessRemovedNodesRetryCount",
                5);

            while (removedNodeNames.Count > 0)
            {
                this.Environment.DefaultTraceType.WriteInfo(
                    "Processing removed nodes (count = {0}) for task '{1}'",
                    removedNodeNames.Count(),
                    this.repairTask.TaskId);

                bool succeeded = true;
                var retryRemoveNodeNames = new List<string>();

                foreach (var nodeName in removedNodeNames)
                {
                    succeeded &= await ProcessRemovedNodeAsync(activityId, nodeName, retryRemoveNodeNames);
                }

                if (!succeeded)
                {
                    if (this.IsTaskRecentlyExecuted())
                    {
                        this.Environment.DefaultTraceType.WriteWarning(
                            "Processing removed nodes (count = {0}) failed for task '{1}'; failing restore action to retry automatically later",
                            removedNodeNames.Count(),
                            this.repairTask.TaskId);

                        throw new FabricException("Failed to process node removals", FabricErrorCode.OperationNotComplete);
                    }
                    else
                    {
                        this.Environment.DefaultTraceType.WriteError(
                            "Processing removed nodes (count = {0}) failed for task '{1}'; check for abandoned nodes and call NodeStateRemoved",
                            removedNodeNames.Count(),
                            this.repairTask.TaskId);

                        return;
                    }
                }

                if (retryRemoveNodeNames.Count > 0)
                {
                    // Retry briefly for nodes that were expected to be removed, but are still up.
                    // This accounts for the fact that the node may have really been removed, and
                    // is no longer running, but the Service Fabric lease has not yet expired.

                    if (retryCount > 0)
                    {
                        TimeSpan retryDelay = TimeSpan.FromSeconds(
                            this.Environment.Config.ReadConfigValue(
                                Constants.ConfigKeys.ConfigKeyPrefix + "ProcessRemovedNodesRetryIntervalInSeconds",
                                30));

                        this.Environment.DefaultTraceType.WriteWarning(
                            "Waiting {0} seconds before retrying processing for removed nodes (last processed node count = {1}, next retry node count = {2}, retries remaining = {3})",
                            retryDelay.TotalSeconds,
                            removedNodeNames.Count,
                            retryRemoveNodeNames.Count,
                            retryCount);

                        --retryCount;
                        await Task.Delay(retryDelay);
                    }
                    else
                    {
                        // Some removed nodes are still up. This could be for one of two reasons:
                        //
                        // 1. The job did not actually remove it, in which case ignoring the failure and returning success here is correct.
                        // 2. The job did remove it, but the FM has not been informed or has not processed this fact in a timely manner,
                        //    in which case the node is going be abandoned at this point, requiring manual cleanup via NodeStateRemoved.
                        //
                        // Unfortunately, it is not easy to tell the difference between these two cases. But the consequence of case #2 is
                        // not very severe, and the mitigation is straightforward. And it should be a fairly rare case, since it occurs only
                        // if there are lengthy delays in failure detection or FM processing, coinciding with a node removal job.

                        this.Environment.DefaultTraceType.WriteWarning(
                            "Retry exhausted while processing was still pending for removed nodes (count = {0}); check for abandoned nodes and call NodeStateRemoved",
                            retryRemoveNodeNames.Count);

                        retryRemoveNodeNames.Clear();
                    }
                }
                else
                {
                    this.Environment.DefaultTraceType.WriteInfo("All removed nodes were processed successfully");
                }

                removedNodeNames = retryRemoveNodeNames;
            }
        }

        private async Task<bool> ProcessRemovedNodeAsync(Guid activityId, string nodeName, IList<string> retryList)
        {
            bool succeeded = false;

            try
            {
                // Notify the FM that the node has been removed from the cluster topology,
                // so that it no longer shows up in the node list and HM.
                await this.repairManager.RemoveNodeStateAsync(activityId, nodeName, FabricClient.DefaultTimeout, CancellationToken.None);
                succeeded = true;
            }
            catch (FabricException e)
            {
                if (e.ErrorCode == FabricErrorCode.NodeNotFound)
                {
                    // Make repeated remove calls idempotent
                    succeeded = true;
                }
                else if (e.ErrorCode == FabricErrorCode.NodeIsUp)
                {
                    // The job did not actually shut the VM down; nothing we can do at this point, so no point getting stuck here
                    succeeded = true;

                    // Add node to the list for potential short-term processing retry
                    // (e.g. machine is gone but node is still 'up' only because lease has not yet expired)
                    retryList.Add(nodeName);

                    this.Environment.DefaultTraceType.WriteWarning("ProcessRemovedNode failed because node {0} is still up; continuing", nodeName);
                }
                else
                {
                    this.Environment.DefaultTraceType.WriteWarning("ProcessRemovedNode failed for {0}: {1}", nodeName, e.ErrorCode);
                }
            }
            catch (Exception e)
            {
                this.Environment.DefaultTraceType.WriteWarning("ProcessRemovedNode failed for {0}: {1}", nodeName, e);
            }

            return succeeded;
        }

        public override string ToString()
        {
            return "{0}, RepairTask: {1}, IsSurpriseJob: {2}, CancelRestoringHealthCheck: {3}, ProcessRemovedNodes: {4}".ToString(
                base.ToString(), repairTask, surpriseJob, cancelRestoringHealthCheck, processRemovedNodes);
        }
    }
}