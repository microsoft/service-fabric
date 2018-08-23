// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;

    using Tools.EtlReader;

    // This class implements a wrapper around the ETL producer worker object
    internal class EtlInMemoryProducer : IDcaProducer
    {
        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Creates an instance of the trace file event reader
        private readonly ITraceFileEventReaderFactory traceFileEventReaderFactory;

        // Worker object for reading ETW events from ETL files
        private readonly EtlInMemoryProducerWorker producerWorker;

        // Producer settings
        private readonly EtlInMemoryProducerSettings etlInMemoryProducerSettings;

        // Consumer sinks
        private readonly IEnumerable<object> consumerSinks;

        private readonly DiskSpaceManager diskSpaceManager;

        // Whether or not the object has been disposed
        private bool disposed;

        internal EtlInMemoryProducer(
            DiskSpaceManager diskSpaceManager,
            IEtlInMemoryProducerConfigReaderFactory configReaderFactory, 
            ITraceFileEventReaderFactory traceFileEventReaderFactory,
            ITraceEventSourceFactory traceEventSourceFactory,
            ProducerInitializationParameters initParam)
        {
            this.diskSpaceManager = diskSpaceManager;
            this.traceFileEventReaderFactory = traceFileEventReaderFactory;

            // Initialization
            this.traceSource = traceEventSourceFactory.CreateTraceEventSource(FabricEvents.Tasks.FabricDCA);
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.consumerSinks = initParam.ConsumerSinks;

            // Read settings
            var configReader = configReaderFactory.CreateEtlInMemoryProducerConfigReader(this.traceSource, this.logSourceId);
            this.etlInMemoryProducerSettings = configReader.GetSettings();

            // ETL in-memory file processing is not enabled or we are not processing
            // winfab etl files, so return immediately
            if (false == this.etlInMemoryProducerSettings.Enabled || false == this.etlInMemoryProducerSettings.ProcessingWinFabEtlFiles)
            {
                return;
            }

            // Create a new worker object
            var newWorkerParam = new EtlInMemoryProducerWorker.EtlInMemoryProducerWorkerParameters()
            {
                TraceSource = this.traceSource,
                LogDirectory = initParam.LogDirectory,
                ProducerInstanceId = this.logSourceId,
                EtlInMemoryProducer = this,
                LatestSettings = this.etlInMemoryProducerSettings
            };

            var newWorker = new EtlInMemoryProducerWorker(
                newWorkerParam,
                this.diskSpaceManager,
                this.traceFileEventReaderFactory);
            this.producerWorker = newWorker;
        }

        public IEnumerable<string> AdditionalAppConfigSections
        {
            get
            {
                return new string[0];
            }
        }

        public IEnumerable<string> ServiceConfigSections
        {
            get
            {
                return new string[0];
            }
        }

        internal IEnumerable<object> ConsumerSinks
        {
            get
            {
                return this.consumerSinks;
            }
        }

        internal EtlInMemoryProducerSettings Settings
        {
            get
            {
                return this.etlInMemoryProducerSettings;
            }
        }

        public void FlushData()
        {
            var worker = this.producerWorker;
            if (null != worker)
            {
                worker.FlushData();
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (this.etlInMemoryProducerSettings.ProcessingWinFabEtlFiles)
            {
                if (null != this.producerWorker)
                {
                    this.producerWorker.Dispose();
                }
            }

            GC.SuppressFinalize(this);
        }
    }
}