// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Fabric.Repair;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Linq;
    using System.Threading.Tasks;    
    using System.Threading;    

    /// <summary>
    /// Coordinates reboot of server machine by coordinating repair task created by each node.
    /// </summary>
    internal sealed class WindowsServerRestartCoordinator : IPolicyStore, IInfrastructureCoordinator
    {
        private static readonly TraceType TraceType = new TraceType("WindowsServerRestartCoordinator");
        private readonly object stateLock = new object();
        private readonly FabricClient fabricClient;
        private readonly FabricClient.RepairManagementClient repairManagementClient;
        private readonly IRestartPolicy restartPolicy;
        private readonly IRetryPolicyFactory retryPolicyFactory;

        private TaskCompletionSource<object> runTaskCompletionSource;
        private int primaryEpoch;
        private uint nextSequenceNumber;

        private readonly TimeSpan pollingInterval;

        public WindowsServerRestartCoordinator(IConfigStore configStore, string configSectionName, Guid partitionId, long replicaId)
        {
            this.fabricClient = new FabricClient();
            this.repairManagementClient = this.fabricClient.RepairManager;
            var healthPolicy = new WindowsServerRestartHealthCheckFactory(this.fabricClient).Create();
            this.retryPolicyFactory = new LinearRetryPolicyFactory(
                TraceType,
                WindowsServerRestartConstant.BackoffPeriodInMilliseconds,
                WindowsServerRestartConstant.MaxRetryAttempts,
                IsRetriableException);
            this.restartPolicy = new WindowsServerRestartPolicyFactory(
                healthPolicy,
                this,
                this.repairManagementClient,
                configStore,
                configSectionName).Create();

            this.pollingInterval = WindowsServerRestartConstant.CoordinatorPollingIntervalDefault;            
        }

        #region IInfrastructureCoordinator Members

        public async Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            if (primaryEpoch < 0)
            {
                throw new ArgumentOutOfRangeException("primaryEpoch");
            }

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>();

            lock (this.stateLock)
            {
                if (this.runTaskCompletionSource != null)
                {
                    throw new InvalidOperationException("Coordinator has already been started");
                }

                if (primaryEpoch < this.primaryEpoch)
                {
                    throw new ArgumentException("Primary epoch must be non-decreasing", "primaryEpoch");
                }

                if (this.primaryEpoch < primaryEpoch)
                {
                    this.nextSequenceNumber = 0;
                }

                this.primaryEpoch = primaryEpoch;

                Trace.WriteInfo(TraceType, "RunAsync: primaryEpoch = 0x{0:X}, nextSequenceNumber = 0x{1:X}",
                    this.primaryEpoch,
                    this.nextSequenceNumber);

                this.runTaskCompletionSource = tcs;
            }

            await this.InitializeStoreNameAsync(token);
            await this.restartPolicy.InitializeAsync(token);
            await this.DoWorkAsync(token);
        }

        #endregion

        private async Task InitializeStoreNameAsync(CancellationToken token)
        {
            await this.retryPolicyFactory.Create().ExecuteAsync<bool>(
                async (t) =>
                {
                    bool nameExists = await this.fabricClient.PropertyManager.NameExistsAsync(
                        WindowsServerRestartConstant.StoreName, 
                        WindowsServerRestartConstant.FabricOperationTimeout, t);
                    if (nameExists)
                    {
                        Trace.WriteInfo(
                            TraceType,
                            "RestartPolicyStore: Store name '{0}' already exists!",
                            WindowsServerRestartConstant.StoreName);
                    }
                    else
                    {
                        await this.fabricClient.PropertyManager.CreateNameAsync(
                            WindowsServerRestartConstant.StoreName,
                            WindowsServerRestartConstant.FabricOperationTimeout,
                            t);
                        Trace.WriteInfo(
                            TraceType,
                            "RestartPolicyStore: Successfully created store name '{0}'",
                            WindowsServerRestartConstant.StoreName);
                    }

                    return true;
                },
                "InitializeStoreNameAsync",
                token).ContinueWith(
                    task =>
                    {
                        if (!task.IsFaulted)
                        {
                            return;
                        }

                        AggregateException ex = task.Exception as AggregateException;
                        if (ex != null)
                        {
                            ex.Flatten().Handle(e => e is FabricElementAlreadyExistsException);
                            Trace.WriteWarning(
                                TraceType,
                                "RestartPolicyStore: Store name '{0}' already exists!",
                                WindowsServerRestartConstant.StoreName);
                        }
                        else
                        {
                            throw task.Exception;
                        }
                    }, token);            
        }

        private async Task DoWorkAsync(CancellationToken token)
        {
            while (true)
            {
                await this.RunLoopAsync(token).ContinueWith(t =>
                {
                    token.ThrowIfCancellationRequested();
                    if (t.IsFaulted)
                    {
                        Trace.WriteError(
                            TraceType, 
                            "WindowsServerRestartCoordinator: Execption in main loop: {0}", 
                            t.Exception);
                    }
                }, token);

                await Task.Delay(this.pollingInterval, token);
                token.ThrowIfCancellationRequested();
            }
        }

        private async Task RunLoopAsync(CancellationToken token)
        {
            Trace.WriteNoise(TraceType, "Get repair task to approve");
            var repairTaskToApproveList = await this.restartPolicy.GetRepairTaskToApproveAsync(token);
            if (repairTaskToApproveList == null)
            {
                return;
            }

            foreach (var task in repairTaskToApproveList)
            {
                task.State = RepairTaskState.Preparing;
                var targetDescription = task.Target as NodeRepairTargetDescription;
                if (targetDescription == null)
                {
                    Trace.WriteError(TraceType, "Repair task target description is null. TaskId:{0}", task.TaskId);
                    continue;
                }

                if (targetDescription.Nodes.Count() != 1)
                {
                    Trace.WriteError(TraceType, "Repair task with Id:{0} Target description node count:{1}", task.TaskId, targetDescription.Nodes.Count());
                    continue;
                }

                task.Impact = new NodeRepairImpactDescription()
                {
                    ImpactedNodes =
                    {
                        new NodeImpact(targetDescription.Nodes[0], NodeImpactLevel.Restart)
                    }
                };

                Trace.WriteInfo(TraceType, "Repair task with Id:{0} state changing to Preparing", task.TaskId);
                string taskId = task.TaskId;
                await this.repairManagementClient.UpdateRepairExecutionStateAsync(
                    task, 
                    WindowsServerRestartConstant.FabricOperationTimeout, 
                    token).ContinueWith(
                    t =>
                    {
                        token.ThrowIfCancellationRequested();
                        if (t.IsFaulted)
                        {
                            Trace.WriteError(TraceType, "State update failed for task Id: {0} with execption:{1}", taskId, t.Exception);
                            return;
                        }

                        Trace.WriteInfo(TraceType, "UpdateRepairExecutionStateAsync: State changed to Preparing. Id:{0}", taskId);
                    },
                    token);
            }
        }
        
        internal static bool IsRetriableException(Exception ex)
        {
            return ex is FabricTransientException || ex is OperationCanceledException || ex is TimeoutException;
        }

        public async Task<bool> KeyPresentAsync(string key, TimeSpan timeout, CancellationToken token)
        {
            var namedProperty = await this.retryPolicyFactory.Create().ExecuteAsync<NamedPropertyMetadata>(
                async (t) => await this.fabricClient.PropertyManager.GetPropertyMetadataAsync(
                    WindowsServerRestartConstant.StoreName,
                    key, 
                    timeout, 
                    t),
                "KeyPresentAsync", 
                token
                ).ContinueWith(
                task =>
                {
                    token.ThrowIfCancellationRequested();

                    if (!task.IsFaulted)
                    {
                        return task.Result;
                    }

                    var aex = task.Exception as AggregateException;
                    if (aex != null)
                    {
                        aex.Flatten().Handle(ex => ex is FabricElementNotFoundException);
                    }

                    return null;
                }, token);
            return namedProperty != null;
        }

        public async Task<byte[]> GetDataAsync(string key, TimeSpan timeout, CancellationToken token)
        {
            var namedValue = await this.retryPolicyFactory.Create().ExecuteAsync<NamedProperty>(
                async (t) => await this.fabricClient.PropertyManager.GetPropertyAsync(
                    WindowsServerRestartConstant.StoreName,
                    key, 
                    timeout,
                    t),
                "GetDataAsync",
                token
                );
            return namedValue.GetValue<byte[]>();
        }

        public async Task AddOrUpdate(string key, byte[] data, TimeSpan timeout, CancellationToken token)
        {
            await this.retryPolicyFactory.Create().ExecuteAsync<bool>(
                async (t) =>
                {
                    await this.fabricClient.PropertyManager.PutPropertyAsync(
                        WindowsServerRestartConstant.StoreName, 
                        key, 
                        data, 
                        timeout, 
                        t);
                    return true;
                },
                "AddOrUpdate",
                token
                );
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }
    }
}