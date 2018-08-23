// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Monitoring
{
    /// <summary>
    /// Encapsulates monitoring parameters for a single API call
    /// </summary>
    internal class FabricMonitoringParameters
    {
        private readonly bool isHealthReportEnabled;
        private readonly bool isApiSlowTraceEnabled;
        private readonly bool isApiLifeCycleTraceEnabled;
        private readonly TimeSpan apiSlowDuration;

        public FabricMonitoringParameters(bool isHealthReportEnabled, bool isApiSlowTraceEnabled, bool isApiLifeCycleTraceEnabled, TimeSpan apiSlowDuration)
        {
            this.isHealthReportEnabled = isHealthReportEnabled;
            this.isApiSlowTraceEnabled = isApiSlowTraceEnabled;
            this.isApiLifeCycleTraceEnabled = isApiLifeCycleTraceEnabled;
            this.apiSlowDuration = apiSlowDuration;
        }

        /// <summary>
        /// Determines health reporting when apiSlowDuration is exceeded
        /// </summary>
        public bool IsHealthReportEnabled
        {
            get { return this.isHealthReportEnabled; }
        }

        /// <summary>
        /// Determines API slow tracing when apiSlowDuration is exceeded
        /// </summary>
        public bool IsApiSlowTraceEnabled
        {
            get { return this.isApiSlowTraceEnabled; }
        }

        /// <summary>
        /// Determines API lifecycle tracing, at begin and end of the API's invocation.
        /// </summary>
        public bool IsApiLifeCycleTraceEnabled
        {
            get { return this.isApiLifeCycleTraceEnabled; }
        }

        /// <summary>
        /// Time after which health reports or API slow traces occur
        /// </summary>
        public TimeSpan ApiSlowDuration
        {
            get { return this.apiSlowDuration; }
        }
    }
}