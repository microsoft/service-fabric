// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class AsyncCallOutAdapter2<TResult> : NativeCommon.IFabricAsyncOperationCallback
    {
        private readonly string functionTag;
        private readonly Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc;
        private readonly Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc;
        private readonly InteropExceptionTracePolicy tracePolicy;
        private readonly SharedNativeObject<NativeCommon.IFabricAsyncOperationContext> nativeContext;
        private readonly TaskCompletionSource<TResult> tcs;
        private string traceId;

        private IDisposable cancellationTokenRegistration;

        private bool isCompleted;
        private bool wasCancelled;

        public static Task<TResult> WrapNativeAsyncInvoke(
            string functionTag,
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc,
            InteropExceptionTracePolicy tracePolicy,
            CancellationToken cancellationToken)
        {
            var adapter = new AsyncCallOutAdapter2<TResult>(functionTag, beginFunc, endFunc, tracePolicy);
            return adapter.Start(cancellationToken);
        }

        private AsyncCallOutAdapter2(
            string functionTag,
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc,
            InteropExceptionTracePolicy tracePolicy)
        {
            this.traceId = string.Format(CultureInfo.InvariantCulture, "AsyncCalloutAdapter-{0}", this.GetHashCode());

            this.functionTag = functionTag;
            this.beginFunc = beginFunc;
            this.endFunc = endFunc;
            this.tracePolicy = tracePolicy;
            this.nativeContext = new SharedNativeObject<NativeCommon.IFabricAsyncOperationContext>(this.functionTag);
            this.tcs = new TaskCompletionSource<TResult>();
            this.isCompleted = false;
        }

        private Task<TResult> Start(CancellationToken cancellationToken)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                this.tcs.SetCanceled();
                return this.tcs.Task;
            }

            NativeCommon.IFabricAsyncOperationContext context = null;

            try
            {
                AppTrace.TraceSource.WriteNoise(this.functionTag, "{0}: begin", this.traceId);

                // This is where the call to the native API is being made
                context = this.beginFunc(this);
            }
            catch (Exception ex)
            {
                COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                if (comEx != null)
                {
                    this.TraceException(ex, "{0}: begin delegate threw an exception", this.traceId);
                    // The begin call failed hence there is nothing further to do
                    throw Utility.TranslateCOMExceptionToManaged(comEx, this.functionTag);
                }

                if (ex is ArgumentException)
                {
                    throw Utility.TranslateArgumentException(ex as ArgumentException);
                }

                throw;
            }

            // Try to initialize the shared context with the context returned from the begin function
            // If the shared context was already initialized by the callback then the context will be freed 
            // (Release of COM object)
            this.InitializeSharedContext(context);

            // try to acquire the shared native object and using it try to finish the operation if the 
            // operation was completed synchronously
            this.nativeContext.TryAcquireAndInvoke((inner) => this.Finish(inner, true));

            this.RegisterForCancellation(cancellationToken);

            return this.tcs.Task;
        }

        private void RegisterForCancellation(CancellationToken token)
        {
            if (token == CancellationToken.None)
            {
                return;
            }

            // Acquire the native context
            this.nativeContext.TryAcquireAndInvoke((context) => this.RegisterForCancellation(context, token));
        }

        private void RegisterForCancellation(NativeCommon.IFabricAsyncOperationContext context, CancellationToken token)
        {
            // Do not check for completion using the IFabricAsyncOperationContext
            // Some implementations of IFabricAsyncOperationContext may not support the check for IsCompleted
            // after the async operation completes and adding this check now could break backward compatibility
            if (this.isCompleted)
            {
                // No-Op
                return;
            }

            // Register with the cancellation token
            var registration = token.Register(this.Cancel) as IDisposable;
            Interlocked.Exchange(ref this.cancellationTokenRegistration, registration);

            // The async operation could have completed and it is possible that the FinishAsyncOperation method observed null because the value was not set
            // If that is the case then this thread must dispose the registration
            if (this.isCompleted)
            {
                bool isRegistrationActive =
                    object.ReferenceEquals(
                        Interlocked.CompareExchange(ref this.cancellationTokenRegistration, null, registration),
                        registration);
                if (isRegistrationActive)
                {
                    // At this point the context is complete but the registration was not disposed by the 
                    // thread that processed the callback hence dispose it here
                    registration.Dispose();
                }
            }
        }

        private void Finish(NativeCommon.IFabricAsyncOperationContext context, bool expectedCompletedSynchronously)
        {
            if (NativeTypes.FromBOOLEAN(context.CompletedSynchronously()) != expectedCompletedSynchronously)
            {
                // This invoke did not finish the operation
                return;
            }

            try
            {
                AppTrace.TraceSource.WriteNoise(this.functionTag, "{0}: end", this.traceId);
                TResult operationResult = this.endFunc(context);
                this.tcs.SetResult(operationResult);
            }
            catch (Exception ex)
            {
                COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                if (comEx != null)
                {
                    this.FailTask(Utility.TranslateCOMExceptionToManaged(comEx, this.functionTag));
                }
                else
                {
                    if (ex is ArgumentException)
                    {
                        this.FailTask(Utility.TranslateArgumentException(ex as ArgumentException));
                    }
                    else
                    {
                        this.FailTask(ex);
                    }
                }
            }
            finally
            {
                this.DisposeCancelCallbackRegistrationOnAsyncOperationCompletion();

                // release the context set by Initialize
                this.nativeContext.Release();
            }
        }

        private void DisposeCancelCallbackRegistrationOnAsyncOperationCompletion()
        {
            // The cancellation token could have been registered
            // This registration should be disposed
            // If the token has been registered then the field would have been set
            // Atomically set the field to null and get the current registration so that this thread disposes it
            var registration = Interlocked.Exchange(ref this.cancellationTokenRegistration, null);

            // If the registration is null then the registration has not been performed yet 
            // and the operation has completed very quickly
            // The thread that is performing the registration in the constructor is responsible for the dispose
            if (registration == null)
            {
                return;
            }

            if (this.wasCancelled)
            {
                // It is possible that the cancellation completed synchronously when cancel was invoked
                // Thus, the async operation is being completed on the callback handler registered 
                // with the cancellation token

                // Disposing the cancellation token registration requires a wait for all the outstanding callbacks to complete
                // This will cause a deadlock
                // Dispose the registration on a separate thread
                Task.Factory.StartNew(registration.Dispose);
            }
            else
            {
                // Cancellation has not happened yet and the async operation is already complete
                // It is safe to dispose the registration on this thread because even if the cancel callback is now invoked
                // And we invoke cancel on the native context that call will complete and there is no risk of a deadlock
                registration.Dispose();
            }
        }

        private void FailTask(Exception e)
        {
            this.TraceException(e, "{0}: end delegate threw an exception", this.traceId);

            this.tcs.SetException(e);
        }

        private void Cancel()
        {
            this.nativeContext.TryAcquireAndInvoke(this.Cancel);
        }

        private void Cancel(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.wasCancelled = true;
            AppTrace.TraceSource.WriteNoise(this.functionTag, "{0}: cancel", this.traceId);
            Utility.WrapNativeSyncInvokeInMTA(() => context.Cancel(), "Cancel");
        }

        void NativeCommon.IFabricAsyncOperationCallback.Invoke(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise(this.functionTag, "{0}: callback", this.traceId);

            // The callback has been invoked so the operation is completed at this point
            this.isCompleted = true;

            this.InitializeSharedContext(context);

            // Do not use the above context here - it could have been released by InitializeSharedContext
            this.nativeContext.TryAcquireAndInvoke((inner) => this.Finish(inner, false));
        }

        private void InitializeSharedContext(NativeCommon.IFabricAsyncOperationContext context)
        {
            // try to initialize the shared context with the context provided in the callback
            if (!this.nativeContext.TryInitialize(context))
            {
                // if failed, release this object as we are no longer going to be using it
                // this will happen if the constructor set the context after calling the begin method
                Utility.SafeReleaseComObject(context);
            }
        }

        private void TraceException(Exception ex, string format, params object[] args)
        {
            switch (this.tracePolicy)
            {
            case InteropExceptionTracePolicy.Warning:
                AppTrace.TraceSource.WriteExceptionAsWarning(this.functionTag, ex, format, args);
                break;

            case InteropExceptionTracePolicy.Info:
                AppTrace.TraceSource.WriteExceptionAsInfo(this.functionTag, ex, format, args);
                break;

            case InteropExceptionTracePolicy.WarningExceptInfoForTransient:
                if (ex is FabricTransientException || ex is TimeoutException)
                {
                    AppTrace.TraceSource.WriteExceptionAsInfo(this.functionTag, ex, format, args);
                }
                else
                {
                    AppTrace.TraceSource.WriteExceptionAsWarning(this.functionTag, ex, format, args);
                }
                break;

            case InteropExceptionTracePolicy.None:
            default:
                break;
            }
        }
    }
}