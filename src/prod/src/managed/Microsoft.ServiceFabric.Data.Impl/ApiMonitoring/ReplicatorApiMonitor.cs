// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Fabric.Common.Monitoring;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// ReplicatorApiMonitor is used to monitor API calls within the V2 Replicator. FabricApiMonitoringComponent is the class that handles tracing and health reporting for the replicator
    /// </summary>
    internal class ReplicatorApiMonitor
    {
        /// <summary>
        /// The partition in which the ReplicatorApiMonitor will monitor API calls. The partition object is used to report replica health.
        /// </summary>
        private IStatefulServicePartition partition;

        /// <summary>
        /// Context in which the ReplicatorApiMonitor is instantiated - contains information used to populate FabricApiMonitoringData objects
        /// </summary>
        private ServiceContext context;

        /// <summary>
        /// Settings for this instance of the replicator
        /// </summary>
        private TransactionalReplicatorSettings replicatorSettings;

        /// <summary>
        /// Represents the type of trace to be used by the ReplicatorApiMonitor
        /// </summary>
        private string traceType;

        /// <summary>
        /// The component that handles the monitoring logic
        /// </summary>
        private FabricApiMonitoringComponent fabricMonitor;

        /// <summary>
        /// Boolean value to determine if the monitor has been initialized with the appropriate fields
        /// </summary>
        private bool initialized;

        /// <summary>
        /// Gets the replicatorSettings object
        /// </summary>
        public TransactionalReplicatorSettings Settings
        {
            get { return this.replicatorSettings; }
            set { this.replicatorSettings = value; }
        }

        /// <summary>
        /// Initializes an instance of the ReplicatorApiMonitor class
        /// ReplicatorApiMonitor takes ServiceContext, StatefulServicePartition and Replicator setttings as arguments. 
        /// The default value of SlowApiMonitoringDuration in the replicator settings object is 120 seconds
        /// </summary>
        /// <param name="serviceContext">Current service serviceContext</param>
        /// <param name="statefulPartition">Current statefulPartition</param>
        /// <param name="settings">Current replicator settings</param>
        public void Initialize(ServiceContext serviceContext, IStatefulServicePartition statefulPartition, TransactionalReplicatorSettings settings)
        {
            this.replicatorSettings = settings;
            this.context = serviceContext;
            this.partition = statefulPartition;
            this.traceType = string.Format("{0}.{1}.{2}", SR.ReplicatorApiMonitor_TraceTypeBase, this.context.PartitionId, this.context.ReplicaOrInstanceId);

            var monitorParameters = new MonitoringComponentParameters(this.replicatorSettings.PublicSettings.SlowApiMonitoringDuration.Value);
            this.fabricMonitor = new FabricApiMonitoringComponent(monitorParameters);
            this.initialized = true;
        }

        /// <summary>
        /// Invoked by BackupManager to monitor backup callbacks 
        /// </summary>
        /// <returns> FabricApiCallDescription being monitored, or null if settings are invalid </returns>
        public FabricApiCallDescription StartBackupCallbackAsyncMonitoring()
        {
            if (!this.ValidateSettings())
            {
                return null;
            }

            var fabricMonitorParams = new FabricMonitoringParameters(true, true, true, this.Settings.PublicSettings.SlowApiMonitoringDuration.Value);

            var fabricMonitorData = new FabricMonitoringData(
                this.context,
                SR.ReplicatorApiMonitor_BackupApiName,
                SR.HealthProperty_BackupCallbackSlow);

            var backupCallbackStart = this.GetApiCallDescription(fabricMonitorData, fabricMonitorParams);
                
            return this.StartMonitoringApi(backupCallbackStart);
        }

        /// <summary>
        /// Stop monitoring a backup call
        /// </summary>
        /// <param name="apiCall">The API call to be monitored</param>
        public void StopBackupMonitoring(FabricApiCallDescription apiCall)
        {
            if (apiCall != null)
            {
                this.StopMonitoringApi(apiCall);
            }
        }

        /// <summary>
        /// Gets ApiCallDescription from FabricMonitoringData and FabricMonitoringParameters
        /// </summary>
        /// <param name="data"> FabricApiMonitoringData </param>
        /// <param name="parameters"> FabricApiMonitoringParameters </param>
        /// <returns> New FabricApiCall description object </returns>
        private FabricApiCallDescription GetApiCallDescription(FabricMonitoringData data, FabricMonitoringParameters parameters)
        {
            return new FabricApiCallDescription(
                data,
                parameters,
                this.partition);
        }

        /// <summary>
        /// Argument check to ensure no invalid API is monitored. TimeSpan.Zero indicates no monitoring should be instantiated
        /// </summary>
        /// <returns> True if settings are valid for monitoring (non-null and above TimeSpan.Zero), false otherwise </returns>
        private bool ValidateSettings()
        {
            Utility.Assert(this.initialized, "ReplicatorApiMonitor must be initialized before use");

            if (this.replicatorSettings == null)
            {
                FabricEvents.Events.Warning(this.traceType, SR.ReplicatorApiMonitor_NullSettings);
                return false;
            }

            if (this.replicatorSettings.PublicSettings.SlowApiMonitoringDuration == null || this.replicatorSettings.PublicSettings.SlowApiMonitoringDuration == TimeSpan.Zero)
            {
                FabricEvents.Events.Warning(this.traceType, SR.ReplicatorApiMonitor_InvalidMonitoringInterval);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Stop Monitoring 
        /// </summary>
        /// <param name="endMonitoringApiCall">The API call to stop monitoring</param>
        private void StopMonitoringApi(FabricApiCallDescription endMonitoringApiCall)
        {
            Utility.Assert(endMonitoringApiCall != null, "StopMonitoringApi requires a non-null argument");

            this.fabricMonitor.StopMonitoring(endMonitoringApiCall);
        }
        
        /// <summary>
        /// Start Monitoring an API call. Returns the provided API call description (or null) which should be used as an argument to StopMonitoringApi
        /// </summary>
        /// <param name="startMonitoringApiCall"> API call to be monitored </param>
        /// <returns>The api call passed to the FabricMonitoringComponent</returns>
        private FabricApiCallDescription StartMonitoringApi(FabricApiCallDescription startMonitoringApiCall)
        {
            Utility.Assert(this.replicatorSettings.PublicSettings.SlowApiMonitoringDuration != TimeSpan.Zero, "StartMonitoringApi requires TimeSpan above zero");

            this.fabricMonitor.StartMonitoring(startMonitoringApiCall);

            return startMonitoringApiCall;
        }
    }
}