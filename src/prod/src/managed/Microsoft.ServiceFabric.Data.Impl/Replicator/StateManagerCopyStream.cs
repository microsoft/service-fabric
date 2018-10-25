// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

#pragma warning disable 1998

    /// <summary>
    /// Manages copy stream for state manager.
    /// </summary>
    internal class StateManagerCopyStream : IOperationDataStream, IDisposable
    {
        /// <summary>
        /// State manager's state enumerator.
        /// </summary>
        private readonly IEnumerator<SerializableMetadata> stateEnumerator;

        /// <summary>
        /// State manager's copy data.
        /// </summary>
        private readonly IEnumerator<OperationData> stateManagerFileCopyData;

        /// <summary>
        /// Flag that is used to track redundant calls to dispose.
        /// </summary>
        private bool disposed;

        /// <summary>
        /// Trace information.
        /// </summary>
        private string traceType;

        /// <summary>
        ///  Initializes a new instance of the <see cref="StateManagerCopyStream"/> class. 
        /// </summary>
        internal StateManagerCopyStream(IEnumerable<SerializableMetadata> state, string traceType)
        {
            this.stateEnumerator = state.GetEnumerator();
            this.stateManagerFileCopyData =
                StateManagerFile.GetCopyData(this.stateEnumerator, traceType).GetEnumerator();
            this.traceType = traceType;
        }

        /// <summary>
        /// Clears state enumerator.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Get next operation data.
        /// </summary>
        public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            if (this.stateManagerFileCopyData.MoveNext())
            {
                return Task.FromResult(this.stateManagerFileCopyData.Current);
            }

            return Task.FromResult<OperationData>(null);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (this.stateEnumerator != null)
                    {
                        this.stateEnumerator.Dispose();
                    }

                    if (this.stateManagerFileCopyData != null)
                    {
                        this.stateManagerFileCopyData.Dispose();
                    }
                }
            }

            this.disposed = true;
        }
    }
}