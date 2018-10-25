// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using Health;

    /// <summary>
    /// Used for reporting health from FabricDCA.
    /// </summary>
    internal static class HealthClient
    {    
        private const string SourceId = "FabricDCA";
        private const string Property = "DataCollectionAgent";

        private const string TraceType = "HealthClient";

        // Recommendation was to not use TimeSpan.Zero, but something small.
        private static readonly TimeSpan DefaultClearReportTimeToLive = TimeSpan.FromMilliseconds(5);

        private static readonly object ClientInitLock = new object();

        private static FabricClient fabricClient;

        static HealthClient()
        {
            Disabled = false;
        }

        /// <summary>
        /// Allows for health reporting to be disabled for testing scenarios.
        /// </summary>
        internal static bool Disabled { get; set; }

        /// <summary>
        /// Sends the Health report against the current node (i.e. same node where DCA is running)
        /// </summary>
        public static void SendNodeHealthReport(
            string description,
            HealthState healthState,
            string subProperty = "")
        {
            InternalSendNodeHealthReport(description, healthState, subProperty, TimeSpan.MaxValue, true);
        }

        /// <summary>
        /// For sub-properties, the health report can be set healthy and removed.
        /// </summary>
        /// <param name="subProperty">Name of sub-property to clear.</param>
        public static void ClearNodeHealthReport(string subProperty = "")
        {
            InternalSendNodeHealthReport(string.Empty, HealthState.Ok, subProperty, DefaultClearReportTimeToLive, true);
        }

        private static void InternalSendNodeHealthReport(
            string description, 
            HealthState healthState, 
            string subProperty, 
            TimeSpan timeToLive,
            bool removeWhenExpired)
        {
            if (Disabled)
            {
                return;
            }

            InitializeClient();
            var property = string.IsNullOrEmpty(subProperty) ? Property : string.Join(".", Property, subProperty);

            HealthInformation healthInfo;
            if (timeToLive != TimeSpan.Zero)
            {
                healthInfo = new HealthInformation(SourceId, property, healthState)
                {
                    Description = description,
                    TimeToLive = timeToLive,
                    RemoveWhenExpired = removeWhenExpired
                };
            }
            else
            {
                healthInfo = new HealthInformation(SourceId, property, healthState)
                {
                    Description = description,
                    TimeToLive = timeToLive,
                    RemoveWhenExpired = removeWhenExpired
                };
            }

            var nodeHealthReport = new NodeHealthReport(Utility.FabricNodeName, healthInfo);

            try
            {
                fabricClient.HealthManager.ReportHealth(nodeHealthReport);
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteExceptionAsWarning(TraceType, e);
            }
        }

        /// <summary>
        /// Initializes the fabric client as needed. Client is lazily initialized to allow for test scenarios.
        /// </summary>
        private static void InitializeClient()
        {
            lock (ClientInitLock)
            {
                if (fabricClient == null)
                {
                    fabricClient = new FabricClient();
                }
            }
        }
    }
}