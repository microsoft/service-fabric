// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Diagnostics;    
    using System.Globalization;
    using System.IO;

    using Microsoft.Search.Autopilot;    

    internal static class TextLogger
    {
        private static bool useTestMode = false;

        private static CustomLogID logId = null;

        private static LogLevel minimumLogLevel = LogLevel.Info;

        public static void InitializeLogging(string customLogIdName)
        {
            InitializeLogging(customLogIdName, false, string.Empty, string.Empty, string.Empty);
        }

        public static void InitializeLogging(string customLogIdName, bool toUseTestMode, string testModeLogFileNameTemplate, string testModeLogDirectoryName, string testModeTraceListenerName)
        {
            logId = new CustomLogID(customLogIdName);

            minimumLogLevel = LogLevel.Info;

            useTestMode = toUseTestMode;            

            if (useTestMode)
            {
                string logFileName =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        testModeLogFileNameTemplate,
                        DateTime.Now.Ticks);

                string logDirectory = Path.Combine(Path.GetPathRoot("C:\\"), testModeLogDirectoryName);

                Directory.CreateDirectory(logDirectory);

                string logFile =
                    Path.Combine(
                        logDirectory,
                        logFileName);

                TextWriterTraceListener traceListener = new TextWriterTraceListener(logFile, testModeTraceListenerName);
                traceListener.TraceOutputOptions = TraceOptions.DateTime;

                Trace.AutoFlush = true;
                Trace.Listeners.Add(traceListener);
            }
        }        
        
        public static void SetLogLevel(LogLevel logLevel)
        {
            minimumLogLevel = logLevel;
        }

        public static void LogVerbose(string format, params object[] args)
        {
            Log(LogLevel.Debug, format, args);
        }

        public static void LogInfo(string format, params object[] args)
        {
            Log(LogLevel.Info, format, args);
        }

        public static void LogWarning(string format, params object[] args)
        {
            Log(LogLevel.Warning, format, args);
        }

        public static void LogError(string format, params object[] args)
        {
            Log(LogLevel.Error, format, args);
        }

        public static void Flush()
        {
            if (useTestMode)
            {
                Trace.Flush();
            }
            else
            {
                Microsoft.Search.Autopilot.Logger.Flush();                
            }
        }

        private static void Log(LogLevel logLevel, string format, params object[] args)
        {
            if (logLevel < minimumLogLevel)
            {
                return;
            }

            if (useTestMode)
            {             
                switch (logLevel)
                {
                    case LogLevel.Debug:
                    case LogLevel.Info:
                        Trace.TraceInformation(format, args);
                        break;
                    case LogLevel.Warning:
                        Trace.TraceWarning(format, args);
                        break;
                    case LogLevel.Error:
                        Trace.TraceError(format, args);
                        break;
                    default:                        
                        break;
                }           
            }
            else
            {
                Microsoft.Search.Autopilot.Logger.Log(logId, logLevel, format, args);                
            }            
        }
    }
}