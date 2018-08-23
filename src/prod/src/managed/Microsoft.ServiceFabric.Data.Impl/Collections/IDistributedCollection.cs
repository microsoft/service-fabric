// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// IDistributedCollection interface.
    /// </summary>
    internal interface IDistributedCollection
    {
        /// <summary>
        /// Gets the boolean indicating whether the collection is registered or not.
        /// </summary>
        bool IsRegistered { get; }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        Task<long> GetCountAsync(Transaction tx);

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <param name="tx"></param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        Task<long> GetCountAsync(Transaction tx, TimeSpan timeout, CancellationToken cancellationToken);
    }
}