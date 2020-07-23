// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("NodeEvent")]
    public class NodeEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the NodeEvent class.
        /// </summary>
        public NodeEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the NodeEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>
        /// <param name="timeStamp">Time the event was logged</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="nodeName">The name of a Service Fabric node.</param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public NodeEvent(Guid eventInstanceId, DateTime timeStamp, string category, string nodeName, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
            NodeName = nodeName;
        }

        /// <summary>
        /// Gets or sets the name of a Service Fabric node.
        /// </summary>
        [JsonProperty(PropertyName = "NodeName")]
        public string NodeName { get; set; }
    }
}
