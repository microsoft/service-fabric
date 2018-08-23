// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Empty operation data stream.
    /// </summary>
    internal class EmptyOperationDataStream : IOperationDataStream
    {
        #region IOperationDataStream Members

        /// <summary>
        /// No operation is returned.
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            return await Task.FromResult<OperationData>(null);
        }

        #endregion
    }
}