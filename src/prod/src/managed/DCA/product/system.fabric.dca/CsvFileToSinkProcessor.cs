// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using Collections.Generic;
    using Common.Tracing;
    using Diagnostics;
    using System;
    using System.Collections.ObjectModel;
    using System.IO;
    using System.Text;

    // This class implements the generic logic to process ETW traces, independent
    // of the destination to which they are being sent
    internal class CsvFileToSinkProcessor
    {
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        private FileInfo fileInfo;
        private ReadOnlyCollection<ICsvFileSink> sinks;

        private enum CsvEventLineParts
        {
            Timestamp,
            ProcessId,
            ThreadId,
            EventType,
            Level,
            EventText,
            Count
        }

        private Dictionary<string, byte> LevelMap = new Dictionary<string, byte>()
        {
            { "Silent", 0},
            { "Critical", 1},
            { "Error", 2},
            { "Warning", 3},
            { "Info", 4},
            { "Informational", 4},  // added for compatibility with EtlReader generated .dtr
            { "Noise", 5}
        };

        public CsvFileToSinkProcessor(
            FileInfo fileInfo,
            ReadOnlyCollection<ICsvFileSink> sinks,
            string logSourceId,
            FabricEvents.ExtensionsEvents TraceSource)
        {
            this.fileInfo = fileInfo;
            this.sinks = sinks;
            this.logSourceId = logSourceId;
            this.traceSource = TraceSource;
        }

        public void ProcessFile()
        {
            StreamReader tableStream = new StreamReader(File.OpenRead(this.fileInfo.FullName));
            List<string> listA = new List<string>();

            foreach (var sink in this.sinks)
            {
                sink.OnCsvEventProcessingPeriodStart();
            }

            List<CsvEvent> parsedEventList = new List<CsvEvent>();
            while (!tableStream.EndOfStream)
            {
                var line = new StringBuilder();

                while (!tableStream.EndOfStream)
                {
                    var tableChar = tableStream.Read();
                    // Do not put \r and \n in the trace but do consume it from stream
                    if (tableChar == '\r' && tableStream.Peek() == '\n')
                    {
                        // Consume the \n in the end too which we peeked
                        tableStream.Read();
                        break;
                    }

                    line.Append(Convert.ToChar(tableChar));
                }

                CsvEvent csvEvent;
                
                try
                {
                    this.ParseEvent(line.ToString(), out csvEvent);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Exception encountered while parsing event. The event will be skipped. Event: {0}. Exception information: {1}",
                        line,
                        e);
                    continue;
                }

                // Our attempt to decode the event succeeded. Put it
                // in the event list.
                parsedEventList.Add(csvEvent);
            }
            PublishEvent(parsedEventList);

            foreach (var sink in this.sinks)
            {
                sink.OnCsvEventProcessingPeriodStop();
            }
        }

        private void PublishEvent(List<CsvEvent> events)
        {
            foreach (var sink in this.sinks)
            {
                sink.OnCsvEventsAvailable(events);
            }
        }

        private void ParseEvent(string eventRecord, out CsvEvent csvEvent)
        {
            csvEvent = new CsvEvent();

            var parts = eventRecord.Split(new char[] {','}, 6);

            csvEvent.Timestamp = DateTime.Parse(parts[0]);
            csvEvent.Level = this.LevelMap[parts[1]];
            csvEvent.ProcessId = uint.Parse(parts[2]);
            csvEvent.ThreadId = uint.Parse(parts[3]);
            csvEvent.EventType = parts[4];

            var eventNameIndex = csvEvent.EventType.IndexOf('.');
            csvEvent.TaskName = csvEvent.EventType.Substring(0, eventNameIndex);
            csvEvent.EventName = csvEvent.EventType.Substring(eventNameIndex + 2);
            csvEvent.EventNameId = "";

            csvEvent.EventTypeWithoutId = csvEvent.EventType;
            var eventNameIdIndex = csvEvent.EventType.IndexOf('@');
            if (eventNameIdIndex != -1)
            {
                csvEvent.EventTypeWithoutId = csvEvent.EventType.Substring(0, eventNameIdIndex);
            }

            eventNameIdIndex = csvEvent.EventName.IndexOf('@');
            if (eventNameIdIndex != -1)
            {
                var tempEventName = csvEvent.EventName;
                csvEvent.EventName = tempEventName.Substring(0, eventNameIdIndex);
                csvEvent.EventNameId = tempEventName.Substring(eventNameIdIndex + 1);
            }

            eventNameIndex = csvEvent.EventName.LastIndexOf('_');
            csvEvent.EventName = csvEvent.EventName.Substring(eventNameIndex + 1);

            csvEvent.EventText = parts[5];
            csvEvent.StringRepresentation = eventRecord;

            // If this is an FMM event, update the last timestamp of FMM events
            if (csvEvent.TaskName.Equals(Utility.FmmTaskName))
            {
                Utility.LastFmmEventTimestamp = csvEvent.Timestamp;
            }

            if (csvEvent.IsMalformed())
            {
                throw new ArgumentException("The event is malformed");
            }
        }
    }
}