// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    internal abstract class ActivityEvent : IActivityEvent
    {
        public Guid ActivityId { get; set; }

        public ActivityEventType EventType { get; set; }

        public DateTimeOffset Timestamp { get; set; }

        public string Name { get; set; }

        public string ExtraData { get; set; }

        /// <summary>
        /// For automatic serialization (e.g. to/from JSON)
        /// </summary>
        protected ActivityEvent()
        {            
        }

        protected ActivityEvent(
            Guid activityId,
            ActivityEventType eventType,
            DateTimeOffset timestamp,
            string extraData,
            string name)
        {                        
            this.ActivityId = activityId;
            this.EventType = eventType;
            this.Timestamp = timestamp;            
            this.ExtraData = extraData;
            this.Name = name; // no null check since the default constructor (used by json library) doesn't construct a correct object in this case.
        }

        public override string ToString()
        {
            return "{0}, {1}, {2:oZ}, {3}".ToString(Name, EventType, Timestamp, ActivityId);
        }
    }
}