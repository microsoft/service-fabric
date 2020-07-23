// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Tools.EtlReader;
    using TraceDiagnostic.Store;

    /// <summary>
    /// Represents the event storage in LogRoot traces
    /// </summary>
    internal sealed class LocalEventStore : IEventStore
    {
        /// <summary>
        /// Name of the manifest file
        /// </summary>
        private const string WindowsFabricManifestFileName = "Microsoft-ServiceFabric-Events.man";

        /// <summary>
        /// Trace file name pattern
        /// </summary>
        private const string TraceFilePattern = "query_traces_*.etl";

        /// <summary>
        /// Trace folder in LogRoot
        /// </summary>
        private const string TraceFolder = @"\QueryTraces";

        /// <summary>
        /// LogRoot folder
        /// </summary>
        private const string LogFolder = @"\log";
        private const string WorkFolder = "work";

        private const string NodesTable = "Nodes";

        private const string PartitionsTable = "Partitions";

        private const string TableName = "TableName";

        /// <summary>
        /// Regex pattern to extract EventType from trace record type
        /// </summary>
        private static readonly Regex TableAndEventTypeRegex = new Regex(@"^.*?_(?<TableName>.*?)_(?<EventType>[^@]*)@?", RegexOptions.Compiled);

        /// <summary>
        /// Cache for the event manifest
        /// </summary>
        private static ManifestCache manifestCache;

        /// <summary>
        /// Container for constraints on events of interest
        /// </summary>
        private readonly Dictionary<string, HashSet<string>> equalsEvents = new Dictionary<string, HashSet<string>>();

        /// <summary>
        /// Table whose markers need to be considered for a client request
        /// </summary>
        private readonly HashSet<string> activeTables = new HashSet<string>();

        /// <summary>
        /// Container for Add*Event delegates
        /// </summary>
        private readonly Dictionary<string, EventAdder> eventAdders = new Dictionary<string, EventAdder>();

        /// <summary>
        /// Parser for ETL trace files
        /// </summary>
        private TraceFileParser traceFilerParser;

        /// <summary>
        /// Container for events of interest
        /// </summary>
        private List<string> inEvents = new List<string>();

        /// <summary>
        /// Events whose trace records contain _Nodes_ marker
        /// </summary>
        private List<NodeEvent> nodeEvents = new List<NodeEvent>();

        /// <summary>
        /// Events whose trace records contain _Partitions_ marker
        /// </summary>
        private List<PartitionEvent> partitionEvents = new List<PartitionEvent>();

        /// <summary>
        /// Initializes a new instance of the <see cref="LocalEventStore"/> class
        /// </summary>
        public LocalEventStore()
        {
            LoadManifestCache();
            this.LoadEventAdders();
            this.InitializeTraceFileParser();
        }

        /// <summary>
        /// Delegate for Add*Event method
        /// </summary>
        /// <param name="record">The trace record pertaining to an event</param>
        /// <param name="captures">The properties captured from the trace record</param>
        /// <param name="eventType">The type of event, e.g. node</param>
        private delegate void EventAdder(TraceRecord record, Dictionary<string, string> captures, string eventType);

        /// <summary>
        /// Retrieves the node events of interest
        /// </summary>
        /// <param name="nodeId">Node Id</param>
        /// <param name="startTime">Start time of the interval of interest</param>
        /// <param name="endTime">End time of the interval of interest</param>
        /// <param name="filteringCriteria">Constraints on the events of interest; equivalent to
        /// fieldName1 in (p11, p12, ...) AND fieldName2 in (p21, p22, ...) AND ...</param>
        /// <param name="inEventTypes">Events of interest</param>
        /// <returns>List of NodeEvents</returns>
        public IEnumerable<NodeEvent> GetNodeEvent(
            string nodeId,
            DateTime? startTime = null,
            DateTime? endTime = null,
            string[] inEventTypes = null,
            Dictionary<string, string[]> filteringCriteria = default(Dictionary<string, string[]>))
        {
            if (!string.IsNullOrEmpty(nodeId))
            {
                this.equalsEvents.Add(FieldNames.NodeId, new HashSet<string>() { nodeId });
            }

            if (filteringCriteria != default(Dictionary<string, string[]>))
            {
                foreach (var kvp in filteringCriteria)
                {
                    this.equalsEvents.Add(kvp.Key, new HashSet<string>(kvp.Value));
                }
            }

            if (inEventTypes != null)
            {
                inEvents.AddRange(inEventTypes);
            }

            this.activeTables.Add(NodesTable);

            this.nodeEvents.Clear();

            // nodeEvents is populated again
            this.ExtractEvents(startTime, endTime);

            return this.nodeEvents;
        }

        /// <summary>
        /// Retrieves the partition events of interest
        /// </summary>
        /// <param name="partitionId">Partition Id</param>
        /// <param name="startTime">Start time of the interval of interest</param>
        /// <param name="endTime">End time of the interval of interest</param>
        /// <param name="eventTypes">Events of interest</param>
        /// <param name="count">Number of events to be retrieved (not used)</param>
        /// <returns>List of NodeEvents</returns>
        public IEnumerable<PartitionEvent> GetPartitionEvent(
            string partitionId,
            DateTime? startTime = null,
            DateTime? endTime = null,
            string[] eventTypes = null,
            int count = 0)
        {
            if (!string.IsNullOrEmpty(partitionId))
            {
                this.equalsEvents.Add(FieldNames.PartitionId, new HashSet<string>() { partitionId });

            }

            if (eventTypes != null)
            {
                inEvents.AddRange(eventTypes);
            }

            this.activeTables.Add(PartitionsTable);

            this.partitionEvents.Clear();

            // partitionEvents is populated again
            this.ExtractEvents(startTime, endTime);

            return this.partitionEvents.GetRange(0, Math.Min(count, this.partitionEvents.Count));
        }

        /// <summary>
        /// <remarks>Adapted from DCA</remarks>
        /// </summary>
        private static void LoadManifestCache()
        {
            try
            {
                string manifestFileDirectory = FabricEnvironment.GetCodePath();

                // Get the Windows Fabric manifest file
                string manifestFile = Path.Combine(
                                          manifestFileDirectory,
                                          WindowsFabricManifestFileName);

                // Load the manifest
                manifestCache = new ManifestCache(Path.Combine(GetLogRoot(), WorkFolder));
                manifestCache.LoadManifest(manifestFile);
            }
            catch (Exception e)
            {
                Console.WriteLine(StringResources.EventStoreError_EventManifestLoadFailure, e.Message);
            }
        }

        /// <summary>
        /// Gets the path of the log root directory from FabricEnvironment
        /// </summary>
        /// <returns>Path of the log root directory</returns>
        private static string GetLogRoot()
        {
            string logroot = FabricEnvironment.GetLogRoot();
            if (string.IsNullOrEmpty(logroot))
            {
                logroot = FabricEnvironment.GetDataRoot() + LogFolder;
            }

            if (string.IsNullOrEmpty(logroot))
            {
                throw new InvalidOperationException(StringResources.EventStoreError_LogrootNotFound);
            }

            return logroot;
        }

        /// <summary>
        /// From the last element towards the first, searches for the file that might contain events around the target time
        /// </summary>
        /// <param name="files">Array of files sorted by creation time</param>
        /// <param name="targetTime">The time of interest</param>
        /// <returns>Index of the file in the array or -1 if targetTime is before the creation of the first file</returns>
        private static int ReverseLinearSearch(FileInfo[] files, DateTime targetTime)
        {
            int i = files.Length - 1;
            for (; i >= 0; i--)
            {
                int comparedValue = targetTime.CompareTo(files[i].CreationTime.ToUniversalTime());
                if (comparedValue >= 0)
                {
                    break;
                }
            }

            return i;
        }

        /// <summary>
        /// From trace folder, gets the paths for trace files whose creation dates fall in the interval
        /// </summary>
        /// <param name="start">Start time of the interval</param>
        /// <param name="end">End time of the interval</param>
        /// <returns>Trace file paths</returns>
        private static string[] GetInRangeTraceFiles(DateTime start, DateTime end)
        {
            var dir = new DirectoryInfo(GetLogRoot() + TraceFolder);

            if (!dir.Exists)
            {
                return null;
            }

            var sortedFiles = dir.GetFiles(TraceFilePattern, SearchOption.TopDirectoryOnly)
                .OrderBy(f => f.CreationTime)
                .ToArray();

            if (sortedFiles.Length == 0)
            {
                return null;
            }

            int startIndex = ReverseLinearSearch(sortedFiles, start);
            int endIndex = ReverseLinearSearch(sortedFiles, end);

            if (endIndex < 0)
            {
                return null;
            }

            startIndex = Math.Max(0, startIndex);

            var inRangeFiles = new string[endIndex - startIndex + 1];
            for (int i = startIndex, j = 0; i <= endIndex; i++, j++)
            {
                inRangeFiles[j] = sortedFiles[i].FullName;
            }

            return inRangeFiles;
        }

        /// <summary>
        /// Loads delegates for AddEvent methods
        /// </summary>
        private void LoadEventAdders()
        {
            this.eventAdders.Add(NodesTable, this.AddNodeEvent);
            this.eventAdders.Add(PartitionsTable, this.AddPartitionEvent);
        }

        /// <summary>
        /// Instantiates the trace file parser
        /// </summary>
        private void InitializeTraceFileParser()
        {
            this.traceFilerParser = new TraceFileParser(manifestCache, this.ProcessTraceRecord);
        }

        /// <summary>
        /// Creates a NodeEvent object using the property values in captures and adds to the list
        /// </summary>
        /// <param name="traceRecord">Trace record pertaining to the PartitionEvent</param>
        /// <param name="captures">Container for the event properties</param>
        /// <param name="eventType">The type of event, e.g. node</param>
        private void AddNodeEvent(TraceRecord traceRecord, Dictionary<string, string> captures, string eventType)
        {
            if (!string.IsNullOrEmpty(eventType))
            {
                this.nodeEvents.Add(
                    EntityHelperLocal.CreateNodeEvent(
                        eventType,
                        captures[FieldNames.PartitionKey],
                        traceRecord.Time,
                        captures));
            }
        }

        /// <summary>
        /// Creates a PartitionEvent object using the property values in captures and adds to the list
        /// </summary>
        /// <param name="traceRecord">Trace record pertaining to the PartitionEvent</param>
        /// <param name="captures">Container for the event properties</param>
        /// <param name="eventType">The type of event, e.g. node</param>
        private void AddPartitionEvent(TraceRecord traceRecord, Dictionary<string, string> captures, string eventType)
        {
            if (!string.IsNullOrEmpty(eventType))
            {
                this.partitionEvents.Add(
                    EntityHelperLocal.CreatePartitionEvent(
                        eventType,
                        captures[FieldNames.PartitionKey],
                        traceRecord.Time,
                        captures));
            }
        }

        /// <summary>
        /// Extracts information from a given trace record and loads into an appropriate event object
        /// </summary>
        /// <param name="record">The parsed record</param>
        private void ProcessTraceRecord(string record)
        {
            var fields = record.Split(',');
            var traceRecord = new TraceRecord
            {
                Time = DateTime.Parse(fields[0], CultureInfo.InvariantCulture),
                Type = fields[4]
            };

            var tableAndEvent = TableAndEventTypeRegex.MatchNamedCaptures(traceRecord.Type);

            // The entire input is always included in case of a successful regex match
            if (tableAndEvent.Count == 1)
            {
                return;
            }

            string tableName = tableAndEvent[TableName];
            string eventType = tableAndEvent[FieldNames.EventType];

            if (this.inEvents.Count == 0
                || !this.activeTables.Contains(tableName))
            {
                return;
            }

            var matchedEvent = this.inEvents.Find(e => e.Equals(eventType, StringComparison.Ordinal));

            if (string.IsNullOrEmpty(matchedEvent))
            {
                return;
            }

            var captures = RegexHelper.EventRegexes[matchedEvent].MatchNamedCaptures(record);
            captures.Remove("0"); // first capture is the whole expression


            if (this.equalsEvents.Count > 0)
            {
                var commons = from kvp1 in this.equalsEvents
                              join kvp2 in captures on kvp1.Key equals kvp2.Key
                              select new { value1 = kvp1.Value, value2 = kvp2.Value };
                if (!commons.All(x => x.value1.Contains(x.value2)))
                {
                    return;
                }
            }

            this.eventAdders[tableName](traceRecord, captures, eventType);
        }

        /// <summary>
        /// Resets the event containers for the next client request
        /// </summary>
        private void Reset()
        {
            this.inEvents.Clear();
            this.equalsEvents.Clear();
            this.activeTables.Clear();
        }

        /// <summary>
        /// Extracts the trace records between the start and end times from the trace files in LogRoot
        /// </summary>
        /// <param name="startTime">Start time of the interval of interest</param>
        /// <param name="endTime">End time of the interval of interest</param>
        private void ExtractEvents(DateTime? startTime, DateTime? endTime)
        {
            var start = startTime ?? DateTime.Now.AddYears(-1);
            var end   = endTime ?? DateTime.Now.AddYears(1);
            if (start.CompareTo(end) > 0)
            {
                this.Reset();
                return;
            }

            var traceFilePaths = GetInRangeTraceFiles(start, end);

            if (traceFilePaths == null)
            {
                this.Reset();
                return;
            }

            try
            {
                Array.ForEach(traceFilePaths, f => this.traceFilerParser.ParseTraces(f, start, end));
            }
            finally
            {
                this.Reset();
            }
        }


        /// <summary>
        /// Represents a trace record
        /// </summary>
        private struct TraceRecord
        {
            public DateTime Time { get; set; }

            public string Type { get; set; }
        }
    }
}


#pragma warning restore 1591