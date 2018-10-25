// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Monitoring
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Threading;
    
    /// <summary>
    /// FabricApiMonitoringComponent is used to monitor Api calls in the managed layer.
    /// This component handles tracing and optional health reporting for any desired API call
    /// </summary>
    internal class FabricApiMonitoringComponent
    {
        private static readonly string ApiSlowTraceType = "slowTraceCallback";
        private static readonly string ApiStopTraceType = "stopTraceCallback";
        private static readonly string ApiStartTraceType = "startTraceCallback";

        /// <summary>
        /// This store maintains API calls currently monitored for managed fabric components
        /// Key - FabricApiCallDescription
        /// Value - byte
        /// </summary>
        private readonly ConcurrentDictionary<FabricApiCallDescription, byte> store;

        /// <summary>
        /// Interval at which OnTimer event is raised
        /// </summary>
        private readonly TimeSpan scanInterval;

        /// <summary>
        /// Timer used to periodically raise api monitoring event
        /// </summary>
        private readonly System.Threading.Timer timer;
        private readonly object timerLock = new object();
        private bool timerEnabled;

        private readonly MonitoringComponentParameters componentParameters;
        private readonly Action<FabricApiCallDescription> apiStartTraceCallback;
        private readonly Action<FabricApiCallDescription> apiSlowTraceCallback;
        private readonly Action<FabricApiCallDescription> apiStopTraceCallback;
        
        internal FabricApiMonitoringComponent(
            MonitoringComponentParameters parameters,
            Action<FabricApiCallDescription> startTraceCallback = null,
            Action<FabricApiCallDescription> slowTraceCallback = null,
            Action<FabricApiCallDescription> stopTraceCallback = null)
        {
            this.apiStartTraceCallback = startTraceCallback ?? this.DefaultApiStartTraceCallback;
            this.apiSlowTraceCallback = slowTraceCallback ?? this.DefaultApiSlowTraceCallback;
            this.apiStopTraceCallback = stopTraceCallback ?? this.DefaultApiStopTraceCallback;
            this.componentParameters = parameters;
            this.scanInterval = parameters.ScanInterval;
            this.store = new ConcurrentDictionary<FabricApiCallDescription, byte>();
            this.timer = new Timer(this.OnTimer, null, Timeout.Infinite, Timeout.Infinite);
            this.timerEnabled = false;
        }
        
        /// <summary>
        /// Begin monitoring an API call. If monitoring has not started, timer is set and enabled in StartTimerIfNeeded. 
        /// Verify that the FabricMonitoringParameters object has a value greater than zero before calling start monitoring.
        /// Refer to the ValidateSettings method in the ReplicatorApiMonitor for implementation examples. 
        /// </summary>
        /// <param name="description"></param>
        public void StartMonitoring(FabricApiCallDescription description)
        {
            // Add new description to store
            this.AddApiCallToStore(description);

            this.StartTimerIfNeeded();

            this.TraceFabricApiStartMonitoringIfEnabled(description);
        }

        /// <summary>
        /// End monitoring for an API call. 
        /// Invokes the ClearSlowHealthReportCallback provided to remove warning HealthReports in the HM.
        /// If there are no remaining FabricApiCallDescriptions being monitored the timer is closed and wil be reopened by the next StartMonitoring method call.
        /// </summary>
        /// <param name="description"></param>
        public void StopMonitoring(FabricApiCallDescription description)
        {
            byte removedData;
            if (!this.store.TryRemove(description, out removedData))
            {
                return;
            }

            if (description.MonitoringParameters.IsHealthReportEnabled)
            {
                this.componentParameters.ClearSlowHealthReportCallback(description);
            }

            this.TraceFabricApiStopMonitoringIfEnabled(description);

            this.StopTimerIfNeeded();
        }

        /// <summary>
        /// Return the current number of items in the store.
        /// This method should exclusively be used for testing.
        /// </summary>
        /// <returns></returns>
        internal int Test_GetCount()
        {
            return this.store.Count;
        }

        private void DefaultApiSlowTraceCallback(FabricApiCallDescription apiCall)
        {
            FabricEvents.Events.FabricApiSlowTrace(
                ApiSlowTraceType,
                apiCall.MonitoringData.ApiName,
                apiCall.MonitoringData.Partitionid,
                apiCall.MonitoringData.ReplicaId,
                apiCall.MonitoringData.StartTime.Elapsed.TotalMilliseconds);
        }

        private void DefaultApiStopTraceCallback(FabricApiCallDescription apiCall)
        {
            FabricEvents.Events.FabricApiStopMonitoring(
                ApiStopTraceType,
                apiCall.MonitoringData.ApiName,
                apiCall.MonitoringData.Partitionid,
                apiCall.MonitoringData.ReplicaId,
                apiCall.MonitoringData.StartTime.Elapsed.TotalMilliseconds);
        }

        private void DefaultApiStartTraceCallback(FabricApiCallDescription apiCall)
        {
            FabricEvents.Events.FabricApiStartMonitoring(
                ApiStartTraceType,
                apiCall.MonitoringData.ApiName,
                apiCall.MonitoringData.Partitionid,
                apiCall.MonitoringData.ReplicaId);
        }

        /// <summary>
        /// Determine which items have expired, then perform enabled operations for each FabricApiCallDescription (ApiSlow tracing or health reporting)
        /// </summary>
        /// <param name="sender"></param>
        private void OnTimer(object sender)
        {
            var expiredApiCalls = this.FindExpiredItems();
            if (expiredApiCalls.Count == 0)
            {
                return;
            }

            this.TraceSlowIfEnabled(expiredApiCalls);
            this.ReportSlowHealthIfEnabled(expiredApiCalls);
        }

        /// <summary>
        /// Open timer. If timer is already enabled, return.
        /// </summary>
        private void StartTimerIfNeeded()
        {
            lock (this.timerLock)
            {
                if (!this.timerEnabled)
                {
                    this.timer.Change(this.scanInterval, this.scanInterval);
                    this.timerEnabled = true;
                }
            }
        }

        /// <summary>
        /// Stops timer if no elements remain in store, otherwise returns. 
        /// </summary>
        private void StopTimerIfNeeded()
        {
            lock (this.timerLock)
            {
                if (this.store.Count == 0 && this.timerEnabled)
                {
                    this.timer.Change(Timeout.Infinite, Timeout.Infinite);
                    this.timerEnabled = false;
                }
            }
        }
        
        /// <summary>
        /// Generate slow health callback list
        /// </summary>
        /// <param name="expiredItems"></param>
        /// <returns></returns>
        private void ReportSlowHealthIfEnabled(IEnumerable<ExpiredApiCall> expiredItems)
        {
            foreach (var apiCall in expiredItems)
            {
                if (apiCall.ShouldReportHealth && apiCall.ApiDescription.MonitoringParameters.IsHealthReportEnabled)
                {
                    FabricEvents.Events.FabricApiInvokeHealthReport(ApiSlowTraceType, apiCall.ApiDescription.MonitoringData.ApiName, apiCall.ApiDescription.MonitoringData.Partitionid);
                    this.componentParameters.SlowHealthReportCallback(apiCall.ApiDescription);
                }
            }
        }

        /// <summary>
        /// Find expired items
        /// </summary>
        /// <returns>
        /// List of ExpiredApiCall objects containing a boolean  and the apicall description
        /// </returns>
        private List<ExpiredApiCall> FindExpiredItems()
        {
            var expiredItems = new List<ExpiredApiCall>();

            foreach (var apiCall in this.store)
            {
                // Order matters for the following statements
                // If the api call is expiring for the first time, report health and trace slow (if enabled)
                // If the api call has already expired, do not report health but trace slow (if enabled)
                // If the api call has not expired, do not add it to the list
                if (apiCall.Key.IsFirstExpiry())
                {
                    expiredItems.Add(new ExpiredApiCall(true, apiCall.Key));
                }
                else if (apiCall.Key.HasExpired)
                {
                    expiredItems.Add(new ExpiredApiCall(false, apiCall.Key));
                }
            }

            return expiredItems;
        }

        /// <summary>
        /// Add API call to store.
        /// Throw if the FabricApiCallDescription has already been added.
        /// </summary>
        /// <param name="description"></param>
        private void AddApiCallToStore(FabricApiCallDescription description)
        {
            if (!this.store.TryAdd(description, byte.MinValue))
            {
                throw new FabricElementAlreadyExistsException("FabricApiCallDescription already exists");
            }
        }

        /// <summary>
        /// Invoke slowTraceCallback if isApiSlowTraceEnabled is true in FabricMonitoringParameters
        /// </summary>
        /// <param name="callbackItems"></param>
        private void TraceSlowIfEnabled(IEnumerable<ExpiredApiCall> callbackItems)
        {
            foreach (var apiCall in callbackItems)
            {
                if (apiCall.ApiDescription.MonitoringParameters.IsApiSlowTraceEnabled)
                {
                    this.apiSlowTraceCallback.Invoke(apiCall.ApiDescription);
                }
            }
        }

        /// <summary>
        /// Invoke startTraceCallback if isApiLifeCycleTraceEnabled is true in FabricMonitoringParameters
        /// </summary>
        /// <param name="description"></param>
        private void TraceFabricApiStartMonitoringIfEnabled(FabricApiCallDescription description)
        {
            if (description.MonitoringParameters.IsApiLifeCycleTraceEnabled)
            {
                this.apiStartTraceCallback.Invoke(description);
            }
        }

        /// <summary>
        /// Invoke ClearSlowHealthReportCallback if isHealthReportEnabled in FabricMonitoringParameters.
        /// Invoke stopTraceCallback if isApiLifeCycleTraceEnabled is true in FabricMonitoringParameters
        /// </summary>
        private void TraceFabricApiStopMonitoringIfEnabled(FabricApiCallDescription description)
        {
            if (description.MonitoringParameters.IsApiLifeCycleTraceEnabled)
            {
                this.apiStopTraceCallback.Invoke(description);
            }
        }
    }
}