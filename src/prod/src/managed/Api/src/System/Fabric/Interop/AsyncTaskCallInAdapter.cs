// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    using BOOLEAN = System.SByte;

    internal class AsyncTaskCallInAdapter : NativeCommon.IFabricAsyncOperationContext
    {
        private readonly Task callbackContinuation;
        private readonly Task userTask;
        private readonly InteropApi interopApi;
        private readonly string functionTag;
        private int completedSynchronously;
        private NativeCommon.IFabricAsyncOperationCallback nativeCallback;
        private CancellationTokenSource cancellationTokenSource;

        private int wasEndCalled = 0;

        static AsyncTaskCallInAdapter()
        {
            AsyncTaskCallInAdapter.ReleaseComObjectWrapperInstance = new ReleaseComObjectWrapper();
        }

        public AsyncTaskCallInAdapter(
            NativeCommon.IFabricAsyncOperationCallback callback, 
            Task userTask, 
            InteropApi interopApi,
            CancellationTokenSource cancellationTokenSource = null, 
            string functionTag = null)
        {
            Requires.Argument<Task>("task", userTask).NotNull();
            Requires.Argument("interopApi", interopApi).NotNull();

            this.interopApi = interopApi;
            this.nativeCallback = callback;
            this.userTask = userTask;
            this.cancellationTokenSource = cancellationTokenSource;

            // CompletedSynchronously is used to identify whether the task is done or not
            // The AsyncTaskCallInAdapter is usually the last item to be constructed and returned from a BeginXXX function
            // Thus, if the task is not complete at this point it is executing in a separate thread
            // and the Completion callback will be called on a separate thread
            
            // we have not yet observed whether the user task is complete
            this.completedSynchronously = (int)CompletedSynchronouslyState.Unknown;

            // setup the continuation. 
            // if the user task completes before we are able to determine then the callback will set the completed synchronously state
            // it does not matter whether the continuation is executed on this thread or the thread on which the user task is running
            this.callbackContinuation = this.userTask.ContinueWith((t) => this.ProcessNativeCallback(), TaskContinuationOptions.ExecuteSynchronously);

            // if the completedSynchronously state is still unknown then the continuation has not executed as yet which means that it is queued up for execution in a separate thread
            // thus the completed synchronously should be set to ASync
            // if on the other hand the continuation has begun executing (and we are still in the BeginXXX method because we are in the constructor)
            // then CompletedSynchronously would have been set to Synchronous by the continuation
            Interlocked.CompareExchange(ref this.completedSynchronously, (int)CompletedSynchronouslyState.Asynchronously, (int)CompletedSynchronouslyState.Unknown);

            this.functionTag = functionTag ?? "AsyncTaskCallInAdapter";
        }

        private enum CompletedSynchronouslyState : int
        {
            Unknown,
            Synchronously,
            Asynchronously
        }

        // for unit tests
        internal static ReleaseComObjectWrapper ReleaseComObjectWrapperInstance
        {
            get;
            set;
        }

        public static void End(NativeCommon.IFabricAsyncOperationContext nativeContext)
        {
            AsyncTaskCallInAdapter.EndHelper(nativeContext);
        }

        public static TResult End<TResult>(NativeCommon.IFabricAsyncOperationContext nativeContext)
        {
            AsyncTaskCallInAdapter adapter = AsyncTaskCallInAdapter.EndHelper(nativeContext);

            return (adapter.userTask as Task<TResult>).Result;
        }

        public BOOLEAN CompletedSynchronously()
        {
            CompletedSynchronouslyState state = (CompletedSynchronouslyState)this.completedSynchronously;
            if (state == CompletedSynchronouslyState.Unknown)
            {
                AppTrace.TraceSource.WriteError("AsyncTaskCallInAdapter.CompletedSynchronously", "A coding error has occurred - the completed synchronously was not set for {0}", this.functionTag);
                ReleaseAssert.Failfast("AsyncTaskCallInAdapter.CompletedSynchronously", StringResources.Error_AsyncTaskCallInAdapter_Completed_Sync_Not_Set);
                throw new InvalidOperationException(StringResources.Error_AsyncTaskCallInAdapter_Completed_Sync_Not_Set);
            }

            // Return true only if the operation has completed, and it completed synchronously
            return NativeTypes.ToBOOLEAN(state == CompletedSynchronouslyState.Synchronously);
        }

        public BOOLEAN IsCompleted()
        {
            return NativeTypes.ToBOOLEAN(this.userTask.IsCompleted);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
        public NativeCommon.IFabricAsyncOperationCallback get_Callback()
        {
            return this.nativeCallback;
        }

        public void Cancel()
        {
            CancellationTokenSource localCopyOfCancellationTokenSource = this.cancellationTokenSource;

            if (localCopyOfCancellationTokenSource != null)
            {
                try
                {
                    localCopyOfCancellationTokenSource.Cancel();
                }
                catch (ObjectDisposedException)
                {
                    // If callback happend while Cancel is executed we could get object disposed exception
                }
            }
        }

        private static AsyncTaskCallInAdapter EndHelper(NativeCommon.IFabricAsyncOperationContext nativeContext)
        {
            AsyncTaskCallInAdapter adapter = nativeContext as AsyncTaskCallInAdapter;
            Requires.Argument<AsyncTaskCallInAdapter>("adapter", adapter).NotNull();

            if (Interlocked.CompareExchange(ref adapter.wasEndCalled, 1, 0) != 0)
            {
                // calling end twice
                AppTrace.TraceSource.WriteError(adapter.functionTag, "AsyncTaskCallInAdapter.End was called twice");
                ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_AsyncTaskCallInAdapter_End_Called_Twice_Formatted, adapter.functionTag));
                throw new InvalidOperationException(StringResources.Error_AsyncTaskCallInAdapter_End_Called_Twice); // ReleaseAssert.Fail will throw
            }

            AppTrace.TraceSource.WriteNoise(adapter.functionTag, "End");

            try
            {
                adapter.userTask.Wait();
            }
            catch (AggregateException ex)
            {
                Exception inner = ex.InnerException;
                AppTrace.TraceSource.WriteExceptionAsWarning(adapter.functionTag, inner, "Exception thrown during async task execution");
                adapter.interopApi.HandleException(ex.GetBaseException());
                Utility.TryTranslateManagedExceptionToCOMAndThrow(inner);
                throw inner;
            }

            return adapter;
        }

        private void ProcessNativeCallback()
        {
            // if we have not yet determined whether the callback completed synchronously or not
            // it means we are still in the constructor and the BeginXXX method where this object was created
            // has not returned so when that method returns CompletedSynchronously needs to be true
            Interlocked.CompareExchange(ref this.completedSynchronously, (int)CompletedSynchronouslyState.Synchronously, (int)CompletedSynchronouslyState.Unknown);

            if (this.nativeCallback != null)
            {
                this.nativeCallback.Invoke(this);

                AsyncTaskCallInAdapter.ReleaseComObjectWrapperInstance.ReleaseComObject(this.nativeCallback);

                this.nativeCallback = null;
            }

            // Dispose the cancellation token source when the user task has completed
            if (this.cancellationTokenSource != null)
            {
                this.cancellationTokenSource.Dispose();
            }
        }

        internal class ReleaseComObjectWrapper
        {
            public virtual void ReleaseComObject(NativeCommon.IFabricAsyncOperationCallback comObject)
            {
                try
                {
                    Marshal.ReleaseComObject(comObject);
                }
                catch (Exception)
                {
                }
            }
        }
    }
}