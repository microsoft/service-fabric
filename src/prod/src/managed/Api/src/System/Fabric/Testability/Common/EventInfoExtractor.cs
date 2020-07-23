// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using TraceDiagnostic.Store;

    /// <summary>
    /// Extracts information from the events retrieved from Azure Tables or LogRoot traces,
    /// to populate the properties of different information objects the user receives as
    /// the result of calling diagnostics actions
    /// </summary>
    internal static class EventInfoExtractor
    {
        /// <summary>
        /// From UnexpectedTerminationEvent, extracts the information to populate 
        /// the properties of an AbnormalProcessTerminationInformation object with
        /// </summary>
        /// <param name="terminationEvent">UnexpectedTerminationEvent from which property values need to be extracted</param>
        /// <returns>AbnormalProcessTerminationInformation object</returns>
        internal static AbnormalProcessTerminationInformation ExtractAbnormalProcessTerminationInformation(UnexpectedTerminationEvent terminationEvent)
        {
            var terminationInfo = new AbnormalProcessTerminationInformation();

            terminationInfo.NodeID = terminationEvent.NodeId;
            terminationInfo.FailureTime = terminationEvent.Time;
            terminationInfo.ProcessName = terminationEvent.ProcessName;
            terminationInfo.ExitCode = terminationEvent.ReturnCode;

            return terminationInfo;
        }

        /// <summary>
        /// From apiFaultEvents, populates an ApiFaultInformation object
        /// </summary>
        /// <param name="apiFaultEvents">List of source events</param>
        /// <param name="apiFaultInfo">The ApiFaultInformation object that is populated</param>
        internal static void ExtractApiFaultInformation(List<ApiFaultEvent> apiFaultEvents, out ApiFaultInformation apiFaultInfo)
        {
            apiFaultInfo = new ApiFaultInformation();

            foreach (var fault in apiFaultEvents)
            {
                var errorEvent = fault as ApiErrorEvent;
                if (errorEvent != null)
                {
                    apiFaultInfo.AddErrorInformation(
                        new ApiErrorInformation(
                                errorEvent.PartitionId,
                                errorEvent.Time,
                                errorEvent.Operation,
                                errorEvent.Error));
                }
                else
                {
                    apiFaultInfo.AddSlowInformation(
                        new ApiSlowInformation(
                            fault.PartitionId,
                            fault.Time,
                            fault.Operation));
                }
            }
        }
    }
}

#pragma warning restore 1591