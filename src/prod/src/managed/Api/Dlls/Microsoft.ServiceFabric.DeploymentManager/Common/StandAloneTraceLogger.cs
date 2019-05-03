// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal class StandAloneTraceLogger : ITraceLogger
    {
        private static readonly FabricEvents.ExtensionsEvents TraceSource =
            FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.WRP);

        public StandAloneTraceLogger()
        {
        }

        public StandAloneTraceLogger(string traceId)
        {
            this.TraceId = traceId;
        }

        public string TraceId
        {
            get;
            set;
        }

        public void WriteTrace(TraceType traceType, Exception ex, string format, params object[] args)
        {
            if (ex == null)
            {
                this.WriteInfo(traceType, format, args);
            }
            else
            {
                this.WriteExceptionAsWarning(traceType, ex, format, args);
            }
        }

        public void WriteNoise(TraceType traceType, string format, params object[] args)
        {
            if (!string.IsNullOrEmpty(this.TraceId))
            {
                TraceSource.WriteNoiseWithId(traceType.Name, this.TraceId, format, args);
            }
            else
            {
                TraceSource.WriteNoise(traceType.Name, format, args);
            }
        }

        public void WriteInfo(TraceType traceType, string format, params object[] args)
        {
            if (!string.IsNullOrEmpty(this.TraceId))
            {
                TraceSource.WriteInfoWithId(traceType.Name, this.TraceId, format, args);
            }
            else
            {
                TraceSource.WriteInfo(traceType.Name, format, args);
            }
        }

        public void WriteWarning(TraceType traceType, string format, params object[] args)
        {
            if (!string.IsNullOrEmpty(this.TraceId))
            {
                TraceSource.WriteWarningWithId(traceType.Name, this.TraceId, format, args);
            }
            else
            {
                TraceSource.WriteWarning(traceType.Name, format, args);
            }
        }

        public void WriteError(TraceType traceType, string format, params object[] args)
        {
            if (!string.IsNullOrEmpty(this.TraceId))
            {
                TraceSource.WriteErrorWithId(traceType.Name, this.TraceId, format, args);
            }
            else
            {
                TraceSource.WriteError(traceType.Name, format, args);
            }
        }

        public void ConsoleWriteLine(TraceType traceType, string format, params object[] args)
        {
            if (!string.IsNullOrEmpty(this.TraceId))
            {
                TraceSource.WriteInfoWithId(traceType.Name, this.TraceId, format, args);
            }
            else
            {
                TraceSource.WriteInfo(traceType.Name, format, args);
            }

            Console.WriteLine(format, args);
        }

        public void WriteExceptionAsWarning(TraceType traceType, Exception ex, string format, params object[] args)
        {
            if (!string.IsNullOrEmpty(this.TraceId))
            {
                TraceSource.WriteExceptionAsWarningWithId(traceType.Name, this.TraceId, ex, format, args);
            }
            else
            {
                TraceSource.WriteExceptionAsWarning(traceType.Name, ex, format, args);
            }
        }
    }
}