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

    using Tools.EtlReader;

    // This class implements a wrapper around the ETL producer worker object
    internal class EtlProducer : IDcaProducer
    {
        // List of worker objects currently available
        private static readonly List<EtlProducerWorkerInfo> ProducerWorkers = new List<EtlProducerWorkerInfo>();

        private readonly DiskSpaceManager diskSpaceManager;
        private readonly string logDirectory;
        private readonly IEnumerable<object> consumerSinks;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Sections in the service manifest from which we read configuration information.
        private readonly List<string> serviceConfigSections;

        // Worker object for reading ETW events from ETL files
        private readonly EtlProducerWorker producerWorker;

        // Settings
        private readonly EtlProducerSettings etlProducerSettings;

        private readonly ITraceFileEventReaderFactory traceFileEventReaderFactory;

        // Whether or not the object has been disposed
        private bool disposed;

        internal EtlProducer(
            DiskSpaceManager diskSpaceManager, 
            IEtlProducerConfigReaderFactory configReaderFactory, 
            ITraceFileEventReaderFactory traceFileEventReaderFactory,
            ITraceEventSourceFactory traceEventSourceFactory,
            ProducerInitializationParameters initParam)
        {
            this.diskSpaceManager = diskSpaceManager;
            this.traceFileEventReaderFactory = traceFileEventReaderFactory;
            
            // Initialization
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.traceSource = traceEventSourceFactory.CreateTraceEventSource(FabricEvents.Tasks.FabricDCA);
            this.serviceConfigSections = new List<string>();
            this.logDirectory = initParam.LogDirectory;
            this.consumerSinks = initParam.ConsumerSinks;

            // Read instance-specific settings from the settings file
            var configReader = configReaderFactory.CreateEtlProducerConfigReader(this.traceSource, this.logSourceId);
            this.etlProducerSettings = configReader.GetSettings();
            if (false == this.etlProducerSettings.Enabled)
            {
                // ETL file processing is not enabled, so return immediately
                return;
            }

            if (!this.etlProducerSettings.ProcessingWinFabEtlFiles)
            {
                // If we are collecting ETW events on behalf of an app, then we will
                // need information from the <ETW> section of the service manifest.
                this.serviceConfigSections.Add(ServiceConfig.EtwElement);

                // Check if we can use an existing worker object
                string applicationType = this.etlProducerSettings.ApplicationType;
                lock (ProducerWorkers)
                {
                    EtlProducerWorkerInfo workerInfo = ProducerWorkers.FirstOrDefault(w => w.ApplicationType.Equals(
                                                                                      applicationType,
                                                                                      StringComparison.Ordinal));
                    if (null != workerInfo)
                    {
                        // Existing worker object is available. 
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Existing ETL producer worker object for application type {0} is available. Restarting the worker object now.",
                            applicationType);

                        // Restart the worker object
                        workerInfo.ProducerWorker.Dispose();
                        workerInfo.ProducerWorker = null;

                        List<EtlProducer> etlProducers = new List<EtlProducer>(workerInfo.EtlProducers) { this };
                        EtlProducerWorker.EtlProducerWorkerParameters newWorkerParam = new EtlProducerWorker.EtlProducerWorkerParameters()
                            {
                                TraceSource = this.traceSource,
                                IsReadingFromApplicationManifest = !this.etlProducerSettings.ProcessingWinFabEtlFiles,
                                ApplicationType = applicationType,
                                LogDirectory = initParam.LogDirectory,
                                ProducerInstanceId = applicationType,
                                EtlProducers = etlProducers,
                                LatestSettings = this.etlProducerSettings
                            };
                        try
                        {
                            EtlProducerWorker newWorker = new EtlProducerWorker(
                                newWorkerParam, 
                                this.diskSpaceManager, 
                                this.traceFileEventReaderFactory);
                            workerInfo.EtlProducers.Add(this);
                            workerInfo.ProducerWorker = newWorker;
                        }
                        catch (InvalidOperationException)
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Failed to restart ETL producer worker object for application type {0}.",
                                applicationType);
                        }
                    }
                    else
                    {
                        // Create a new worker object
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Creating ETL producer worker object for application type {0} ...",
                            applicationType);

                        List<EtlProducer> etlProducers = new List<EtlProducer> { this };
                        EtlProducerWorker.EtlProducerWorkerParameters newWorkerParam = new EtlProducerWorker.EtlProducerWorkerParameters()
                        {
                            TraceSource = this.traceSource,
                            IsReadingFromApplicationManifest = !this.etlProducerSettings.ProcessingWinFabEtlFiles,
                            ApplicationType = applicationType,
                            LogDirectory = initParam.LogDirectory,
                            ProducerInstanceId = applicationType,
                            EtlProducers = etlProducers,
                            LatestSettings = this.etlProducerSettings
                        };
                        try
                        {
                            EtlProducerWorker newWorker = new EtlProducerWorker(
                                newWorkerParam, 
                                this.diskSpaceManager,
                                this.traceFileEventReaderFactory);
                            workerInfo = new EtlProducerWorkerInfo()
                            {
                                ApplicationType = applicationType,
                                EtlProducers = new List<EtlProducer>(),
                                ProducerWorker = newWorker
                            };
                            workerInfo.EtlProducers.Add(this);

                            ProducerWorkers.Add(workerInfo);
                        }
                        catch (InvalidOperationException)
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Failed to create ETL producer worker object for application type {0}.",
                                applicationType);
                        }
                    }
                }
            }
            else
            {
                // Create a new  worker object
                List<EtlProducer> etlProducers = new List<EtlProducer> { this };
                EtlProducerWorker.EtlProducerWorkerParameters newWorkerParam = new EtlProducerWorker.EtlProducerWorkerParameters()
                {
                    TraceSource = this.traceSource,
                    IsReadingFromApplicationManifest = !this.etlProducerSettings.ProcessingWinFabEtlFiles,
                    ApplicationType = string.Empty,
                    LogDirectory = initParam.LogDirectory,
                    ProducerInstanceId = this.logSourceId,
                    EtlProducers = etlProducers,
                    LatestSettings = this.etlProducerSettings
                };
                try
                {
                    EtlProducerWorker newWorker = new EtlProducerWorker(
                        newWorkerParam, 
                        this.diskSpaceManager,
                        this.traceFileEventReaderFactory);
                    this.producerWorker = newWorker;
                }
                catch (InvalidOperationException)
                {
                }
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

        public IEnumerable<string> ServiceConfigSections
        {
            get
            {
                return this.serviceConfigSections;
            }
        }

        internal static List<string> MarkerFileDirectoriesForApps
        {
            get
            {
                return EtlProducerWorker.MarkerFileDirectoriesForApps;
            }
        }

        internal IEnumerable<object> ConsumerSinks 
        {
            get
            {
                return this.consumerSinks;
            }
        }

        internal EtlProducerSettings Settings
        {
            get
            {
                return this.etlProducerSettings;
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

            if (!this.etlProducerSettings.ProcessingWinFabEtlFiles)
            {
                string applicationType = this.etlProducerSettings.ApplicationType;
                lock (ProducerWorkers)
                {
                    EtlProducerWorkerInfo workerInfo = ProducerWorkers.FirstOrDefault(w => w.ApplicationType.Equals(
                                                                                                applicationType,
                                                                                                StringComparison.Ordinal));
                    if (null != workerInfo)
                    {
                        // Tell the worker object to stop
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Stopping ETL producer worker object for application type {0} ...",
                            applicationType);

                        workerInfo.ProducerWorker.Dispose();
                        workerInfo.ProducerWorker = null;

                        workerInfo.EtlProducers.Remove(this);
                        if (0 == workerInfo.EtlProducers.Count)
                        {
                            // No one else is using the worker object. No need to 
                            // restart it.
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "No other ETL producer is using the ETL producer worker object for application type {0}, so no need to restart it.",
                                applicationType);
                            ProducerWorkers.Remove(workerInfo);
                        }
                        else
                        {
                            // Restart the worker. Arbitrarily choose one of the remaining
                            // producers for supplying settings.
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Restarting the ETL producer worker object for application type {0} because other ETL producers are using it.",
                                applicationType);
                            EtlProducer producer = workerInfo.EtlProducers[0];
                            EtlProducerWorker.EtlProducerWorkerParameters newWorkerParam = new EtlProducerWorker.EtlProducerWorkerParameters()
                            {
                                TraceSource = producer.traceSource,
                                IsReadingFromApplicationManifest = !producer.etlProducerSettings.ProcessingWinFabEtlFiles,
                                ApplicationType = applicationType,
                                LogDirectory = producer.logDirectory,
                                ProducerInstanceId = applicationType,
                                EtlProducers = workerInfo.EtlProducers,
                                LatestSettings = producer.etlProducerSettings
                            };
                            try
                            {
                                EtlProducerWorker newWorker = new EtlProducerWorker(
                                    newWorkerParam,
                                    this.diskSpaceManager,
                                    this.traceFileEventReaderFactory);
                                workerInfo.ProducerWorker = newWorker;
                            }
                            catch (InvalidOperationException)
                            {
                                this.traceSource.WriteError(
                                    this.logSourceId,
                                    "Failed to restart the ETL producer worker object for application type {0}.",
                                    applicationType);
                            }
                        }
                    }
                }
            }
            else
            {
                if (null != this.producerWorker)
                {
                    this.producerWorker.Dispose();
                }
            }

            GC.SuppressFinalize(this);
        }

        internal static bool IsInstanceForDefaultWinFabEtlFiles(ConfigReader configurationReader, string customSectionName)
        {
            return string.IsNullOrEmpty(
                configurationReader.GetUnencryptedConfigValue(
                    customSectionName,
                    EtlProducerValidator.TestEtlPathParamName,
                    string.Empty));
        }

        private class EtlProducerWorkerInfo
        {
            internal string ApplicationType { get; set; }

            internal EtlProducerWorker ProducerWorker { get; set; }

            internal List<EtlProducer> EtlProducers { get; set; }
        }
    }
}