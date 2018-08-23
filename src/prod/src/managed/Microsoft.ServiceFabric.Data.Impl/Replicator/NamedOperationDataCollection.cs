// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Collection of named operation data
    /// </summary>
    internal class NamedOperationDataCollection : IOperationDataStream, IDisposable
    {
        /// <summary>
        /// Flag that is used to detect redundant calls to dispose.
        /// </summary>
        private bool disposed;

        /// <summary>
        /// Current index of the operation collection.
        /// </summary>
        private int currentIndex = 0;

        /// <summary>
        /// Collection of operation data.
        /// </summary>
        private readonly List<Tuple<StateProviderIdentifier, IOperationDataStream>> operationDataStreamCollection =
            new List<Tuple<StateProviderIdentifier, IOperationDataStream>>();

        /// <summary>
        /// Trace information.
        /// </summary>
        private string traceType;

        /// <summary>
        /// Current wire format version.
        /// </summary>
        private int version;

        /// <summary>
        ///  Initializes a new instance of the <see cref="NamedOperationDataCollection"/> class.
        /// </summary>
        public NamedOperationDataCollection(
            string traceType,
            int version)
        {
            this.traceType = traceType;
            this.version = version;
        }

        /// <summary>
        /// Adds operation data stream to the collection.
        /// </summary>
        public void Add(StateProviderIdentifier stateProviderIdentifier, IOperationDataStream operationDataStream)
        {
            Utility.Assert(
                stateProviderIdentifier != null,
                "{0}: Named operation data cannot be empty in named operation data collection.",
                this.traceType);
            Utility.Assert(
                operationDataStream != null, 
                "{0}: Operation data stream cannot be null.",
                this.traceType);

            this.operationDataStreamCollection.Add(
                new Tuple<StateProviderIdentifier, IOperationDataStream>(stateProviderIdentifier, operationDataStream));
        }

        /// <summary>
        /// Disposes operation data stream.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Gets operation data from state providers.
        /// </summary>
        public async Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            try
            {
                while (this.currentIndex < this.operationDataStreamCollection.Count)
                {
                    StateProviderIdentifier stateProviderIdentifier = this.operationDataStreamCollection[this.currentIndex].Item1;
                    IOperationDataStream operationDataStream = this.operationDataStreamCollection[this.currentIndex].Item2;

                    OperationData operationData = await operationDataStream.GetNextAsync(cancellationToken).ConfigureAwait(false);

                    if (operationData == null)
                    {
                        ++this.currentIndex;
                        continue;
                    }

                    // Reserve the List space with expected size: Version, ID, OperationData
                    List<ArraySegment<byte>> namedBytes = new List<ArraySegment<byte>>(2 + operationData.Count);
                    byte[] versionAsBytes = BitConverter.GetBytes(this.version);
                    namedBytes.Add(new ArraySegment<byte>(versionAsBytes));

                    long stateProviderId = stateProviderIdentifier.StateProviderId;
                    byte[] idAsBytes = BitConverter.GetBytes(stateProviderId);
                    namedBytes.Add(new ArraySegment<byte>(idAsBytes));

                    foreach (ArraySegment<byte> roll in operationData)
                    {
                        namedBytes.Add(roll);
                    }

                    return new OperationData(namedBytes);
                }

                FabricEvents.Events.NamedOperationDataCollectionGetNext(this.traceType);

                return null;
            }
            catch (Exception e)
            {
                string msg = string.Format(
                    CultureInfo.InvariantCulture,
                    "OperationDataStreamCollection Count: {0}, CurrentIndex: {1}.",
                    this.operationDataStreamCollection.Count,
                    this.currentIndex);
                StateManagerStructuredEvents.TraceException(
                    this.traceType,
                    "NamedOperationDataCollection.GetNextAsync.",
                    msg,
                    e);
                throw;
            }
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (this.operationDataStreamCollection != null)
                    {
                        foreach (Tuple<StateProviderIdentifier, IOperationDataStream> namedOperationDataStream in this.operationDataStreamCollection)
                        {
                            if (namedOperationDataStream != null)
                            {
                                if (namedOperationDataStream.Item2 != null)
                                {
                                    IDisposable disposableCopyStateStream =
                                        namedOperationDataStream.Item2 as IDisposable;
                                    if (disposableCopyStateStream != null)
                                    {
                                        disposableCopyStateStream.Dispose();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            this.disposed = true;
        }
    }
}