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
    using System.Linq;

    // This class implements a wrapper around the ETL producer worker object
    internal class LttProducer : IDcaProducer
    {
        // List of worker objects currently available
        private static readonly List<LttProducerWorkerInfo> ProducerWorkers = new List<LttProducerWorkerInfo>();

        private readonly IEnumerable<object> consumerSinks;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        public LttProducer(
            DiskSpaceManager diskSpaceManager,
            ITraceEventSourceFactory traceEventSourceFactory,
            ProducerInitializationParameters initParam)
        {
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.traceSource = traceEventSourceFactory.CreateTraceEventSource(FabricEvents.Tasks.FabricDCA);
            this.serviceConfigSections = new List<string>();
            this.consumerSinks = initParam.ConsumerSinks;
            
            // Read the timer config value from dca section
            var configReader = new ConfigReader(initParam.ApplicationInstanceId);
            long newLttReadIntervalMinutes = configReader.GetUnencryptedConfigValue(
                                                      initParam.SectionName,
                                                      LttProducerConstants.LttReadIntervalParamName,
                                                      LttProducerConstants.DefaultLttReadIntervalMinutes);

            lock (ProducerWorkers)
            {
                LttProducerWorkerInfo workerInfo = ProducerWorkers.FirstOrDefault();
                if (null != workerInfo)
                {
                    // Existing worker object is available. 
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Existing Ltt producer worker object. Restarting the worker object now.");

                    // Save the old value for comparision
                    long oldLttReadIntervalMinutes = workerInfo.ProducerWorker.LttReadIntervalMinutes;

                    // Restart the worker object
                    workerInfo.ProducerWorker.Dispose();
                    workerInfo.ProducerWorker = null;

                    // Keep the smaller value intact
                    // as this worker handles both producers Ltt trace conversion and table events
                    if (oldLttReadIntervalMinutes < newLttReadIntervalMinutes)
                    {
                        newLttReadIntervalMinutes = oldLttReadIntervalMinutes;
                    }

                    List<LttProducer> LttProducers = new List<LttProducer>(workerInfo.LttProducers) {this};
                    LttProducerWorker.LttProducerWorkerParameters newWorkerParam = new LttProducerWorker.LttProducerWorkerParameters()
                    {
                        TraceSource = this.traceSource,
                        LogDirectory = initParam.LogDirectory,
                        ProducerInstanceId = this.logSourceId,
                        LttProducers = LttProducers,
                        LatestSettings = initParam,
                        LttReadIntervalMinutes = newLttReadIntervalMinutes
                    };
                    try
                    {
                        LttProducerWorker newWorker = new LttProducerWorker(newWorkerParam);
                        workerInfo.LttProducers.Add(this);
                        workerInfo.ProducerWorker = newWorker;
                    }
                    catch (InvalidOperationException)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to restart Ltt producer worker object.");
                    }
                }
                else
                {
                    // Create a new worker object
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Creating Ltt producer worker object ...");

                    List<LttProducer> LttProducers = new List<LttProducer> {this};
                    LttProducerWorker.LttProducerWorkerParameters newWorkerParam = new LttProducerWorker.LttProducerWorkerParameters()
                    {
                        TraceSource = this.traceSource,
                        LogDirectory = initParam.LogDirectory,
                        ProducerInstanceId = this.logSourceId,
                        LttProducers = LttProducers,
                        LatestSettings = initParam,
                        LttReadIntervalMinutes = newLttReadIntervalMinutes
                    };
                    try
                    {
                        LttProducerWorker newWorker = new LttProducerWorker(newWorkerParam);
                        workerInfo = new LttProducerWorkerInfo()
                        {
                            LttProducers = new List<LttProducer>(),
                            ProducerWorker = newWorker
                        };
                        workerInfo.LttProducers.Add(this);
                        ProducerWorkers.Add(workerInfo);
                    }
                    catch (InvalidOperationException)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to create Ltt producer worker object.");
                    }
                }
            }
        }

        internal IEnumerable<object> ConsumerSinks
        {
            get
            {
                return this.consumerSinks;
            }
        }

        // Whether or not we should finish processing all pending data 
        // during dispose
        public bool FlushDataOnDispose
        {
            set
            {
            }
        }

        // Additional sections in the cluster manifest or application manifest from which
        // we read configuration information.
        public IEnumerable<string> AdditionalAppConfigSections
        {
            get
            {
                return null;
            }
        }

        // Sections in the service manifest from which we read configuration information.
        private List<string> serviceConfigSections;
        public IEnumerable<string> ServiceConfigSections
        {
            get
            {
                return serviceConfigSections;
            }
        }

        public void FlushData()
        {
        }
        
        public void Dispose()
        {
        }

        private class LttProducerWorkerInfo
        {
            internal LttProducerWorker ProducerWorker { get; set; }

            internal List<LttProducer> LttProducers { get; set; }
        }
    }
}
