// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Common
{
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Used internally for diagnostics/debugging.
    /// </summary>
    internal static class Diagnostics
    {
        /// <summary>
        /// Common type name.
        /// </summary>
        public const string CommonTypeName = "Data.Common";

        /// <summary>
        /// Used to produce a break in the attached debugger.
        /// </summary>
        [DllImport("kernel32.dll")]
        public static extern void DebugBreak();

        /// <summary>
        /// Used to determine the existence of a Win32 debugger (such as windbg).
        /// </summary>
        /// <returns>Boolean indicating whether debugger is present.</returns>
        [DllImport("kernel32.dll")]
        public static extern bool IsDebuggerPresent();

        #region ProcessException
        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="message">Format string.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessException(string type, Exception ex, string message)
        {
            return ProcessExceptionHelper(type, ex, message);
        }

        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="format">Format string.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessException<T>(string type, Exception ex, string format, T arg0)
        {
            var message = string.Format(CultureInfo.InvariantCulture, format, arg0);
            return ProcessExceptionHelper(type, ex, message);
        }

        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="format">Format string.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessException<T, U>(string type, Exception ex, string format, T arg0, U arg1)
        {
            var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1);
            return ProcessExceptionHelper(type, ex, message);
        }

        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="format">Format string.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessException<T, U, V>(string type, Exception ex, string format, T arg0, U arg1, V arg2)
        {
            var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1, arg2);
            return ProcessExceptionHelper(type, ex, message);
        }

        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="format">Format string.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        /// <param name="arg3">The fourth object to format.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessException<T, U, V, W>(string type, Exception ex, string format, T arg0, U arg1, V arg2, W arg3)
        {
            var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1, arg2, arg3);
            return ProcessExceptionHelper(type, ex, message);
        }

        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="message">Message.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessExceptionHelper(string type, Exception ex, string message)
        {
            FabricEvents.Events.ProcessExceptionWarning(type, message, ex.ToString());

            if (ex is AggregateException)
            {
                foreach (var e in ((AggregateException)ex).Flatten().InnerExceptions)
                {
                    if (e == null)
                    {
                        continue;
                    }

                    FabricEvents.Events.ProcessExceptionWarning(type, message, e.ToString());
                }

                return ex.InnerException;
            }

            return ex;
        }
        #endregion

        #region ProcessExceptionAsError
        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="format">Format string.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        public static Exception ProcessExceptionAsError<T>(string type, Exception ex, string format, T arg0)
        {
            var message = string.Format(CultureInfo.InvariantCulture, format, arg0);
            return ProcessExceptionAsErrorHelper(type, ex, message);
        }

        /// <summary>
        /// Processes a task exception. The exception is traced.
        /// </summary>
        /// <param name="type">Type of the event.</param>
        /// <param name="ex">The exception to process.</param>
        /// <param name="message">Message.</param>
        /// <returns>If the exception is an aggregated exception, it returns the inner exception.</returns>
        private static Exception ProcessExceptionAsErrorHelper(string type, Exception ex, string message)
        {
            FabricEvents.Events.ProcessExceptionError(type, message, ex.ToString());

            if (ex is AggregateException)
            {
                foreach (var e in ((AggregateException)ex).Flatten().InnerExceptions)
                {
                    if (e == null)
                    {
                        continue;
                    }

                    FabricEvents.Events.ProcessExceptionError(type, message, e.ToString());
                }

                return ex.InnerException;
            }

            return ex;
        }
        #endregion

        #region TraceIfNot
        /// <summary>
        /// Checks for a condition; if the condition is false, it will trace the exception.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="traceTypeSuffix">Trace type suffix.</param>
        /// <param name="message">Debugging message.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void TraceIfNot(bool condition, string traceTypeSuffix, string message)
        {
            if (!condition)
            {
                var e = new Exception(Environment.StackTrace);
                FabricEvents.Events.DiagnosticsMsg(traceTypeSuffix, message, e.ToString(), DateTime.UtcNow);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will trace the exception.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="traceTypeSuffix">Trace type suffix.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void TraceIfNot<T>(bool condition, string traceTypeSuffix, string format, T arg0)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, arg0);
                var e = new Exception(Environment.StackTrace);
                FabricEvents.Events.DiagnosticsMsg(traceTypeSuffix, message, e.ToString(), DateTime.UtcNow);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will trace the exception.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="traceTypeSuffix">Trace type suffix.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void TraceIfNot<T, U>(bool condition, string traceTypeSuffix, string format, T arg0, U arg1)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1);
                var e = new Exception(Environment.StackTrace);
                FabricEvents.Events.DiagnosticsMsg(traceTypeSuffix, message, e.ToString(), DateTime.UtcNow);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will trace the exception.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="traceTypeSuffix">Trace type suffix.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void TraceIfNot<T, U, V>(bool condition, string traceTypeSuffix, string format, T arg0, U arg1, V arg2)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1, arg2);
                var e = new Exception(Environment.StackTrace);
                FabricEvents.Events.DiagnosticsMsg(traceTypeSuffix, message, e.ToString(), DateTime.UtcNow);
            }
        }
        #endregion

        #region ThrowIfNotValid
        /// <summary>
        /// Throws InvalidOperationException if there is an unexpected operation.
        /// </summary>
        public static void ThrowIfNotValid<T>(bool condition, string format, T arg0)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, arg0);
                throw new InvalidOperationException(message);
            }
        }

        /// <summary>
        /// Throws InvalidOperationException if there is an unexpected operation.
        /// </summary>
        public static void ThrowIfNotValid<T, U>(bool condition, string format, T arg0, U arg1)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1);
                throw new InvalidOperationException(message);
            }
        }

        /// <summary>
        /// Throws InvalidOperationException if there is an unexpected operation.
        /// </summary>
        public static void ThrowIfNotValid<T, U, V>(bool condition, string format, T arg0, U arg1, V arg2)
        {
            if (!condition)
            {
                var message = string.Format(CultureInfo.InvariantCulture, format, arg0, arg1, arg2);
                throw new InvalidOperationException(message);
            }
        }
        #endregion

        #region Asserts
        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="message">Debugging message.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert(bool condition, string message)
        {
            if (!condition)
            {
                AssertHelper(CommonTypeName, message, string.Empty);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="sourceTypeName"></param>
        /// <param name="format">Debugging message.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert(bool condition, string sourceTypeName, string format)
        {
            if (!condition)
            {
                AssertHelper(sourceTypeName, format, string.Empty);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="sourceTypeName">The source type name.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T>(bool condition, string sourceTypeName, string format, T arg0)
        {
            if (!condition)
            {
                AssertHelper(sourceTypeName, format, arg0);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="sourceTypeName">The source type name.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U>(bool condition, string sourceTypeName, string format, T arg0, U arg1)
        {
            if (!condition)
            {
                AssertHelper(sourceTypeName, format, arg0, arg1);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="sourceTypeName">The source type name.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U, V>(bool condition, string sourceTypeName, string format, T arg0, U arg1, V arg2)
        {
            if (!condition)
            {
                AssertHelper(sourceTypeName, format, arg0, arg1, arg2);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="sourceTypeName">The source type name.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        /// <param name="arg3">The fourth object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U, V, W>(bool condition, string sourceTypeName, string format, T arg0, U arg1, V arg2, W arg3)
        {
            if (!condition)
            {
                AssertHelper(sourceTypeName, format, arg0, arg1, arg2, arg3);
            }
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="sourceTypeName">The source type name.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        /// <param name="arg3">The fourth object to format.</param>
        /// <param name="arg4">The fifth object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U, V, W, X>(bool condition, string sourceTypeName, string format, T arg0, U arg1, V arg2, W arg3, X arg4)
        {
            if (!condition)
            {
                AssertHelper(sourceTypeName, format, arg0, arg1, arg2, arg3, arg4);
            }
        }

        /// <summary>
        /// It will attempt to break in the debugger when the debugger is attached.
        /// If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="sourceTypeName">The source type name.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="args"></param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static void AssertHelper(string sourceTypeName, string format, params object[] args)
        {
            var message = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, args);
            if (Debugger.IsAttached || IsDebuggerPresent())
            {
                FabricEvents.Events.DiagnosticsError(sourceTypeName, message, DateTime.UtcNow);
                Diagnostics.DebugBreak();
            }
            else
            {
                Exception e = new Exception(Environment.StackTrace);
                FabricEvents.Events.DiagnosticsMsg(sourceTypeName, e.ToString(), message, DateTime.UtcNow);
                Environment.FailFast(message, e);
            }
        }
        #endregion
    }
}