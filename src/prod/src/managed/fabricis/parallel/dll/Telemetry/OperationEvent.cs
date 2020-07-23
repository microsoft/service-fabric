// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    /// <summary>
    /// Records an operation event. E.g. time taken for a method to execute, its name etc.
    /// Do not instantiate this class directly. Instead, use the <see cref="IActivityLogger"/> interface
    /// for logging operations.
    /// </summary>
    internal class OperationEvent : ActivityEvent
    {
        public TimeSpan Duration { get; set; }

        public OperationResult Result { get; set; }

        public Exception Exception { get; set; }

        public OperationEvent() { }

        public OperationEvent(
            Guid activityId,            
            DateTimeOffset startTime,
            DateTimeOffset endTime,            
            
            OperationResult result,
            Exception exception,
            string extraData,
            string name)
            : base(activityId, ActivityEventType.Operation, startTime, extraData, name)
        {
            Duration = endTime - startTime;
            Result = result;
            Exception = exception;            
        }

        public override string ToString()
        {
            return "{0}, Result: {1}, Duration: {2}".ToString(base.ToString(), Result, Duration);
        }
    }
}