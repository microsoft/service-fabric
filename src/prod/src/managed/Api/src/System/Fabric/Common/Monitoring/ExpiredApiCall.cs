// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Monitoring
{
    /// <summary>
    /// ExpiredApiCall is used to return the items that need to be traced and reported on in the onTimerMethod of FabricApiMonitoringComponent.
    /// </summary>
    internal class ExpiredApiCall
    {
        private bool shouldReportHealth;
        private FabricApiCallDescription apiDescription;

        public ExpiredApiCall(bool shouldReportHealth, FabricApiCallDescription apiDescription)
        {
            this.shouldReportHealth = shouldReportHealth;
            this.apiDescription = apiDescription;
        }

        /// <summary>
        /// Indicates whether this expired api call should fire a health report
        /// </summary>
        public bool ShouldReportHealth
        {
            get { return this.shouldReportHealth; }
        }

        /// <summary>
        /// The api call description that has expired
        /// </summary>
        public FabricApiCallDescription ApiDescription
        {
            get { return this.apiDescription; }
        }
    }
}