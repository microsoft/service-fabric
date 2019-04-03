// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca.Exceptions;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Threading;
    using Microsoft.Win32;

    // This class implements miscellaneous utility functions
    public static class Utility
    {
        // Constants
        internal const string WindowsFabricApplicationInstanceId = "WindowsFabric";
        internal const string ShortWindowsFabricIdForPaths = "WFab";

        internal const string FmmTaskName = "FMM";

        internal const string EtwConsumerWorkSubFolderIdPrefix = "etwConsumer:";
        internal const string CsvConsumerWorkSubFolderIdPrefix = "csvConsumer:";
        internal const string AppDiagnosticStoreAccessRequiresImpersonationValueName = "AppDiagnosticStoreAccessRequiresImpersonation";
        internal const string TestOnlyDtrDeletionAgeMinutesValueName = "TestOnlyDtrDeletionAgeInMinutes";

        // We need to ignore a few linux specific files for kernel
        // crash dumps.
        internal static readonly List<string> IgnoreUploadFileList = new List<string>() { "kexec_cmd" };

        // Timestamp up to which application ETW traces should be decoded
        internal static readonly object ApplicationEtwTracesEndTimeLock = new object();

        // Map between the name and number for ETW levels
        internal static readonly Dictionary<string, string> EtwLevelNameToNumber = new Dictionary<string, string>()
        {
            { "Error", "2" },
            { "Warning", "3" },
            { "Informational", "4" },
            { "Verbose", "5" },
        };

        private const string FabricExeWERKeyName = "SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\Fabric.exe";
        private const string DumpFolderValueName = "DumpFolder";

        private const string TraceType = "Utility";
        private const string FabricNodeSectionName = "FabricNode";
        private const string NodeIdValueName = "NodeId";
        private const string DcaWorkFolderName = "work";
        private const string DcaTempFolderName = "temp";
        private const string TempFileNameExtension = ".tmp";
        private const string NodeInstanceValueName = "InstanceName";
        private const string MapFileName = "WorkSubFolderMap.dat";
        private const int MapFileFormatVersion = 1;
        private const string PluginFlushTimeoutSecondsValueName = "PluginFlushTimeoutInSeconds";
        private const int DefaultPluginFlushTimeoutSeconds = 180;

#if !DotNetCoreClrLinux
        // Directory for Windows Fabric crash dumps, i.e. %FabricDataRoot%\log\CrashDumps
        private static readonly Lazy<string> CrashDumpsDirectoryLazy = new Lazy<string>(() => (string)GetRegistryValue(
                                                      Registry.LocalMachine,
                                                      FabricExeWERKeyName,
                                                      DumpFolderValueName));
#endif

        private static readonly object FmmEventTimestampLock = new object();

        private static Lazy<string> logDirectoryLazy = new Lazy<string>(
            () =>
            {
                string logRoot;
                try
                {
                    logRoot = FabricEnvironment.GetLogRoot();
                }
                catch (FabricException)
                {
                    logRoot = Path.Combine(FabricEnvironment.GetDataRoot(), "log");
                }

                TraceSource.WriteInfo(
                    TraceType,
                    "Log directory: {0}",
                    logRoot);
                return logRoot;
            });

        private static DateTime applicationEtwTracesEndTime;
        private static DateTime lastFmmEventTimestamp = DateTime.MaxValue;

        // Config store
        private static IConfigStore configStore;

        // Delegate representing the methods used to customize the deletion of files
        // from a folder
        internal delegate bool ShouldAbortDeletion();

        internal static DateTime LastFmmEventTimestamp
        {
            get
            {
                lock (FmmEventTimestampLock)
                {
                    if (lastFmmEventTimestamp.Equals(DateTime.MaxValue))
                    {
                        throw new System.NotImplementedException();
                    }

                    return lastFmmEventTimestamp;
                }
            }

            set
            {
                lock (FmmEventTimestampLock)
                {
                    // Update only if the caller's timestamp is more recent
                    // or if the current timestamp if the initial value of
                    // DateTime.MaxValue;
                    if ((value.CompareTo(lastFmmEventTimestamp) > 0) ||
                        lastFmmEventTimestamp.Equals(DateTime.MaxValue))
                    {
                        lastFmmEventTimestamp = value;
                    }
                }
            }
        }

        // Reference to the object that implements tracing
        internal static FabricEvents.ExtensionsEvents TraceSource { get; set; }

        // Fabric node ID
        internal static string FabricNodeId { get; private set; }

        // Fabric node name
        internal static string FabricNodeName { get; private set; }

        // The Windows Fabric log directory, i.e. %FabricDataRoot%\log
        internal static string LogDirectory
        {
            get
            {
                return logDirectoryLazy.Value;
            }

            set
            {
                // This is a workaround for tests which change LogDirectory after initialization.
                logDirectoryLazy = new Lazy<string>(
                    () =>
                    {
                        // This might not be initialized yet.
                        if (TraceSource != null)
                        {
                            TraceSource.WriteInfo(TraceType, "Log directory: {0}", value);
                        }

                        return value;
                    });
            }
        }

        internal static string CrashDumpsDirectory
        {
#if DotNetCoreClrLinux
            get { return Path.Combine(Utility.LogDirectory, "CrashDumps"); }
#else
            get { return CrashDumpsDirectoryLazy.Value; }
#endif
        }

        internal static string DcaWorkFolder { get; private set; }

        internal static string DcaProgramDirectory { get; set; }

        internal static DateTime ApplicationEtwTracesEndTime
        {
            get
            {
                lock (ApplicationEtwTracesEndTimeLock)
                {
                    return applicationEtwTracesEndTime;
                }
            }

            set
            {
                lock (ApplicationEtwTracesEndTimeLock)
                {
                    applicationEtwTracesEndTime = value;
                }
            }
        }

        private static string DcaTempFolder { get; set; }

        public static byte[] SecureStringToByteArray(SecureString secureString)
        {
            char[] charArray = new char[secureString.Length];
            byte[] byteArray = null;
#if !DotNetCoreClrLinux
            IntPtr ptr = Marshal.SecureStringToGlobalAllocUnicode(secureString);
#else
            IntPtr ptr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(secureString);
#endif
            try
            {
                Marshal.Copy(ptr, charArray, 0, secureString.Length);

                // Since the chars are base 64 encoded, calling the following to convert to byte array
                byteArray = Convert.FromBase64CharArray(charArray, 0, charArray.Length);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(ptr);
                Array.Clear(charArray, 0, charArray.Length);
            }

            return byteArray;
        }

        internal static bool IsSystemApplicationInstanceId(string applicationInstanceId)
        {
            return WindowsFabricApplicationInstanceId == applicationInstanceId;
        }

        internal static void InitializeFabricNodeInfo()
        {
            FabricNodeId = configStore.ReadUnencryptedString(
                               FabricNodeSectionName,
                               NodeIdValueName);
            FabricNodeName = configStore.ReadUnencryptedString(
                                   FabricNodeSectionName,
                                   NodeInstanceValueName);
        }

        internal static void InitializeWorkDirectory()
        {
            // Create a sub-folder for DCA under the work directory
            DcaWorkFolder = Path.Combine(LogDirectory, DcaWorkFolderName);
            FabricDirectory.CreateDirectory(DcaWorkFolder);

            // Create a sub-folder for temporary files under the DCA work directory
            DcaTempFolder = Path.Combine(DcaWorkFolder, DcaTempFolderName);
            FabricDirectory.CreateDirectory(DcaTempFolder);
        }

        internal static string GetOrCreateContainerWorkDirectory(string containerLogDirectory)
        {
            // Create a sub-folder for work under the container log directory
            string workFolder = Path.Combine(containerLogDirectory, DcaWorkFolderName);
            FabricDirectory.CreateDirectory(workFolder);

            return workFolder;
        }

        internal static string GetTempFileName()
        {
            string tempFileName = string.Concat(
                                    Guid.NewGuid().ToString("B"),
                                    TempFileNameExtension);
            string tempFilePath = Path.Combine(
                                    DcaTempFolder,
                                    tempFileName);
            return tempFilePath;
        }

        internal static string GetTempDirName()
        {
            string tempDirPath = Path.Combine(
                                    DcaTempFolder,
                                    Guid.NewGuid().ToString("B"));
            return tempDirPath;
        }

        internal static void InitializeConfigStore(IConfigStoreUpdateHandler updateHandler)
        {
            configStore = NativeConfigStore.FabricGetConfigStore(updateHandler);
        }

        internal static void InitializeConfigStore(IConfigStore newConfigStore)
        {
            configStore = newConfigStore;
        }

        internal static void InitializeTracing()
        {
            TraceConfig.InitializeFromConfigStore();
        }

        internal static T GetUnencryptedConfigValue<T>(string sectionName, string valueName, T defaultValue)
        {
            string newValue = configStore.ReadUnencryptedString(sectionName, valueName);

            if (string.IsNullOrEmpty(newValue))
            {
                return defaultValue;
            }
            else
            {
                try
                {
                    return (T)TypeDescriptor.GetConverter(typeof(T)).ConvertFromString(null, CultureInfo.InvariantCulture, newValue);
                }
                catch (Exception ex)
                {
                    string message = string.Format(
                        "Could not convert ClusterManifest entry at SectionName = {0}, ValueName = {1}, value = {2} to type {3}. Exception details : {4}",
                            sectionName,
                            valueName,
                            newValue,
                            typeof(T).Name,
                            ex.Message);
                    throw new ConfigurationException(message, ex);
                }
            }
        }

        internal static string ReadConfigValue(string sectionName, string valueName, out bool isEncrypted)
        {
            return configStore.ReadString(sectionName, valueName, out isEncrypted);
        }

        internal static bool IsNetworkError(int httpStatusCode)
        {
            const int HttpCodeInternalServerError = 500;
            const int HttpCodeBadGateway = 502;
            const int HttpCodeServiceUnavailable = 503;
            const int HttpCodeGatewayTimeout = 504;

            return (httpStatusCode == HttpCodeInternalServerError) ||
                    (httpStatusCode == HttpCodeBadGateway) ||
                    (httpStatusCode == HttpCodeServiceUnavailable) ||
                    (httpStatusCode == HttpCodeGatewayTimeout);
        }

        internal static bool IsNetworkError(HttpStatusCode httpStatusCode)
        {
            return (httpStatusCode == HttpStatusCode.InternalServerError) ||
                    (httpStatusCode == HttpStatusCode.BadGateway) ||
                    (httpStatusCode == HttpStatusCode.ServiceUnavailable) ||
                    (httpStatusCode == HttpStatusCode.GatewayTimeout);
        }

        internal static void PerformWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler)
        {
            const int InitialRetryIntervalMs = 10000; // 10 seconds
            const int MaxRetryCount = 3;
            const int MaxRetryIntervalMs = int.MaxValue;

            PerformWithRetries(worker, context, exceptionHandler, InitialRetryIntervalMs, MaxRetryCount, MaxRetryIntervalMs);
        }

        internal static void PerformWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler, int maxRetryCount)
        {
            const int InitialRetryIntervalMs = 10000; // 10 seconds
            const int MaxRetryIntervalMs = int.MaxValue;

            PerformWithRetries(worker, context, exceptionHandler, InitialRetryIntervalMs, maxRetryCount, MaxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries(Action worker, int maxRetryCount)
        {
            const int InitialRetryIntervalMs = 10000; // 10 seconds
            const int MaxRetryIntervalMs = int.MaxValue;
            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       InitialRetryIntervalMs,
                       maxRetryCount,
                       MaxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries(Action worker, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       initialRetryIntervalMs,
                       maxRetryCount,
                       maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries<T>(Action<T> worker, T context, int maxRetryCount)
        {
            const int InitialRetryIntervalMs = 10000; // 10 seconds
            const int MaxRetryIntervalMs = int.MaxValue;
            PerformWithRetries(
                       worker,
                       context,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       InitialRetryIntervalMs,
                       maxRetryCount,
                       MaxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries<T>(Action<T> worker, T context, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            PerformWithRetries(
                       worker,
                       context,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       initialRetryIntervalMs,
                       maxRetryCount,
                       maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries(Action worker)
        {
            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler));
        }

        internal static void PerformIOWithRetries<T>(Action<T> worker, T context)
        {
            PerformWithRetries(
                       worker,
                       context,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler));
        }

        internal static void PerformInitializationWithRetries(Action worker, RetriableOperationExceptionHandler exceptionHandler)
        {
            // If a failure occurs during DCA initialization, we retry the operation
            // persistently as compared to a failure in the steady state where we give
            // up after a certain number of retries.
            // This is because an initialization failure means that the DCA will not
            // upload any traces to the destination, which is a lot more severe compared
            // to a failure in the steady state where only a subset of the traces might
            // be lost.
            const int InitialRetryIntervalMs = 10000; // 10 seconds
            const int RetryCount = int.MaxValue;
            const int MaxRetryIntervalMs = 15 * 60 * 1000; // 15 minutes
            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       exceptionHandler,
                       InitialRetryIntervalMs,
                       RetryCount,
                       MaxRetryIntervalMs);
        }

        internal static void PerformInitializationWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler)
        {
            // If a failure occurs during DCA initialization, we retry the operation
            // persistently as compared to a failure in the steady state where we give
            // up after a certain number of retries.
            // This is because an initialization failure means that the DCA will not
            // upload any traces to the destination, which is a lot more severe compared
            // to a failure in the steady state where only a subset of the traces might
            // be lost.
            const int InitialRetryIntervalMs = 10000; // 10 seconds
            const int RetryCount = int.MaxValue;
            const int MaxRetryIntervalMs = 15 * 60 * 1000; // 15 minutes
            PerformWithRetries(
                       worker,
                       context,
                       exceptionHandler,
                       InitialRetryIntervalMs,
                       RetryCount,
                       MaxRetryIntervalMs);
        }

        internal static IEnumerable<FileInfo> DeleteOldFilesFromFolder(
                                 string traceType,
                                 string folderPath,
                                 string fileNamePattern,
                                 DateTime cutoffTime,
                                 ShouldAbortDeletion shouldAbortDeletion,
                                 bool dontDeleteNewestFile)
        {
            if (false == FabricDirectory.Exists(folderPath))
            {
                // Folder does not exist. Nothing more to be done here.
                return new FileInfo[0];
            }

            TraceSource.WriteInfo(
                traceType,
                "Files in folder {0}{1} with last write time older than {2} ({3}) will be deleted.",
                folderPath,
                string.IsNullOrEmpty(fileNamePattern) ? string.Empty : string.Concat(" that match the pattern ", fileNamePattern, " and"),
                cutoffTime,
                cutoffTime.Ticks);

            // Get the ETL files from the ETL directory
            DirectoryInfo dirInfo = new DirectoryInfo(folderPath);
            IEnumerable<FileInfo> files = string.IsNullOrEmpty(fileNamePattern) ?
                                            dirInfo.EnumerateFiles() :
                                            dirInfo.EnumerateFiles(fileNamePattern);

            if (dontDeleteNewestFile)
            {
                // Sort the files by creation time, such that the most recent
                // file is first.
                FileInfo[] enumeratedFiles = files.ToArray();
                Array.Sort(enumeratedFiles, CompareFileCreationTimesNewFirst);

                if (enumeratedFiles.Length > 1)
                {
                    // Exclude the file with the most recent creation time as specified
                    // by the caller.
                    FileInfo[] filesExceptNewest = new FileInfo[enumeratedFiles.Length - 1];
                    Array.Copy(enumeratedFiles, 1, filesExceptNewest, 0, enumeratedFiles.Length - 1);
                    files = filesExceptNewest;
                }
                else
                {
                    // This folder has at most one file.
                    // If there is only one file, it is the newest file by default. Since
                    // we have been asked not to delete the newest file, there is nothing
                    // more to be done here.
                    return new FileInfo[0];
                }
            }

            // Remove the files that are not old enough to be deleted
            files = files
                    .Where(file => file.LastWriteTimeUtc.CompareTo(cutoffTime) < 0);

            // Delete the remaining files
            return DeleteFiles(traceType, files, shouldAbortDeletion);
        }

        internal static IEnumerable<FileInfo> DeleteFiles(string traceType, IEnumerable<FileInfo> files, ShouldAbortDeletion shouldAbortDeletion)
        {
            return DeleteFiles(traceType, files, shouldAbortDeletion, uint.MaxValue);
        }

        internal static bool IsImpersonationRequiredForAppDiagnosticStoreAccess()
        {
            return GetUnencryptedConfigValue(
                        ConfigReader.DiagnosticsSectionName,
                        AppDiagnosticStoreAccessRequiresImpersonationValueName,
                        true);
        }

        internal static TimeSpan GetPluginFlushTimeout()
        {
            return TimeSpan.FromSeconds(GetUnencryptedConfigValue(
                        ConfigReader.DiagnosticsSectionName,
                        PluginFlushTimeoutSecondsValueName,
                        DefaultPluginFlushTimeoutSeconds));
        }

        internal static void MoveFile(
                        string traceType,
                        string srcFile,
                        string dstFile)
        {
            PerformIOWithRetries(() =>
            {
                FabricFile.Move(srcFile, dstFile);
            });
        }

#if !DotNetCoreClrLinux
        internal static object GetRegistryValue(RegistryKey key, string subKeyName, string valueName, bool valueMustBePresent = true)
        {
            RegistryKey subKey = null;
            object value;
            try
            {
                subKey = key.OpenSubKey(subKeyName);
                if (null == subKey)
                {
                    if (valueMustBePresent)
                    {
                        TraceSource.WriteError(
                            TraceType,
                            "Unable to open registry key. Key name: {0}, Sub-key name: {1}.",
                            key.Name,
                            subKeyName);
                    }
                    else
                    {
                        TraceSource.WriteInfo(
                            TraceType,
                            "Unable to open registry key. Key name: {0}, Sub-key name: {1}.",
                            key.Name,
                            subKeyName);
                    }

                    return null;
                }

                value = subKey.GetValue(valueName);
                if (null == value)
                {
                    if (valueMustBePresent)
                    {
                        TraceSource.WriteError(
                            TraceType,
                            "Unable to read registry value. Key name: {0}, Sub-key name: {1}, Value name: {2}",
                            key.Name,
                            subKeyName,
                            valueName);
                    }
                    else
                    {
                        TraceSource.WriteInfo(
                            TraceType,
                            "Unable to read registry value. Key name: {0}, Sub-key name: {1}, Value name: {2}",
                            key.Name,
                            subKeyName,
                            valueName);
                    }

                    return null;
                }
            }
            finally
            {
                if (null != subKey)
                {
                    subKey.Dispose();
                }
            }

            return value;
        }
#endif

        internal static bool AreFoldersAppInstanceSpecific(IEnumerable<string> folderNames, string applicationLogFolder, out bool appInstanceSpecificFolders)
        {
            int i = 0;
            appInstanceSpecificFolders = true;
            foreach (string folderName in folderNames)
            {
                // Figure out whether the current folder is specific to the application instance
                bool currentFolderAppInstanceSpecific = IsFolderAppInstanceSpecific(folderName, applicationLogFolder);
                if (i == 0)
                {
                    // We expected either all folders to be app instance specific or none of them.
                    // So the result for the first folder is the result that we return to the caller.
                    // In the remaining iterations, we'll verify that the results for the remaining
                    // folders are the same as that for the first folder.
                    appInstanceSpecificFolders = currentFolderAppInstanceSpecific;
                }
                else
                {
                    if (currentFolderAppInstanceSpecific != appInstanceSpecificFolders)
                    {
                        TraceSource.WriteError(
                            TraceType,
                            "Either all source folders should be app instance specific, or none of them should be. Folder list: {0}",
                            string.Join(", ", folderNames.ToArray()));
                        return false;
                    }
                }

                i++;
            }

            return true;
        }

        internal static bool CreateWorkSubDirectory(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                string level1Key,
                                string level2Key,
                                string parentFolder,
                                out string workFolder)
        {
            workFolder = null;
            string level1WorkFolder;
            bool success = CreateWorkSubDirectory(
                                traceSource,
                                logSourceId,
                                level1Key,
                                parentFolder,
                                out level1WorkFolder);
            if (success)
            {
                success = CreateWorkSubDirectory(
                                traceSource,
                                logSourceId,
                                level2Key,
                                level1WorkFolder,
                                out workFolder);
            }

            return success;
        }

        internal static bool CreateWorkSubDirectory(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                string key,
                                string parentFolder,
                                out string workFolder)
        {
            workFolder = null;

            FabricDirectory.CreateDirectory(parentFolder);
            string workSubFolderName;
            if (false == GetSubDirectoryNameFromMap(
                            traceSource,
                            logSourceId,
                            key,
                            parentFolder,
                            out workSubFolderName))
            {
                return false;
            }

            workFolder = Path.Combine(parentFolder, workSubFolderName);
            FabricDirectory.CreateDirectory(workFolder);
            return true;
        }

        internal static bool GetSubDirectoryNameFromMap(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                string key,
                                string parentFolder,
                                string mapFileName,
                                int mapFileFormatVersion,
                                out string subFolderName)
        {
            subFolderName = null;
            string workSubFolderMapFile = Path.Combine(parentFolder, mapFileName);
            PersistedIndex<string> workSubFolderMap = new PersistedIndex<string>(
                                                            traceSource,
                                                            logSourceId,
                                                            StringComparer.OrdinalIgnoreCase,
                                                            workSubFolderMapFile,
                                                            mapFileFormatVersion);
            int workSubFolderIndex;
            if (false == workSubFolderMap.GetIndex(key, out workSubFolderIndex))
            {
                return false;
            }

            subFolderName = workSubFolderIndex.ToString(CultureInfo.InvariantCulture);
            return true;
        }

        internal static void PerformWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            int retryCount = 0;
            int retryIntervalMs = initialRetryIntervalMs;

            for (;;)
            {
                try
                {
                    // Perform the operation
                    worker(context);
                    break;
                }
                catch (Exception e)
                {
                    // Invoke the exception handler if present
                    RetriableOperationExceptionHandler.Response response = exceptionHandler.Handler(e);

                    if (RetriableOperationExceptionHandler.Response.Abort == response)
                    {
                        // The nature of the failure is such that we should not retry
                        throw;
                    }

                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        Thread.Sleep(retryIntervalMs);

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here.
                        // The caller is responsible for ensuring that this doesn't overflow by providing a
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        // The operation failed even after the maximum number of retries
                        throw;
                    }
                }
            }
        }

        private static RetriableOperationExceptionHandler.Response RetriableIOExceptionHandler(Exception e)
        {
            if ((e is IOException) || (e is FabricException))
            {
                // If the path is too long, there's no point retrying. Otherwise,
                // we can retry.
                if (false == (e is PathTooLongException))
                {
                    // Retry
                    TraceSource.WriteWarning(
                        TraceType,
                        "Exception encountered when performing retriable I/O operation. Operation will be retried if retry limit has not been reached. Exception information: {0}.",
                        e);
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }

            // Give up
            TraceSource.WriteError(
                TraceType,
                "Exception encountered when performing retriable I/O operation. Exception information: {0}.",
                e);
            return RetriableOperationExceptionHandler.Response.Abort;
        }

        private static int CompareFileCreationTimesNewFirst(FileInfo file1, FileInfo file2)
        {
            // We want the file with the newest creation time to come first
            return DateTime.Compare(file2.CreationTime, file1.CreationTime);
        }

        private static IEnumerable<FileInfo> DeleteFiles(
                                string traceType,
                                IEnumerable<FileInfo> files,
                                ShouldAbortDeletion shouldAbortDeletion,
                                long minDiskSpaceToFreeMb)
        {
            ulong diskSpaceFreed = 0;
            ulong minDiskSpaceToFree = ((ulong)minDiskSpaceToFreeMb) * 1024 * 1024;
            var successfullyDeleted = new List<FileInfo>();

            // Walk through each file and delete it
            foreach (FileInfo fileToDelete in files)
            {
                ulong fileSize = (ulong)fileToDelete.Length;
                try
                {
                    PerformIOWithRetries(
                        () =>
                        {
                            FabricFile.Delete(fileToDelete.FullName);
                        });
                    diskSpaceFreed += fileSize;
                    successfullyDeleted.Add(fileToDelete);
                }
                catch (Exception e)
                {
                    // Deletion is best-effort. It can fail in some situations,
                    // e.g. someone has a handle open to the file. In that case
                    // we just log an error and continue.
                    TraceSource.WriteError(
                        traceType,
                        "Failed to delete old file {0}. {1}",
                        fileToDelete.FullName,
                        e);
                }

                if (diskSpaceFreed > minDiskSpaceToFree)
                {
                    TraceSource.WriteInfo(
                        traceType,
                        "We have freed {0} bytes of disk space by deleting old files. No more old files will be deleted at this time.",
                        diskSpaceFreed);
                    break;
                }

                if (shouldAbortDeletion())
                {
                    TraceSource.WriteInfo(
                        traceType,
                        "No more old files will be deleted because the component is being stopped.");
                    break;
                }
            }

            return successfullyDeleted;
        }

        private static bool IsFolderAppInstanceSpecific(string folderName, string applicationLogFolder)
        {
            do
            {
                if (folderName.Equals(applicationLogFolder, StringComparison.OrdinalIgnoreCase))
                {
                    // The folder is a subdirectory of the log folder of the current application
                    // instance. Hence it is an app-instance-specific folder.
                    return true;
                }

                folderName = FabricPath.GetDirectoryName(folderName);
            }
            while (false == string.IsNullOrEmpty(folderName));

            // The folder is not a subdirectory of the log folder of the current application
            // instance. Hence it is not an app-instance-specific folder.
            return false;
        }

        private static bool GetSubDirectoryNameFromMap(
                                FabricEvents.ExtensionsEvents traceSource,
                                string logSourceId,
                                string key,
                                string parentFolder,
                                out string subFolderName)
        {
            return GetSubDirectoryNameFromMap(
                        traceSource,
                        logSourceId,
                        key,
                        parentFolder,
                        MapFileName,
                        MapFileFormatVersion,
                        out subFolderName);
        }
    }
}