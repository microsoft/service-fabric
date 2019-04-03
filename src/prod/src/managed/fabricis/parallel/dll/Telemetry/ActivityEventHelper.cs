// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    internal static class ActivityEventHelper
    {
        // It is easier to see the JSON via TraceViewer. 
        // It may be harder to parse JSON and post-process it via Cosmos. If that is the case,
        // then we may need another sink (observer) that dumps key/value formats for Cosmos to process
        // Anyway, additional observers are required if we do the following
        // a. pipe all activities from customer clusters to SFRP
        // b. pipe to MDM or operationalinsights.
        public static void Log(this IActivityEvent activityEvent, TraceType traceType)
        {
            activityEvent.Validate("activityEvent");

            traceType.WriteInfo("{0}", activityEvent.ToJson());
        }
    }
}