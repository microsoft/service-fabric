// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Runtime.CompilerServices;

    /// <summary>
    /// Dumps structured activity events to trace listeners. 
    /// Currently only DCA's Trace is hooked up. In future, we could add a MDM listener etc
    /// (and move to a full-blown observable/observers model if required)
    /// </summary>
    internal class TraceActivitySubject : IActivityLogger
    {
        private readonly TraceType traceType;

        public TraceActivitySubject(TraceType traceType)
        {
            this.traceType = traceType.Validate("traceType");
        }

        public void LogChangeState<TObject, TObjectState>(
            Guid activityId, 
            TObject @object,                         
            TObjectState oldState, 
            TObjectState newState, 
            TimeSpan timeInState,
            string extraData,
            string name)
        {
            name.Validate("name");
            var @event = new ChangeStateEvent(
                activityId, 
                DateTimeOffset.UtcNow, 
                @object.GetType().Name,                
                oldState.ToString(), 
                newState.ToString(), 
                timeInState,
                extraData,
                name);
            @event.Log(this.traceType);
        }

        public void LogOperation(
            Guid activityId, 
            DateTimeOffset startTime,                         
            OperationResult result = OperationResult.Success, 
            Exception exception = null,
            string extraData = null,
            [CallerMemberName] string name = "<unknown>")
        {
            var @event = new OperationEvent(activityId, startTime, DateTimeOffset.UtcNow, result, exception, extraData, name);
            @event.Log(traceType);
        }
    }
}