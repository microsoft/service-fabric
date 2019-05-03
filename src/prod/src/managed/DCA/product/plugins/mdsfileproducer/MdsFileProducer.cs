// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Fabric.Interop;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;
    
    using MdsHelper;
    using Microsoft.Cis.Monitoring.Tables;

    using Tools.EtlReader;

    using Utility = System.Fabric.Dca.Utility;

    // This class implements the creation of MDS TSF files from Windows Fabric ETW traces
    internal class MdsFileProducer : IDcaInMemoryConsumer
    {
        internal const string TableSchemaVersion = "SV1";
        private const string LogDirName = "Log";

        private static readonly string[] MdsTableFieldDescriptions = 
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

        private static readonly TableFieldType[] MdsTableFieldTypes = 
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

        // Regular expression describing valid table names
        private static readonly Regex RegEx = new Regex(MdsFileProducerConstants.TableNameRegularExpression);

        // List of table paths across all instances of this plugin
        private static readonly HashSet<string> TablePaths = new HashSet<string>();

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly ConfigReader configReader;

        // Folder to hold bootstrap log files
        private readonly string etwLogDirName;

        // ETL to in-memory buffer writer interface
        private readonly EtlToInMemoryBufferWriter etlToInMemoryBufferWriter;

        // MDS table writers for trace file sub folders
        private readonly Dictionary<string, MdsTableWriter> tableWriters;

        // Consumer initialization parameters
        private readonly ConsumerInitializationParameters initParam;

        // Object that manages bookmarks
        private readonly ConsumerProgressManager progressManager;

        // Performance counters logging 
        private readonly MdsFileProducerPerformance mdsFileProducerPerf;

        // Last event index processed
        private EventIndex lastEventIndexProcessed;
        
        // Settings for creating MDS files
        private MdsFileProducerSettings mdsFileProducerSettings;

        // Used to record bytes read when getting max index already processed.
        private long bytesReadGetMaxIndexAlreadyProcessed;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        /// <summary>
        /// Count of records written to MDS
        /// When this count reaches the bookmark batch size,
        /// we write out the event index to disk
        /// </summary>
        private int bookmarkBatchCount;

        public MdsFileProducer(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.stopping = false;
            this.initParam = initParam;
            this.logSourceId = string.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);
            this.tableWriters = new Dictionary<string, MdsTableWriter>();
            this.mdsFileProducerPerf = new MdsFileProducerPerformance();
            this.progressManager = new ConsumerProgressManager(
                this.traceSource, 
                this.logSourceId,
                MdsFileProducerConstants.MethodExecutionInitialRetryIntervalMs,
                MdsFileProducerConstants.MethodExecutionMaxRetryCount,
                MdsFileProducerConstants.MethodExecutionMaxRetryIntervalMs);

            // Read MDS-specific settings from settings.xml
            this.GetSettings();
            if (false == this.mdsFileProducerSettings.Enabled)
            {
                // MDS tables creation is not enabled, so return immediately
                return;
            }

            string destinationKey = Path.Combine(
                this.mdsFileProducerSettings.DirectoryName,
                this.mdsFileProducerSettings.TableName);

            // initialize bookmark folders and files
            var initializeBookmarkFoldersAndFilesSuccess = this.progressManager.InitializeBookmarkFoldersAndFiles(
                this.initParam.WorkDirectory,
                destinationKey);
            if (false == initializeBookmarkFoldersAndFilesSuccess)
            {
                const string Message = "Failed to initialize bookmark folders and files.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Create etw log directory
            this.etwLogDirName = this.CreateEtwLogDirectory();
            if (string.IsNullOrEmpty(this.etwLogDirName))
            {
                const string Message = "Failed to create etw log directory.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Create the helper object that writes events delivered from the ETL
            // files into an in-memory buffer.
            this.etlToInMemoryBufferWriter = new EtlToInMemoryBufferWriter(
                new TraceEventSourceFactory(),
                this.logSourceId,
                this.initParam.FabricNodeId,
                this.etwLogDirName,
                false,
                this);

            // Set the event filter
            this.etlToInMemoryBufferWriter.SetEtwEventFilter(
                this.mdsFileProducerSettings.Filter,
                WinFabDefaultFilter.StringRepresentation,
                WinFabSummaryFilter.StringRepresentation,
                true);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "MDS file producer is configured. Table path: {0}, Table name: {1}, Table priority: {2}, Disk quota (MB): {3}, Data deletion age (minutes): {4}",
                this.mdsFileProducerSettings.DirectoryName,
                this.mdsFileProducerSettings.TableName,
                this.mdsFileProducerSettings.TablePriority,
                this.mdsFileProducerSettings.DiskQuotaInMB,
                this.mdsFileProducerSettings.DataDeletionAge);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Windows Fabric event filters for MDS file producer: {0}",
                this.mdsFileProducerSettings.Filter);
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

        internal enum WinFabIdParts
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

        internal enum EtwEventParts
        {
            Timestamp,
            Level,
            ThreadId,
            ProcessId,
            WinFabId,
            Text,

            Count
        }

        public Action<DecodedEventWrapper, string> ConsumerProcessTraceEventAction
        {
            get
            {
                return this.ProcessTraceEvent;
            }
        }

        public Func<string, EventIndex> MaxIndexAlreadyProcessed
        {
            get
            {
                return this.GetMaxIndexAlreadyProcessed;
            }
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

            if (null != this.etlToInMemoryBufferWriter)
            {
                // Tell the etl-to-in-memory writer to stop
                this.etlToInMemoryBufferWriter.Dispose();
            }

            if (null != this.mdsFileProducerPerf)
            {
                this.mdsFileProducerPerf.Dispose();
            }

            foreach (MdsTableWriter tableWriter in this.tableWriters.Values)
            {
                // Tell the table writer to stop
                tableWriter.Dispose();
            }

            lock (TablePaths)
            {
                TablePaths.Remove(
                    Path.Combine(
                        this.mdsFileProducerSettings.DirectoryName,
                        this.mdsFileProducerSettings.TableName));
            }

            GC.SuppressFinalize(this);
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.mdsFileProducerSettings.Enabled ? ((IEtlInMemorySink)this.etlToInMemoryBufferWriter) : null;
        }

        public void FlushData()
        {
        }

        public void OnProcessingPeriodStart(string traceFileName, bool isActiveEtl, string traceFileSubFolder)
        {
            this.mdsFileProducerPerf.OnProcessingPeriodStart();

            // GetMaxIndexAlreadyProcessed is invoked before the perf helper pass begins. So we save the bytes read
            // during the call to GetMaxIndexAlreadyProcessed and then record it here.
            this.mdsFileProducerPerf.BytesRead(this.bytesReadGetMaxIndexAlreadyProcessed);

            // Reset the last event index
            this.lastEventIndexProcessed.Set(DateTime.MinValue, -1);
        }

        public void OnProcessingPeriodStop(string traceFileName, bool isActiveEtl, string traceFileSubFolder)
        {
            // retrieve bookmark folder
            string bookmarkFolder = this.progressManager.BookmarkFolders[traceFileSubFolder];
            if (string.IsNullOrEmpty(bookmarkFolder))
            {
                return;
            }

            // Last event read position
            long bookmarkLastEventReadPosition = this.progressManager.BookmarkLastEventReadPosition[traceFileSubFolder];

            // When the bookmark batch count is less than the bookmark batch size we
            // will miss saving the true last event index at the end of the processing
            // period. Ensure we do not overwrite a valid timestamp with MinValue.
            if (this.lastEventIndexProcessed.Timestamp != DateTime.MinValue && this.lastEventIndexProcessed.TimestampDifferentiator != -1)
            {
                long bytesWritten = 0;
                this.progressManager.UpdateBookmarkFile(
                    bookmarkFolder, 
                    bookmarkLastEventReadPosition, 
                    this.lastEventIndexProcessed,
                    out bytesWritten);

                this.mdsFileProducerPerf.BytesWritten(bytesWritten);
            }

            this.mdsFileProducerPerf.OnProcessingPeriodStop();
        }

        /// <summary>
        /// Action passed to EtlToInMemoryBufferWriter that
        /// writes the decoded event to the MDS table and updates the bookmark with the event index
        /// </summary>
        /// <param name="decodedEventWrapper"></param>
        /// <param name="traceFileSubFolder"></param>
        private void ProcessTraceEvent(DecodedEventWrapper decodedEventWrapper, string traceFileSubFolder)
        {
            if (this.stopping)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The consumer is being stopped. Therefore, no more in-memory trace events will be processed in this pass.");
                return;
            }

            // retrieve bookmark folder
            string bookmarkFolder = this.progressManager.BookmarkFolders[traceFileSubFolder];
            if (string.IsNullOrEmpty(bookmarkFolder))
            {
                return;
            }

            // Get the MDS table writer
            MdsTableWriter mdsTableWriter;
            if (false == this.GetOrCreateMdsTableWriter(traceFileSubFolder, out mdsTableWriter))
            {
                return;
            }

            Debug.Assert(null != mdsTableWriter, "MdsTableWriter should be set on success.");

            // Last event read position
            long bookmarkLastEventReadPosition = this.progressManager.BookmarkLastEventReadPosition[traceFileSubFolder];

            string etwEvent = decodedEventWrapper.StringRepresentation.Replace("\r\n", "\r\t").Replace("\n", "\t");

            // Parse the event
            string[] etwEventParts = etwEvent.Split(new[] { ',' }, (int)EtwEventParts.Count);
            if (etwEventParts.Length != (int)EtwEventParts.Count)
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "The event is not in the expected format. Event info: {0}",
                    etwEvent);
                return;
            }

            DateTime eventTime;
            if (false == DateTime.TryParse(etwEventParts[(int)EtwEventParts.Timestamp], out eventTime))
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "The event has invalid timestamp {0}",
                    etwEventParts[(int)EtwEventParts.Timestamp]);
                return;
            }

            string eventLevel = etwEventParts[(int)EtwEventParts.Level];
            int threadId;
            if (false == int.TryParse(etwEventParts[(int)EtwEventParts.ThreadId], out threadId))
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "The event has invalid thread ID {0}",
                    etwEventParts[(int)EtwEventParts.ThreadId]);
                return;
            }

            int processId;
            if (false == int.TryParse(etwEventParts[(int)EtwEventParts.ProcessId], out processId))
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "The event has invalid process ID {0}",
                    etwEventParts[(int)EtwEventParts.ProcessId]);
                return;
            }

            string eventText = etwEventParts[(int)EtwEventParts.Text];

            string taskName;
            string eventType;
            string id;
            if (false == this.GetWinFabIdParts(etwEventParts[(int)EtwEventParts.WinFabId], out taskName, out eventType, out id))
            {
                return;
            }

            // Initialize the fields for the current event in the MDS table
            long mdsEventBytesCount = 0;
            object[] tableFields = new object[(int)MdsTableFields.Count];
            tableFields[(int)MdsTableFields.Timestamp] = eventTime.ToLocalTime(); // MDS expects local time
            mdsEventBytesCount += eventTime.ToBytes().Length;
            tableFields[(int)MdsTableFields.Level] = eventLevel;
            mdsEventBytesCount += eventLevel.Length;
            tableFields[(int)MdsTableFields.ProcessId] = processId;
            mdsEventBytesCount += sizeof(int);
            tableFields[(int)MdsTableFields.ThreadId] = threadId;
            mdsEventBytesCount += sizeof(int);
            tableFields[(int)MdsTableFields.TaskName] = taskName;
            mdsEventBytesCount += taskName.Length;
            tableFields[(int)MdsTableFields.EventType] = eventType;
            tableFields[(int)MdsTableFields.Id] = id;
            mdsEventBytesCount += id.Length;
            tableFields[(int)MdsTableFields.Text] = eventText;
            mdsEventBytesCount += eventText.Length;

            this.bookmarkBatchCount++;

            // Write the event to the MDS table
            var result = mdsTableWriter.WriteEvent(tableFields);

            if (MdsTableWriter.MdNoError != result)
            {
                this.mdsFileProducerPerf.MdsWriteError();
            }
            else
            {
                this.mdsFileProducerPerf.BytesWritten(mdsEventBytesCount);
                this.mdsFileProducerPerf.MdsWriteSuccess();
            }

            this.lastEventIndexProcessed.Set(decodedEventWrapper.Timestamp, decodedEventWrapper.TimestampDifferentiator);

            // Update the bookmark to indicate that we have processed the last event 
            // written when we reach the bookmark batch size
            if (this.bookmarkBatchCount == this.mdsFileProducerSettings.BookmarkBatchSize)
            {
                this.bookmarkBatchCount = 0;
                long bytesWritten = 0;
                this.progressManager.UpdateBookmarkFile(
                    bookmarkFolder, 
                    bookmarkLastEventReadPosition, 
                    this.lastEventIndexProcessed,
                    out bytesWritten);

                this.mdsFileProducerPerf.BytesWritten(bytesWritten);
            }
        }

        private bool GetWinFabIdParts(string winfabId, out string taskName, out string eventType, out string id)
        {
            taskName = null;
            eventType = null;
            id = null;

            // WinFab ID is of the following format:
            //      <TaskName>.<EventType>[@<id>]
            // The '@<id>' part above is optional
            string[] winfabIdParts = winfabId.Split(new[] { '.', '@' }, (int)WinFabIdParts.MaxCount);
            if (winfabIdParts.Length < (int)WinFabIdParts.MinCount)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The event has invalid event identifier (TaskName/EventType/ID) {0}.",
                    winfabId);
                return false;
            }

            taskName = winfabIdParts[(int)WinFabIdParts.TaskName];
            eventType = winfabIdParts[(int)WinFabIdParts.EventType];

            // The DTR file in V3+ has the full event type name. However, we want to display 
            // only the event group name in the MDS table.
            string eventGroupName = EventDefinition.GetEventGroupName(eventType);
            if (false == string.IsNullOrEmpty(eventGroupName))
            {
                eventType = eventGroupName;
            }

            if (winfabIdParts.Length == (int)WinFabIdParts.MinCount)
            {
                // This event does not have an ID field
                id = string.Empty;
            }
            else
            {
                // This event has an ID field
                id = winfabIdParts[(int)WinFabIdParts.Id];
            }

            return true;
        }

        private void GetSettings()
        {
            this.mdsFileProducerSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                 this.initParam.SectionName,
                                                 MdsFileProducerConstants.EnabledParamName,
                                                 MdsFileProducerConstants.MdsUploadEnabledByDefault);
            if (this.mdsFileProducerSettings.Enabled)
            {
                if (false == this.GetOutputDirectoryName(out this.mdsFileProducerSettings.DirectoryName))
                {
                    this.mdsFileProducerSettings.Enabled = false;
                    return;
                }

                this.mdsFileProducerSettings.TableName = this.configReader.GetUnencryptedConfigValue(
                                                       this.initParam.SectionName,
                                                       MdsFileProducerConstants.TableParamName,
                                                       MdsFileProducerConstants.DefaultTableName);
                lock (TablePaths)
                {
                    string tablePath = Path.Combine(
                                           this.mdsFileProducerSettings.DirectoryName,
                                           this.mdsFileProducerSettings.TableName);
                    if (TablePaths.Contains(tablePath))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The table {0} - specified via section {1}, parameters {2} and {3} - is already in use by another plugin.",
                            tablePath,
                            this.initParam.SectionName,
                            MdsFileProducerConstants.DirectoryParamName,
                            MdsFileProducerConstants.TableParamName);
                        this.mdsFileProducerSettings.Enabled = false;
                        return;
                    }

                    TablePaths.Add(tablePath);
                }

                this.mdsFileProducerSettings.TablePriority = this.GetTablePriority();
                this.mdsFileProducerSettings.DiskQuotaInMB = this.configReader.GetUnencryptedConfigValue(
                                                           this.initParam.SectionName,
                                                           MdsFileProducerConstants.DiskQuotaParamName,
                                                           MdsFileProducerConstants.DefaultDiskQuotaInMB);

                this.mdsFileProducerSettings.BookmarkBatchSize = this.configReader.GetUnencryptedConfigValue(
                                                                this.initParam.SectionName,
                                                                MdsFileProducerConstants.BookmarkBatchSizeParamName,
                                                                MdsFileProducerConstants.DefaultBookmarkBatchSize);

                var dataDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                              this.initParam.SectionName,
                                              MdsFileProducerConstants.DataDeletionAgeParamName,
                                              (int)MdsFileProducerConstants.DefaultDataDeletionAge.TotalDays));
                if (dataDeletionAge > MdsFileProducerConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        dataDeletionAge,
                        this.initParam.SectionName,
                        MdsFileProducerConstants.DataDeletionAgeParamName,
                        MdsFileProducerConstants.MaxDataDeletionAge);
                    dataDeletionAge = MdsFileProducerConstants.MaxDataDeletionAge;
                }

                this.mdsFileProducerSettings.DataDeletionAge = dataDeletionAge;

                this.mdsFileProducerSettings.Filter = this.configReader.GetUnencryptedConfigValue(
                                                    this.initParam.SectionName,
                                                    MdsFileProducerConstants.LogFilterParamName,
                                                    WinFabDefaultFilter.StringRepresentation);

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                     this.initParam.SectionName,
                                                     MdsFileProducerConstants.TestDataDeletionAgeParamName,
                                                     0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > MdsFileProducerConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.initParam.SectionName,
                            MdsFileProducerConstants.TestDataDeletionAgeParamName,
                            MdsFileProducerConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = MdsFileProducerConstants.MaxDataDeletionAge;
                    }

                    this.mdsFileProducerSettings.DataDeletionAge = logDeletionAgeTestValue;
                }
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "MDS file producer not enabled");
            }
        }

        private bool GetOutputDirectoryName(out string dirName)
        {
            dirName = string.Empty;
            string dirNameTemp = this.configReader.GetUnencryptedConfigValue(
                                    this.initParam.SectionName,
                                    MdsFileProducerConstants.DirectoryParamName,
                                    string.Empty);
            if (string.IsNullOrEmpty(dirNameTemp))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value for section {0}, parameter {1} cannot be an empty string.",
                    this.initParam.SectionName,
                    MdsFileProducerConstants.DirectoryParamName);
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
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value for section {0}, parameter {1} could not be parsed as a URI. Exception information: {2}.",
                    this.initParam.SectionName,
                    MdsFileProducerConstants.DirectoryParamName,
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
                                     MdsFileProducerConstants.TablePriorityParamName,
                                     string.Empty);
            if ((false == Enum.TryParse(priorityStr, out priority)) ||
                (TablePriority.PriUndefined == priority))
            {
                if (false == string.IsNullOrEmpty(priorityStr))
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value '{0}' specified for section {1}, parameter {2} is not supported. Therefore, the default value '{3}' will be used instead.",
                        priorityStr,
                        this.initParam.SectionName,
                        MdsFileProducerConstants.TablePriorityParamName,
                        MdsFileProducerConstants.DefaultTablePriority);
                }

                priority = (TablePriority)Enum.Parse(typeof(TablePriority), MdsFileProducerConstants.DefaultTablePriority);
            }

            return priority;
        }

        private string CreateEtwLogDirectory()
        {
            string etwParentFolder = Path.Combine(
                this.initParam.WorkDirectory,
                Utility.ShortWindowsFabricIdForPaths);

            string destinationKey = Path.Combine(
                this.mdsFileProducerSettings.DirectoryName,
                this.mdsFileProducerSettings.TableName);

            string etwFolder;
            bool success = Utility.CreateWorkSubDirectory(
                this.traceSource,
                this.logSourceId,
                destinationKey,
                string.Concat(Utility.EtwConsumerWorkSubFolderIdPrefix, this.logSourceId),
                etwParentFolder,
                out etwFolder);

            if (success)
            {
                return Path.Combine(etwFolder, LogDirName);
            }

            return null;
        }

        private bool GetOrCreateMdsTableWriter(string traceFileSubFolder, out MdsTableWriter tableWriter)
        {
            tableWriter = null;
            if (this.tableWriters.ContainsKey(traceFileSubFolder))
            {
                tableWriter = this.tableWriters[traceFileSubFolder];
                return true;
            }

            // Figure out the directory name and table name
            string dirName;
            string tableName;
            if (false == this.GetDirectoryAndTableName(traceFileSubFolder, out dirName, out tableName))
            {
                return false;
            }

            // Create and initialize the table writer
            try
            {
                tableWriter = new MdsTableWriter(
                    dirName,
                    tableName,
                    this.mdsFileProducerSettings.TablePriority,
                    (long)this.mdsFileProducerSettings.DataDeletionAge.TotalMinutes,
                    this.mdsFileProducerSettings.DiskQuotaInMB,
                    MdsTableFieldDescriptions,
                    MdsTableFieldTypes);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to create MDS table writer helper object.");
                return false;
            }

            this.tableWriters[traceFileSubFolder] = tableWriter;
            return true;
        }

        private bool GetDirectoryAndTableName(string traceFileSubFolder, out string dirName, out string tableName)
        {
            if (traceFileSubFolder.Equals(this.etwLogDirName, StringComparison.OrdinalIgnoreCase))
            {
                // For the files in the top-level folder, we don't create a subdirectory at
                // the destination
                dirName = this.mdsFileProducerSettings.DirectoryName;
            }
            else
            {
                // For subfolders under the top-level folder, we create a subfolder at the destination too
                string destSubDirName = Path.GetFileName(traceFileSubFolder);
                dirName = Path.Combine(this.mdsFileProducerSettings.DirectoryName, destSubDirName);
                Directory.CreateDirectory(dirName);
            }

            tableName = string.Concat(
                            this.mdsFileProducerSettings.TableName,
                            TableSchemaVersion);

            // Check if the table name is in the correct format
            if (false == RegEx.IsMatch(tableName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is an invalid table name. Table names must match the regular expression {1}.",
                    tableName,
                    MdsFileProducerConstants.TableNameRegularExpression);
                return false;
            }

            return true;
        }

        private EventIndex GetMaxIndexAlreadyProcessed(string traceFileSubFolder)
        {
            string bookmarkFolder = this.progressManager.BookmarkFolders[traceFileSubFolder];
            this.bytesReadGetMaxIndexAlreadyProcessed = 0;
            var lastEventIndex = this.progressManager.ReadBookmarkFile(
                bookmarkFolder,
                out this.bytesReadGetMaxIndexAlreadyProcessed);

            return lastEventIndex;
        }

        // Settings related to creation of MDS files
        private struct MdsFileProducerSettings
        {
            internal bool Enabled;
            internal string DirectoryName;
            internal string TableName;
            internal TablePriority TablePriority;
            internal int DiskQuotaInMB;
            internal TimeSpan DataDeletionAge;
            internal string Filter;
            internal int BookmarkBatchSize;
        }
    }
}