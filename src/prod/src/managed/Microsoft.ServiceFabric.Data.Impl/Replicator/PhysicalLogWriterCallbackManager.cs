// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Threading.Tasks;

    internal class PhysicalLogWriterCallbackManager : IDisposable
    {
        // Callback related datastructures
        private readonly object callbackLock = new object();

        private PhysicalSequenceNumber flushedPsn;

        private Action<LoggedRecords> flushedRecordsCallback;

        private bool isCallingback;

        private bool isDisposed;

        private List<LoggedRecords> loggedRecords;

        private ITracer tracer;

        // Used by the logmanager to enable tracing
        internal PhysicalLogWriterCallbackManager(Action<LoggedRecords> flushedRecordsCallback, ITracer tracer)
        {
            this.flushedRecordsCallback = flushedRecordsCallback;
            this.tracer = tracer;
            this.isDisposed = false;

            this.isCallingback = false;
            this.loggedRecords = null;
            this.flushedPsn = PhysicalSequenceNumber.InvalidPsn;
        }

        // Used by the replicator to disable tracing
        internal PhysicalLogWriterCallbackManager(Action<LoggedRecords> flushedRecordsCallback)
        {
            this.flushedRecordsCallback = flushedRecordsCallback;
            this.tracer = null;
            this.isDisposed = false;

            this.isCallingback = false;
            this.loggedRecords = null;
            this.flushedPsn = PhysicalSequenceNumber.InvalidPsn;
        }

        ~PhysicalLogWriterCallbackManager()
        {
            this.Dispose(false);
        }

        internal Action<LoggedRecords> Callback
        {
            get { return this.flushedRecordsCallback; }
        }

        internal PhysicalSequenceNumber FlushedPsn
        {
            get { return this.flushedPsn; }

            set { this.flushedPsn = value; }
        }

        public void Dispose()
        {
            this.Dispose(true);
        }

        internal void ProcessFlushedRecords(LoggedRecords flushedRecords)
        {
            lock (this.callbackLock)
            {
                if (this.isCallingback == false)
                {
                    Utility.Assert(this.loggedRecords == null, "this.loggedRecords == null");
                    this.isCallingback = true;
                }
                else
                {
                    if (this.loggedRecords == null)
                    {
                        this.loggedRecords = new List<LoggedRecords>();
                    }

                    this.loggedRecords.Add(flushedRecords);
                    return;
                }
            }

            var records = new List<LoggedRecords>();
            records.Add(flushedRecords);
            if (this.flushedPsn == PhysicalSequenceNumber.InvalidPsn)
            {
                this.flushedPsn = flushedRecords.Records[0].Psn;
            }

            Task.Factory.StartNew(this.InvokeFlushedRecordsCallback, records).IgnoreExceptionVoid();
            return;
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing == true)
            {
                if (this.isDisposed == false)
                {
                    lock (this.callbackLock)
                    {
                        if (this.loggedRecords != null)
                        {
                            foreach (var records in this.loggedRecords)
                            {
                                Utility.Assert(records.Exception != null, "records.Exception != null");
                            }
                        }
                    }

                    GC.SuppressFinalize(this);
                    this.isDisposed = true;
                }
            }

            return;
        }

        private void InvokeFlushedRecordsCallback(object state)
        {
            var flushedRecords = (List<LoggedRecords>) state;
            var callback = this.flushedRecordsCallback;
            while (true)
            {
                foreach (var loggedRecords in flushedRecords)
                {
                    var records = loggedRecords.Records;
                    var exception = loggedRecords.Exception;
                    if (exception != null)
                    {
                        foreach (var record in records)
                        {
                            this.TraceLoggingException(record, exception);
                        }
                    }
                    else
                    {
                        Utility.Assert(
                            this.flushedPsn == records[0].Psn,
                            "this.flushedPsn {0} == records[0].Psn {1}",
                            this.flushedPsn, records[0].Psn);

                        this.flushedPsn += records.Count;
                    }

                    try
                    {
                        callback(loggedRecords);
                    }
                    catch (Exception e)
                    {
                        Utility.ProcessUnexpectedException(
                            "PhysicalLogWriterCallbackManager::InvokeFlushedRecordsCallback",
                            this.tracer,
                            "invoking flushed records callback",
                            e);
                    }
                }

                lock (this.callbackLock)
                {
                    Utility.Assert(this.isCallingback == true, "this.isCallingback == true");
                    if (this.loggedRecords == null)
                    {
                        this.isCallingback = false;
                        break;
                    }

                    flushedRecords = this.loggedRecords;
                    this.loggedRecords = null;
                }
            }

            return;
        }

        private void TraceLoggingException(LogRecord record, Exception exception)
        {
            if (this.tracer != null)
            {
                var trace = Environment.NewLine + "    Failed to log record. Type: {0} LSN: {1}" + Environment.NewLine
                            + "    Logging exception. Type: {2} Message: {3} HResult: 0x{4:X8}" + Environment.NewLine;

                this.tracer.WriteWarning(
                    "PhysicalLogWriterCallbackManager::TraceLogException",
                    trace,
                    record.RecordType,
                    record.Lsn.LSN,
                    exception.GetType().ToString(),
                    exception.Message,
                    exception.HResult);
            }

            return;
        }
    }

    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "related classes")]
    internal class LoggedRecords
    {
        private Exception exception;

        private List<LogRecord> records;

        internal LoggedRecords(List<LogRecord> records, Exception exception)
        {
            this.records = records;
            this.exception = exception;
        }

        internal LoggedRecords(LogRecord record, Exception exception)
        {
            this.records = new List<LogRecord>();
            this.records.Add(record);
            this.exception = exception;
        }

        internal Exception Exception
        {
            get { return this.exception; }
        }

        internal List<LogRecord> Records
        {
            get { return this.records; }
        }
    }
}