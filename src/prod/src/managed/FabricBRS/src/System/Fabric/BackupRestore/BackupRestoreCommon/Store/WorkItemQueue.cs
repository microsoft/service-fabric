// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using Microsoft.ServiceFabric.Data.Collections;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;

    internal class WorkItemQueue : BaseWorkItemQueue
    {
        private static volatile WorkItemQueue workItemQueue;
        private const string TraceTypeConst  =  "WorkItemQueue";
        private static object syncRoot = new Object();

        internal static async Task<WorkItemQueue> CreateOrGetWorkItemQueue(StatefulService statefulService)
        {
            if (workItemQueue == null)
            {
                IReliableQueue<WorkItemProcessInfo> workItemReliableQueue =
                    await statefulService.StateManager.GetOrAddAsync<IReliableQueue<WorkItemProcessInfo>>(Constants.WorkItemQueue);

                lock (syncRoot)
                {
                    if (workItemQueue == null)
                    {
                        workItemQueue = new WorkItemQueue(workItemReliableQueue, 0, WorkItemQueueRunType.WorkItemQueue, statefulService);
                    }
                }
            }

            return workItemQueue;
        }

        internal void DisposeWorkItemQueue()
        {
            this.DisposeWorkItemDeQueuer();
            workItemQueue = null;
        }

        private WorkItemQueue(IReliableQueue<WorkItemProcessInfo> workItemReliableQueue, int maxWaitTimeInMinutes, WorkItemQueueRunType workItemQueueRunType,
            StatefulService statefulService) : base(workItemReliableQueue, maxWaitTimeInMinutes, workItemQueueRunType, TraceTypeConst, statefulService)
        {
        }

        internal async Task<bool> AddWorkItem(WorkItem workItem, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction = null)
        {
            WorkItemProcessInfo workItemProcessInfo = new WorkItemProcessInfo(workItem.WorkItemGuid, DateTime.UtcNow);
            if (transaction == null)
            {
                using (transaction = this.StatefulService.StateManager.CreateTransaction())
                {
                    await this.WorkItemStore.AddAsync(workItem.WorkItemGuid, workItem,transaction);
                    await this.AddWorkItem( workItemProcessInfo,timeout,cancellationToken,  transaction);
                    await transaction.CommitAsync();
                }
            }
            else
            {
                await this.WorkItemStore.AddAsync(workItem.WorkItemGuid, workItem, transaction);
                await this.AddWorkItem(workItemProcessInfo, timeout, cancellationToken, transaction);
            }
            return true;
        }
    }

    internal class WorkItemQueue1Min : BaseWorkItemQueue
    {
        private static WorkItemQueue1Min workItemQueue;
        private const string TraceTypeConst = "WorkItem1MinQueue";

        internal static async Task<WorkItemQueue1Min> CreateOrGetWorkItemQueue(StatefulService statefulService)
        {
            if (workItemQueue == null)
            {
                IReliableQueue<WorkItemProcessInfo> workItemReliableQueue = await statefulService.StateManager.GetOrAddAsync<IReliableQueue<WorkItemProcessInfo>>(Constants.WorkItemQueue1Min);
                workItemQueue = new WorkItemQueue1Min(workItemReliableQueue, 1, WorkItemQueueRunType.WorkItemQueue1MinDelay,
                    statefulService);
            }
            return workItemQueue;
        }

        private WorkItemQueue1Min(IReliableQueue<WorkItemProcessInfo> workItemReliableQueue, int maxWaitTimeInMinutes,
            WorkItemQueueRunType workItemQueueRunType,
            StatefulService statefulService)
            : base(workItemReliableQueue, maxWaitTimeInMinutes, workItemQueueRunType, TraceTypeConst, statefulService)
        {
        }

        internal void DisposeWorkItemQueue()
        {
            this.DisposeWorkItemDeQueuer();
            workItemQueue = null;
        }

    }

    internal class WorkItemQueue2Min : BaseWorkItemQueue
    {
        private static WorkItemQueue2Min workItemQueue;
        private const string TraceTypeConst = "WorkItem2MinQueue";
        
        internal static async Task<WorkItemQueue2Min> CreateOrGetWorkItemQueue(StatefulService statefulService)
        {
            if (workItemQueue == null)
            {
                IReliableQueue<WorkItemProcessInfo> workItemReliableQueue = await statefulService.StateManager.GetOrAddAsync<IReliableQueue<WorkItemProcessInfo>>(Constants.WorkItemQueue2Min);
                workItemQueue = new WorkItemQueue2Min(workItemReliableQueue, 2, WorkItemQueueRunType.WorkItemQueue2MinDelay,
                    statefulService);
            }
            return workItemQueue;
        }

        private WorkItemQueue2Min(IReliableQueue<WorkItemProcessInfo> workItemReliableQueue, int maxWaitTimeInMinutes,
            WorkItemQueueRunType workItemQueueRunType,
            StatefulService statefulService)
            : base(workItemReliableQueue, maxWaitTimeInMinutes, workItemQueueRunType, TraceTypeConst, statefulService)
        {
        }

        internal void DisposeWorkItemQueue()
        {
            this.DisposeWorkItemDeQueuer();
            workItemQueue = null;
        }

    }

    internal class WorkItemQueue4Min : BaseWorkItemQueue
    {
        private static WorkItemQueue4Min workItemQueue;
        private const string TraceTypeConst = "WorkItem4MinQueue";

        internal static async Task<WorkItemQueue4Min> CreateOrGetWorkItemQueue(StatefulService statefulService)
        {
            if (workItemQueue == null)
            {
                IReliableQueue<WorkItemProcessInfo> workItemReliableQueue = await statefulService.StateManager.GetOrAddAsync<IReliableQueue<WorkItemProcessInfo>>(Constants.WorkItemQueue4Min);
                workItemQueue = new WorkItemQueue4Min(workItemReliableQueue, 4, WorkItemQueueRunType.WorkItemQueue4MinDelay,
                    statefulService);
            }
            return workItemQueue;
        }

        private WorkItemQueue4Min(IReliableQueue<WorkItemProcessInfo> workItemReliableQueue, int maxWaitTimeInMinutes,
            WorkItemQueueRunType workItemQueueRunType,
            StatefulService statefulService)
            : base(workItemReliableQueue, maxWaitTimeInMinutes, workItemQueueRunType, TraceTypeConst, statefulService)
        {
        }

        internal void DisposeWorkItemQueue()
        {
            this.DisposeWorkItemDeQueuer();
            workItemQueue = null;
        }

    }

    internal class WorkItemQueue8Min : BaseWorkItemQueue
    {
        private static WorkItemQueue8Min workItemQueue;
        private const string TraceTypeConst = "WorkItem8MinQueue";
      
        internal static async Task<WorkItemQueue8Min> CreateOrGetWorkItemQueue(StatefulService statefulService)
        {
            if (workItemQueue == null)
            {
                IReliableQueue<WorkItemProcessInfo> workItemReliableQueue = await statefulService.StateManager.GetOrAddAsync<IReliableQueue<WorkItemProcessInfo>>(Constants.WorkItemQueue8Min);
                workItemQueue = new WorkItemQueue8Min(workItemReliableQueue, 8, WorkItemQueueRunType.WorkItemQueue8MinDelay,
                    statefulService);
            }
            return workItemQueue;
        }

        private WorkItemQueue8Min(IReliableQueue<WorkItemProcessInfo> workItemReliableQueue, int maxWaitTimeInMinutes,
            WorkItemQueueRunType workItemQueueRunType,
            StatefulService statefulService)
            : base(workItemReliableQueue, maxWaitTimeInMinutes, workItemQueueRunType, TraceTypeConst, statefulService)
        {
        }
        internal void DisposeWorkItemQueue()
        {
            this.DisposeWorkItemDeQueuer();
            workItemQueue = null;
        }

    }
}