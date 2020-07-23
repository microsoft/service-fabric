// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System;

    internal class MockupTraceLogger : ITraceLogger
    {
        public MockupTraceLogger()
        {

        }

        public string TraceId
        {
            get;
            set;
        }

        public void WriteTrace(TraceType traceType, Exception ex, string format, params object[] args)
        {

        }

        public void WriteNoise(TraceType traceType, string format, params object[] args)
        {

        }

        public void WriteInfo(TraceType traceType, string format, params object[] args)
        {

        }

        public void WriteWarning(TraceType traceType, string format, params object[] args)
        {

        }

        public void WriteError(TraceType traceType, string format, params object[] args)
        {

        }

        public void ConsoleWriteLine(TraceType traceType, string format, params object[] args)
        {

        }

        public void WriteExceptionAsWarning(TraceType traceType, Exception ex, string format, params object[] args)
        {

        }
    }
}