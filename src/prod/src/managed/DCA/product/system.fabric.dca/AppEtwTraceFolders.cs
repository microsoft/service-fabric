// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using Tools.EtlReader;

    // Class to manage ETW trace folders for applications
    internal static class AppEtwTraceFolders
    {
        private const string TraceType = "AppEtwTraceFolders";
        private const string AppTraceSessionNamePrefix = "FabricApplicationTraces_";
        private const int ErrorWmiInstanceNotFound = 4201;

        private static readonly Dictionary<string, string> AppEtwTraceFolder = new Dictionary<string, string>();

        internal static string GetTraceFolderForNode(string nodeName)
        {
            string traceFolder = null;
            lock (AppEtwTraceFolder)
            {
                if (false == AppEtwTraceFolder.ContainsKey(nodeName))
                {
                    string traceSessionName = string.Concat(AppTraceSessionNamePrefix, nodeName);
                    try
                    {
                        Utility.PerformWithRetries(
                            ctx =>
                            {
                                traceFolder = TraceSession.GetLogFilePath(traceSessionName);
                            },
                            (object)null,
                            new RetriableOperationExceptionHandler(GetLogFilePathExceptionHandler));
                        AppEtwTraceFolder[nodeName] = traceFolder;
                    }
                    catch (Exception e)
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Error occurred while determining the ETL file path for trace session {0}. {1}",
                            traceSessionName,
                            e);
                    }
                }
                else
                {
                    traceFolder = AppEtwTraceFolder[nodeName];
                }
            }

            return traceFolder;
        }

        internal static List<string> GetAll()
        {
            List<string> traceFolders;
            lock (AppEtwTraceFolder)
            {
                traceFolders = new List<string>(AppEtwTraceFolder.Values);
            }

            return traceFolders;
        }

        private static RetriableOperationExceptionHandler.Response GetLogFilePathExceptionHandler(Exception e)
        {
            Win32Exception win32Exception = e as Win32Exception;
            if ((null != win32Exception) &&
                (win32Exception.NativeErrorCode == ErrorWmiInstanceNotFound))
            {
                // Session is not running, so retry
                Utility.TraceSource.WriteWarning(
                    TraceType,
                    "Exception encountered when retrieving log file path for ETW session. Operation will be retried if retry limit has not been reached. Exception information: {0}.",
                    e);
                return RetriableOperationExceptionHandler.Response.Retry;
            }
            else
            {
                // Give up
                return RetriableOperationExceptionHandler.Response.Abort;
            }
        }
    }
}