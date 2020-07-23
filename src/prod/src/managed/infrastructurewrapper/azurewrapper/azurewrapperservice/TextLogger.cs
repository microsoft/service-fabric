// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define TRACE

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Diagnostics;       
    using System.Globalization;
    using System.IO;

    internal static class TextLogger
    {                
        private static TraceLevel textLogTraceLevel = TraceLevel.Info;

        public static void InitializeLogging(string textLogRootDirectory, TraceLevel initialTraceLevel)
        {
            textLogTraceLevel = initialTraceLevel;

            string traceFileName =
                string.Format(
                    CultureInfo.InvariantCulture,
                    WindowsFabricAzureWrapperServiceCommon.TextLogFileNameTemplate,
                    DateTime.Now.Ticks);

            string traceFileFullPath =
                Path.Combine(
                    textLogRootDirectory,
                    traceFileName);

            TextWriterTraceListener textWriterTraceListener = new TextWriterTraceListener(traceFileFullPath, WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceTraceListenerName);
            textWriterTraceListener.TraceOutputOptions = TraceOptions.DateTime;

            Trace.AutoFlush = true;
            Trace.Listeners.Add(textWriterTraceListener);

            LogInfo("Trace path : {0}. Initial trace file : {1}", textLogRootDirectory, traceFileFullPath);
        }

        public static void SetTraceLevel(TraceLevel nextTraceLevel)
        {
            textLogTraceLevel = nextTraceLevel;
        }

        public static void LogVerbose(string format, params object[] args)
        {
            if (textLogTraceLevel >= TraceLevel.Verbose)
            {
                Trace.TraceInformation(format, args);
            }
        }

        public static void LogInfo(string format, params object[] args)
        {
            if (textLogTraceLevel >= TraceLevel.Info)
            {
                Trace.TraceInformation(format, args);
            }
        }

        public static void LogWarning(string format, params object[] args)
        {
            if (textLogTraceLevel >= TraceLevel.Warning)
            {
                Trace.TraceWarning(format, args);
            }
        }

        public static void LogError(string format, params object[] args)
        {
            if (textLogTraceLevel >= TraceLevel.Error)
            {
                Trace.TraceError(format, args);
            }
        }

        public static void LogErrorToEventLog(string format, params object[] args)
        {
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            EventLog.WriteEntry(WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName, message, EventLogEntryType.Error);
        }
    }
}