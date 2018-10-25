// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the operation currently being executed by the Replicator, either via <see cref="System.Fabric.IReplicator" /> 
    /// or <see cref="System.Fabric.IPrimaryReplicator" /> interface.</para>
    /// </summary>
    [Flags]
    public enum ReplicatorOperationName
    {
        /// <summary>
        /// <para>Default value if the replicator is not yet ready.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID,
        
        /// <summary>
        /// <para>Replicator is not running any operation from Service Fabric perspective.</para>
        /// </summary>
        None = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_NONE,
        
        /// <summary>
        /// <para>Replicator is opening.</para>
        /// </summary>
        Open = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_OPEN,
        
        /// <summary>
        /// <para>Replicator is in the process of changing its role.</para>
        /// </summary>
        ChangeRole = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CHANGEROLE,
        
        /// <summary>
        /// <para>Due to a change in the replica set, replicator is being updated with its Epoch.</para>
        /// </summary>
        UpdateEpoch = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_UPDATEEPOCH,
        
        /// <summary>
        /// <para>Replicator is closing.</para>
        /// </summary>
        Close = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CLOSE,
        
        /// <summary>
        /// <para>Replicator is being aborted.</para>
        /// </summary>
        Abort = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ABORT,
        
        /// <summary>
        /// <para>Replicator is handling the data loss condition, where the user service may potentially be recovering state from an external source.</para>
        /// </summary>
        OnDataLoss = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ONDATALOSS,
        
        /// <summary>
        /// <para>Replicator is waiting for a quorum of replicas to be caught up to the latest state.</para>
        /// </summary>
        WaitForCatchup = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_WAITFORCATCHUP,
        
        /// <summary>
        /// <para>Replicator is in the process of building one or more replicas.</para>
        /// </summary>
        Build = NativeTypes.FABRIC_QUERY_REPLICATOR_OPERATION_NAME.FABRIC_QUERY_REPLICATOR_OPERATION_NAME_BUILD
    }
}