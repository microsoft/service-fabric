// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    /// <summary>
    /// Interface for representing a generic structured status event. 
    /// Note that these events are different from unstructured log events.
    /// </summary>
    internal interface IActivityEvent
    {
        Guid ActivityId { get; set; }

        ActivityEventType EventType { get; set; }

        DateTimeOffset Timestamp { get; set; }

        /// <summary>
        /// The name of the activity or the object that caused the activity.
        /// Mainly used for query purposes. (e.g. a Cosmos query for all state changes on the Name)
        /// </summary>
        string Name { get; set; }

        /// <summary>
        /// For adding additional data which may be useful for diagnostics. E.g. JSON serialized representation of the object.
        /// </summary>
        string ExtraData { get; set; }
    }
}