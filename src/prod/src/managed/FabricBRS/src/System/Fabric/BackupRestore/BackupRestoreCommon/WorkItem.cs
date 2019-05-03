// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Service;

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Fabric.Common;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Text;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;


    [DataContract]
    [KnownType(typeof(ResolveToPartitionWorkItem))]
    [KnownType(typeof(SendToServiceNodeWorkItem))]
    [KnownType(typeof(UpdateEnablementWorkItem))]
    [KnownType(typeof(RestorePartitionWorkItem))]
    [KnownType(typeof(BackupPartitionWorkItem))]

    abstract class WorkItem : IWorkItem<bool>
    {
        [DataMember]
        protected internal Guid WorkItemGuid { get; private set; }

        private WorkItemInProcessStore workItemInProcessStore;


        internal WorkItem()
        {
            this.WorkItemGuid = Guid.NewGuid();
        }
        public abstract Task<bool> Process(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken,string processQueueTypeTrace);

        public virtual async Task ProcessSuccessHandle(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken, string processQueueTypeTrace)
        {
            WorkItemStore workItemStore = await WorkItemStore.CreateOrGetWorkItemStore(statefulService);
            using (ITransaction transaction = statefulService.StateManager.CreateTransaction())
            {
                await workItemStore.DeleteValueAsync(this.WorkItemGuid,timeout,cancellationToken, transaction);
                await workItemInProcessStore.DeleteValueAsync(this.WorkItemGuid,timeout,cancellationToken, transaction);
                await transaction.CommitAsync();
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Process Success Handle WorkItemStore and WorkItemInProcessStore Delete From Store WorkItem : {0} is success.", this);
            }
        }

        public virtual async Task ProcessFailureHandle(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken, string processQueueTypeTrace)
        {
            WorkItemProcessInfo workItemProcessInfo = await workItemInProcessStore.GetValueAsync(this.WorkItemGuid);
            BaseWorkItemQueue baseWorkItemQueue = await BaseWorkItemQueue.GetNextWorkItemQueue(workItemProcessInfo.WorkItemQueueRunType, statefulService);
            using (ITransaction transaction = statefulService.StateManager.CreateTransaction())
            {
                await workItemInProcessStore.DeleteValueAsync(this.WorkItemGuid,timeout,cancellationToken, transaction);
                await baseWorkItemQueue.AddWorkItem(workItemProcessInfo,timeout,cancellationToken, transaction);
                await transaction.CommitAsync();
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Process Failure Handle WorkItemInProcessStore Delete WorkItem {0} And Enqueue in {1}  " , this, baseWorkItemQueue.GetType());
            }
        }

        public async Task TryToProcessAsync(StatefulService  statefulService, string processQueueTypeTrace)
        {
            var workItemTimeout = BackupRestoreServiceConfig.WorkItemTimeout;
            try
            {
                workItemInProcessStore = await WorkItemInProcessStore.CreateOrGetWorkItemInProcessStore(statefulService);
                bool processStatus = false;
                try
                {
                    processStatus = await UtilityHelper.InvokeWithRetry<bool>( () =>
                    {
                        var cancellationTokenSource = new CancellationTokenSource(workItemTimeout);
                        return this.Process(statefulService, workItemTimeout, cancellationTokenSource.Token,processQueueTypeTrace);
                    } 
                    );
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "The WorkItem {0} is Processed as {1}",this,processStatus);
                }
                catch (Exception exception)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace, "Exception While TryToProcessAsync");
                    AggregateException aggregateException = exception as AggregateException;
                
                    if (aggregateException != null)
                    {
                        BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace, "Aggregate Exception Stack Trace : {0}",
                            exception.StackTrace);
                        foreach (Exception innerException in aggregateException.InnerExceptions)
                        {
                            BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace,
                                "Inner Exception : {0} , Message : {1} , Stack Trace : {2} ",
                                innerException.InnerException, innerException.Message, innerException.StackTrace);

                        }
                    }
                    else
                    {
                        BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace,
                            "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                            exception.InnerException, exception.Message, exception.StackTrace);
                    }
                }
                if (processStatus)
                {
                    var cancellationTokenSource = new CancellationTokenSource(workItemTimeout);
                    await UtilityHelper.InvokeWithRetry((() => this.ProcessSuccessHandle(statefulService, workItemTimeout, cancellationTokenSource.Token, processQueueTypeTrace)));
                }
                else
                {
                    var cancellationTokenSource = new CancellationTokenSource(workItemTimeout);
                    await UtilityHelper.InvokeWithRetry((() => this.ProcessFailureHandle(statefulService, workItemTimeout, cancellationTokenSource.Token, processQueueTypeTrace)));
                }
            }
            catch (Exception exception)
            {
                AggregateException aggregateException = exception as AggregateException;
                ;
                if (aggregateException != null)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace, "Aggregate Exception Stack Trace : {0}",
                        exception.StackTrace);
                    foreach (Exception innerException in aggregateException.InnerExceptions)
                    {
                        BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace,
                            "Inner Exception : {0} , Message : {1} , Stack Trace : {2} ",
                            innerException.InnerException, innerException.Message, innerException.StackTrace);

                    }
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace,
                        "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        exception.InnerException, exception.Message, exception.StackTrace);
                }
            }

        }

        public override string ToString()
        {
            return string.Format("WorkItemGuid as {0}",this.WorkItemGuid);
        }
    }
}