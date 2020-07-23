// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Fabric.Interop;

    public enum OperationTypeEx
    {
        Invalid = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_INVALID,
        Copy = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_COPY,
        SingleOperation = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_SINGLE_OPERATION,
        CreateAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_CREATE_ATOMIC_GROUP,
        Redo = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_REDO,
        Undo = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_UNDO,
        CommitAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_COMMIT_ATOMIC_GROUP,
        RollbackAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_ROLLBACK_ATOMIC_GROUP,
        AbortAtomicGroup = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_ABORT_ATOMIC_GROUP,
        RedoPassComplete = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_REDO_PASS_COMPLETE,
        UndoPassComplete = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_UNDO_PASS_COMPLETE,
        EndOfStream = NativeTypes.FABRIC_OPERATION_TYPE_EX.FABRIC_OPERATION_TYPE_EX_END_OF_STREAM,
    }
}