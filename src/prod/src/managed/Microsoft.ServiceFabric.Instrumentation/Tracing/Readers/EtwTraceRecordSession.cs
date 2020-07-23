// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Reader
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Tools.EtlReader;

    /// <summary>
    /// An ETW file Reader. TODO Stop using ETL reader, we don't need it.
    /// </summary>
    public class EtwTraceRecordSession : TraceRecordSession
    {
        private IList<string> traceFiles;
        private CancellationToken token;

        public EtwTraceRecordSession(string file) : this(new List<string> { file })
        {
        }

        public EtwTraceRecordSession(IList<string> files)
        {
            Assert.IsNotNull(files, "List of files can't be null");
            foreach (var oneFile in files)
            {
                if (!File.Exists(oneFile))
                {
                    throw new FileNotFoundException(oneFile);
                }
            }

            this.traceFiles = files;
        }

        /// <inheritdoc />
        public override bool IsServerSideFilteringAvailable
        {
            get { return false; }
        }

        /// <inheritdoc />
        public override Task StartReadingAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken cancelToken)
        {
            this.token = cancelToken;
            foreach (var oneFile in this.traceFiles)
            {
                using (var etwReader = new TraceFileEventReader(oneFile))
                {
                    etwReader.EventRead += this.Reader_TraceRead;
                    etwReader.ReadEvents(startTime.UtcDateTime, endTime.UtcDateTime);
                }
            }

            return Task.FromResult(true);
        }

        /// <inheritdoc />
        public override Task StartReadingAsync(
            IList<TraceRecordFilter> filters,
            DateTimeOffset startTime,
            DateTimeOffset endTime,
            CancellationToken cancelToken)
        {
            throw new NotImplementedException();
        }

        private void Reader_TraceRead(object sender, EventRecordEventArgs e)
        {
            if (this.token.IsCancellationRequested)
            {
                // If cancellation has been request, let the reader know that it's time to stop.
                e.Cancel = true;

                // Call completed.
                this.OnCompleted();

                return;
            }

            if(e.Record.EventHeader.EventDescriptor.Version < MinimumVersionSupported)
            {
                return;
            }

            var traceRecord = this.Lookup(new TraceIdentity(e.Record.EventHeader.EventDescriptor.Id, (TaskName)e.Record.EventHeader.EventDescriptor.Task));

            if (traceRecord?.Target == null)
            {
                return;
            }

            traceRecord.Level = (TraceLevel)e.Record.EventHeader.EventDescriptor.Level;
            traceRecord.TracingProcessId = e.Record.EventHeader.ProcessId;
            traceRecord.ThreadId = e.Record.EventHeader.ThreadId;
            traceRecord.TimeStamp = DateTime.FromFileTimeUtc(e.Record.EventHeader.TimeStamp);
            traceRecord.Version = e.Record.EventHeader.EventDescriptor.Version;
            traceRecord.PropertyValueReader = ValueReaderCreator.CreateRawByteValueReader(traceRecord.GetType(), traceRecord.Version, e.Record.UserData, e.Record.UserDataLength);

            this.DispatchAsync(traceRecord, CancellationToken.None).GetAwaiter().GetResult();
        }
    }
}