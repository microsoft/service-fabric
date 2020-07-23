// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableMessaging
{
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Used internally for diagnostics/debugging.
    /// </summary>
    internal static class Diagnostics
    {
        /// <summary>
        /// Reliable messaging type name.
        /// </summary>
        private const string ReliableMessagingTypeName = "ReliableMessaging";

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
            return Data.Common.Diagnostics.ProcessException(type, ex, format, arg0);
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
            return Data.Common.Diagnostics.ProcessException(type, ex, format, arg0, arg1);
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
            return Data.Common.Diagnostics.ProcessException(type, ex, format, arg0, arg1, arg2);
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
            return Data.Common.Diagnostics.ProcessException(type, ex, format, arg0, arg2, arg3);
        }

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
            return Data.Common.Diagnostics.ProcessExceptionAsError(type, ex, format, arg0);
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="message">Debugging message.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert(bool condition, string message)
        {
            Data.Common.Diagnostics.Assert(condition, ReliableMessagingTypeName, message);
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T>(bool condition, string format, T arg0)
        {
            Data.Common.Diagnostics.Assert(condition, ReliableMessagingTypeName, format, arg0);
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U>(bool condition, string format, T arg0, U arg1)
        {
            Data.Common.Diagnostics.Assert(condition, ReliableMessagingTypeName, format, arg0, arg1);
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U, V>(bool condition, string format, T arg0, U arg1, V arg2)
        {
            Data.Common.Diagnostics.Assert(condition, ReliableMessagingTypeName, format, arg0, arg1, arg2);
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        /// <param name="arg3">The fourth object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U, V, W>(bool condition, string format, T arg0, U arg1, V arg2, W arg3)
        {
            Data.Common.Diagnostics.Assert(condition, ReliableMessagingTypeName, format, arg0, arg1, arg2, arg3);
        }

        /// <summary>
        /// Checks for a condition; if the condition is false, it will attempt to break in the debugger 
        /// when the debugger is attached. If the debugger is not attached, it will fail fast.
        /// </summary>
        /// <param name="condition">The conditional expression to evaluate.</param>
        /// <param name="format">Debugging message.</param>
        /// <param name="arg0">The first object to format.</param>
        /// <param name="arg1">The second object to format.</param>
        /// <param name="arg2">The third object to format.</param>
        /// <param name="arg3">The fourth object to format.</param>
        /// <param name="arg4">The fifth object to format.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Assert<T, U, V, W, X>(bool condition, string format, T arg0, U arg1, V arg2, W arg3, X arg4)
        {
            Data.Common.Diagnostics.Assert(condition, ReliableMessagingTypeName, format, arg0, arg1, arg2, arg3, arg4);
        }
    }
}