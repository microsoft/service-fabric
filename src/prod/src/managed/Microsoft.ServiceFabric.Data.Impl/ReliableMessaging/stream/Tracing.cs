// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    using System.Fabric.Common;

    internal class Tracing
    {
        private static string traceTypeBase = "ReliableStream.";

        // TODO: differentiate between noise and info for stable tracing
        internal static void WriteNoise(string type, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteInfo(traceTypeBase + type, format, args);
        }

        internal static void WriteInfo(string type, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteInfo(traceTypeBase + type, format, args);
        }

        internal static void WriteWarning(string type, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteWarning(traceTypeBase + type, format, args);
        }

        internal static void WriteError(string type, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteError(traceTypeBase + type, format, args);
        }

        internal static void WriteExceptionAsWarning(string type, Exception e, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteExceptionAsWarning(traceTypeBase + type, e, format, args);
        }

        internal static void WriteExceptionAsError(string type, Exception e, string format, params object[] args)
        {
            AppTrace.TraceSource.WriteExceptionAsError(traceTypeBase + type, e, format, args);
        }
    }
}