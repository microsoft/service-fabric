// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Concurrent;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Threading;
    using System.Threading.Tasks;

    internal class InMemorySink
    {
        // Capacity of the BlockedCollection, producer will block when this limit is reached.
        private const int BoundedCapacity = 1024;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // In memory sink using bounding and blocking paradigm
        private BlockingCollection<DecodedEventWrapper> inMemorySink;

        // Processing action to invoke in the consumer
        private Action<DecodedEventWrapper, string> consumerProcessTraceEventAction;

        // manual reset event used to signal processing completed
        private ManualResetEvent consumerProcessingCompleted;

        // trace file sub folder to write bookmarks to
        private string traceFileSubFolder;

        internal InMemorySink(FabricEvents.ExtensionsEvents traceSource, string logSourceId, string traceFileSubFolder, Action<DecodedEventWrapper, string> consumerProcessTraceEventAction)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.inMemorySink = new BlockingCollection<DecodedEventWrapper>(BoundedCapacity);
            this.consumerProcessTraceEventAction = consumerProcessTraceEventAction;
            this.consumerProcessingCompleted = new ManualResetEvent(false);
            this.traceFileSubFolder = traceFileSubFolder;
            this.CreateAndRunTraceEventConsumerTask();
        }

        internal void WaitForConsumerProcessingToComplete()
        {
            if (this.inMemorySink.Count > 0)
            {
                this.consumerProcessingCompleted.WaitOne();
            }
        }

        internal void Add(DecodedEventWrapper decodedEtwEventWrapper)
        {
            this.inMemorySink.Add(decodedEtwEventWrapper);
        }

        internal void CompleteAdding()
        {
            this.inMemorySink.CompleteAdding();
        }

        /// <summary>
        /// Async consumer processing of trace events in the blocking
        /// collection.
        /// </summary>
        private void CreateAndRunTraceEventConsumerTask()
        {
            Task.Run(() =>
            {
                while (this.inMemorySink.Count > 0 || !this.inMemorySink.IsCompleted)
                {
                    try
                    {
                        DecodedEventWrapper decodedEtwEventWrapper = this.inMemorySink.Take();
                        this.consumerProcessTraceEventAction(decodedEtwEventWrapper, this.traceFileSubFolder);
                    }
                    catch (InvalidOperationException)
                    {
                        // Ignore exception when Take is invoked on a queue that is marked as completed
                    }
                    catch (Exception ex)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to process trace event. {0}",
                            ex);
                    }
                }

                this.consumerProcessingCompleted.Set();
            });
        }
    }
}