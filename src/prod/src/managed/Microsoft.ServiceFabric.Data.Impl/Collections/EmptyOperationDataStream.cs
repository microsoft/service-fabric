// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    internal class EmptyOperationDataStream : IOperationDataStream
    {
        Task<OperationData> IOperationDataStream.GetNextAsync(CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<OperationData>();

            tcs.SetResult(null);

            return tcs.Task;
        }
    }
}