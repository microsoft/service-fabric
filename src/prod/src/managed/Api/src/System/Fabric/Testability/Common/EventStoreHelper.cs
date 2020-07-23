// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using System.Linq;
    using TraceDiagnostic.Store;

    /// <summary>
    /// Helper methods to get diagnostics information of interest from: Azure Tables or LogRoot traces
    /// </summary>
    internal static class EventStoreHelper
    {
        /// <summary>
        /// Retrieves the information about abnormal process terminations from the events in Azure Tables or LogRoot traces
        /// </summary>
        /// <param name="eventStoreConnection">Connection to Event Store</param>
        /// <param name="startTime">Start time of the interval of interest</param>
        /// <param name="endTime">End time of the interval of interest</param>
        /// <param name="hostName">Name of the host process</param>
        /// <returns>List of AbnormalProcessTerminationInformation objects that belong to the interval of interest</returns>
        public static List<AbnormalProcessTerminationInformation> GetAbnormalProcessTerminationInformation(
            EventStoreConnection eventStoreConnection,
            DateTime? startTime,
            DateTime? endTime,
            string hostName = null)
        {
            // Events of interest
            var inEventTypes = new[]
            {
                EventTypes.ProcessUnexpectedTermination, 
                EventTypes.HostedServiceUnexpectedTermination
            };

            var filteringCriteria = default(Dictionary<string, string[]>);
            if (hostName != null)
            {
                filteringCriteria = new Dictionary<string, string[]>();
                filteringCriteria[FieldNames.ProcessName] = new[] { hostName };
            }

            var terminationInfo = new List<AbnormalProcessTerminationInformation>();

            var terminationEvents = eventStoreConnection.Store.GetNodeEvent(
                null, /*nodeId*/
                startTime,
                endTime,
                inEventTypes,
                filteringCriteria).ToList();

            if (terminationEvents.Count == 0)
            {
                return terminationInfo;
            }

            terminationInfo.AddRange(
                terminationEvents
                .OfType<UnexpectedTerminationEvent>()
                .Select(EventInfoExtractor.ExtractAbnormalProcessTerminationInformation));

            return terminationInfo;
        }

        /// <summary>
        /// Retrieves the information about API faults from the events in Azure Tables or LogRoot traces
        /// </summary>
        /// <param name="eventStoreConnection">Connection to Event Store</param>
        /// <param name="startTime">Start time of the interval of interest</param>
        /// <param name="endTime">End time of the interval of interest</param>
        /// <returns>List of ApiFaultInformation objects that belong to the interval of interest</returns>
        public static ApiFaultInformation GetApiFaultInformation(
            EventStoreConnection eventStoreConnection,
            DateTime? startTime,
            DateTime? endTime)
        {
            // From which events the fault information is to be extracted
            var eventsOfInterest = new[]
            {
                EventTypes.ApiError, 
                EventTypes.ApiSlow
            };

            var faultEvents = eventStoreConnection.Store.GetPartitionEvent(
                string.Empty, /*partition id*/
                startTime,
                endTime,
                eventsOfInterest,
                int.MaxValue)
                .OfType<ApiFaultEvent>()
                .ToList();

            if (faultEvents.Count == 0)
            {
                return new ApiFaultInformation();
            }

            ApiFaultInformation apiFaultInfo;

            EventInfoExtractor.ExtractApiFaultInformation(faultEvents, out apiFaultInfo);

            return apiFaultInfo;
        }
    }
}

#pragma warning restore 1591