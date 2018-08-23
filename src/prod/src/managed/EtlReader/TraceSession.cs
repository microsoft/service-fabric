// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading;

    public sealed class TraceSession : IDisposable
    {
        private const uint WnodeFlagTracedGuid = 0x00020000;
        private const uint EventTraceControlQuery = 0x0;
        private const uint EventTraceControlStop = 0x1;
        private const uint EventTraceRealTimeMode = 0x00000100;
        private const uint EventTraceFileModeCircular = 0x00000002;
        private const uint ProcessTraceModeEventRecord = 0x10000000;
        private const uint ProcessTraceModeRealTime = 0x00000100;

        private static readonly int EventTracePropertiesSize = Marshal.SizeOf(typeof(EventTraceProperties));
        private static readonly ulong InvalidHandle = 0x00000000FFFFFFFF ;

        private readonly ProviderInfo providerInfo;
        private readonly TimeSpan flushInterval;
        private readonly Guid sessionId;
        private readonly string sessionName;

        private ulong sessionHandle;
        private bool traceEnabled;
        private ulong traceHandle;
        private GCHandle callbackHandle;

        public TraceSession(string name, ProviderInfo providerInfo, TimeSpan flushInterval)
        {
            this.providerInfo = providerInfo;
            this.flushInterval = flushInterval;
            this.sessionId = Guid.NewGuid();
            this.sessionName = name;

            EventTraceProperties properties = TraceSession.CreateProperties(this.sessionId, false);
            int errorCode = NativeMethods.StartTrace(out this.sessionHandle, this.sessionName, ref properties);
            if (errorCode != 0)
            {
                throw new Win32Exception();
            }
        }

        ~TraceSession()
        {
            this.Dispose(false);
        }

        public event EventHandler<EventRecordEventArgs> EventRead;

        public uint EventsLost
        {
            get;
            private set;
        }

        public void Open()
        {
            Guid localProviderId = this.providerInfo.Id;
            int errorCode = NativeMethods.EnableTraceEx(
                ref localProviderId,
                IntPtr.Zero,
                this.sessionHandle,
                1,
                this.providerInfo.Level,
                (ulong)this.providerInfo.Keywords,
                0,
                0,
                IntPtr.Zero);
            if (errorCode != 0)
            {
                throw new Win32Exception();
            }

            this.traceEnabled = true;
            EventTraceLogfile logFile = new EventTraceLogfile();
            logFile.ProcessTraceMode = TraceSession.ProcessTraceModeRealTime | TraceSession.ProcessTraceModeEventRecord;
            logFile.LoggerName = this.sessionName;

            // The callback must be pinned to keep it in scope outside of this method
            EventRecordCallback callback = new EventRecordCallback(this.EventRecordCallback);
            this.callbackHandle = GCHandle.Alloc(callback);
            logFile.EventRecordCallback = callback;

            this.traceHandle = NativeMethods.OpenTrace(ref logFile);
            if (this.traceHandle == InvalidHandle)
            {
                throw new Win32Exception();
            }
        }

        public void Process()
        {
            int errorCode;
            using (Timer timer = new Timer(this.OnTimerElapsed, null, Timeout.Infinite, Timeout.Infinite))
            {
                if (this.flushInterval > TimeSpan.Zero)
                {
                    timer.Change(this.flushInterval, Timeout.InfiniteTimeSpan);
                }

                unsafe
                {
                    errorCode = NativeMethods.ProcessTrace(new ulong[] { this.traceHandle }, 1, null, null);
                }

                if (errorCode != 0)
                {
                    throw new Win32Exception();
                }
            }
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private static EventTraceProperties CreateProperties(Guid sessionId, bool createForQuery)
        {
            EventTraceProperties properties = new EventTraceProperties();
            properties.Wnode = new EventTraceProperties.WnodeHeader();
            properties.Wnode.BufferSize = (uint)TraceSession.EventTracePropertiesSize;
            properties.Wnode.Flags = TraceSession.WnodeFlagTracedGuid;
            properties.Wnode.Guid = sessionId;
            properties.FlushTimer = 0;
            properties.LogFileMode = createForQuery ? 0 : TraceSession.EventTraceRealTimeMode;
            properties.LoggerNameOffset = (uint)(TraceSession.EventTracePropertiesSize -
                                                    (Marshal.SystemDefaultCharSize * 
                                                        (EventTraceProperties.MaxLoggerNameLength + 
                                                         EventTraceProperties.MaxLogFileNameLength)));
            properties.LogFileNameOffset = (uint)(TraceSession.EventTracePropertiesSize -
                                                    (Marshal.SystemDefaultCharSize * EventTraceProperties.MaxLogFileNameLength));

            return properties;
        }

        private uint EventRecordCallback([In] ref EventRecord eventRecord)
        {
            this.EventRead(this, new EventRecordEventArgs(eventRecord));
            return uint.MaxValue;
        }

        private void OnTimerElapsed(object sender)
        {
            EventTraceProperties properties = TraceSession.CreateProperties(this.sessionId, false);
            if (NativeMethods.FlushTrace(this.sessionHandle, this.sessionName, ref properties) == 0)
            {
                this.EventsLost = properties.EventsLost;
            }
        }

        [SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", MessageId = "Tools.EtlReader.NativeMethods.EnableTraceEx(System.Guid@,System.IntPtr,System.UInt64,System.UInt32,System.Byte,System.UInt64,System.UInt64,System.UInt32,System.IntPtr)", Justification = "Best effort cleanup, nothing can be done if this returns an error.")]
        [SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", MessageId = "Tools.EtlReader.NativeMethods.ControlTrace(System.UInt64,System.String,Tools.EtlReader.EventTraceProperties@,System.UInt32)", Justification = "Best effort cleanup, nothing can be done if this returns an error.")]
        [SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", MessageId = "Tools.EtlReader.NativeMethods.CloseTrace(System.UInt64)", Justification = "Best effort cleanup, nothing can be done if this returns an error")]
        private void Dispose(bool disposing)
        {
            if (this.traceHandle != TraceSession.InvalidHandle)
            {
                NativeMethods.CloseTrace(this.traceHandle);
                this.traceHandle = TraceSession.InvalidHandle;
            }

            if (this.traceEnabled)
            {
                Guid localProviderId = this.providerInfo.Id;
                NativeMethods.EnableTraceEx(
                    ref localProviderId,
                    IntPtr.Zero,
                    this.sessionHandle,
                    0,
                    0,
                    0,
                    0,
                    0,
                    IntPtr.Zero);
                this.traceEnabled = false;
            }

            if (this.sessionHandle != 0)
            {
                EventTraceProperties properties = TraceSession.CreateProperties(this.sessionId, false);
                NativeMethods.ControlTrace(this.sessionHandle, null, ref properties, TraceSession.EventTraceControlStop);
                this.sessionHandle = 0;
            }

            if (disposing)
            {
                if (this.callbackHandle.IsAllocated)
                {
                    this.callbackHandle.Free();                    
                }
            }
        }

        public static string GetLogFilePath(string traceSessionName)
        {
            EventTraceProperties properties = TraceSession.CreateProperties(Guid.Empty, true);
            int errorCode = NativeMethods.ControlTrace(0, traceSessionName, ref properties, TraceSession.EventTraceControlQuery);
            if (errorCode != 0)
            {
                throw new Win32Exception(errorCode);
            }
            return Path.GetDirectoryName(properties.LogFileName);
        }
    }
}