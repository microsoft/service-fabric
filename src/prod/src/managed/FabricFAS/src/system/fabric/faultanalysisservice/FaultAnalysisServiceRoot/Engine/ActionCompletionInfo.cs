// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Engine
{
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ActionCompletionInfo
    {
        public ActionCompletionInfo(Guid key, Task task, SemaphoreSlim semaphore)
        {            
            ThrowIf.Null(task, "task");
            ThrowIf.Null(semaphore, "semaphore");

            this.Key = key;
            this.Task = task;
            this.Semaphore = semaphore;
        }

        public Guid Key
        {
            get;
            private set;
        }

        public Task Task
        {
            get;
            private set;
        }

        public SemaphoreSlim Semaphore
        {
            get;
            private set;
        }
    }
}