// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;


    internal interface IWorkItem<TResult>
    {
        Task<TResult> Process(StatefulService statefulService,TimeSpan timeout,CancellationToken cancellationToken, string processQueueTypeTrace);

        Task ProcessSuccessHandle(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken, string processQueueTypeTrace);

        Task ProcessFailureHandle(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken, string processQueueTypeTrace);
    }
    
}