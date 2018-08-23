// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the type of operation that is received via the copy or replication stream.</para>
    /// </summary>
    public enum OperationType
    {
        /// <summary>
        /// <para>Specifies that the operation is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_INVALID,
        
        /// <summary>
        /// <para>Specifies that the operation is not part of an atomic group and should be processed as a standalone copy or replication operation.</para>
        /// </summary>
        Normal = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_NORMAL,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        EndOfStream = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_END_OF_STREAM,
        
        /// <summary>
        /// <para>Specifies that a particular atomic group to be created.</para>
        /// </summary>
        CreateAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP,
        
        /// <summary>
        /// <para>Specifies that the operation is a part of an atomic group.</para>
        /// </summary>
        AtomicGroupOperation = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_ATOMIC_GROUP_OPERATION,
        
        /// <summary>
        /// <para>Specifies that a particular atomic group to be committed.</para>
        /// </summary>
        CommitAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP,
        
        /// <summary>
        /// <para>Specifies that a particular atomic group should be rolled back.</para>
        /// </summary>
        RollbackAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP,
        
        /// <summary>
        /// <para>Specifies that the operation has an atomic group mask.</para>
        /// </summary>
        HasAtomicGroupMask = NativeTypes.FABRIC_OPERATION_TYPE.FABRIC_OPERATION_TYPE_HAS_ATOMIC_GROUP_MASK
    }
}