// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Monitoring
{
    /// <summary>
    /// Encapsulates parameters for an instance of the FabricApiMonitoringComponent, including default health callbacks
    /// </summary>
    internal class MonitoringComponentParameters
    {
        private Action<FabricApiCallDescription> slowHealthReportCallback;
        private Action<FabricApiCallDescription> clearSlowHealthReportCallback;
        private TimeSpan scanInterval;

        public MonitoringComponentParameters(
            TimeSpan interval,
            Action<FabricApiCallDescription> slowHealthReport = null,
            Action<FabricApiCallDescription> clearSlowHealthReport = null)
        {
            this.slowHealthReportCallback = slowHealthReport ?? this.DefaultSlowHealthCallback;
            this.clearSlowHealthReportCallback = clearSlowHealthReport ?? this.DefaultClearSlowHealthReportCallback;
            this.scanInterval = interval;
        }
        
        public Action<FabricApiCallDescription> ClearSlowHealthReportCallback
        {
            get { return this.clearSlowHealthReportCallback; }
            set { this.clearSlowHealthReportCallback = value; }
        }

        public Action<FabricApiCallDescription> SlowHealthReportCallback
        {
            get { return this.slowHealthReportCallback; }
            set { this.slowHealthReportCallback = value; }
        }

        public TimeSpan ScanInterval
        {
            get { return this.scanInterval; }
            set { this.scanInterval = value; }
        }

        /// <summary>
        /// Default slow health callback, invokes ReportReplicaHealth on the partition object from apiCall 
        /// using HealthInformation object from FabricApiCallDescription.GetHealthInformation_Slow.
        /// </summary>
        /// <param name="apiCall"></param>
        private void DefaultSlowHealthCallback(FabricApiCallDescription apiCall)
        {
            var healthInfo = apiCall.GetHealthInformation_Slow();
            apiCall.Partition.ReportReplicaHealth(healthInfo);
        }

        /// <summary>
        /// Default clear slow health callback, invokes ReportReplicaHealth on the partition object from apiCall 
        /// using HealthInformation object FabricApiCallDescription.GetHealthInformation_OK.
        /// </summary>
        /// <param name="apiCall"></param>
        private void DefaultClearSlowHealthReportCallback(FabricApiCallDescription apiCall)
        {
            var clearHealthInfo = apiCall.GetHealthInformation_OK();
            apiCall.Partition.ReportReplicaHealth(clearHealthInfo);
        }
    }
}