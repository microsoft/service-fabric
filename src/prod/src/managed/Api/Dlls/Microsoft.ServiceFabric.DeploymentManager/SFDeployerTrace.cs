// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;

    internal class SFDeployerTrace
    {
        private const string TraceType = "SFDeployer";
        private static readonly FabricEvents.ExtensionsEvents TraceSource;

        static SFDeployerTrace()
        {
            SFDeployerTrace.TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.SystemFabricDeployer);
            TraceConfig.SetDefaultLevel(TraceSinkType.Console, EventLevel.Error);
            TraceConfig.AddFilter(TraceSinkType.Console, "SystemFabricDeployer:4");
        }

        public static string LastTraceDirectory { get; private set; }

        public static void UpdateFileLocation(string traceFolder)
        {
            string traceFileName = string.Format(CultureInfo.InvariantCulture, "{0}-{1}.trace", SFDeployerTrace.TraceType, DateTime.Now.Ticks);
            if (!Directory.Exists(traceFolder))
            {
                WriteInfo("Trace folder doesn't exist. Creating trace folder: {0}", traceFolder);
                Directory.CreateDirectory(traceFolder);
            }
            else
            {
                WriteInfo("Trace folder already exists. Traces will be written to existing trace folder: {0}", traceFolder);
            }

            string traceFileFullPath = Path.Combine(traceFolder, traceFileName);
            TraceTextFileSink.SetPath(traceFileFullPath);
            TraceConfig.SetDefaultLevel(TraceSinkType.TextFile, EventLevel.Verbose);
            LastTraceDirectory = traceFolder;
        }

        public static void CloseHandle()
        {
            TraceTextFileSink.SetPath(string.Empty);
            LastTraceDirectory = string.Empty;
        }

        public static void WriteInfo(string format, params object[] args)
        {
            SFDeployerTrace.TraceSource.WriteInfo(SFDeployerTrace.TraceType, format, args);
        }

        public static void WriteWarning(string format, params object[] args)
        {
            SFDeployerTrace.TraceSource.WriteWarning(SFDeployerTrace.TraceType, format, args);
        }

        public static void WriteError(string format, params object[] args)
        {
            SFDeployerTrace.TraceSource.WriteError(SFDeployerTrace.TraceType, format, args);
        }

        internal static void WriteNoise(string format, params object[] args)
        {
            SFDeployerTrace.TraceSource.WriteNoise(SFDeployerTrace.TraceType, format, args);
        }

        private static bool IsTestMode()
        {
            string result = Environment.GetEnvironmentVariable(Microsoft.ServiceFabric.DeploymentManager.Constants.FabricUnitTestNoTraceName);
            int intResult;
            if (result == null || !int.TryParse(result, out intResult) || intResult != 1)
            {
                return false;
            }

            return true;
        }
    }
}