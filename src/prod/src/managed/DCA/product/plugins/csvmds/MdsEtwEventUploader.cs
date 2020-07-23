// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using MdsHelper;
    using Microsoft.Cis.Monitoring.Tables;
    using System;
    using System.IO;
    using System.Linq;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Diagnostics;
    using System.Security;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Fabric.Dca;
    using System.Fabric.Dca.Utility;

    // This class implements the upload of Windows Fabric ETW traces to MDS
    public class MdsEtwEventUploader : IDcaConsumer
    {
        // Settings related to upload of data to MDS
        struct MdsUploadSettings
        {
            internal bool Enabled;
            internal string DirectoryName;
            internal string TableName;
            internal TablePriority TablePriority;
            internal long UploadIntervalMinutes;
            internal int DiskQuotaInMB;
            internal long DataDeletionAgeMinutes;
            internal string Filter;
        };

        private static string[] mdsTableFields = new string[]
        {
            "ETWTimestamp",
            "Level",
            "ProcessId",
            "ThreadId",
            "TaskName",
            "EventType",
            "Id",
            "Text",
        };

        private static TableFieldType[] mdsTableFieldTypes = new TableFieldType[]
        {
            TableFieldType.TableTypeUTC,
            TableFieldType.TableTypeWStr,
            TableFieldType.TableTypeInt32,
            TableFieldType.TableTypeInt32,
            TableFieldType.TableTypeWStr,
            TableFieldType.TableTypeWStr,
            TableFieldType.TableTypeWStr,
            TableFieldType.TableTypeWStr,
        };

        // Constants
        private const string DtrDirName = "Log";
        private const string BookmarkDirName = "BMk";
        private const string BookmarkFileName = "LastReadFile.dat";
        private const string BookmarkFileFormatVersionString = "Version: 1";
        private const int EventSizeInCharsHint = 2048;
        internal const string TableSchemaVersion = "SV1";
        internal const string DtrReadTimerIdSuffix = "_DtrReadTimer";

        // Tag used to represent the source of the log message
        private string logSourceId;
        
        // Object used for tracing
        private FabricEvents.ExtensionsEvents traceSource;

        // Consumer initialization parameters
        private ConsumerInitializationParameters initParam;

        // Settings for uploading data to MDS
        private MdsUploadSettings mdsUploadSettings;

        // Configuration reader
        private ConfigReader configReader;

        // Folder to hold CSV files containing ETW events
        private string CsvFolder;

        // Bookmark folders for DTR directories
        private Dictionary<string, string> bookmarkFolders;

        // MDS table writers for DTR directories
        private Dictionary<string, MdsTableWriter> tableWriters;

        // Timer used to read CSV files periodically
        private DcaTimer dtrReadTimer;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        // Regular expression describing valid table names
        private static Regex regEx = new Regex(MdsConstants.TableNameRegularExpression);

        // List of table paths across all instances of this plugin
        private static HashSet<string> tablePaths = new HashSet<string>();

        // Whether or not we should finish processing all pending data 
        // during dispose
        public bool FlushDataOnDispose { set; private get; }

        public bool Initialize(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.FlushDataOnDispose = false;
            this.stopping = false;
            this.initParam = initParam;
            this.logSourceId = String.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = FabricEvents.Extensions;
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);
            this.bookmarkFolders = new Dictionary<string, string>();
            this.tableWriters = new Dictionary<string, MdsTableWriter>();

            // Read MDS-specific settings from settings.xml
            GetSettings();
            if (false == this.mdsUploadSettings.Enabled)
            {
                // Upload to MDS tables is not enabled, so return immediately
                return true;
            }

            // Create a sub-directory for the ETL-to-CSV writer under the log directory
            bool success = GetEtwCsvSubDirectory();

            if (success)
            {
                // Start timer for reading events from the CSV files.
                string timerId = String.Concat(
                                     this.logSourceId,
                                     DtrReadTimerIdSuffix);
                this.dtrReadTimer = new DcaTimer(
                                            timerId,
                                            this.ReadDtrFiles,
                                            this.mdsUploadSettings.UploadIntervalMinutes * 60 * 1000);
                this.dtrReadTimer.Start();

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Upload to MDS is configured. Local CSV path: {0}, Table path: {1}, Table name: {2}" +
                    "Upload interval (minutes): {3}, Table priority: {4}, Disk quota (MB): {5}, Data deletion age (minutes): {6}",
                    this.CsvFolder,
                    this.mdsUploadSettings.DirectoryName,
                    this.mdsUploadSettings.TableName,
                    this.mdsUploadSettings.UploadIntervalMinutes,
                    this.mdsUploadSettings.TablePriority,
                    this.mdsUploadSettings.DiskQuotaInMB,
                    this.mdsUploadSettings.DataDeletionAgeMinutes);
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Windows Fabric event filters for MDS upload: {0}",
                    this.mdsUploadSettings.Filter);
            }

            return true;
        }

        public object GetDataSink()
        {
            return null;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;

            // Keep track of the fact that the consumer is stopping
            this.stopping = true;

            if (null != this.dtrReadTimer)
            {
                // Stop the timer that triggers periodic read of ETW events
                this.dtrReadTimer.StopAndDispose();
                this.dtrReadTimer.DisposedEvent.WaitOne();
            }

            foreach (MdsTableWriter tableWriter in this.tableWriters.Values)
            {
                // Tell the table writer to stop
                tableWriter.Dispose();
            }

            lock (tablePaths)
            {
                tablePaths.Remove(
                    Path.Combine(
                        this.mdsUploadSettings.DirectoryName, 
                        this.mdsUploadSettings.TableName));
            }

            GC.SuppressFinalize(this);
            return;
        }

        private void GetSettings()
        {
            // Check for values in settings.xml
            this.mdsUploadSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                 this.initParam.SectionName,
                                                 MdsConstants.EnabledParamName,
                                                 MdsConstants.MdsUploadEnabledByDefault);
            if (this.mdsUploadSettings.Enabled)
            {
                if (false == this.GetOutputDirectoryName(out this.mdsUploadSettings.DirectoryName))
                {
                    this.mdsUploadSettings.Enabled = false;
                    return;
                }

                this.mdsUploadSettings.TableName = this.configReader.GetUnencryptedConfigValue(
                                                       this.initParam.SectionName,
                                                       MdsConstants.TableParamName,
                                                       MdsConstants.DefaultTableName);
                lock (tablePaths)
                {
                    string tablePath = Path.Combine(
                                           this.mdsUploadSettings.DirectoryName,
                                           this.mdsUploadSettings.TableName);
                    if (tablePaths.Contains(tablePath))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The table {0} - specified via section {1}, parameters {2} and {3} - is already in use by another plugin.",
                            tablePath,
                            this.initParam.SectionName,
                            MdsConstants.DirectoryParamName,
                            MdsConstants.TableParamName);
                        this.mdsUploadSettings.Enabled = false;
                        return;
                    }
                    tablePaths.Add(tablePath);
                }

                this.mdsUploadSettings.UploadIntervalMinutes = this.configReader.GetUnencryptedConfigValue(
                                                                   this.initParam.SectionName,
                                                                   MdsConstants.UploadIntervalParamName,
                                                                   MdsConstants.DefaultUploadIntervalMinutes);
                if (this.mdsUploadSettings.UploadIntervalMinutes > MdsConstants.MaxUploadIntervalMinutes)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        this.mdsUploadSettings.UploadIntervalMinutes,
                        this.initParam.SectionName,
                        MdsConstants.UploadIntervalParamName,
                        MdsConstants.MaxUploadIntervalMinutes);
                    this.mdsUploadSettings.UploadIntervalMinutes = MdsConstants.MaxUploadIntervalMinutes;
                }
                this.mdsUploadSettings.TablePriority = GetTablePriority();
                this.mdsUploadSettings.DiskQuotaInMB = this.configReader.GetUnencryptedConfigValue(
                                                           this.initParam.SectionName,
                                                           MdsConstants.DiskQuotaParamName,
                                                           MdsConstants.DefaultDiskQuotaInMB);

                int dataDeletionAgeDays = this.configReader.GetUnencryptedConfigValue(
                                              this.initParam.SectionName,
                                              MdsConstants.DataDeletionAgeParamName,
                                              MdsConstants.DefaultDataDeletionAgeDays);
                if (dataDeletionAgeDays > MdsConstants.MaxDataDeletionAgeDays)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        dataDeletionAgeDays,
                        this.initParam.SectionName,
                        MdsConstants.DataDeletionAgeParamName,
                        MdsConstants.MaxDataDeletionAgeDays);
                    dataDeletionAgeDays = MdsConstants.MaxDataDeletionAgeDays;
                }
                this.mdsUploadSettings.DataDeletionAgeMinutes = ((long)dataDeletionAgeDays) * MdsConstants.MinutesInADay;

                this.mdsUploadSettings.Filter = this.configReader.GetUnencryptedConfigValue(
                                                    this.initParam.SectionName,
                                                    MdsConstants.LogFilterParamName,
                                                    WinFabDefaultFilter.StringRepresentation);

                // Check for test settings
                int logDeletionAgeTestValue = this.configReader.GetUnencryptedConfigValue(
                                                     this.initParam.SectionName,
                                                     MdsConstants.TestDataDeletionAgeParamName,
                                                     0);
                if (logDeletionAgeTestValue != 0)
                {
                    if (logDeletionAgeTestValue > MdsConstants.MaxDataDeletionAgeMinutes)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.initParam.SectionName,
                            MdsConstants.TestDataDeletionAgeParamName,
                            MdsConstants.MaxDataDeletionAgeMinutes);
                        logDeletionAgeTestValue = MdsConstants.MaxDataDeletionAgeMinutes;
                    }
                    this.mdsUploadSettings.DataDeletionAgeMinutes = logDeletionAgeTestValue;
                }
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "MDS upload not enabled");
            }

            return;
        }

        private bool GetEtwCsvSubDirectory()
        {
            string etwCsvParentFolder = Path.Combine(
                                            this.initParam.WorkDirectory,
                                            Utility.ShortWindowsFabricIdForPaths);
            string destinationKey = Path.Combine(
                                        this.mdsUploadSettings.DirectoryName,
                                        this.mdsUploadSettings.TableName);
            bool success = Utility.CreateWorkSubDirectory(
                                this.traceSource,
                                this.logSourceId,
                                destinationKey,
                                String.Concat(Utility.EtwConsumerWorkSubFolderIdPrefix, this.logSourceId),
                                etwCsvParentFolder,
                                out this.CsvFolder);
            if (success)
            {
                this.CsvFolder = Path.Combine(this.CsvFolder, DtrDirName);
            }
            return success;
        }

        private bool GetOutputDirectoryName(out string dirName)
        {
            dirName = String.Empty;
            string dirNameTemp = this.configReader.GetUnencryptedConfigValue(
                                    this.initParam.SectionName,
                                    MdsConstants.DirectoryParamName,
                                    String.Empty);
            if (String.IsNullOrEmpty(dirNameTemp))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value for section {0}, parameter {1} cannot be an empty string.",
                    this.initParam.SectionName,
                    MdsConstants.DirectoryParamName);
                return false;
            }
            dirNameTemp = Environment.ExpandEnvironmentVariables(dirNameTemp);
            try
            {
                Uri uri = new Uri(dirNameTemp);
                dirNameTemp = uri.LocalPath;
            }
            catch (UriFormatException e)
            {
                traceSource.WriteError(
                    logSourceId,
                    "The value for section {0}, parameter {1} could not be parsed as a URI. Exception information: {2}.",
                    this.initParam.SectionName,
                    MdsConstants.DirectoryParamName,
                    e);
                return false;
            }
            dirName = dirNameTemp;
            return true;
        }

        private TablePriority GetTablePriority()
        {
            TablePriority priority;
            string priorityStr = this.configReader.GetUnencryptedConfigValue(
                                     this.initParam.SectionName,
                                     MdsConstants.TablePriorityParamName,
                                     String.Empty);
            if ((false == Enum.TryParse(priorityStr, out priority)) ||
                (TablePriority.PriUndefined == priority))
            {
                if (false == String.IsNullOrEmpty(priorityStr))
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value '{0}' specified for section {1}, parameter {2} is not supported. Therefore, the default value '{3}' will be used instead.",
                        priorityStr,
                        this.initParam.SectionName,
                        MdsConstants.TablePriorityParamName,
                        MdsConstants.DefaultTablePriority);
                }
                priority = (TablePriority) Enum.Parse(typeof(TablePriority), MdsConstants.DefaultTablePriority);
            }

            return priority;
        }

        enum CsvParts
        {
            Timestamp,
            Level,
            ThreadId,
            ProcessId, 
            WinFabId,
            Text,

            Count
        }

        internal enum MdsTableFields
        {
            Timestamp,
            Level,
            ProcessId,
            ThreadId,
            TaskName,
            EventType,
            Id,
            Text,

            Count
        }

        private void ReadDtrFiles(object state)
        {
            // Process dtr files in the current folder and also the immediate (i.e. first level) subfolders
            List<string> dtrDirs = new List<string>();
            dtrDirs.Add(this.CsvFolder);
            string[] dtrSubDirs = FabricDirectory.GetDirectories(this.CsvFolder);
            dtrDirs.AddRange(dtrSubDirs);

            foreach (string dtrDir in dtrDirs)
            {
                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, no more DTR folders will be processed in this pass.");
                    break;
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Starting DTR file read pass in folder {0} ...",
                    dtrDir);

                // Process DTR files
                ReadDtrFilesFromDir(dtrDir);

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Finished DTR file read pass in folder {0}.",
                    dtrDir);
            }

            // Schedule the next pass
            this.dtrReadTimer.Start();
        }

        private void ReadDtrFilesFromDir(string tracesFolder)
        {
            FileInfo[] dtrFiles;
            int firstFileBookmark;

            // Get the folder containing the bookmark file
            string bookmarkFolder;
            if (false == GetBookmarkFolder(tracesFolder, out bookmarkFolder))
            {
                return;
            }
            Debug.Assert(null != bookmarkFolder);

            // Figure out which DTR files we should read in this pass
            if (false == GetDtrFilesToRead(tracesFolder, bookmarkFolder, out dtrFiles, out firstFileBookmark))
            {
                return;
            }
            Debug.Assert((null != dtrFiles) && (dtrFiles.Length > 0));

            // Get the MDS table writer
            MdsTableWriter mdsTableWriter;
            if (false == GetMdsTableWriter(tracesFolder, out mdsTableWriter))
            {
                return;
            }
            Debug.Assert(null != mdsTableWriter);

            // Read the DTR files
            StringBuilder strBuilder = new StringBuilder(EventSizeInCharsHint);
            for (int j=0; j < dtrFiles.Length; j++)
            {
                FileInfo dtrFile = dtrFiles[j];

                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, no more DTR files from folder {0} will be processed in this pass.",
                        tracesFolder);
                    break;
                }

                if ((j == 0) && (firstFileBookmark > 0))
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Processing events from DTR file {0}, starting at event #{1} ...",
                        dtrFile.FullName,
                        firstFileBookmark);
                }
                else
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Processing events from DTR file {0} ...",
                        dtrFile.FullName);
                }

                // Create the bookmark file that records the current DTR file as the
                // last DTR file that we read. As we read events from this DTR file,
                // we'll keep updating the bookmark file to keep track of how far 
                // we have read into this DTR file.
                long lastReadEventIndexPosition;
                if (false == CreateBookmarkFile(bookmarkFolder, dtrFile, out lastReadEventIndexPosition))
                {
                    continue;
                }

                // Read events from the current DTR file
                using (StreamReader reader = new StreamReader(File.Open(dtrFile.FullName, FileMode.Open, FileAccess.Read)))
                {
                    bool reachedEndOfFile = false;
                    for (int i=0; ; i++)
                    {
                        if (this.stopping)
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "The consumer is being stopped. Therefore, no more events will be read from {0} in this pass.",
                                dtrFile.FullName);
                            break;
                        }

                        // We may have already processed some events from the first file. Check
                        // and make sure we don't process them again.
                        if ((j == 0) && (i < firstFileBookmark))
                        {
                            // We've already process this event. Skip it.
                            continue;
                        }

                        // Read the next event from the file
                        string etwEvent = ReadNextEvent(reader, strBuilder, dtrFile.FullName);
                        if (null == etwEvent)
                        {
                            // Reached end of file or encountered an error that prevents us
                            // from reading any more events.
                            reachedEndOfFile = true;
                            break;
                        }

                        // Parse the event
                        string[] etwEventParts = etwEvent.Split(new char[] { ',' }, (int)CsvParts.Count);
                        if (etwEventParts.Length != (int) CsvParts.Count)
                        {
                            this.traceSource.WriteWarning(
                                this.logSourceId,
                                "An event in file {0} is not in the expected format. Event info: {1}",
                                dtrFile.FullName,
                                etwEvent);
                            continue;
                        }
                        DateTime eventTime;
                        if (false == DateTime.TryParse(etwEventParts[(int)CsvParts.Timestamp], out eventTime))
                        {
                            this.traceSource.WriteWarning(
                                this.logSourceId,
                                "An event in file {0} has invalid timestamp {1}",
                                dtrFile.FullName,
                                etwEventParts[(int)CsvParts.Timestamp]);
                            continue;
                        }
                        string eventLevel = etwEventParts[(int)CsvParts.Level];
                        int threadId;
                        if (false == Int32.TryParse(etwEventParts[(int)CsvParts.ThreadId], out threadId))
                        {
                            this.traceSource.WriteWarning(
                                this.logSourceId,
                                "An event in file {0} has invalid thread ID {1}",
                                dtrFile.FullName,
                                etwEventParts[(int)CsvParts.ThreadId]);
                            continue;
                        }
                        int processId;
                        if (false == Int32.TryParse(etwEventParts[(int)CsvParts.ProcessId], out processId))
                        {
                            this.traceSource.WriteWarning(
                                this.logSourceId,
                                "An event in file {0} has invalid process ID {1}",
                                dtrFile.FullName,
                                etwEventParts[(int)CsvParts.ProcessId]);
                            continue;
                        }
                        string eventText = etwEventParts[(int)CsvParts.Text];

                        string taskName;
                        string eventType;
                        string id;
                        if (false == GetWinFabIdParts(dtrFile.FullName, etwEventParts[(int)CsvParts.WinFabId], out taskName, out eventType, out id))
                        {
                            continue;
                        }

                        // Initialize the fields for the current event in the MDS table
                        object[] tableFields = new object[(int)MdsTableFields.Count];
                        tableFields[(int)MdsTableFields.Timestamp] = eventTime.ToLocalTime(); // MDS expects local time
                        tableFields[(int)MdsTableFields.Level] = eventLevel;
                        tableFields[(int)MdsTableFields.ProcessId] = processId;
                        tableFields[(int)MdsTableFields.ThreadId] = threadId;
                        tableFields[(int)MdsTableFields.TaskName] = taskName;
                        tableFields[(int)MdsTableFields.EventType] = eventType;
                        tableFields[(int)MdsTableFields.Id] = id;
                        tableFields[(int)MdsTableFields.Text] = eventText;

                        // Write the event to the MDS table
                        mdsTableWriter.WriteEvent(tableFields);

                        // Update the bookmark to indicate that we have processed this event
                        UpdateBookmarkFile(dtrFile.FullName, bookmarkFolder, lastReadEventIndexPosition, (i+1));
                    }

                    if (reachedEndOfFile)
                    {
                        // Update the bookmark to indicate that we have processed all events from this file
                        UpdateBookmarkFile(dtrFile.FullName, bookmarkFolder, lastReadEventIndexPosition, Int32.MaxValue);
                    }
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Finished processing events from DTR file {0} in the current pass.",
                    dtrFile.FullName);
            }
        }

        private bool CreateBookmarkSubDirectory(string sourceFolder, out string bookmarkFolder)
        {
            bookmarkFolder = null;
            string bookmarkParentFolder = Path.Combine(
                                            this.initParam.WorkDirectory,
                                            Utility.ShortWindowsFabricIdForPaths);
            string destinationKey = Path.Combine(
                                        this.mdsUploadSettings.DirectoryName,
                                        this.mdsUploadSettings.TableName);
            bool success = Utility.CreateWorkSubDirectory(
                                this.traceSource,
                                this.logSourceId,
                                destinationKey,
                                sourceFolder,
                                bookmarkParentFolder,
                                out bookmarkFolder);
            if (success)
            {
                bookmarkFolder = Path.Combine(bookmarkFolder, BookmarkDirName);
            }
            FabricDirectory.CreateDirectory(bookmarkFolder);
            return success;
        }

        private bool GetBookmarkFolder(string dtrDir, out string bookmarkFolder)
        {
            bookmarkFolder = null;
            if (this.bookmarkFolders.ContainsKey(dtrDir))
            {
                bookmarkFolder = this.bookmarkFolders[dtrDir];
                return true;
            }

            // Create the subfolder where the bookmark file is saved
            if (false == CreateBookmarkSubDirectory(dtrDir, out bookmarkFolder))
            {
                return false;
            }
            this.bookmarkFolders[dtrDir] = bookmarkFolder;
            return true;
        }

        private bool GetDirectoryAndTableName(string dtrDir, out string dirName, out string tableName)
        {
            tableName = null;
            dirName = null;
            if (dtrDir.Equals(this.CsvFolder, StringComparison.OrdinalIgnoreCase))
            {
                // For the files in the top-level DTR folder, we don't create a subdirectory at
                // the destination
                dirName = this.mdsUploadSettings.DirectoryName;
            }
            else
            {
                // For subfolders under the top-level DTR folder, we create a subfolder at the destination too
                string destSubDirName = Path.GetFileName(dtrDir);
                dirName = Path.Combine(this.mdsUploadSettings.DirectoryName, destSubDirName);
                Directory.CreateDirectory(dirName);
            }
            tableName = String.Concat(
                            this.mdsUploadSettings.TableName,
                            TableSchemaVersion);

            // Check if the table name is in the correct format
            if (false == regEx.IsMatch(tableName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is an invalid table name. Table names must match the regular expression {1}.",
                    tableName,
                    MdsConstants.TableNameRegularExpression);
                return false;
            }
            return true;
        }

        private bool GetMdsTableWriter(string dtrDir, out MdsTableWriter tableWriter)
        {
            tableWriter = null;
            if (this.tableWriters.ContainsKey(dtrDir))
            {
                tableWriter = this.tableWriters[dtrDir];
                return true;
            }

            // Figure out the directory name and table name
            string dirName;
            string tableName;
            if (false == GetDirectoryAndTableName(dtrDir, out dirName, out tableName))
            {
                return false;
            }

            // Create and initialize the table writer
            tableWriter = new MdsTableWriter();
            if (false == tableWriter.Initialize(
                             this.traceSource,
                             this.logSourceId,
                             dirName,
                             tableName,
                             this.mdsUploadSettings.TablePriority,
                             this.mdsUploadSettings.DataDeletionAgeMinutes,
                             this.mdsUploadSettings.DiskQuotaInMB,
                             mdsTableFields,
                             mdsTableFieldTypes))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to create MDS table writer helper object.");
                return false;
            }

            this.tableWriters[dtrDir] = tableWriter;
            return true;
        }

        private bool GetDtrFilesToRead(string tracesFolder, string bookmarkFolder, out FileInfo[] dtrFiles, out int firstFileBookmark)
        {
            dtrFiles = null;
            firstFileBookmark = -1;

            // Get information about the last file that we processed
            LastReadFileInfo lastReadFileInfo;
            ReadBookmarkFile(bookmarkFolder, out lastReadFileInfo);

            // Get all dtr files in the folder and sort them according to the
            // time range of events they contain
            DirectoryInfo dirInfo = new DirectoryInfo(tracesFolder);
            FileInfo[] candidateFiles = dirInfo.GetFiles(String.Concat("*.", EtlConsumerConstants.FilteredEtwTraceFileExtension))
                                        .OrderBy(f => GetLastEventTimestamp(f))
                                        .ThenBy(f => GetFirstEventTimestamp(f))
                                        .ThenBy(f => f.LastWriteTimeUtc)
                                        .ToArray();
            if (0 == candidateFiles.Length)
            {
                // No DTR files to process. We're done.
                return false;
            }

            int startIndex;
            if (String.IsNullOrEmpty(lastReadFileInfo.FileName))
            {
                // We haven't read any files yet. So we should start
                // reading from the first file.
                startIndex = 0;
            }
            else 
            {
                // Find the index of the next file that we should read.
                if (lastReadFileInfo.LastReadEventIndex != Int32.MaxValue)
                {
                    // We didn't fully finish reading the last file that we read.
                    // Therefore, we should resume reading from that file (if it
                    // is still available).
                    startIndex = Array.FindIndex(candidateFiles, lastReadFileInfo.IsGreaterThanOrEqualToThis);
                }
                else
                {
                    // We had fully finished reading the last file that we read.
                    // So we should resume reading from the file after the last
                    // file that we read.
                    startIndex = Array.FindIndex(candidateFiles, lastReadFileInfo.IsGreaterThanThis);
                }
            }
            if (startIndex == -1)
            {
                // No new dtr files found for processing
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "No {0} files found for processing.",
                    EtlConsumerConstants.FilteredEtwTraceFileExtension);
                return false;
            }
            else if (startIndex > 0)
            {
                // Discard files we have already processed
                FileInfo[] oldCandidates = candidateFiles;
                candidateFiles = new FileInfo[oldCandidates.Length - startIndex];
                Array.Copy(oldCandidates, startIndex, candidateFiles, 0, candidateFiles.Length);
            }

            // Get the files that were recently added by the ETL-to-CSV writer
            int recentFileStartIndex;
            bool recentFilesHaveGaps;
            GetRecentFileInfo(candidateFiles, out recentFileStartIndex, out recentFilesHaveGaps);

            // If we find gaps in the processed events for recent files, we'll skip those files
            // in the current pass. This is because we want to give the ETL-to-CSV writer another
            // another chance to fill those gaps before we move our bookmark past those gaps.
            if (recentFilesHaveGaps)
            {
                FileInfo[] oldCandidates = candidateFiles;
                if (recentFileStartIndex > 0)
                {
                    // Skip the recent files. They have gaps in them and we want to
                    // give the ETL-to-CSV writer some more time to fill those gaps.
                    candidateFiles = new FileInfo[recentFileStartIndex];
                    Array.Copy(oldCandidates, candidateFiles, candidateFiles.Length);
                }
                else
                {
                    // All our candidate files are recent. So we won't process any 
                    // files in the current pass.
                    candidateFiles = null;
                }

                string skippedDtrs = oldCandidates[recentFileStartIndex].Name;
                for (int i = (recentFileStartIndex+1); i < oldCandidates.Length; i++)
                {
                    skippedDtrs = String.Concat(skippedDtrs, ", ", oldCandidates[i].Name);
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Processing of some {0} files will be skipped in this pass because the ETL-to-CSV file writer has not fully processed their source ETLs. Skipped files: {1}.",
                    EtlConsumerConstants.FilteredEtwTraceFileExtension,
                    skippedDtrs);

                if (null == candidateFiles)
                {
                    return false;
                }
            }

            // If we're going to be dropping some events because we're not able to keep up with
            // the incoming event volume, write a trace message indicating this.
            List<string> unprocessedEtls;
            if (EtlToCsvFileWriter.GapsFoundInDtrs(candidateFiles, 0, candidateFiles.Length - 1, out unprocessedEtls))
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Unable to keep up with incoming event volume. Some events from the following ETL files may be dropped. {0}.",
                    String.Join(", ", unprocessedEtls.ToArray()));
            }

            dtrFiles = candidateFiles;

            // If we're going to continue reading from the file that we 
            // last read, then return the index up to which we have already
            // read previously. This will ensure we don't process the same
            // events again.
            firstFileBookmark = candidateFiles[0].Name.Equals(
                                    lastReadFileInfo.FileName,
                                    StringComparison.OrdinalIgnoreCase) ?
                                  lastReadFileInfo.LastReadEventIndex : 0;
            return true;
        }

        class LastReadFileInfo
        {
            private MdsEtwEventUploader uploader;
            internal string FileName;
            internal int LastReadEventIndex;
            internal DateTime LastEventTimestamp;
            internal DateTime FirstEventTimestamp;
            internal DateTime LastWriteTimestampUtc;

            internal LastReadFileInfo(MdsEtwEventUploader uploader)
            {
                this.uploader = uploader;
                this.LastReadEventIndex = -1;
            }

            enum ComparisonType
            {
                GreaterThanOrEqualTo,
                GreaterThan
            }

            private bool CompareWithThis(FileInfo fileInfo, ComparisonType comparisonType)
            {
                // First compare the timestamps of the last events in the two files
                DateTime lastEventTimestamp = this.uploader.GetLastEventTimestamp(fileInfo);
                int result = lastEventTimestamp.CompareTo(this.LastEventTimestamp);
                if (result < 0)
                {
                    return false;
                }
                else if (result > 0)
                {
                    return true;
                }
                else
                {
                    // The last events in the two files have identical timestamps.
                    // So compare the timestamps of the first events in the two files.
                    DateTime firstEventTimestamp = this.uploader.GetFirstEventTimestamp(fileInfo);
                    result = firstEventTimestamp.CompareTo(this.FirstEventTimestamp);
                    if (result < 0)
                    {
                        return false;
                    }
                    else if (result > 0)
                    {
                        return true;
                    }
                    else
                    {
                        // The first events in the two files have identical timestamps.
                        // So compare the last write times of the two files.
                        result = fileInfo.LastWriteTimeUtc.CompareTo(this.LastWriteTimestampUtc);
                        if (result < 0)
                        {
                            return false;
                        }
                        else if (result == 0)
                        {
                            // The last write times of the two files are equal. If we are
                            // checking for greater-than-or-equal-to, then return true. If
                            // we are checking for greater than, then return false
                            return (comparisonType == ComparisonType.GreaterThanOrEqualTo);
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
            }

            internal bool IsGreaterThanOrEqualToThis(FileInfo fileInfo)
            {
                return CompareWithThis(fileInfo, ComparisonType.GreaterThanOrEqualTo);
            }

            internal bool IsGreaterThanThis(FileInfo fileInfo)
            {
                return CompareWithThis(fileInfo, ComparisonType.GreaterThan);
            }
        }

        enum LastReadFileInfoParts
        {
            FileName,
            FirstEventTimestampBinary,
            FirstEventTimestamp,
            LastWriteTimeBinary,
            LastWriteTime,
            LastReadEventIndex,

            // This is not an actual piece of information in the file, but a value
            // representing the count of the pieces of information we store in the file.
            Count
        }

        private void ReadBookmarkFile(string bookmarkFolder, out LastReadFileInfo fileInfo)
        {
            string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);
            fileInfo = new LastReadFileInfo(this);
            if (false == FabricFile.Exists(bookmarkFile))
            {
                // Bookmark file doesn't exist
                return;
            }

            StreamReader reader = null;
            try
            {
                // Open the file
                if (false == Utility.PerformIOWithRetries(
                                 ctx =>
                                 {
                                     reader = new StreamReader(File.Open(bookmarkFile, FileMode.Open, FileAccess.Read));
                                 },
                                 null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to open bookmark file {0}.",
                        bookmarkFile);
                    return;
                }

                // Get the version
                string versionString = String.Empty;
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    versionString = reader.ReadLine();
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to read version information from bookmark file {0}.",
                        bookmarkFile);
                    return;
                }

                // Check the version
                if (false == versionString.Equals(BookmarkFileFormatVersionString, StringComparison.Ordinal))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unexpected version string {0} encountered in bookmark file {1}.",
                        versionString,
                        bookmarkFile);
                    return;
                }

                // Get information about the last file that we read
                string infoLine = String.Empty;
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    infoLine = reader.ReadLine();
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to read information about last file read from bookmark file {0}.",
                        bookmarkFile);
                    return;
                }

                string[] infoLineParts = infoLine.Split(',');
                if (infoLineParts.Length != (int) LastReadFileInfoParts.Count)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The information in bookmark file {0} about the last file read is not in the expected format. {1}",
                        bookmarkFile,
                        infoLine);
                    return;
                }

                string fileName = infoLineParts[(int)LastReadFileInfoParts.FileName].Trim();
                DateTime lastEventTimestamp;
                if (false == EtlToCsvFileWriter.GetLastEventTimestamp(fileName, out lastEventTimestamp))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to retrieve timestamp of last event from file name {0} of the last read file. Error encountered while reading bookmark file {1}.",
                        fileName,
                        bookmarkFile);
                    return;
                }

                long firstEventTimestampBinary;
                if (false == Int64.TryParse(
                                infoLineParts[(int)LastReadFileInfoParts.FirstEventTimestampBinary].Trim(),
                                out firstEventTimestampBinary))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The binary value '{0}' of timestamp of first event in file {1} could not be parsed as a long value. Error encountered while reading bookmark file {2}.",
                        infoLineParts[(int)LastReadFileInfoParts.FirstEventTimestampBinary].Trim(),
                        fileName,
                        bookmarkFile);
                    return;
                }
                DateTime firstEventTimestamp = DateTime.FromBinary(firstEventTimestampBinary);

                long lastWriteTimeBinary;
                if (false == Int64.TryParse(
                                infoLineParts[(int)LastReadFileInfoParts.LastWriteTimeBinary].Trim(),
                                out lastWriteTimeBinary))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The binary value '{0}' of last write time of file {1} could not be parsed as a long value. Error encountered while reading bookmark file {2}.",
                        infoLineParts[(int)LastReadFileInfoParts.LastWriteTimeBinary].Trim(),
                        fileName,
                        bookmarkFile);
                    return;
                }
                DateTime lastWriteTimestampUtc = DateTime.FromBinary(lastWriteTimeBinary);

                int lastReadEventIndex;
                if (false == Int32.TryParse(
                                infoLineParts[(int)LastReadFileInfoParts.LastReadEventIndex].Trim(),
                                out lastReadEventIndex))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The last read event index '{0}' for file {1} could not be parsed as an int value. Error encountered while reading bookmark file {2}.",
                        infoLineParts[(int)LastReadFileInfoParts.LastReadEventIndex].Trim(),
                        fileName,
                        bookmarkFile);
                    return;
                }

                // Now that we've successfully retrieved all the information, populate the
                // class that we will return to the caller
                fileInfo.FileName = fileName;
                fileInfo.LastReadEventIndex = lastReadEventIndex;
                fileInfo.LastEventTimestamp = lastEventTimestamp;
                fileInfo.FirstEventTimestamp = firstEventTimestamp;
                fileInfo.LastWriteTimestampUtc = lastWriteTimestampUtc;
            }
            finally
            {
                if (null != reader)
                {
                    reader.Dispose();
                }
            }
        }

        private bool CreateBookmarkFile(string bookmarkFolder, FileInfo dtrFileInfo, out long lastReadEventIndexPosition)
        {
            lastReadEventIndexPosition = 0;
            FileStream fs = null;
            StreamWriter writer = null;
            string tempFileName = Utility.GetTempFileName();
            try
            {
                // Create the bookmark file in the temp folder
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    fs = new FileStream(tempFileName, FileMode.Create);
                                    writer = new StreamWriter(fs);
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to create bookmark file {0} while processing events from {1}.",
                        tempFileName,
                        dtrFileInfo.FullName);
                    return false;
                }

                // Write the version information
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    writer.WriteLine(BookmarkFileFormatVersionString);
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to write version information in bookmark file {0} while processing events from {1}.",
                        tempFileName,
                        dtrFileInfo.FullName);
                    return false;
                }

                // Get the timestamp for the first event in the file
                DateTime firstEventTimestamp = GetFirstEventTimestamp(dtrFileInfo);
                if (DateTime.MinValue.Equals(firstEventTimestamp))
                {
                    if (dtrFileInfo.Length != 0)
                    {
                        // This is not a zero-byte file, so we should have been able
                        // to get the timestamp of the first event.
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Unable to retrieve timestamp of first event in file {0}, for writing to bookmark file {1}.",
                            dtrFileInfo.FullName,
                            tempFileName);
                    }
                    return false;
                }
                long firstEventTimestampBinary = firstEventTimestamp.ToBinary();

                // Get the last write time for the file
                DateTime lastWriteTime = dtrFileInfo.LastWriteTimeUtc;
                long lastWriteTimeBinary = lastWriteTime.ToBinary();

                string lastReadFileInfo = String.Format(
                                              "{0},{1},{2},{3},{4},",
                                              dtrFileInfo.Name,
                                              firstEventTimestampBinary,
                                              firstEventTimestamp,
                                              lastWriteTimeBinary,
                                              lastWriteTime);

                // Write the last read file info
                long streamPosition = 0;
                if (false == Utility.PerformIOWithRetries(
                                 ctx =>
                                 {
                                     writer.Write(lastReadFileInfo);
                                     writer.Flush();
                                     streamPosition = fs.Position;
                                 },
                                 null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to write last read file information in bookmark file {0} while processing events from {1}.",
                        tempFileName,
                        dtrFileInfo.FullName);
                    return false;
                }

                // Write the index of the last event read and close the file
                const int lastEventIndex = -1;
                if (false == Utility.PerformIOWithRetries(
                                 ctx =>
                                 {
                                     writer.WriteLine(lastEventIndex.ToString());
                                     writer.Dispose();
                                     fs.Dispose();
                                 },
                                 null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to write default value ({0}) for last event index in bookmark file {1} while processing events from {2}.",
                        lastEventIndex,
                        tempFileName,
                        dtrFileInfo.FullName);
                    return false;
                }

                // Copy the bookmark file into the appropriate bookmark directory
                string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);
                if (false == Utility.PerformIOWithRetries(
                                 ctx =>
                                 {
                                     FabricFile.Copy(tempFileName, bookmarkFile, true);
                                 },
                                 null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to copy bookmark file while processing events from {0}. Source: {1}, Destination: {2}",
                        dtrFileInfo.FullName,
                        tempFileName,
                        bookmarkFile);
                    return false;
                }

                lastReadEventIndexPosition = streamPosition;
            }
            finally
            {
                if (null != writer)
                {
                    writer.Dispose();
                }
                if (null != fs)
                {
                    fs.Dispose();
                }
                if (FabricFile.Exists(tempFileName))
                {
                    Utility.PerformIOWithRetries(
                        ctx =>
                        {
                            FabricFile.Delete(tempFileName);
                        },
                        null);
                }
            }
                            
            return true;
        }

        private bool UpdateBookmarkFile(string dtrFileName, string bookmarkFolder, long lastReadEventIndexPosition, int lastReadEventIndex)
        {
            FileStream fs = null;
            StreamWriter writer = null;
            try
            {
                // Open the bookmark file
                string bookmarkFile = Path.Combine(bookmarkFolder, BookmarkFileName);
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    fs = new FileStream(bookmarkFile, FileMode.Open);
                                    writer = new StreamWriter(fs);
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to open bookmark file {0} while processing events from {1}.",
                        bookmarkFile,
                        dtrFileName);
                    return false;
                }

                // Move the stream position to the point where the event index needs to be written
                fs.Position = lastReadEventIndexPosition;

                // Write the index of the last event that we read and close the file
                string indexAsString = lastReadEventIndex.ToString();
                if (false == Utility.PerformIOWithRetries(
                                 ctx =>
                                 {
                                     writer.WriteLine(indexAsString);
                                     fs.SetLength(fs.Position);
                                     writer.Dispose();
                                     fs.Dispose();
                                 },
                                 null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to write index of last event read in bookmark file {0} while processing events from {1}.",
                        bookmarkFile,
                        dtrFileName);
                    return false;
                }
            }
            finally
            {
                if (null != writer)
                {
                    writer.Dispose();
                }
                if (null != fs)
                {
                    fs.Dispose();
                }
            }

            return true;
        }

        internal DateTime GetFirstEventTimestamp(FileInfo fileInfo)
        {
            DateTime firstEventTimestamp = DateTime.MinValue;
            StringBuilder strBuilder = new StringBuilder(EventSizeInCharsHint);
            StreamReader reader = null;
            try
            {
                // Open the file
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    reader = new StreamReader(File.Open(fileInfo.FullName, FileMode.Open, FileAccess.Read));
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to open file {0} to retrieve the timestamp of its first event. We will assume the first event timestamp to be {1}.",
                        fileInfo.FullName,
                        firstEventTimestamp);
                    return firstEventTimestamp;
                }
                
                // Normally we should be able to successfully parse the first event
                // and exit the loop. But in case of errors, we try to parse subsequent
                // events until we find an event that we are able to parse.
                for (; ; )
                {
                    string etwEvent = ReadNextEvent(reader, strBuilder, fileInfo.FullName);
                    if (null == etwEvent)
                    {
                        // Reached end of file without finding a single event
                        // whose timestamp we could parse. 
                        if (fileInfo.Length != 0)
                        {
                            // It is not a zero-byte file
                            this.traceSource.WriteWarning(
                                this.logSourceId,
                                "Unable to retrieve timestamp of first event from file {0}. We will assume the first event timestamp to be {1}.",
                                fileInfo.FullName,
                                firstEventTimestamp);
                        }
                        break;
                    }
                    string[] etwEventParts = etwEvent.Split(new char[] { ',' }, (int)CsvParts.Count);
                    if (etwEventParts.Length != (int)CsvParts.Count)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "While determining the timestamp of the first event in file {0}, we encountered an event that is not in the expected format. Event info: {1}",
                            fileInfo.FullName,
                            etwEvent);
                        continue;
                    }
                    if (false == DateTime.TryParse(
                                    etwEventParts[(int)CsvParts.Timestamp],
                                    out firstEventTimestamp))
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "While determining the timestamp of the first event in file {0}, we encountered an event whose timestamp we could not parse. Timestamp info: {1}",
                            fileInfo.FullName,
                            etwEventParts[(int)CsvParts.Timestamp]);
                        continue;
                    }

                    // Timestamp successfully obtained. Break out of the loop.
                    break;
                }
            }
            finally
            {
                if (null != reader)
                {
                    reader.Dispose();
                }
            }
            return firstEventTimestamp;
        }

        internal DateTime GetLastEventTimestamp(FileInfo fileInfo)
        {
            DateTime lastEventTimestamp;
            if (false == EtlToCsvFileWriter.GetLastEventTimestamp(fileInfo.Name, out lastEventTimestamp))
            {
                lastEventTimestamp = DateTime.MinValue;
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Unable to retrieve timestamp of last event from file {0}. We will assume the first event timestamp to be {1}.",
                    fileInfo.FullName,
                    lastEventTimestamp);
            }
            return lastEventTimestamp;
        }

        private void GetRecentFileInfo(FileInfo[] candidateFiles, out int recentFileStartIndex, out bool recentFilesHaveGaps)
        {
            recentFilesHaveGaps = false;

            // Look for files that we consider to be recent
            DateTime recentMarker = DateTime.UtcNow.AddMinutes(-1 * this.mdsUploadSettings.UploadIntervalMinutes);
            recentFileStartIndex = Array.FindIndex(
                                       candidateFiles,
                                       f =>
                                       {
                                           return (f.LastWriteTimeUtc.CompareTo(recentMarker) > 0);
                                       });
            if (-1 == recentFileStartIndex)
            {
                // None of the files are considered recent. They've all been written a while back.
                return;
            }

            // Look for gaps in the recent files.
            // In order to search for gaps, we start at the file before the first recent file 
            // (if applicable)
            List<string> dontCare;
            recentFilesHaveGaps = EtlToCsvFileWriter.GapsFoundInDtrs(
                                    candidateFiles, 
                                    (recentFileStartIndex > 0) ? (recentFileStartIndex-1) : recentFileStartIndex,
                                    (candidateFiles.Length - 1),
                                    out dontCare);
        }

        private string ReadNextEvent(StreamReader reader, StringBuilder strBuilder, string fileName)
        {
            strBuilder.Clear();
            for (;;)
            {
                int nextChar = -1;

                // Read next character
                if (false == Utility.PerformIOWithRetries(
                                ctx =>
                                {
                                    nextChar = reader.Read();
                                },
                                null))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to read next event from file {0}.",
                        fileName);
                    return null;
                }
                
                if (-1 == nextChar)
                {
                    // End of file reached
                    if (strBuilder.Length != 0)
                    {
                        // End of file was unexpected
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "Unable to read next event from file {0} because we unexpectedly reached the end of the file.",
                            fileName);
                    }
                    return null;
                }
                else if ('\n' == nextChar)
                {
                    // Finished reading the current event. Break out of the loop.
                    break;
                }

                // Add the character to the event
                strBuilder.Append((char) nextChar);
            }

            return strBuilder.ToString();
        }

        enum WinFabIdParts
        {
            TaskName,
            EventType,

            // This is not a part of the WinFab ID. It is the minimum count
            // of WinFab ID parts.
            MinCount,

            Id = MinCount,

            // This is not a part of the WinFab ID. It is the maximum count
            // of WinFab ID parts.
            MaxCount
        }

        private bool GetWinFabIdParts(string dtrFileName, string winfabId, out string taskName, out string eventType, out string id)
        {
            taskName = null;
            eventType = null;
            id = null;

            // WinFab ID is of the following format:
            //      <TaskName>.<EventType>[@<id>]
            // The '@<id>' part above is optional
            string[] winfabIdParts = winfabId.Split(new char[] {'.', '@'}, (int) WinFabIdParts.MaxCount);
            if (winfabIdParts.Length < (int)WinFabIdParts.MinCount)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "An event in file {0} has invalid event identifier (TaskName/EventType/ID) {1}.",
                    dtrFileName,
                    winfabId);
                return false;
            }

            taskName = winfabIdParts[(int) WinFabIdParts.TaskName];
            eventType = winfabIdParts[(int) WinFabIdParts.EventType];

            // The DTR file in V3+ has the full event type name. However, we want to display 
            // only the event group name in the MDS table.
            string eventGroupName = EventDefinition.GetEventGroupName(eventType);
            if (false == String.IsNullOrEmpty(eventGroupName))
            {
                eventType = eventGroupName;
            }

            if (winfabIdParts.Length == (int) WinFabIdParts.MinCount)
            {
                // This event does not have an ID field
                id = String.Empty;
            }
            else
            {
                // This event has an ID field
                id = winfabIdParts[(int) WinFabIdParts.Id];
            }
            return true;
        }
    }
}