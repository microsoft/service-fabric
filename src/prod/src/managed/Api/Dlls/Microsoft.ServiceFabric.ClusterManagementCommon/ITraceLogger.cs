// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;

    public interface ITraceLogger
    {
        string TraceId { get; set; }

        void WriteNoise(TraceType traceType, string format, params object[] args);

        void WriteInfo(TraceType traceType, string format, params object[] args);

        void WriteWarning(TraceType traceType, string format, params object[] args);

        void WriteError(TraceType traceType, string format, params object[] args);

        void WriteTrace(TraceType traceType, Exception ex, string format, params object[] args);

        void ConsoleWriteLine(TraceType traceType, string format, params object[] args);
    }
}