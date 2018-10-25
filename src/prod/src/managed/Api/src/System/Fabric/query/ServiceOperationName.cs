// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the current active life-cycle operation on a stateful service replica or stateless service instance 
    /// retrieved by calling <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(System.String, System.Uri)" />.</para>
    /// </summary>
    public enum ServiceOperationName
    {
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_QUERY_SERVICE_OPERATION_NAME.FABRIC_QUERY_SERVICE_OPERATION_NAME_INVALID,
        
        /// <summary>
        /// <para>The service replica or instance is not going through any life-cycle changes.</para>
        /// </summary>
        None = NativeTypes.FABRIC_QUERY_SERVICE_OPERATION_NAME.FABRIC_QUERY_SERVICE_OPERATION_NAME_NONE,
        
        /// <summary>
        /// <para>The service replica or instance is being opened.</para>
        /// </summary>
        Open = NativeTypes.FABRIC_QUERY_SERVICE_OPERATION_NAME.FABRIC_QUERY_SERVICE_OPERATION_NAME_OPEN,
        
        /// <summary>
        /// <para>The service replica is changing roles.</para>
        /// </summary>
        ChangeRole = NativeTypes.FABRIC_QUERY_SERVICE_OPERATION_NAME.FABRIC_QUERY_SERVICE_OPERATION_NAME_CHANGEROLE,
        
        /// <summary>
        /// <para>The service replica or instance is being closed.</para>
        /// </summary>
        Close = NativeTypes.FABRIC_QUERY_SERVICE_OPERATION_NAME.FABRIC_QUERY_SERVICE_OPERATION_NAME_CLOSE,
        
        /// <summary>
        /// <para>The service replica or instance is being aborted.</para>
        /// </summary>
        Abort = NativeTypes.FABRIC_QUERY_SERVICE_OPERATION_NAME.FABRIC_QUERY_SERVICE_OPERATION_NAME_ABORT,
    }
}