// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Runtime.CompilerServices;

    /// <summary>
    /// The interface used specifically for activity/status logging.
    /// This is NOT to be used for unstructured logging.
    /// </summary>
    /// <remarks>
    /// The name parameter is at the end for <see cref="CallerMemberNameAttribute"/> to be used as much as possible in <see cref="LogOperation"/>.
    /// <see cref="LogChangeState{TObject, TObjectState}"/> just tries to be consistent with parameters as <see cref="LogOperation"/>.
    /// </remarks>
    internal interface IActivityLogger
    {
        void LogChangeState<TObject, TObjectState>(
            Guid activityId,
            TObject @object,
            TObjectState oldState, 
            TObjectState newState, 
            TimeSpan timeInState,            
            string extraData,
            string name);

        void LogOperation(
            Guid activityId, 
            DateTimeOffset startTime,                        
            OperationResult result = OperationResult.Success, 
            Exception exception = null,
            string extraData = null,
            [CallerMemberName] string name = "<unknown>");
    }
}