// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    /// <summary>
    /// Records an event when an object changes state.
    /// Do not instantiate this class directly. Instead, use the <see cref="IActivityLogger"/> interface
    /// for logging state changes.
    /// </summary>
    internal class ChangeStateEvent : ActivityEvent
    {
        public string ObjectType { get; set; }

        public string OldState { get; set; }

        public string NewState { get; set; }

        public TimeSpan TimeInState { get; set; }

        public ChangeStateEvent() { }

        public ChangeStateEvent(
            Guid activityId,
            DateTimeOffset timestamp,
            string objectType,                        
            string oldState,
            string newState,
            TimeSpan timeInState,
            string extraData,
            string name)
            : base(activityId, ActivityEventType.ChangeState, timestamp, extraData, name)
        {
            this.ObjectType = objectType;
            this.OldState = oldState;
            this.NewState = newState;
            this.TimeInState = timeInState;
        }

        public override string ToString()
        {
            return "{0}, Type: {1}, {2} => {3}, InState: {4}".ToString(base.ToString(), ObjectType, OldState, NewState, TimeInState);
        }
    }
}