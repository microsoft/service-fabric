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
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;

    internal static class Utility
    {
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.InitializeReferenceTypeStaticFieldsInline,
               Justification = "Need an explicit static constructor to put FxCop suppressions on.")]
        [SuppressMessage(FxCop.Category.Maintainability, FxCop.Rule.AvoidExcessiveClassCoupling,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        [SuppressMessage(FxCop.Category.Maintainability, FxCop.Rule.AvoidExcessiveComplexity,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        [SuppressMessage(FxCop.Category.Usage, FxCop.Rule.DoNotRaiseReservedExceptionTypes,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]

        private const double MaxValidTimeSpanMilliseconds = (double)(uint.MaxValue - 1);

        /// <summary>
        /// Fail in release mode
        /// </summary>
        /// <param name="format"></param>
        /// <param name="args"></param>
        public static void ReleaseFail(string format, params object[] args)
        {
            System.Fabric.Common.ReleaseAssert.Failfast(format, args);
        }

        public static void ReleaseAssert(bool cond, string format, params object[] args)
        {
            System.Fabric.Common.ReleaseAssert.AssertIfNot(cond, format, args);
        }

        public static uint ToMilliseconds(TimeSpan timespan, string argumentName)
        {
            if (timespan == TimeSpan.MaxValue)
            {
                return uint.MaxValue;
            }
            else if (timespan <= TimeSpan.Zero)
            {
                return (uint)0;
            }
            else
            {
                double total = timespan.TotalMilliseconds;
                if (total < MaxValidTimeSpanMilliseconds)
                {
                    return (uint)total;
                }
                else
                {
                    throw new ArgumentOutOfRangeException(argumentName);
                }
            }
        }

        public static Exception TranslateCOMExceptionToManaged(COMException e, string functionTag)
        {
            string errorDetails = GetErrorDetails();

            int hr = e.HResult;

            Func<Exception, string, Exception> translateExceptionFunction;

            if (!InteropExceptionMap.NativeToManagedConversion.TryGetValue(hr, out translateExceptionFunction))
            {
                AppTrace.TraceSource.WriteWarning(
                    "Utility.TranslateCOMExceptionToManaged", 
                    "Unknown HRESULT - no translation exists for 0x{0:X}", 
                    hr);

                return new System.Fabric.FabricException(
                    (string.IsNullOrEmpty(errorDetails) ? StringResources.Error_Unknown : errorDetails), 
                    e,
                    hr);
            }
            return translateExceptionFunction(e, errorDetails);
        }

        /// <summary>
        /// Wrapper class over COMException as there is no other way to set both HResult and InnerException in COMException.
        /// Sets same HResult as that of innerException.
        /// InnerException is added to keep stack trace for debugging.
        /// </summary>
        private sealed class COMWrapperException : COMException
        {
            public COMWrapperException(Exception innerException)
                : base("Service Fabric Exception", innerException)
            {
                this.HResult = innerException.HResult;
            }
        }

        /// <summary>
        /// Transaltes Exception to COMException. This is requried as CoreCLR runtime returns Exception while FullClr returns COMException.
        /// If exception is not directly convertible to COMException, we create new COMException with same HResult as e and set e as InnerException.
        /// </summary>
        /// <param name="e">Exception which needs to be transalted to COMException</param>
        /// <returns>A COMException with same HResult as Exception e, if it is a COMException otherwise null.</returns>
        public static COMException TryTranslateExceptionToCOM(Exception e)
        {
            if (e is COMException) // fullclr case.
            {
                return e as COMException;
            }

            try
            {
                // should be changed to Marshal.GetExceptionForHR when it starts returning COMException for Unknown HResult.
                Marshal.ThrowExceptionForHR(e.HResult);
            }
            catch (COMException)
            {
                return new COMWrapperException(e);
            }
            catch (Exception)
            {
                return null;
            }

            return null;
        }

        /// <summary>
        /// Translates a managed exception to a COMException for native. We try to translate to an appropriate COMException when possible.
        /// The caller would use this for setting the Exception on Tasks with a more appropriate exception to send to native code.
        /// </summary>
        /// <param name="e"></param>
        public static Exception TryTranslateManagedExceptionToCOM(Exception e)
        {
            // First if this is a fabric exception we convert to a COMException with the matching fabric error code
            var fabricException = e as FabricException;

            if (fabricException != null && fabricException.ErrorCode != 0)
            {
                return new COMException(e.Message, (int)fabricException.ErrorCode);
            }

            // Next if there is an inner exception that is a com exception return it.
            // The reason is when a COM exception is available that should be used for native code to understand the originating error.
            return e.InnerException != null ? e.InnerException as COMException : null;
        }

        public static Exception TranslateArgumentException(ArgumentException e)
        {
            string errorDetails = GetErrorDetails();

            if (!string.IsNullOrEmpty(errorDetails))
            {
                return new ArgumentException(errorDetails, e); 
            }

            return e;
        }

        private static string GetErrorDetails()
        {
            string errorDetails = null;

            try
            {
                var nativeMsg = NativeCommon.FabricGetLastErrorMessage();
                var managedMsg = StringResult.FromNative(nativeMsg);

                if (!string.IsNullOrEmpty(managedMsg))
                {
                    errorDetails = managedMsg;
                }
            }
            catch (Exception e)
            {
                // non-critical: just ignore error and use default error messages
                //
                AppTrace.TraceSource.WriteWarning(
                    "Utility.GetErrorDetails", 
                    "Failed to get error details: {0}",
                    e);
            }

            return errorDetails;
        }

        /// <summary>
        /// Translates a managed exception to rethrow to native. We try to translate to an appropriate COMException when possible.
        /// The caller would use this to re-throw the more appropriate exception to send to native code.
        /// </summary>
        /// <param name="e"></param>
        public static void TryTranslateManagedExceptionToCOMAndThrow(Exception e)
        {
            Exception translated = TryTranslateManagedExceptionToCOM(e);

            if (translated != null)
            {
                throw translated;
            }
        }

        #region Wrap Native Async Implementation

        // The WrapNativeAsyncMethodImplementation functions are meant to be used whenever a native interface implemented in managed
        // has a method that returns an Async Operation
        //
        // This function will take care of tracing, translating exceptions etc and returning the correct type of object
        // It takes in a delegate that should return a Task
        // 
        // There are multiple overloads that let you handle passing in multiple arguments
        // Pass in a separate function instead of an actual inline delegate to avoid Partial Trust/Transparency issues
        public static NativeCommon.IFabricAsyncOperationContext WrapNativeAsyncMethodImplementation(
            Func<CancellationToken, Task> func,
            NativeCommon.IFabricAsyncOperationCallback callback,
            string functionTag)
        {
            return WrapNativeAsyncMethodImplementation(func, callback, functionTag, InteropApi.Default);
        }

        public static NativeCommon.IFabricAsyncOperationContext WrapNativeAsyncMethodImplementation(
            Func<CancellationToken, Task> func,
            NativeCommon.IFabricAsyncOperationCallback callback,
            string functionTag,
            InteropApi interopApi)
        {
            CancellationTokenSource cancellationTokenSource = null;
            try
            {
                AppTrace.TraceSource.WriteNoise(functionTag, "WrapNativeAsyncImpl - Begin");
                cancellationTokenSource = new CancellationTokenSource();
                Task t = func(cancellationTokenSource.Token);
                return new AsyncTaskCallInAdapter(callback, t, interopApi, cancellationTokenSource, functionTag);
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteExceptionAsWarning(functionTag, ex, "WrapNativeAsyncImpl - Begin threw exception");
                if (cancellationTokenSource != null)
                {
                    cancellationTokenSource.Dispose();
                }

                interopApi.HandleException(ex);
                TryTranslateManagedExceptionToCOMAndThrow(ex);
                throw;
            }
        }

        #endregion

        #region Wrap Synchronous Implementation

        // These should be used when managed code is implementing a native interface
        // and the method in question is a synchronous method
        public static TResult WrapNativeSyncMethodImplementation<TResult>(Func<TResult> func, string functionTag)
        {
            return WrapNativeSyncMethodImplementation(func, functionTag, InteropApi.Default);
        }

        public static TResult WrapNativeSyncMethodImplementation<TResult>(Func<TResult> func, string functionTag, InteropApi interopApi)
        {
            try
            {
                AppTrace.TraceSource.WriteNoise(functionTag, "Begin (sync)");
                TResult result = func();
                AppTrace.TraceSource.WriteNoise(functionTag, "End (sync");
                return result;
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteExceptionAsWarning(functionTag, ex, "WrapNativeSyncMethodImplementation failed");
                interopApi.HandleException(ex);
                TryTranslateManagedExceptionToCOMAndThrow(ex);
                throw;
            }
        }

        public static void WrapNativeSyncMethodImplementation(Action action, string functionTag)
        {
            Utility.WrapNativeSyncMethodImplementation(action, functionTag, InteropApi.Default);
        }

        public static void WrapNativeSyncMethodImplementation(Action action, string functionTag, InteropApi interopApi)
        {
            Utility.WrapNativeSyncMethodImplementation<object>(
                () =>
                {
                    action();
                    return null;
                },
                functionTag, interopApi);
        }
        
        #endregion

        #region Wrap Native Async Invoke

        public static Task WrapNativeAsyncInvokeInMTA(
           Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
           Action<NativeCommon.IFabricAsyncOperationContext> endFunc,
           CancellationToken cancellationToken,
           string functionTag)
        {
            return WrapNativeAsyncInvokeInMTA(beginFunc, endFunc, InteropExceptionTracePolicy.Default, cancellationToken, functionTag);
        }

        public static Task WrapNativeAsyncInvokeInMTA(
           Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
           Action<NativeCommon.IFabricAsyncOperationContext> endFunc,
           InteropExceptionTracePolicy tracePolicy,
           CancellationToken cancellationToken,
           string functionTag)
        {
            return Utility.RunInMTA(() => { return WrapNativeAsyncInvoke(beginFunc, endFunc, tracePolicy, cancellationToken, functionTag); });
        }

        public static Task<TResult> WrapNativeAsyncInvokeInMTA<TResult>(
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc, 
            CancellationToken cancellationToken, 
            string functionTag)
        {
            return WrapNativeAsyncInvokeInMTA<TResult>(beginFunc, endFunc, InteropExceptionTracePolicy.Default, cancellationToken, functionTag);
        }

        public static Task<TResult> WrapNativeAsyncInvokeInMTA<TResult>(
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc, 
            InteropExceptionTracePolicy tracePolicy,
            CancellationToken cancellationToken, 
            string functionTag)
        {
            return Utility.RunInMTA(() => WrapNativeAsyncInvoke(beginFunc, endFunc, tracePolicy, cancellationToken, functionTag));
        }

        public static Task WrapNativeAsyncInvoke(
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Action<NativeCommon.IFabricAsyncOperationContext> endFunc,
            CancellationToken cancellationToken,
            string functionTag)
        {
            return Utility.WrapNativeAsyncInvoke(beginFunc, endFunc, InteropExceptionTracePolicy.Default, cancellationToken, functionTag);
        }

        public static Task WrapNativeAsyncInvoke(
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Action<NativeCommon.IFabricAsyncOperationContext> endFunc,
            InteropExceptionTracePolicy tracePolicy,
            CancellationToken cancellationToken,
            string functionTag)
        {
            return Utility.WrapNativeAsyncInvoke<object>(
              beginFunc,
              (context) =>
              {
                  endFunc(context);
                  return null;
              },
              tracePolicy,
              cancellationToken,
              functionTag);
        }

        public static Task<TResult> WrapNativeAsyncInvoke<TResult>(
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc,
            CancellationToken cancellationToken,
            string functionTag)
        {
            return WrapNativeAsyncInvoke<TResult>(beginFunc, endFunc, InteropExceptionTracePolicy.Default, cancellationToken, functionTag);
        }

        public static Task<TResult> WrapNativeAsyncInvoke<TResult>(
            Func<NativeCommon.IFabricAsyncOperationCallback, NativeCommon.IFabricAsyncOperationContext> beginFunc,
            Func<NativeCommon.IFabricAsyncOperationContext, TResult> endFunc,
            InteropExceptionTracePolicy tracePolicy,
            CancellationToken cancellationToken,
            string functionTag)
        {
            return AsyncCallOutAdapter2<TResult>.WrapNativeAsyncInvoke(functionTag, beginFunc, endFunc, tracePolicy, cancellationToken);
        }

        #endregion

        #region Wrap Native Sync Invoke

        public static void WrapNativeSyncInvokeInMTA(Action action, string functionTag)
        {
            Utility.RunInMTA(() => Utility.WrapNativeSyncInvoke(action, functionTag));
        }

        public static TResult WrapNativeSyncInvokeInMTA<TResult>(Func<TResult> func, string functionTag)
        {
            return Utility.RunInMTA(() => { return Utility.WrapNativeSyncInvoke<TResult>(func, functionTag); });
        }

        // This has a different name because overload resolution between Action and Func<> is ambigous.
        public static void WrapNativeSyncInvoke(Action action, string functionTag, string functionArgs = "")
        {
            Utility.WrapNativeSyncInvoke<object>(() => { action(); return null; }, functionTag, functionArgs);
        }

        public static TResult WrapNativeSyncInvoke<TResult>(Func<TResult> func, string functionTag, string functionArgs = "")
        {
            try
            {
                TResult result = func();
                return result;
            }
            catch(Exception ex)
            {
                COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                if (comEx != null)
                {
                    throw Utility.TranslateCOMExceptionToManaged(comEx, functionTag);
                }

                ArgumentException argEx = ex as ArgumentException;
                if (argEx != null)
                {
                    throw Utility.TranslateArgumentException(argEx);
                }

                throw;
            }
        }

        #endregion

        #region COM Object Release Helpers

        public static void SafeReleaseComObject(object obj)
        {
            if (Marshal.IsComObject(obj) || obj?.GetType()?.FullName == "System.__ComObject") { Marshal.ReleaseComObject(obj); }
        }

        public static void SafeFinalReleaseComObject(object obj)
        {
            if (Marshal.IsComObject(obj) || obj?.GetType()?.FullName == "System.__ComObject") { Marshal.FinalReleaseComObject(obj); }
        }

        #endregion

        #region Helpers

        public static Task<TResult> CreateCompletedTask<TResult>(TResult result)
        {
            var tcs = new TaskCompletionSource<TResult>();
            tcs.SetResult(result);
            return tcs.Task;
        }
        
        private static TResult RunInMTA<TResult>(Func<TResult> func)
        {
            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                return func();
            }
            else
            {
                try
                {
                    Task<TResult> task = Task.Factory.StartNew<TResult>(func);
                    task.Wait();
                    return task.Result;
                }
                catch (AggregateException e)
                {
                    throw e.InnerException;
                }
            }
        }

        private static void RunInMTA(Action action)
        {
            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                action();
            }
            else
            {
                try
                {
                    Task task = Task.Factory.StartNew(action);
                    task.Wait();
                }
                catch (AggregateException e)
                {
                    throw e.InnerException;
                }
            }
        }

        internal static T ReadXml<T>(string fileName, string schemaFile)
        {
            const string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";
            XmlReaderSettings schemaValidatingReaderSettings = new XmlReaderSettings();
            schemaValidatingReaderSettings.ValidationType = ValidationType.Schema;
            schemaValidatingReaderSettings.Schemas.Add(FabricNamespace, schemaFile);
            schemaValidatingReaderSettings.XmlResolver = null;
            using (FileStream stream = File.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (XmlReader validatingReader = XmlReader.Create(stream, schemaValidatingReaderSettings))
                {
                    return ReadXml<T>(validatingReader);
                }
            }
        }

        internal static T ReadXmlString<T>(string xmlString, string schemaFile)
        {
            const string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";

            XmlReaderSettings schemaValidatingReaderSettings = new XmlReaderSettings();
            schemaValidatingReaderSettings.ValidationType = ValidationType.Schema;
            schemaValidatingReaderSettings.Schemas.Add(FabricNamespace, schemaFile);
            schemaValidatingReaderSettings.XmlResolver = null;

            using (MemoryStream stream = new MemoryStream(Encoding.UTF8.GetBytes(xmlString)))
            {
                using (XmlReader validatingReader = XmlReader.Create(stream, schemaValidatingReaderSettings))
                {
                    return ReadXml<T>(validatingReader);
                }
            }
        }

        internal static T ReadXml<T>(string fileName)
        {
            using (XmlReader reader = XmlReader.Create(fileName, new XmlReaderSettings() {XmlResolver =  null}))
            {
                return ReadXml<T>(reader);
            }
        }

        internal static T ReadXml<T>(XmlReader reader)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(T));
            T obj = (T)serializer.Deserialize(reader);
            return obj;
        }

#endregion

        
    }
}