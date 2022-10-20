// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;

    internal class WorkItemHandler
    {
        private const string TraceType = "WorkItemHandler";

        private static WorkItemQueue workItemQueue;
        private static WorkItemQueue1Min workItemQueue1Min;
        private static WorkItemQueue2Min workItemQueue2Min;
        private static WorkItemQueue4Min workItemQueue4Min;
        private static WorkItemQueue8Min workItemQueue8Min;

        internal static async Task StartAndScheduleWorkItemHandler(StatefulService statefulService)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Initializing Work Item Queue and Handlers");

            workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(statefulService);
            workItemQueue1Min = await WorkItemQueue1Min.CreateOrGetWorkItemQueue(statefulService);
            workItemQueue2Min = await WorkItemQueue2Min.CreateOrGetWorkItemQueue(statefulService);
            workItemQueue4Min = await WorkItemQueue4Min.CreateOrGetWorkItemQueue(statefulService);
            workItemQueue8Min = await WorkItemQueue8Min.CreateOrGetWorkItemQueue(statefulService);

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Initialized Work Item Queue and Handlers");

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Initializing RecoverInProcessWorkItemWrapper");
            await RecoverInProcessWorkItemWrapper(statefulService);
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "RecoverInProcessWorkItemWrapper Completed");
        }

        internal static void StopWorkItemHandler()
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Stopping Work Item Queue and Handlers");

            workItemQueue?.DisposeWorkItemQueue();
            workItemQueue1Min?.DisposeWorkItemQueue();
            workItemQueue2Min?.DisposeWorkItemQueue();
            workItemQueue4Min?.DisposeWorkItemQueue();
            workItemQueue8Min?.DisposeWorkItemQueue();

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Stopped Work Item Queue and Handlers");
        }

        private static async Task RecoverInProcessWorkItemWrapper(StatefulService statefulService)
        {
            await RecoverInProcessWorkItems(statefulService);
        }

        private static async Task RecoverInProcessWorkItems(StatefulService statefulService)
        {
            try
            {
                WorkItemInProcessStore workItemInProcessStore =
                    await WorkItemInProcessStore.CreateOrGetWorkItemInProcessStore(statefulService);
                WorkItemStore workItemStore = await WorkItemStore.CreateOrGetWorkItemStore(statefulService);
                List<WorkItemProcessInfo> workItemProcessInfos = await workItemInProcessStore.GetValuesAsync();

                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Work Item to be Recovered Count {0}" , workItemProcessInfos.Count);
                foreach (WorkItemProcessInfo workItemProcessInfo in workItemProcessInfos)
                {
                    WorkItem workItem = await workItemStore.GetValueAsync(workItemProcessInfo.WorkItemId);
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Work Item Recovered Type : {0} , Id : {1}", workItem.GetType(), workItemProcessInfo.WorkItemId);
                    Task processTask = workItem.TryToProcessAsync(statefulService, TraceType);
                }
            }
            catch (Exception exception)
            {
                AggregateException aggregateException = exception as AggregateException;
                ;
                if (aggregateException != null)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Aggregate Exception Stack Trace : {0}",
                        exception.StackTrace);
                    foreach (Exception innerException in aggregateException.InnerExceptions)
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
            }
        }
    }
    
}