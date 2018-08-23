// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Fabric.Common.Tracing;

    /// <summary>
    /// Maintains structured trace events for the state manager.
    /// </summary>
    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1401:FieldsMustBePrivate", Justification = "related classes")]
    internal class StateManagerStructuredEvents
    {
        internal static void TraceException(string component, string method, string message, Exception exception)
        {
            var traceMessage = GetExceptionTraceMessage(method, message, exception);
            FabricEvents.Events.Exception_TStateManager(component, traceMessage);
        }

        internal static void TraceExceptionWarning(string component, string method, string message, Exception exception)
        {
            var traceMessage = GetExceptionTraceMessage(method, message, exception);
            FabricEvents.Events.ExceptionWarning(component, traceMessage);
        }

        private static string GetExceptionTraceMessage(string method, string message, Exception exception)
        {
            int innerHResult = 0;
            var flattenedException = Utility.FlattenException(exception, out innerHResult);
            var traceMessage = string.Format(
                CultureInfo.InvariantCulture,
                "Exception in {0}. {1} Type: {2} Message: {3} HResult: 0x{4:X8}. Stack Trace: {5}.",
                method,
                message,
                flattenedException.GetType().ToString(),
                flattenedException.Message,
                flattenedException.HResult != 0 ? flattenedException.HResult : innerHResult,
                flattenedException.StackTrace);
            return traceMessage;
        }
    }
}