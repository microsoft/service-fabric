// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using Microsoft.ServiceFabric.Data;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading;

    /// <summary>
    /// Represents a pool of reusable streams.
    /// </summary>
    internal class StreamPool : IDisposable
    {
        private const int DefaultMaxStreams = 16;

        private List<Stream> streams = new List<Stream>();
        private Func<Stream> streamFactory;
        private int maxStreams;
        private bool dropNextStream = false;
        private bool isDisposed = false;

        public StreamPool(Func<Stream> streamFactory, int maxStreams = DefaultMaxStreams)
        {
            if (streamFactory == null)
            {
                throw new ArgumentNullException(SR.StreamFactory);
            }

            this.streamFactory = streamFactory;
            this.maxStreams = maxStreams;
        }

        /// <summary>
        /// Acquire a re-usable stream for exclusive use.  This will either return an existing
        /// stream from the pool, or create a new one if none exist.
        /// </summary>
        /// <returns>A stream that can be exclusively used by a single thread.</returns>
        public Stream AcquireStream()
        {
            // Try to get an existing stream from the stream pool.
            lock (this.streams)
            {
                if (this.streams.Count > 0)
                {
                    var lastIndex = this.streams.Count - 1;
                    var stream = this.streams[lastIndex];
                    this.streams.RemoveAt(lastIndex);
                    return stream;
                }
            }

            // Create a new stream.
            return this.streamFactory.Invoke();
        }

        /// <summary>
        /// Return a re-usable stream to the pool that was previously acquired from the same pool.
        /// 
        /// If the number of streams in the pool is below the desired max streams, it will be retained.
        /// If the number of streams exceeds the desired maximum, the stream may or may not be disposed and dropped.
        /// 
        /// Because there may be large bursts of concurrent requests, the number of streams in the pool may
        /// temporarily be above the desired maximum.
        /// </summary>
        /// <param name="stream">A re-usable Stream that was previously acquired from the StreamPool.</param>
        /// <param name="forceDispose">Specifies if the stream should be disposed immediately.</param>
        public void ReleaseStream(Stream stream, bool forceDispose = false)
        {
            if (stream != null)
            {
                lock (this.streams)
                {
                    if (this.isDisposed || forceDispose)
                    {
                        stream.Dispose();
                    }
                    else
                    {
                        // Prefer to keep streams only up until the max count.
                        if (this.streams.Count < this.maxStreams)
                        {
                            this.streams.Add(stream);
                            this.dropNextStream = (this.streams.Count >= this.maxStreams);
                        }
                        else
                        {
                            // There are too many streams - drop approximately every other stream.
                            // We only drop every other stream because, if there was a burst of requests, we should
                            // attempt to keep those already created streams around until the load decreases.
                            if (!this.dropNextStream)
                            {
                                this.streams.Add(stream);
                                this.dropNextStream = true;
                            }
                            else
                            {
                                // Drop and dispose the stream.
                                stream.Dispose();
                                this.dropNextStream = false;
                            }
                        }
                    }
                }
            }
        }

        public void Dispose()
        {
            lock (this.streams)
            {
                // It is safe to dispose here.
                foreach (var stream in this.streams)
                {
                    stream.Dispose();
                }

                this.streams.Clear();
                this.isDisposed = true;
            }
        }

        internal int Count
        {
            get
            {
                lock (this.streams)
                {
                    return this.streams.Count;
                }
            }
        }
    }
}