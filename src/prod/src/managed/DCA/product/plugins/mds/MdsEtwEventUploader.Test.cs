// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MdsUploaderTest 
{
    using Microsoft.Cis.Monitoring.Query;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;
    using FabricDCA;

    [TestClass]
    public class MdsUploaderTest
    {
        // Members that are not test specific
        private TestContext testContext;
        public TestContext TestContext
        {
            get { return testContext; }
            set { testContext = value; }
        }

        internal const string TraceType = "MdsUploaderTest";
        private const string TestFabricNodeId = "NodeId";
        private const string TestFabricNodeName = "NodeName";
        private const string LogFolderName = "log";
        private const string WorkFolderName = "work";
        private const string DeletedItemsFolderName = "DeletedItems";
        private const string TestConfigSectionName = "TestConfigSection";
        private const string configFileName = "MdsUploaderTest.cfg";
        private const string configFileContents =
        @"
[Trace/File]
Level = 5
Path = MdsUploader.Test
Option = m

[{0}]
IsEnabled = true
DirectoryName = {1}
TableName = {2}
UploadIntervalInMinutes = {3}
        ";
        private const int DtrReadTimerScaleDown = 30;
        private const int UploadIntervalMinutes = 2;
        private const int DefaultTableNonce = 0;
        private const string TableName = "TestTable";
        private const string TaskNamePrefix = "Task";
        private const string EventTypePrefix = "Event";
        private const string IdPrefix = "Id";
        private const string TextPrefix = "foo";

        private static string testDataDirectory;
        private static string etwCsvFolder;
        private static string EtwCsvFolder
        {
            get
            {
                return Interlocked.CompareExchange(ref etwCsvFolder, null, null);
            }

            set
            {
                Interlocked.Exchange(ref etwCsvFolder, value);
            }
        }

        internal static AutoResetEvent StartDtrRead = new AutoResetEvent(false);
        internal static AutoResetEvent DtrReadCompleted = new AutoResetEvent(false);
        internal static ManualResetEvent EndOfTest = new ManualResetEvent(false);

        [ClassInitialize]
        public static void MdsEtwEventUploaderTestSetup(TestContext testContext)
        {
            // Create the test data directory
            const string testDataDirectoryName = "MdsUploader.Test.Data";
            testDataDirectory = Path.Combine(Directory.GetCurrentDirectory(), testDataDirectoryName);
            Directory.CreateDirectory(testDataDirectory);

            // Create the config file for the test
            string configFileFullPath = Path.Combine(
                                     testDataDirectory,
                                     configFileName);
            string configFileContentsFormatted = String.Format(
                                                     CultureInfo.InvariantCulture,
                                                     configFileContents,
                                                     TestConfigSectionName,
                                                     testDataDirectory,
                                                     TableName,
                                                     UploadIntervalMinutes);
            byte[] configFileBytes = Encoding.ASCII.GetBytes(configFileContentsFormatted); 
            File.WriteAllBytes(configFileFullPath, configFileBytes);
            Environment.SetEnvironmentVariable("FabricConfigFileName", configFileFullPath);

            // Initialize config store
            Utility.InitializeConfigStore((IConfigStoreUpdateHandler)null);

            Utility.InitializeTracing();
            Utility.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            HealthClient.Disabled = true;

            // Scale down the timer interval, so that the test can run quicker
            SystemTimer.ScaleDown = DtrReadTimerScaleDown;
        }

        private const int PassCount = 2;
        private const int MaxPerFileEventCount = 8;
        enum TraceFileType
        {
            Fabric,
            Lease,

            // This is not a trace file type. It represents
            // the count of trace file types.
            Count
        }

        enum TraceFileAttribute
        {
            Name,
            IsLastChunkOfEtl,
            CreationPass,
            LastEventTimestamp,
            AllEventsHaveSameTimestamp,
            LastWriteTime,
            ProcessedInPass,
            DeletionPass,
            EmptyFile,

            // This is not an attribute of a trace file. It represents
            // the count of attributes.
            Count
        }

        class TraceFileInfo
        {
            internal string Name;
            internal bool IsLastChunkOfEtl;
            internal int CreationPass;
            internal DateTime LastEventTimestamp;
            internal bool AllEventsHaveSameTimestamp;
            internal DateTime LastWriteTime;
            internal int ProcessedInPass;
            internal int DeletionPass;
            internal string FullName;
            internal int EventCount;
            internal bool EmptyFile;
        }

        private static Dictionary<TraceFileType, string> configParamNames = new Dictionary<TraceFileType, string>()
        {
            { TraceFileType.Fabric, "FabricDtrFileInfo" },
            { TraceFileType.Lease, "LeaseDtrFileInfo" },
        };
        private static Dictionary<TraceFileType, string> traceSubFolderNames = new Dictionary<TraceFileType, string>()
        {
            { TraceFileType.Fabric, "Fabric" },
            { TraceFileType.Lease, "Lease" },
        };
        private static string[] eventLevels = new string[]
        {
            "Error",
            "Warning",
            "Informational",
            "Verbose",
        };
        private Dictionary<TraceFileType, List<TraceFileInfo>> traceFileInfo = new Dictionary<TraceFileType, List<TraceFileInfo>>();
        private DateTime testStartTime;
        private MdsEtwEventUploader uploader;
        private string deletedItemsFolder;
        

        [TestInitialize]
        public void TestMethodSetup()
        {
            // Initialization
            bool result = StartDtrRead.Reset() && DtrReadCompleted.Reset() && EndOfTest.Reset();
            Verify.IsTrue(result, "Successfully reset all events at the start of the test");

            // Parse the configuration file
            this.testStartTime = DateTime.UtcNow;
            ParseConfig(testStartTime);

            // Create the log and work directories
            Utility.LogDirectory = Path.Combine(testDataDirectory, this.testStartTime.Ticks.ToString(), LogFolderName);
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Log folder: {0}",
                Utility.LogDirectory);
            Utility.InitializeWorkDirectory();

            // Create the deleted items folder
            this.deletedItemsFolder = Path.Combine(testDataDirectory, this.testStartTime.Ticks.ToString(), DeletedItemsFolderName);
            FabricDirectory.CreateDirectory(deletedItemsFolder);
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Deleted items folder: {0}",
                this.deletedItemsFolder);

            // Create the MDS upload consumer
            ConfigReader.AddAppConfig(Utility.WindowsFabricApplicationInstanceId, null);
            var initParam = new ConsumerInitializationParameters(
                Utility.WindowsFabricApplicationInstanceId,
                TestConfigSectionName,
                TestFabricNodeId,
                TestFabricNodeName,
                Utility.LogDirectory,
                Utility.DcaWorkFolder,
                new DiskSpaceManager());

            this.uploader = new MdsEtwEventUploader(initParam);
            EtwCsvFolder = this.uploader.EtwCsvFolder;
        }

        [TestCleanup]
        public void TestMethodCleanup()
        {
            if (null != this.uploader)
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Setting event to allow any pending DTR processing pass to begin ...");
                bool result = EndOfTest.Set();
                Verify.IsTrue(result, "Successfully set event to allow pending DTR processing pass to begin");

                this.uploader.Dispose();
                this.uploader = null;
            }
        }

        private void ParseConfig(DateTime startTime)
        {
            for (int i = 0; i < (int)TraceFileType.Count; i++)
            {
                ParseConfig(startTime, (TraceFileType)i);
            }
        }

        private void ParseConfig(DateTime startTime, TraceFileType traceFileType)
        {
            List<TraceFileInfo> fileInfoList = new List<TraceFileInfo>();

            string paramName = configParamNames[traceFileType];
            string[] valuesAsStrings = this.testContext.DataRow[paramName] as string[];
            foreach (string valueAsString in valuesAsStrings)
            {
                string[] valueParts = valueAsString.Split(new char[] {','}, (int)TraceFileAttribute.Count);
                Verify.IsTrue(valueParts.Length == (int)TraceFileAttribute.Count);
                TraceFileInfo fileInfo = new TraceFileInfo();
                fileInfo.Name = valueParts[(int)TraceFileAttribute.Name].Trim();
                fileInfo.IsLastChunkOfEtl = Boolean.Parse(
                                                    valueParts[(int)TraceFileAttribute.IsLastChunkOfEtl].Trim());
                fileInfo.CreationPass = Int32.Parse(
                                                valueParts[(int)TraceFileAttribute.CreationPass].Trim());
                int lastEventTimestampOffsetMinutes = Int32.Parse(
                                                        valueParts[(int)TraceFileAttribute.LastEventTimestamp].Trim());
                fileInfo.LastEventTimestamp = startTime.AddSeconds(lastEventTimestampOffsetMinutes);
                fileInfo.AllEventsHaveSameTimestamp = Boolean.Parse(
                                                             valueParts[(int)TraceFileAttribute.AllEventsHaveSameTimestamp].Trim());
                int lastWriteTimeOffsetMinutes = Int32.Parse(
                                                    valueParts[(int)TraceFileAttribute.LastWriteTime].Trim());
                fileInfo.LastWriteTime = startTime.AddSeconds(lastWriteTimeOffsetMinutes);
                fileInfo.ProcessedInPass = Int32.Parse(
                                                    valueParts[(int)TraceFileAttribute.ProcessedInPass].Trim());
                fileInfo.DeletionPass = Int32.Parse(
                                                valueParts[(int)TraceFileAttribute.DeletionPass].Trim());
                fileInfo.EmptyFile = Boolean.Parse(
                                         valueParts[(int)TraceFileAttribute.EmptyFile].Trim());
                fileInfoList.Add(fileInfo);
            }

            this.traceFileInfo[traceFileType] = fileInfoList;
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [DataSource("Table:MdsEtwUploaderTestData.xml#MdsEtwEventUpload")]
        public void MdsEtwEventUploadTest()
        {
            for (int i = 0; i < PassCount; i++)
            {
                // Delete the trace files that we need to delete in this pass
                DeleteTraceFiles(i);

                // Create the trace files that we need to create in this pass
                CreateTraceFiles(i);

                // Allow the DTR read pass to proceed
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Setting event to allow DTR processing pass {0} to begin ...",
                    i);
                bool result = StartDtrRead.Set();
                Verify.IsTrue(result, "Successfully set event to allow DTR processing pass to begin");

                // Wait for the DTR read pass to complete
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Waiting for DTR processing pass {0} to finish ...",
                    i);
                result = DtrReadCompleted.WaitOne();
                Verify.IsTrue(result, "Wait for DTR processing pass completion was successful");

                // Verify that the files we expected to be processed in this pass
                // were indeed processed.
                VerifyProcessedFiles(i);
            }

            // Verify that the files we expected to remain unprocessed were not processed
            VerifyUnprocessedFiles();

            // Verify that the total number of records in each table is what we expect
            VerifyTableRecordCount();
        }

        private void CreateTraceFiles(int pass)
        {
            for (int i = 0; i < (int)TraceFileType.Count; i++)
            {
                // The MDS uploader creates a sub-folder at the destination with the same name as the 
                // trace subfolder name at the source. Therefore, we include the test start time in 
                // the trace subfolder name so that we get a different sub-folder at the destination 
                // for each test.
                string traceSubFolderName = Path.Combine(
                                                EtwCsvFolder, 
                                                String.Concat(
                                                    traceSubFolderNames[(TraceFileType)i],
                                                    this.testStartTime.Ticks.ToString()));
                FabricDirectory.CreateDirectory(traceSubFolderName);

                IEnumerable<TraceFileInfo> traceFilesToCreate = this.traceFileInfo[(TraceFileType)i]
                                                                .Where(t => (t.CreationPass == pass));

                foreach (TraceFileInfo traceFileToCreate in traceFilesToCreate)
                {
                    CreateTraceFile(traceFileToCreate, traceSubFolderName);
                }
            }
        }

        private void CreateTraceFile(TraceFileInfo traceFileToCreate, string traceSubFolderName)
        {
            // Compute file name
            int differentiator = traceFileToCreate.IsLastChunkOfEtl ? Int32.MaxValue : 0;
            string fileName = String.Format(
                                "{0}_{1}_{2:D20}_{3}.{4}",
                                TestFabricNodeId,
                                traceFileToCreate.Name,
                                traceFileToCreate.LastEventTimestamp.Ticks,
                                differentiator.ToString("D10"),
                                EtlConsumerConstants.FilteredEtwTraceFileExtension);

            // Compute full path to file
            string fullName = Path.Combine(
                                EtwCsvFolder, 
                                traceSubFolderName, 
                                fileName);

            // Create the file
            Random random = new Random();
            int eventCount = traceFileToCreate.EmptyFile ? 0 : (1 + (random.Next() % MaxPerFileEventCount));
            traceFileToCreate.EventCount = eventCount;
            using (StreamWriter writer = new StreamWriter(fullName))
            {
                for (int i = (eventCount - 1); i >= 0; i--)
                {
                    int offset = 0 - i;
                    DateTime eventTimestamp = traceFileToCreate.AllEventsHaveSameTimestamp ? 
                                                traceFileToCreate.LastEventTimestamp :
                                                traceFileToCreate.LastEventTimestamp.AddSeconds(offset);
                    string eventLevel = eventLevels[random.Next() % eventLevels.Length];
                    int processId = random.Next() % 10000;
                    int threadId = random.Next() % 10000;
                    string taskName = String.Concat(TaskNamePrefix, (random.Next() % 100).ToString());
                    string eventType = String.Concat(EventTypePrefix, (random.Next() % 100).ToString());
                    string id = String.Concat(IdPrefix, random.Next().ToString());
                    // Some events have an id field, some don't
                    string eventTypeWithId = ((random.Next() % 2) == 0) ? eventType : String.Concat(eventType, "@", id);
                    string text = String.Concat(TextPrefix, random.Next().ToString());

                    string eventLine = String.Format(
                                            "{0},{1},{2},{3},{4}.{5},{6}",
                                            eventTimestamp,
                                            eventLevel,
                                            threadId,
                                            processId,
                                            taskName,
                                            eventTypeWithId,
                                            text);
                    writer.WriteLine(eventLine);
                }
            }

            // Set the last write time of the file as specified in the config
            FileInfo fileInfo = new FileInfo(fullName);
            fileInfo.LastWriteTimeUtc = traceFileToCreate.LastWriteTime;

            // Save the full path to the file for future reference
            traceFileToCreate.FullName = fullName;
        }

        private void DeleteTraceFiles(int pass)
        {
            List<TraceFileInfo> traceFilesToDelete = new List<TraceFileInfo>();
            for (int i = 0; i < (int)TraceFileType.Count; i++)
            {
                traceFilesToDelete.AddRange(
                    this.traceFileInfo[(TraceFileType)i]
                    .Where(t => (t.DeletionPass == pass)));
            }

            foreach (TraceFileInfo traceFileToDelete in traceFilesToDelete)
            {
                string fileName = Path.GetFileName(traceFileToDelete.FullName);
                string filePathInDeletedItems = Path.Combine(this.deletedItemsFolder, fileName);
                FabricFile.Move(traceFileToDelete.FullName, filePathInDeletedItems);
            }
        }

        private void VerifyProcessedFiles(int pass)
        {
            for (int i = 0; i < (int)TraceFileType.Count; i++)
            {
                IEnumerable<TraceFileInfo> traceFiles = this.traceFileInfo[(TraceFileType)i]
                                                        .Where(t => (t.ProcessedInPass == pass));

                QueryLocalTable tableQuery = null;
                if (traceFiles.Count() > 0)
                {
                    string dirName = Path.Combine(
                                        testDataDirectory,
                                        String.Concat(
                                            traceSubFolderNames[(TraceFileType)i],
                                            this.testStartTime.Ticks.ToString()));

                    string tableName = String.Concat(
                                            TableName,
                                            MdsEtwEventUploader.TableSchemaVersion,
                                            DefaultTableNonce);
                    tableQuery = new QueryLocalTable(tableName, dirName);
                }
                foreach (TraceFileInfo traceFile in traceFiles)
                {
                    VerifyEventsFromFile(traceFile, tableQuery, (-1 != pass));
                }
            }
        }

        private void VerifyUnprocessedFiles()
        {
            VerifyProcessedFiles(-1);
        }

        private void VerifyEventsFromFile(TraceFileInfo traceFile, QueryLocalTable tableQuery, bool expectProcessed)
        {
            if (traceFile.EmptyFile)
            {
                // We created a file with no events. Nothing to verify here.
                return;
            }

            object[] expectedTableFields = new object[(int)MdsEtwEventUploader.MdsTableFields.Count];

            // Read the events from the table
            TableRecord[] tableRecords = tableQuery.ReadRecords(DateTime.MinValue, DateTime.MaxValue).ToArray();

            // Figure out the full path to the trace file
            string traceFileFullPath;
            if (false == FabricFile.Exists(traceFile.FullName))
            {
                // Maybe the trace file was deleted, i.e. moved to the deleted items folder
                string fileName = Path.GetFileName(traceFile.FullName);
                traceFileFullPath = Path.Combine(this.deletedItemsFolder, fileName);
            }
            else
            {
                traceFileFullPath = traceFile.FullName;
            }

            using (StreamReader reader = new StreamReader(traceFileFullPath))
            {
                // Read event from file
                string eventLine = reader.ReadLine();

                // For this event, get the fields that we expect to see in the table
                string[] eventParts = eventLine.Split(new char[] { ',', '.' });
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.Timestamp] = DateTime.Parse(eventParts[0]);
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.Level] = eventParts[1];
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.ThreadId] = Int32.Parse(eventParts[2]);
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.ProcessId] = Int32.Parse(eventParts[3]);
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.TaskName] = eventParts[4];
                string eventTypeWithId = eventParts[5];
                string[] eventTypeWithIdParts = eventTypeWithId.Split('@');
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.EventType] = eventTypeWithIdParts[0];
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.Id] = (eventTypeWithIdParts.Length > 1) ? eventTypeWithIdParts[1] : String.Empty;
                // ReadLine method removes the '\r' at the end. Add it back.
                expectedTableFields[(int)MdsEtwEventUploader.MdsTableFields.Text] = String.Concat(eventParts[6], "\r");

                // Search the table to see if we have a match
                bool matchFound = false;
                foreach (TableRecord tableRecord in tableRecords)
                {
                    // The timestamp when the event was written to the table (NOT to be confused with
                    // timestamp when the event occurred) also shows up as a field in the record that
                    // we read. However, it is not a field in the record that we write. Looks like MDS
                    // automatically adds the extra field.
                    if ((1 + expectedTableFields.Length) != tableRecord.FieldData.Length)
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Found a record in the table whose schema does not match the expected schema. Record field tags are: {0}",
                            String.Join(",", tableRecord.FieldTags));
                        Verify.Fail("Unexpected schema for table record");
                    }

                    bool currentRecordMatchesEvent = true;
                    for (int i = 0; i < expectedTableFields.Length; i++)
                    {
                        if (false == expectedTableFields[i].Equals(tableRecord.FieldData[i+1]))
                        {
                            currentRecordMatchesEvent = false;
                            break;
                        }
                    }
                    if (currentRecordMatchesEvent)
                    {
                        matchFound = true;
                        break;
                    }
                }

                if (expectProcessed != matchFound)
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "{0} Event info: {1}",
                        expectProcessed ?
                            "An event that we expected to find in the table was not found." :
                            "Event unexpectedly found in table.",
                        eventLine);
                    Verify.Fail("Unexpected event processing status");
                }
            }
        }

        private void VerifyTableRecordCount()
        {
            for (int i = 0; i < (int)TraceFileType.Count; i++)
            {
                IEnumerable<TraceFileInfo> traceFiles = this.traceFileInfo[(TraceFileType)i]
                                                        .Where(t => (t.ProcessedInPass != -1));
                int expectedEventCount = 0;
                foreach (TraceFileInfo traceFile in traceFiles)
                {
                    expectedEventCount += traceFile.EventCount;
                }

                QueryLocalTable tableQuery = null;
                string dirName = Path.Combine(
                                    testDataDirectory,
                                    String.Concat(
                                        traceSubFolderNames[(TraceFileType)i],
                                        this.testStartTime.Ticks.ToString()));
                string tableName = String.Concat(
                                        TableName,
                                        MdsEtwEventUploader.TableSchemaVersion,
                                        DefaultTableNonce);
                tableQuery = new QueryLocalTable(tableName, dirName);

                IEnumerable<TableRecord> tableRecords = tableQuery.ReadRecords(DateTime.MinValue, DateTime.MaxValue);
                int eventCountInTable = tableRecords.Count();

                if (eventCountInTable != expectedEventCount)
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Unexpected event count in table. Expected count: {0}, actual count: {1}",
                        expectedEventCount,
                        eventCountInTable);
                    Verify.Fail("Unexpected event count in table");
                }
            }
        }
    }
}