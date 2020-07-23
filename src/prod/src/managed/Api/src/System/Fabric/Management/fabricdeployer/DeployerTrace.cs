// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Diagnostics.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;

    class DeployerTrace
    {
        private static readonly FabricEvents.ExtensionsEvents traceSource;

        private const string TraceType = "FabricDeployer";

        static DeployerTrace()
        {
#if DotNetCoreClrLinux
            // TODO - Following code will be removed once fully transitioned to structured traces in Linux
            // this is used to read the configuration which tells whether to use structured traces in Linux.
            TraceConfig.InitializeFromConfigStore();
#endif
            traceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.FabricDeployer);
#if DotNetCoreClrLinux
            TraceConfig.SetDefaultLevel(TraceSinkType.Console, EventLevel.Informational);
#else
            TraceConfig.SetDefaultLevel(TraceSinkType.Console, EventLevel.Error);
#endif
            TraceConfig.SetDefaultLevel(TraceSinkType.ETW, EventLevel.Informational);
        }

        public static void UpdateConsoleLevel(EventLevel level)
        {
            TraceConfig.SetDefaultLevel(TraceSinkType.Console, level);
        }
        
        public static void UpdateFileLocation(string traceFolder)
        {
            string drive = Path.GetPathRoot(traceFolder);
#if DotNetCoreClrLinux
            string traceFileName = string.Format(CultureInfo.InvariantCulture, "FabricDeployer-{0}.trace", DateTime.UtcNow.Ticks);
            if (!FabricDirectory.Exists(traceFolder))
            {
                WriteInfo("Trace folder doesn't exist. Creating trace folder: {0}", traceFolder);
                FabricDirectory.CreateDirectory(traceFolder);
            }
#else
            if (!Directory.Exists(drive))
            {
                string newTraceFolder = Path.Combine(Path.GetTempPath(), "FabricDeployerTraces");
                WriteInfo("Default trace destination does not exist: {0}. Using directory instead: {1}.", traceFolder, newTraceFolder);
                traceFolder = newTraceFolder;
            }

            string traceFileName = string.Format(CultureInfo.InvariantCulture, "FabricDeployer-{0}.trace", DateTime.Now.Ticks);
            if (!Directory.Exists(traceFolder))
            {
                WriteInfo("Trace folder doesn't exist. Creating trace folder: {0}", traceFolder);
                Directory.CreateDirectory(traceFolder);
            }
#endif
            else
            {
                WriteInfo("Trace folder already exists. Traces will be written to existing trace folder: {0}", traceFolder);
            }


            string traceFileFullPath = Path.Combine(traceFolder, traceFileName);

#if DotNetCoreClr
            /* TBD: Fix following code.
            if (deployerFileTraces != null)
            {
                deployerFileTraces.Dispose();
            }
            deployerFileTraces = new FileEventListener(traceFileFullPath);
            deployerFileTraces.EnableEvents(FabricEvents.Events, EventLevel.Informational);
            */
#else
            TraceTextFileSink.SetPath(traceFileFullPath);
            TraceConfig.SetDefaultLevel(TraceSinkType.TextFile, EventLevel.Informational);
#endif
        }

        public static void CloseHandle()
        {
            TraceTextFileSink.SetPath(String.Empty);
        }

        public static void WriteInfo(string format, params object[] args)
        {
            traceSource.WriteInfo(DeployerTrace.TraceType, format, args);            
        }

        public static void WriteWarning(string format, params object[] args)
        {
            traceSource.WriteWarning(DeployerTrace.TraceType, format, args);
        }

        public static void WriteError(string format, params object[] args)
        {
            traceSource.WriteError(DeployerTrace.TraceType, format, args);
        }

        internal static void WriteNoise(string format, params object[] args)
        {
            traceSource.WriteNoise(DeployerTrace.TraceType, format, args);
        }
    }
}