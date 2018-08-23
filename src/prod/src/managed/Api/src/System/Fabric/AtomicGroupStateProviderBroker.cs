// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class AtomicGroupStateProviderBroker : StateProviderBroker, NativeRuntime.IFabricAtomicGroupStateProvider
    {
        private readonly IAtomicGroupStateProvider atomicGroupStateProvider;

        public AtomicGroupStateProviderBroker(IStateProvider stateProvider, IAtomicGroupStateProvider atomicGroupStateProvider) :
            base(stateProvider)
        {
            Requires.Argument("atomicGroupStateProvider", atomicGroupStateProvider).NotNull();

            this.atomicGroupStateProvider = atomicGroupStateProvider;
        }

        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricAtomicGroupStateProvider.BeginAtomicGroupCommit(long atomicGroupId, long commitSequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.atomicGroupStateProvider.AtomicGroupCommitAsync(atomicGroupId, commitSequenceNumber, cancellationToken), 
                callback,
                "AtomicGroupStateProviderBroker.AtomicGroupCommit");
        }

        void NativeRuntime.IFabricAtomicGroupStateProvider.EndAtomicGroupCommit(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricAtomicGroupStateProvider.BeginAtomicGroupRollback(long atomicGroupId, long rollbackSequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.atomicGroupStateProvider.AtomicGroupRollbackAsync(atomicGroupId, rollbackSequenceNumber, cancellationToken),
                callback,
                "AtomicGroupStateProviderBroker.AtomicGroupRollback");
        }

        void NativeRuntime.IFabricAtomicGroupStateProvider.EndAtomicGroupRollback(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        NativeCommon.IFabricAsyncOperationContext NativeRuntime.IFabricAtomicGroupStateProvider.BeginUndoProgress(long fromCommitSequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.atomicGroupStateProvider.AtomicGroupUndoProgressAsync(fromCommitSequenceNumber, cancellationToken),
                callback,
                "AtomicGroupStateProviderBroker.AtomicGroupUndoProgress");
        }

        void NativeRuntime.IFabricAtomicGroupStateProvider.EndUndoProgress(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
    }
}