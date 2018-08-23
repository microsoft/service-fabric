// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Monitoring
{
    using System.Fabric.Health;

    /// <summary>
    /// Used to monitor Api Calls by managed fabric components
    /// </summary>
    internal class FabricApiCallDescription
    {
        private FabricMonitoringData monitoringData;
        private FabricMonitoringParameters monitoringParameters;
        private bool hasExpired;
        private IStatefulServicePartition partition;

        /// <summary>
        /// Initialize a FabricApiCallDescription object 
        /// </summary>
        /// <param name="data"></param>
        /// <param name="parameters"></param>
        /// <param name="statefulServicePartition"></param>
        public FabricApiCallDescription(
            FabricMonitoringData data,
            FabricMonitoringParameters parameters,
            IStatefulServicePartition statefulServicePartition)
        {
            this.monitoringData = data;
            this.monitoringParameters = parameters;
            this.hasExpired = false;
            this.partition = statefulServicePartition;
        }
        
        public FabricMonitoringData MonitoringData
        {
            get { return this.monitoringData; }
            set { this.monitoringData = value; }
        }

        public FabricMonitoringParameters MonitoringParameters
        {
            get { return this.monitoringParameters; }
            set { this.monitoringParameters = value; }
        }
        
        /// <summary>
        /// Partition 
        /// </summary>
        public IStatefulServicePartition Partition
        {
            get { return this.partition; }
            set { this.partition = value; }
        }

        public bool HasExpired
        {
            get { return this.hasExpired; }
        }

        /// <summary>
        /// Determines if a given API call has already expired to avoid duplicate traces and health reports
        /// </summary>
        /// <returns></returns>
        public bool IsFirstExpiry()
        {
            if (this.HasExpired)
            {
                return false;
            }

            if (this.monitoringData.StartTime.Elapsed < this.monitoringParameters.ApiSlowDuration)
            {
                return false;
            }

            this.hasExpired = true;
            return true;
        }

        /// <summary>
        /// Returns slow health information object. 
        /// TTL is TimeSpan.MaxValue by default
        /// </summary>
        /// <returns></returns>
        public HealthInformation GetHealthInformation_Slow()
        {
            var slowHealthInformation = new HealthInformation(
                    this.MonitoringData.ApiName,
                    this.MonitoringData.ApiProperty,
                    HealthState.Warning);

            slowHealthInformation.Description = string.Format(
                "{0} duration is greater than expected {1}",
                this.MonitoringData.ApiName,
                this.MonitoringParameters.ApiSlowDuration);

            return slowHealthInformation;
        }

        /// <summary>
        /// Returns clear health information object. 
        /// TTL of 1 second to clear the slow health report
        /// </summary>
        /// <returns></returns>
        public HealthInformation GetHealthInformation_OK()
        {
            var clearHealthInfo = new HealthInformation(
                this.MonitoringData.ApiName,
                this.MonitoringData.ApiProperty,
                HealthState.Ok);

            clearHealthInfo.TimeToLive = TimeSpan.FromMinutes(5);
            clearHealthInfo.RemoveWhenExpired = true;

            return clearHealthInfo;
        }
    }
}