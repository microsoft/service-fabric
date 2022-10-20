// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    internal class AtomicGroupStateProviderExBroker : NativeRuntime.IFabricAtomicGroupStateProviderEx
    {
        private IAtomicGroupStateProviderEx stateProvider;

        public AtomicGroupStateProviderExBroker(IAtomicGroupStateProviderEx stateProvider)
        {
            Requires.Argument<IAtomicGroupStateProviderEx>("stateProvider", stateProvider).NotNull();

            this.stateProvider = stateProvider;
        }

        // Publicly settable because this class is first created
        // and passed into native code which then creates an IOperationDataFactory
        // which this class further uses
        public OperationDataFactoryWrapper OperationDataFactory
        {
            get;
            set;
        }

        public NativeCommon.IFabricAsyncOperationContext BeginAtomicGroupCommit(long atomicGroupId, long commitSequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.stateProvider.AtomicGroupCommitAsync(atomicGroupId, commitSequenceNumber, cancellationToken),
                callback,
                "AtomicGroupStateProviderExBroker.AtomicGroupCommit");
        }

        public void EndAtomicGroupCommit(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginAtomicGroupRollback(long atomicGroupId, NativeRuntime.IFabricOperationStreamEx undoOperationStream, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.stateProvider.AtomicGroupRollbackAsync(atomicGroupId, new OperationStreamEx(undoOperationStream), cancellationToken),
                callback,
                "AtomicGroupStateProviderExBroker.AtomicGroupRollback");
        }

        public void EndAtomicGroupRollback(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginAtomicGroupAbort(long atomicGroupId, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.stateProvider.AtomicGroupAbortAsync(atomicGroupId, cancellationToken),
                callback,
                "AtomicGroupStateProviderExBroker.AtomicGroupAbort");
        }

        public void EndAtomicGroupAbort(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginUndoProgress(long fromSequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.stateProvider.UndoProgressAsync(fromSequenceNumber, cancellationToken),
                callback,
                "AtomicGroupStateProviderExBroker.UndoProgress");
        }

        public void EndUndoProgress(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginCheckpoint(long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.stateProvider.CheckpointAsync(sequenceNumber, cancellationToken),
                callback,
                "AtomicGroupStateProviderExBroker.Checkpoint");
        }

        public void EndCheckpoint(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginOnOperationStable(long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.stateProvider.OnOperationStableAsync(sequenceNumber, cancellationToken),
                callback,
                "AtomicGroupStateProviderExBroker.OperationStable");
        }

        public long EndOnOperationStable(NativeCommon.IFabricAsyncOperationContext context)
        {
            return AsyncTaskCallInAdapter.End<long>(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(IntPtr nativeEpoch, long previousEpochLastSequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.UpdateEpochHelper(nativeEpoch, previousEpochLastSequenceNumber, cancellationToken),
                callback,
                "StateProviderBroker.UpdateEpoch");
        }

        public void EndUpdateEpoch(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public long GetLastCommittedSequenceNumber()
        {
            return this.stateProvider.GetLastCommittedSequenceNumber();
        }

        public NativeCommon.IFabricAsyncOperationContext BeginOnDataLoss(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.stateProvider.OnDataLossAsync(cancellationToken), callback, "StateProviderBroker.OnDataLoss");
        }

        public sbyte EndOnDataLoss(NativeCommon.IFabricAsyncOperationContext context)
        {
            return NativeTypes.ToBOOLEAN(AsyncTaskCallInAdapter.End<bool>(context));
        }

        public NativeRuntime.IFabricOperationDataStream GetCopyContext()
        {
            ReleaseAssert.AssertIfNot(this.OperationDataFactory != null, StringResources.Error_OperationDataFactoryNotSet);
            return Utility.WrapNativeSyncMethodImplementation<NativeRuntime.IFabricOperationDataStream>(this.GetCopyContextHelper, "StateProviderBroker.GetCopyContext");
        }

        public NativeRuntime.IFabricOperationDataStream GetCopyState(long uptoSequenceNumber, NativeRuntime.IFabricOperationDataStream comCopyContext)
        {
            ReleaseAssert.AssertIfNot(this.OperationDataFactory != null, StringResources.Error_OperationDataFactoryNotSet);
            return Utility.WrapNativeSyncMethodImplementation<NativeRuntime.IFabricOperationDataStream>(() => this.GetCopyStateHelper(uptoSequenceNumber, comCopyContext), "StateProviderBroker.GetCopyState");
        }

        private NativeRuntime.IFabricOperationDataStream GetCopyStateHelper(long uptoSequenceNumber, NativeRuntime.IFabricOperationDataStream comCopyContext)
        {
            IOperationDataStreamEx copyState = null;
            AsyncEnumerateOperationData copyContext = comCopyContext == null ? null : new AsyncEnumerateOperationData(comCopyContext);
            copyState = this.stateProvider.GetCopyState(uptoSequenceNumber, copyContext);
            return AsyncEnumerateOperationDataBroker.ToNative(copyState, this.OperationDataFactory);
        }

        private NativeRuntime.IFabricOperationDataStream GetCopyContextHelper()
        {
            IOperationDataStreamEx copyContext = this.stateProvider.GetCopyContext();
            AsyncEnumerateOperationDataBroker comCopyContext = AsyncEnumerateOperationDataBroker.ToNative(copyContext, this.OperationDataFactory);
            return comCopyContext;
        }

        private Task UpdateEpochHelper(IntPtr nativeEpoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
        {
            Epoch epoch = Epoch.FromNative(nativeEpoch);

            return this.stateProvider.UpdateEpochAsync(epoch, previousEpochLastSequenceNumber, cancellationToken);
        }

        private class AsyncEnumerateOperationData : IOperationDataStreamEx
        {
            private readonly NativeRuntime.IFabricOperationDataStream enumerator;

            public AsyncEnumerateOperationData(NativeRuntime.IFabricOperationDataStream enumerator)
            {
                Requires.Argument("enumerator", enumerator).NotNull();
                this.enumerator = enumerator;
            }

            [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes, Justification = "Async pattern: save the exception and throw in the right thread.")]
            [SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.UseApprovedGenericsForPrecompiledAssemblies, Justification = "Not precompiled assembly.")]
            public Task<IOperationData> GetNextAsync(CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvoke<IOperationData>(
                    this.GetNextBeginWrapper,
                    this.GetNextEndWrapper,
                    cancellationToken,
                    "AsyncEnumerateOperationData.GetNext");
            }

            #region GetNext Async

            private NativeCommon.IFabricAsyncOperationContext GetNextBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.enumerator.BeginGetNext(callback);
            }

            private IOperationData GetNextEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                NativeRuntime.IFabricOperationData comData = this.enumerator.EndGetNext(context);
                return comData == null ? null : new ReadOnlyOperationData(comData);
            }

            #endregion
        }

        private class AsyncEnumerateOperationDataBroker : NativeRuntime.IFabricOperationDataStream, NativeRuntimeInternal.IFabricDisposable
        {
            private readonly IOperationDataStreamEx enumerator;
            private readonly OperationDataFactoryWrapper operationDataFactory;

            private AsyncEnumerateOperationDataBroker(IOperationDataStreamEx enumerator, OperationDataFactoryWrapper operationDataFactory)
            {
                Requires.Argument("enumerator", enumerator).NotNull();
                Requires.Argument("operationDataFactory", operationDataFactory).NotNull();
                this.enumerator = enumerator;
                this.operationDataFactory = operationDataFactory;
            }

            public static AsyncEnumerateOperationDataBroker ToNative(IOperationDataStreamEx enumerator, OperationDataFactoryWrapper operationDataFactory)
            {
                if (enumerator == null)
                {
                    return null;
                }
                else
                {
                    var native = new AsyncEnumerateOperationDataBroker(enumerator, operationDataFactory);
                    return native;
                }
            }

            [SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.UseApprovedGenericsForPrecompiledAssemblies, Justification = "Not precompiled assembly.")]
            public NativeCommon.IFabricAsyncOperationContext BeginGetNext(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.enumerator.GetNextAsync(cancellationToken), callback, "AsyncEnumerateOperationDataBroker.GetNext");
            }

            public NativeRuntime.IFabricOperationData EndGetNext(NativeCommon.IFabricAsyncOperationContext context)
            {
                IOperationData buffers = AsyncTaskCallInAdapter.End<IOperationData>(context);
                return this.operationDataFactory.CreateOperationData(buffers);
            }

            public void Dispose()
            {
                Utility.WrapNativeSyncMethodImplementation(this.DisposeHelper, "AsyncEnumerateOperationDataBroker.Dispose");
            }

            private void DisposeHelper()
            {
                IDisposable disposable = this.enumerator as IDisposable;
                if (disposable != null)
                {
                    disposable.Dispose();
                }
            }
        }
    }
}