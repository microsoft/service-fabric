// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Data.Collections;
    
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;
    using System.Threading;

    internal abstract class BaseWorkItemQueue
    {
        private const int FastQueueWaitTime = 5000;
        internal readonly int MaxWaitTimeInMilliSeconds = 0;
        internal readonly long AllowedTicks = 0;

        protected internal IReliableQueue<WorkItemProcessInfo> WorkItemReliableQueue;
        protected internal StatefulService StatefulService;
        protected internal Timer WorkItemHandlerTimer;
        protected internal WorkItemStore WorkItemStore;
        protected internal WorkItemInProcessStore WorkItemInProcessStore;
        private const int MaxWorkItemCount = Constants.MaxWorkItemProcess;
        private readonly WorkItemQueueRunType workItemQueueRunType;

        protected readonly string TraceType;
        private readonly string  emptyTraceType;

        internal BaseWorkItemQueue(IReliableQueue<WorkItemProcessInfo> workItemReliableQueue,int maxWaitTimeInMinutes, 
            WorkItemQueueRunType workItemQueueRunType, string traceType ,StatefulService statefulService)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(traceType, "Constructing BaseWorkItemQueue with following parameters - workItemReliableQueue: {0}, maxWaitTimeInMinutes: {1}, workItemQueueRunType: {2}", 
                workItemReliableQueue.Name,
                maxWaitTimeInMinutes,
                workItemQueueRunType);

            this.WorkItemReliableQueue = workItemReliableQueue;
            this.StatefulService = statefulService;
            this.WorkItemStore = WorkItemStore.CreateOrGetWorkItemStore(this.StatefulService).GetAwaiter().GetResult();
            this.WorkItemInProcessStore =
                WorkItemInProcessStore.CreateOrGetWorkItemInProcessStore(this.StatefulService).GetAwaiter().GetResult();
            this.MaxWaitTimeInMilliSeconds = maxWaitTimeInMinutes == 0 ? FastQueueWaitTime : maxWaitTimeInMinutes * 60 * 1000 ;
            this.AllowedTicks = (this.MaxWaitTimeInMilliSeconds*TimeSpan.TicksPerMillisecond)/20;
            this.workItemQueueRunType = workItemQueueRunType;
            this.TraceType = traceType;
            this.emptyTraceType = string.Format("{0}.{1}", traceType, "Empty");

            this.WorkItemHandlerTimer = new Timer(new TimerCallback(this.ProcessWorkItemHandler),
                                        null,
                                        this.MaxWaitTimeInMilliSeconds,
                                        Timeout.Infinite);
        }

        internal async Task<bool> AddWorkItem(WorkItemProcessInfo workItemProcessInfo,TimeSpan timeout, CancellationToken cancellationToken,  ITransaction transaction = null)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Adding Work Item Id:{0} , DueDateTime:{1} , WorkItemQueueRunType : {2} ", 
                workItemProcessInfo.WorkItemId , workItemProcessInfo.DueDateTime , workItemProcessInfo.WorkItemQueueRunType);
            if (this.MaxWaitTimeInMilliSeconds != FastQueueWaitTime)
            {
                workItemProcessInfo = workItemProcessInfo.ToBuilder()
                                        .WithDueDateTime(DateTime.UtcNow.AddMilliseconds(this.MaxWaitTimeInMilliSeconds))
                                        .WithWorkItemQueueRunTimeType(this.workItemQueueRunType)
                                        .Build();
            }
            else
            {
                workItemProcessInfo = workItemProcessInfo.ToBuilder()
                                        .WithWorkItemQueueRunTimeType(this.workItemQueueRunType)
                                        .Build();
            }

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Adding Work Item Id:{0} , UpdatedDueDateTime:{1} , UpdatedWorkItemQueueRunType : {2} ",
                workItemProcessInfo.WorkItemId, workItemProcessInfo.DueDateTime, workItemProcessInfo.WorkItemQueueRunType);
            if (transaction == null)
            {
                using (transaction = this.StatefulService.StateManager.CreateTransaction())
                {
                    await this.WorkItemReliableQueue.EnqueueAsync(transaction, workItemProcessInfo,timeout,cancellationToken);
                    await transaction.CommitAsync();
                }
            }
            else
            {
                await this.WorkItemReliableQueue.EnqueueAsync(transaction, workItemProcessInfo, timeout, cancellationToken);
            }
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Added Work Item Id:{0} , UpdatedDueDateTime:{1} , WorkItemQueueRunType : {2} ",
                workItemProcessInfo.WorkItemId, workItemProcessInfo.DueDateTime, workItemProcessInfo.WorkItemQueueRunType);
            return true;
        }

        private async void ProcessWorkItemHandler(object dummyObject)
        {
            ConditionalValue<WorkItemProcessInfo> conditionalWorkItemProcessInfo;
            int workItemCount = 0;
            List<WorkItem> workItemTasks = new List<WorkItem>(MaxWorkItemCount);
            
            try
            {
                using (ITransaction transaction = this.StatefulService.StateManager.CreateTransaction())
                {
                    bool currentWorkItemIsDue = true;
                    do
                    {
                        conditionalWorkItemProcessInfo = await this.WorkItemReliableQueue.TryPeekAsync(transaction);
                        if (conditionalWorkItemProcessInfo.HasValue)
                        {
                            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItemProcessInfo Peeked : {0} ",
                                conditionalWorkItemProcessInfo.Value);
                            
                            if ((conditionalWorkItemProcessInfo.Value.DueDateTime.Ticks < DateTime.UtcNow.Ticks) ||
                                ((conditionalWorkItemProcessInfo.Value.DueDateTime.Ticks - DateTime.UtcNow.Ticks) < this.AllowedTicks) )
                            {
                                conditionalWorkItemProcessInfo =
                                    await this.WorkItemReliableQueue.TryDequeueAsync(transaction);
                                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItemProcessInfo DeQueue :{0} ",conditionalWorkItemProcessInfo.Value);
                                if (conditionalWorkItemProcessInfo.HasValue)
                                {
                                    WorkItem workItem =
                                        await
                                            this.WorkItemStore.GetValueAsync(
                                                conditionalWorkItemProcessInfo.Value.WorkItemId);
                                    workItemTasks.Add(workItem);
                                    conditionalWorkItemProcessInfo = new ConditionalValue<WorkItemProcessInfo>(true, 
                                                                        conditionalWorkItemProcessInfo.Value.ToBuilder().WithWorkItemQueueRunTimeType(this.workItemQueueRunType).Build());
                                    await
                                        this.WorkItemInProcessStore.AddAsync(workItem.WorkItemGuid,
                                            conditionalWorkItemProcessInfo.Value, transaction);
                                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItem Scheduled :  {0} ",
                                        workItem);
                                }
                            }
                            else
                            {
                                currentWorkItemIsDue = false;
                            }
                            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "workItemCount {0}, hasValue {1} , currentWorkItemIsDue {2}  ", workItemCount,
                                conditionalWorkItemProcessInfo.HasValue, currentWorkItemIsDue);
                        }
                        else
                        {
                            BackupRestoreTrace.TraceSource.WriteNoise(emptyTraceType, "workItemCount {0}, hasValue {1} , currentWorkItemIsDue {2}  ", workItemCount, 
                                conditionalWorkItemProcessInfo.HasValue,currentWorkItemIsDue);
                        }
                    } while (++workItemCount < MaxWorkItemCount && conditionalWorkItemProcessInfo.HasValue && currentWorkItemIsDue);
                    await transaction.CommitAsync();
                }
                workItemTasks.ForEach((item =>
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItem scheduled to process {0}  ", item);
                    Task task = item.TryToProcessAsync(this.StatefulService, TraceType);
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItem schedules process {0}  ", item);
                }));
                if (workItemCount == MaxWorkItemCount)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItemHandlerTimer Changes Time : 0  ");
                    this.WorkItemHandlerTimer.Change(0, Timeout.Infinite); 
                }
                else if (!conditionalWorkItemProcessInfo.HasValue)
                {
                    BackupRestoreTrace.TraceSource.WriteNoise(emptyTraceType, "WorkItemHandlerTimer Changes Time : {0} ",this.MaxWaitTimeInMilliSeconds);
                    this.WorkItemHandlerTimer.Change(this.MaxWaitTimeInMilliSeconds, Timeout.Infinite);
                }
                else
                {
                    TimeSpan nextUpdateTime = conditionalWorkItemProcessInfo.Value.DueDateTime.Subtract(DateTime.UtcNow);
                    long nextUpdateEffectiveTime =   nextUpdateTime.TotalMilliseconds < 0 ? 0 : Convert.ToInt64(nextUpdateTime.TotalMilliseconds);
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItemHandlerTimer Changes Time : {0}, Next Update Time {1}  ", nextUpdateTime , nextUpdateEffectiveTime);
                    this.WorkItemHandlerTimer.Change(nextUpdateEffectiveTime, Timeout.Infinite);
                }

            }
            catch (Exception exception)
            {
                AggregateException aggregateException = exception as AggregateException;

                if (aggregateException != null)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Aggregate Exception Stack Trace : {0}",
                        exception.StackTrace);
                    foreach (var innerException in aggregateException.InnerExceptions)
                    {
                        BackupRestoreTrace.TraceSource.WriteWarning(TraceType,
                            "Inner Exception : {0} , Message : {1} , Stack Trace : {2} ",
                            innerException.InnerException, innerException.Message, innerException.StackTrace);

                    }
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType,
                        "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        exception.InnerException, exception.Message, exception.StackTrace);
                }

                try
                {
                    this.WorkItemHandlerTimer.Change(this.MaxWaitTimeInMilliSeconds, Timeout.Infinite);
                    //  ProcessWorkItemHandler to continue unless  disposed 
                }
                catch (ObjectDisposedException objectDisposedException)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "{0} : {1}",
                        objectDisposedException.GetType(),
                        objectDisposedException.ToString());
                }
            }
        }

        internal static async Task<BaseWorkItemQueue> GetNextWorkItemQueue(WorkItemQueueRunType workItemQueueRunType, StatefulService statefulService)
        {
            BackupRestoreTrace.TraceSource.WriteInfo("BackupRestore.GetNextWorkItemQueue", "workItemQueueRunType {0} " , workItemQueueRunType);
            BaseWorkItemQueue baseWorkItemQueue = null;
            switch (workItemQueueRunType)
            {
                case WorkItemQueueRunType.WorkItemQueue:
                    baseWorkItemQueue = await WorkItemQueue1Min.CreateOrGetWorkItemQueue(statefulService);
                 break;
                case WorkItemQueueRunType.WorkItemQueue1MinDelay:
                    baseWorkItemQueue = await WorkItemQueue2Min.CreateOrGetWorkItemQueue(statefulService);
                    break;
                case WorkItemQueueRunType.WorkItemQueue2MinDelay:
                    baseWorkItemQueue = await WorkItemQueue4Min.CreateOrGetWorkItemQueue(statefulService);
                    break;
                case WorkItemQueueRunType.WorkItemQueue4MinDelay:
                    baseWorkItemQueue = await WorkItemQueue8Min.CreateOrGetWorkItemQueue(statefulService);
                    break;
                case WorkItemQueueRunType.WorkItemQueue8MinDelay:
                    baseWorkItemQueue = await WorkItemQueue8Min.CreateOrGetWorkItemQueue(statefulService);
                    break;
            }
            BackupRestoreTrace.TraceSource.WriteInfo("BackupRestore.GetNextWorkItemQueue", "Next WorkItemQueue {0} ", baseWorkItemQueue.GetType());
            return baseWorkItemQueue;
        }

        internal void DisposeWorkItemDeQueuer()
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItemQueue about to disposing.");
            this.WorkItemHandlerTimer.Dispose();
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "WorkItemQueue about to disposed.");
        }
    }
}