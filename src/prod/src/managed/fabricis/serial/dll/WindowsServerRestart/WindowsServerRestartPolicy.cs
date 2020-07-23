// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Repair;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Threading;    

    internal sealed class WindowsServerRestartPolicy : IRestartPolicy
    {
        private static readonly TraceType TraceType = new TraceType("WindowsServerRestartPolicy");
        private const RepairTaskStateFilter ActiveStateFilter = RepairTaskStateFilter.Preparing | RepairTaskStateFilter.Approved | RepairTaskStateFilter.Executing | RepairTaskStateFilter.ReadyToExecute | RepairTaskStateFilter.Restoring;
        private const RepairTaskStateFilter ClaimedStateFilter = RepairTaskStateFilter.Claimed;
        private const string ExecutorPrefix = "RestartManager.";
        private const string EmptyUpgradeDomainValue = " ";
        private const string ChosenUpgradePropertyKey = "ChosenUpgradePropertyKey";

        private readonly TimeSpan maxTimeRepairTaskActive;
        private readonly FabricClient.RepairManagementClient repairManagementClient;

        private string currentChosenUpgradeDomain;

        public WindowsServerRestartPolicy(
            IHealthCheck healthCheck,
            IPolicyStore policyStore,
            FabricClient.RepairManagementClient repairMgmtClient,
            IConfigStore configStore,
            string configSectionName)
        {
            this.HealthCheck = healthCheck;
            this.PolicyStore = policyStore;
            this.repairManagementClient = repairMgmtClient;
            this.currentChosenUpgradeDomain = EmptyUpgradeDomainValue;
            this.maxTimeRepairTaskActive = WindowsServerRestartConstant.MaxTimeRepairTaskActiveDefault;            
        }

        #region IRestartPolicy Members

        public IHealthCheck HealthCheck { get; set; }

        public IPolicyStore PolicyStore { get; set; }

        public async Task InitializeAsync(CancellationToken token)
        {
            bool keyExists = await this.PolicyStore.KeyPresentAsync(
                ChosenUpgradePropertyKey,
                WindowsServerRestartConstant.FabricOperationTimeout, 
                token);
            if (keyExists)
            {
                Trace.WriteInfo(
                    TraceType,
                    "RestartPolicy: Key name '{0}' already exists!",
                    ChosenUpgradePropertyKey);
                byte[] value = await this.PolicyStore.GetDataAsync(
                    ChosenUpgradePropertyKey,
                    WindowsServerRestartConstant.FabricOperationTimeout,
                    token);
                this.currentChosenUpgradeDomain = DeserializeValue(value);
            }
            else
            {
                await this.PolicyStore.AddOrUpdate(
                    ChosenUpgradePropertyKey,
                    SerializeValue(EmptyUpgradeDomainValue),
                    WindowsServerRestartConstant.FabricOperationTimeout,
                    token);
                Trace.WriteInfo(TraceType, "RestartPolicy: Successfully created Key name '{0}'", ChosenUpgradePropertyKey);
            }
        }

        public async Task<IList<RepairTask>> GetRepairTaskToApproveAsync(CancellationToken token)
        {
            Trace.WriteNoise(TraceType, "GetRepairTaskToApproveAsync");
            IEnumerable<RepairTask> activeRepairList = await this.GetActiveRepairList(token);
            IEnumerable<RepairTask> claimedRepairTask = await this.GetRepairList(
                ClaimedStateFilter,
                WindowsServerRestartConstant.FabricOperationTimeout, 
                token);

            string activeUd = activeRepairList.Count() != 0 ? activeRepairList.First().ExecutorData : string.Empty;
            Debug.Assert(
                string.IsNullOrWhiteSpace(activeUd) || 
                string.IsNullOrWhiteSpace(this.currentChosenUpgradeDomain) || 
                activeUd == this.currentChosenUpgradeDomain, 
                string.Format("Current set UD: {0}, Repair task UD:{1}", activeUd, this.currentChosenUpgradeDomain));

            if (string.IsNullOrWhiteSpace(activeUd))
            {
                activeUd = ChooseNextUd(claimedRepairTask, this.currentChosenUpgradeDomain);
            }

            if (!string.IsNullOrWhiteSpace(activeUd) && activeUd != this.currentChosenUpgradeDomain && this.HealthCheck != null)
            {
                var healthCheckResult = await this.HealthCheck.PerformCheckAsync(token);
                Trace.WriteInfo(TraceType, "GetRepairTaskToApproveAsync: Health check result {0}", healthCheckResult);
                if (!healthCheckResult)
                {
                    return null;
                }
            }

            if (this.currentChosenUpgradeDomain != activeUd)
            {
                Trace.WriteInfo(TraceType, "GetRepairTaskToApproveAsync: Prev UD:{0} NextUD:{1}", this.currentChosenUpgradeDomain, activeUd );
                this.currentChosenUpgradeDomain = activeUd;
                await this.PolicyStore.AddOrUpdate(
                        ChosenUpgradePropertyKey,
                        SerializeValue(this.currentChosenUpgradeDomain),
                        WindowsServerRestartConstant.FabricOperationTimeout,
                        token);
            }

            return string.IsNullOrWhiteSpace(this.currentChosenUpgradeDomain) ? null : claimedRepairTask.Where(task => task.ExecutorData == this.currentChosenUpgradeDomain).ToList();
        }

        #endregion

        private async Task<IEnumerable<RepairTask>> GetActiveRepairList(CancellationToken token)
        {
            var activeList = await this.GetRepairList(
                ActiveStateFilter,
                WindowsServerRestartConstant.FabricOperationTimeout, 
                token);
            DateTime cuttoTime = DateTime.UtcNow.Subtract(this.maxTimeRepairTaskActive);
            return activeList.Where(
                task =>
                {
                    if (task.State == RepairTaskState.Approved && task.ApprovedTimestamp.HasValue && task.ApprovedTimestamp.Value < cuttoTime)
                    {
                        Trace.WriteWarning(TraceType, "GetActiveRepairList: Approved Task:{0} dropped with approved timestamp:{1}", task.TaskId, task.ApprovedTimestamp);
                        return false;
                    }

                    if (task.State == RepairTaskState.Executing && task.ExecutingTimestamp.HasValue && task.ExecutingTimestamp.Value < cuttoTime)
                    {
                        Trace.WriteWarning(TraceType, "GetActiveRepairList: Executing Task:{0} dropped with executing timestamp:{1}", task.TaskId, task.ExecutingTimestamp);
                        return false;
                    }

                    return true;
                });
        }

        private async Task<IEnumerable<RepairTask>> GetRepairList(RepairTaskStateFilter stateFilter, TimeSpan timeout, CancellationToken token)
        {
            var repairTaskList = await this.repairManagementClient.GetRepairTaskListAsync(
                string.Empty,
                stateFilter,
                string.Empty,
                timeout,
                token);
            return repairTaskList.Where(task => task.Executor.StartsWith(ExecutorPrefix) && !string.IsNullOrWhiteSpace(task.ExecutorData));
        }

        // If there is any other task of the current chosen UD then that don't change the UD otherwise change the UD with least.
        private static string ChooseNextUd(IEnumerable<RepairTask> claimedRepairTasks, string currentActiveUpgradeDomain)
        {
            // If a UD was rebooting and there is claimed repair task of that UD then choose that UD.
            if (!string.IsNullOrWhiteSpace(currentActiveUpgradeDomain) &&
                claimedRepairTasks.Any(task => task.ExecutorData == currentActiveUpgradeDomain))
            {
                return currentActiveUpgradeDomain;
            }

            // Choose the UD belonging to the earliest repair task.
            if (!claimedRepairTasks.Any())
            {
                return EmptyUpgradeDomainValue;
            }

            RepairTask choosenTask = claimedRepairTasks.FirstOrDefault(task => task.ClaimedTimestamp.HasValue);
            if (choosenTask == default(RepairTask))
            {
                choosenTask = claimedRepairTasks.First();
            }

            foreach (var task in claimedRepairTasks)
            {
                if (task.ClaimedTimestamp.HasValue && 
                    choosenTask.ClaimedTimestamp.HasValue && 
                    task.ClaimedTimestamp.Value < choosenTask.ClaimedTimestamp.Value)
                {
                    choosenTask = task;
                }
            }

            return choosenTask.ExecutorData;
        }

        private static byte[] SerializeValue(string value)
        {
            return Encoding.ASCII.GetBytes(value);
        }

        private static string DeserializeValue(byte[] value)
        {
            return Encoding.ASCII.GetString(value);
        }
    }

    internal class WindowsServerRestartPolicyFactory : IRestartPolicyFactory
    {
        private readonly IHealthCheck healthCheck;
        private readonly IPolicyStore policyStore;
        private readonly FabricClient.RepairManagementClient repairManagementClient;
        private readonly IConfigStore configStore;
        private readonly string configSectionName;

        public WindowsServerRestartPolicyFactory(
                                                IHealthCheck healthCheck, 
                                                IPolicyStore policyStore,
                                                FabricClient.RepairManagementClient repairMgmtClient,
                                                IConfigStore configStore,
                                                string configSectionName)
        {
            repairMgmtClient.ThrowIfNull("repairMgmtClient");

            this.policyStore = policyStore;
            this.healthCheck = healthCheck;
            this.repairManagementClient = repairMgmtClient;
            this.configStore = configStore;
            this.configSectionName = configSectionName;
        }

        public IRestartPolicy Create()
        {
            return new WindowsServerRestartPolicy(
                this.healthCheck, 
                this.policyStore,
                this.repairManagementClient,
                this.configStore,
                this.configSectionName);
        }
    }
}