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
        private static readonly Dictionary<string, LttProducerWorkerInfo> AppProducerWorkersInfos = new Dictionary<string, LttProducerWorkerInfo>();
        private static LttProducerWorkerInfo ProducerWorkerInfo = null;
        private static object ProducerWorkerLock = new object();
        private static object AppProducerWorkersLock = new object();

        private readonly IEnumerable<object> consumerSinks;

        private readonly string AppInstanceId;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        bool disposed = false;

        public LttProducer(
            DiskSpaceManager diskSpaceManager,
            ITraceEventSourceFactory traceEventSourceFactory,
            ProducerInitializationParameters initParam)
        {
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.AppInstanceId = initParam.ApplicationInstanceId;
            this.traceSource = traceEventSourceFactory.CreateTraceEventSource(FabricEvents.Tasks.FabricDCA);
            this.serviceConfigSections = new List<string>();
            this.consumerSinks = initParam.ConsumerSinks;
            
            // Read the timer config value from dca section
            var configReader = new ConfigReader(initParam.ApplicationInstanceId);
            long lttReadIntervalMinutes = configReader.GetUnencryptedConfigValue(
                                                      initParam.SectionName,
                                                      LttProducerConstants.LttReadIntervalParamName,
                                                      LttProducerConstants.DefaultLttReadIntervalMinutes);

            if (initParam.ApplicationInstanceId == Utility.WindowsFabricApplicationInstanceId)
            {
                this.CreateWindowsFabricLttProducerWorkerInfo(initParam, lttReadIntervalMinutes);
            }
            else
            {
                this.CreateAppProducerWorkerInfo(initParam, lttReadIntervalMinutes);
            }
        }

        private void CreateWindowsFabricLttProducerWorkerInfo(
            ProducerInitializationParameters initParam,
            long lttReadIntervalMinutes)
        {
            lock (ProducerWorkerLock)
            {
                if (LttProducer.ProducerWorkerInfo == null)
                {
                    LttProducer.ProducerWorkerInfo = this.CreateWorkerInfo(initParam, lttReadIntervalMinutes);
                }
                else
                {
                    this.UpdateWorkerInfo(LttProducer.ProducerWorkerInfo, initParam, lttReadIntervalMinutes);
                }
            }
        }

        private void CreateAppProducerWorkerInfo(
            ProducerInitializationParameters initParam,
            long lttReadIntervalMinutes)
        {
            lock (AppProducerWorkersLock)
            {
                if (!LttProducer.AppProducerWorkersInfos.ContainsKey(initParam.ApplicationInstanceId))
                {
                    LttProducerWorkerInfo workerInfo = CreateWorkerInfo(initParam, lttReadIntervalMinutes);
                    LttProducer.AppProducerWorkersInfos.Add(initParam.ApplicationInstanceId, workerInfo);
                }
                else
                {
                    this.UpdateWorkerInfo(
                        LttProducer.AppProducerWorkersInfos[initParam.ApplicationInstanceId],
                        initParam,
                        lttReadIntervalMinutes);
                }
            }
        }

        private LttProducerWorkerInfo CreateWorkerInfo(ProducerInitializationParameters initParam, long lttReadIntervalMinutes)
        {
            // Create a new worker object
            this.traceSource.WriteInfo(
                this.logSourceId,
                $"Creating Ltt producer worker object for {initParam.ApplicationInstanceId}");

            LttProducerWorkerInfo workerInfo = new LttProducerWorkerInfo();

            this.AddLttProducerWorkerToWorkerInfo(workerInfo, initParam, lttReadIntervalMinutes);

            return workerInfo;
        }

        private void UpdateWorkerInfo(LttProducerWorkerInfo workerInfo, ProducerInitializationParameters initParam, long lttReadIntervalMinutes)
        {
            // Existing worker object is available. 
            this.traceSource.WriteInfo(
                this.logSourceId,
                $"Existing Ltt producer worker object. Restarting the worker object now for {initParam.ApplicationInstanceId}");

            // Save the old value for comparision
            long oldLttReadIntervalMinutes = workerInfo.ProducerWorker.LttReadIntervalMinutes;

            // Restart the worker object
            workerInfo.ProducerWorker.Dispose();
            workerInfo.ProducerWorker = null;

            // Keep the smaller value intact
            // as this worker handles both producers Ltt trace conversion and table events
            if (oldLttReadIntervalMinutes < lttReadIntervalMinutes)
            {
                lttReadIntervalMinutes = oldLttReadIntervalMinutes;
            }

            this.AddLttProducerWorkerToWorkerInfo(workerInfo, initParam, lttReadIntervalMinutes);
        }

        private void AddLttProducerWorkerToWorkerInfo(LttProducerWorkerInfo workerInfo, ProducerInitializationParameters initParam, long lttReadIntervalMinutes)
        {
            LttProducerWorker.LttProducerWorkerParameters parameters = CreateLttProducerWorkerParameters(workerInfo, initParam, lttReadIntervalMinutes);

            try
            {
                LttProducerWorker newWorker = new LttProducerWorker(parameters);
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

        private LttProducerWorker.LttProducerWorkerParameters CreateLttProducerWorkerParameters(LttProducerWorkerInfo workerInfo, ProducerInitializationParameters initParam, long lttReadIntervalMinutes)
        {
            List<LttProducer> lttProducers = new List<LttProducer>(workerInfo.LttProducers) {this};

            return new LttProducerWorker.LttProducerWorkerParameters()
            {
                TraceSource = this.traceSource,
                LogDirectory = initParam.LogDirectory,
                WorkDirectory = initParam.WorkDirectory,
                ProducerInstanceId = this.logSourceId,
                LttProducers = lttProducers,
                LatestSettings = initParam,
                LttReadIntervalMinutes = lttReadIntervalMinutes
            };
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
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (disposed || !disposing)
            {
                return;
            }

            // Disposes in case this LttProducer instance is for the main Service Fabric
            // Note that, currently, to look like there are more than one producer of traces
            // as we have in Windows, this producer can have more than one instance, but the worker
            // is only one (static member) and in fact only one producer exists.
            // This makes the Dispose logic complicated, especially now that we have also collection of
            // logs from containers. Ideally, this needs to be thought through and fixed in the future.
            if (this.AppInstanceId == Utility.WindowsFabricApplicationInstanceId)
            {
                // lock to avoid race condition when a new Producer is being created.
                lock(ProducerWorkerLock)
                {
                    LttProducer.ProducerWorkerInfo.LttProducers.Remove(this);

                    // Since we can have more than one instance of this producer,
                    // only the last one disposed should also dispose the worker object which
                    // is shared (static) between the two.
                    if (LttProducer.ProducerWorkerInfo.LttProducers.Count == 0)
                    {
                        LttProducer.ProducerWorkerInfo.ProducerWorker.Dispose();
                    }
                    disposed = true;
                    return;
                }
            }

            // Disposes in case this LttProducer instance is for one of the Apps or Containers
            // It follows the same idea of the dispose logic for main Service Fabric producer above
            // but with the complication that we may be collecting traces for more than one container.
            // Therefore we have a collection of workers, each one shared by one or more lttproducers of a specific container.
            lock(AppProducerWorkersLock)
            {
                if (!LttProducer.AppProducerWorkersInfos.ContainsKey(this.AppInstanceId))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        $"WorkerInfo expected but not found when trying to dispose LttProducer for Container {this.AppInstanceId}");
 
                    disposed = true;

                    return;
                }

                LttProducer.AppProducerWorkersInfos[this.AppInstanceId].LttProducers.Remove(this);

                // only last LttProducer instance for this specific containeer should dispose shared worker object. 
                if (LttProducer.AppProducerWorkersInfos[this.AppInstanceId].LttProducers.Count == 0)
                {
                    LttProducer.AppProducerWorkersInfos[this.AppInstanceId].ProducerWorker.Dispose();
                    LttProducer.AppProducerWorkersInfos.Remove(this.AppInstanceId);
                }

                disposed = true;
            }
        }

        private class LttProducerWorkerInfo
        {
            internal LttProducerWorkerInfo()
            {
                this.LttProducers = new List<LttProducer>();
            }

            internal LttProducerWorker ProducerWorker { get; set; }

            internal List<LttProducer> LttProducers { get; set; }
        }
    }
}
