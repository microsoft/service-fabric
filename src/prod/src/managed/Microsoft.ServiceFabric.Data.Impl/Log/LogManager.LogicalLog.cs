// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log
{
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Data.Log.Interop;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Testability;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// LogManager.LogicalLog implementation
    /// </summary>
    public static partial class LogManager
    {
        internal partial class LogicalLog : ILogicalLog
        {
            private string _TraceType;

            private int _IsOpen;
            private readonly Guid _Id;
            private readonly Guid _OwnerId;
            private readonly long _OwningHandleId;
            private readonly IPhysicalLogStream _UnderlyingStream;
            private readonly uint _BlockMetadataSize;

            //* Write buffer
            private Buffer _WriteBuffer;

            //* Read buffer and state
            private ReadContext _ReadContext;

            public struct ReadContext
            {
                public long _ReadLocation;
                public Buffer _ReadBuffer;
                public ReadTask _NextReadTask;
            }

            internal class ReadTask
            {
                internal ReadTask(IPhysicalLogStream UnderlyingStream)
                {
                    _UnderlyingStream = UnderlyingStream;
                    _IsValid = false;
                }

                internal bool 
                    IsInRange(long Offset)
                {
                    return ((Offset >= _Offset) && (Offset <= _EndOffset));
                }

                internal long GetOffset()
                {
                    return (_Offset);
                }

                internal bool 
                    IsValid()
                {
                    return (_IsValid);
                }

                internal void 
                    InvalidateRead()
                {
                    _IsValid = false;
                }

                internal bool 
                    HandleWriteThrough(long WriteOffset, int WriteLength)
                {
                    long EndOffset = WriteOffset + WriteLength;

                    if ((_IsValid) &&
                        (IsInRange(WriteOffset) || IsInRange(EndOffset)))
                    {
                        _IsValid = false;
                    }

                    return (_IsValid);
                }

                internal void 
                    StartRead(long Offset, int Length)
                {
                    _Offset = Offset;
                    _Length = Length;
                    _EndOffset = _Offset + _Length;
                    _IsValid = true;
                    _Task = this._UnderlyingStream.MultiRecordReadAsync(
                                                    (ulong)(_Offset + 1),
                                                    (uint)Length,
                                                    CancellationToken.None);                    
                }

                internal async Task<PhysicalLogStreamReadResults> 
                    GetReadResults()
                {
                    PhysicalLogStreamReadResults r = await _Task.ConfigureAwait(false);
                    return (r);
                }


                private readonly IPhysicalLogStream _UnderlyingStream;

                private Task<PhysicalLogStreamReadResults> _Task;
                private bool _IsValid;
                private long _EndOffset;
                internal long _Offset;
                internal int _Length;
            };

            private List<ReadTask> _ReadTasks;
            private List<LogicalLogStream> _Streams;

            private int _InternalFlushInProgress = 0;
            private TaskCompletionSource<int> _AllOperationsDrained;
            private volatile bool _AllowNewOperations = true;
            internal long _OutstandingOperations = 0;

            //* Recoverable stream state
            private long _NextWritePosition;
            private long _NextOperationNumber;
            private uint _MaxBlockSize;
            private long _HeadTruncationPoint;

            private uint _MaximumReadRecordSize;

            private long _RecordOverhead;

            private UInt16 _InterfaceVersion;

            private Random randomGenerator;
            private LogWriteFaultInjectionParameters logWriteFaultInjectionParameters = new LogWriteFaultInjectionParameters();

            public string TraceType
            {
                get { return this._TraceType; }                
            }

            public Guid Id
            {
                get { return this._Id; }
            }

            public Guid OwnerId
            {
                get { return this._OwnerId; }
            }

            public long OwningHandleId
            {
                get { return this._OwningHandleId; }
            }

            public bool IsOpen
            {
                get { return this._IsOpen != 0; }
                set { this._IsOpen = (value ? 1 : 0); }
            }


            public
                LogicalLog(PhysicalLog owner, long owningHandleId, Guid id, IPhysicalLogStream underlyingStream, string traceType)
            {
                this._OwnerId = owner.Id;
                this._Id = id;
                this._OwningHandleId = owningHandleId;
                this._UnderlyingStream = underlyingStream;
                this.IsOpen = false;

                this._NextWritePosition = 0;
                this._NextOperationNumber = 0;
                this._MaxBlockSize = 0;
                this._HeadTruncationPoint = -1;

                this._WriteBuffer = null;
                this._BlockMetadataSize = this._UnderlyingStream.QueryReservedMetadataSize();

                this._ReadContext = new ReadContext();
                this._ReadContext._ReadLocation = 0;
                this._ReadContext._ReadBuffer = null;

                this._ReadTasks = new List<ReadTask>(8);
                this._Streams = new List<LogicalLogStream>(16);

                this._TraceType = "LogicalLog@" + traceType;

                this.randomGenerator = new Random(Environment.TickCount);

                this._InternalFlushInProgress = 0;
                this._AllOperationsDrained = new TaskCompletionSource<int>();
                this._AllowNewOperations = true;
                this._OutstandingOperations = 0;

                unsafe
                {
                    this._RecordOverhead = sizeof(KLogicalLogInformation.MetadataBlockHeader) + sizeof(KLogicalLogInformation.StreamBlockHeader) +
                                           this._BlockMetadataSize;
                }
            }

            ~LogicalLog()
            {
                ((ILogicalLog) this).Abort();
            }

            void
                IDisposable.Dispose()
            {
                ((ILogicalLog) this).Abort();
            }

            public void SetWriteDelay(LogWriteFaultInjectionParameters logWriteFaultInjectionParameters)
            {
                this.logWriteFaultInjectionParameters = logWriteFaultInjectionParameters;
            }

            /// <summary>
            /// Retrieve the tail recovery information for the underlying physical log stream
            /// </summary>
            /// <returns>KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation</returns>
            private async Task<KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation>
                QueryLogStreamRecoveryInfo(CancellationToken cancellationToken)
            {
                var results = await this._UnderlyingStream.IoCtlAsync(
                    (uint) KLogicalLogInformation.LogicalLogControlCodes.QueryLogicalLogTailAsnAndHighestOperationControl,
                    null,
                    cancellationToken).ConfigureAwait(false);

                UInt32 size;
                results.OutBuffer.QuerySize(out size);

                KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation currentState;
                unsafe
                {
                    ReleaseAssert.AssertIfNot(
                        size == sizeof(KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation),
                        "Interface Fault");

                    void* r;
                    results.OutBuffer.GetBuffer(out r);
                    currentState = *(KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation*) r;
                }

                return currentState;
            }

            /// <summary>
            /// Retrieve the record read information for the underlying physical log stream
            /// </summary>
            /// <returns>KLogicalLogInformation.LogicalLogReadInformation</returns>
            private async Task<KLogicalLogInformation.LogicalLogReadInformation>
                QueryLogStreamReadInformation(CancellationToken cancellationToken)
            {
                IoctlResults results;
                KLogicalLogInformation.LogicalLogReadInformation currentState;

                try
                {
                    results = await this._UnderlyingStream.IoCtlAsync(
                        (uint)KLogicalLogInformation.LogicalLogControlCodes.QueryLogicalLogReadInformationControl,
                        null,
                        cancellationToken).ConfigureAwait(false);
                } catch(Exception) {
                    //
                    // It is likely that the driver is older and doesn't support
                    // this IOCTL. In this case default to 1MB
                    //
                    currentState.MaximumReadRecordSize = 1024 * 1024;
                    return currentState;
                }

                UInt32 size;
                results.OutBuffer.QuerySize(out size);

                unsafe
                {
                    ReleaseAssert.AssertIfNot(
                        size == sizeof(KLogicalLogInformation.LogicalLogReadInformation),
                        "Interface Fault");

                    void* r;
                    results.OutBuffer.GetBuffer(out r);
                    currentState = *(KLogicalLogInformation.LogicalLogReadInformation*)r;
                }

                return currentState;
            }


            /// <summary>
            /// Retrieve the build version and type for the driver
            /// </summary>
            /// <returns>KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation</returns>
            private async Task<KLogicalLogInformation.DriverBuildInformation>
                QueryDriverBuildInformation(CancellationToken cancellationToken)
            {
                var results = await this._UnderlyingStream.IoCtlAsync(
                    (uint) KLogicalLogInformation.LogicalLogControlCodes.QueryDriverBuildInformation,
                    null,
                    cancellationToken).ConfigureAwait(false);

                UInt32 size;
                results.OutBuffer.QuerySize(out size);

                KLogicalLogInformation.DriverBuildInformation currentState;
                unsafe
                {
                    ReleaseAssert.AssertIfNot(
                        size == sizeof(KLogicalLogInformation.DriverBuildInformation),
                        "Interface Fault");

                    void* r;
                    results.OutBuffer.GetBuffer(out r);
                    currentState = *(KLogicalLogInformation.DriverBuildInformation*) r;
                }

                return currentState;
            }

            private void 
                QueryUserBuildInformation(out UInt32 BuildNumber, out byte IsFreeBuild)
            {
                NativeLog.QueryUserBuildInformation(out BuildNumber, out IsFreeBuild);
            }

            private async Task 
                VerifyUserAndDriverBuildMatch()
            {
                KLogicalLogInformation.DriverBuildInformation driverBuildInfo;
                UInt32 userBuildNumber, driverBuildNumber;
                byte userIsFreeBuild, driverIsFreeBuild;

                try
                {
                    driverBuildInfo = await this.QueryDriverBuildInformation(CancellationToken.None).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    AppTrace.TraceSource.WriteWarning(
                        TraceType,
                        "VerifyUserAndDriverBuildMatch: Exception {0}",
                        ex.Message
                        );
                    return;
                }

                driverBuildNumber = driverBuildInfo.BuildNumber;
                if (driverBuildInfo.IsFreeBuild == 0)
                {
                    driverIsFreeBuild = 0;
                }
                else
                {
                    driverIsFreeBuild = 1;
                }

                this.QueryUserBuildInformation(out userBuildNumber, out userIsFreeBuild);


                //
                // Separate user build number into high 16 bits and low 16 bits.  High 16 bits is the interface version
                // between managed and native code. Low 16 bits is the build number from the build
                //

                this._InterfaceVersion = (ushort) (userBuildNumber >> 16);
                userBuildNumber = userBuildNumber & 0x0000ffff;

                //
                // Build number cannot be checked in production as the WHQL signed ktllogger driver is 
                // always from an earlier build
                //
                var msg = String.Format(
                    CultureInfo.InvariantCulture,
                    "User/Driver build number mismatch. User: {0} {1}, Driver {2} {3}",
                    userBuildNumber,
                    (userIsFreeBuild == 0) ? "CHK" : "FRE",
                    driverBuildNumber,
                    (driverIsFreeBuild == 0) ? "CHK" : "FRE");

                AppTrace.TraceSource.WriteInfo(TraceType, msg);
            }


            /// <summary>
            /// New LogicalLog created event
            /// </summary>
            public async Task
                OnCreated(CancellationToken cancellationToken)
            {
                await this.VerifyUserAndDriverBuildMatch().ConfigureAwait(false);

                KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation currentState;
                currentState = await this.QueryLogStreamRecoveryInfo(cancellationToken).ConfigureAwait(false);

                KLogicalLogInformation.LogicalLogReadInformation readInfo;
                readInfo = await this.QueryLogStreamReadInformation(cancellationToken).ConfigureAwait(false);

                this._MaximumReadRecordSize = readInfo.MaximumReadRecordSize;

                this._MaxBlockSize = currentState.MaximumBlockSize;
                this._NextOperationNumber = 1;
                this._NextWritePosition = 0;
                this._HeadTruncationPoint = -1;

                if (this._WriteBuffer != null)
                {
                    this._WriteBuffer.Dispose();
                    this._WriteBuffer = null;
                }

                this._WriteBuffer = Buffer.CreateWriteBuffer(
                    this._BlockMetadataSize,
                    this._MaxBlockSize,
                    this._NextWritePosition,
                    this._NextOperationNumber,
                    this._Id);
            }

            /// <summary>
            /// Existing LogicalLog Opened Event - must recover L.Log state from underlying physical log stream
            /// </summary>
            public async Task
                OnRecoverAsync(CancellationToken cancellationToken)
            {
                await this.VerifyUserAndDriverBuildMatch().ConfigureAwait(false);

                // Recover the length and current position of the logical log stream 
                // from the physical logger's log stream specific dedicated log plug-in
                this._NextWritePosition = -1;
                this._NextOperationNumber = -1;
                this._HeadTruncationPoint = -1;

                KLogicalLogInformation.LogicalLogTailAsnAndHighestOperation currentState;
                currentState = await this.QueryLogStreamRecoveryInfo(cancellationToken).ConfigureAwait(false);

                KLogicalLogInformation.LogicalLogReadInformation readInfo;
                readInfo = await this.QueryLogStreamReadInformation(cancellationToken).ConfigureAwait(false);

                this._MaximumReadRecordSize = readInfo.MaximumReadRecordSize;

                if (currentState.HighestOperationCount == 0)
                {
                    // Special case of a recovered empty log
                    ReleaseAssert.AssertIfNot(
                        (currentState.TailAsn == 1),
                        "Inconsistent HighestOperationCount or TailAsn value");

                    currentState.RecoveredLogicalLogHeader.HeadTruncationPoint = -1;
                }

                this._NextOperationNumber = (long) currentState.HighestOperationCount + 1;
                this._NextWritePosition = (long) (currentState.TailAsn - 1);
                this._MaxBlockSize = currentState.MaximumBlockSize;
                this._HeadTruncationPoint = currentState.RecoveredLogicalLogHeader.HeadTruncationPoint;

                if (this._WriteBuffer != null)
                {
                    this._WriteBuffer.Dispose();
                    this._WriteBuffer = null;
                }

                this._WriteBuffer = Buffer.CreateWriteBuffer(
                    this._BlockMetadataSize,
                    this._MaxBlockSize,
                    this._NextWritePosition,
                    this._NextOperationNumber,
                    this._Id);
            }

            private struct InternalReadAsyncResults
            {
                public ReadContext Context;
                public int TotalDone;
            }

            private void 
                AddLogicalLogStream(LogicalLogStream stream)
            {
                lock (_Streams)
                {
                    _Streams.Add(stream);
                }
            }

            private void 
                RemoveLogicalLogStream(LogicalLogStream stream)
            {
                lock (_Streams)
                {
                    _Streams.Remove(stream);
                }
            }

            private void 
                InvalidateAllReads()
            {
                lock (_ReadTasks)
                {
                    foreach (ReadTask task in _ReadTasks)
                    {
                        task.InvalidateRead();
                    }
                }

                lock (_Streams)
                {
                    foreach (LogicalLogStream stream in _Streams)
                    {
                        stream.InvalidateReadAhead();
                    }
                }
            }

            private ReadTask 
                StartBackgroundRead(long Offset, int Length)
            {
                ReadTask readTask = new ReadTask(this._UnderlyingStream);
                readTask.StartRead(Offset, Length);
                lock (_ReadTasks)
                {
                    _ReadTasks.Add(readTask);
                }
                return (readTask);
            }

            private async Task<PhysicalLogStreamReadResults> 
                GetReadResults(ReadTask readTask)
            {
                lock (_ReadTasks)
                {
                    _ReadTasks.Remove(readTask);
                }
                return await readTask.GetReadResults().ConfigureAwait(false);
            }

            private async Task<InternalReadAsyncResults>
                InternalReadAsync(
                ReadContext readContext, byte[] buffer, int offset, int count, int bytesToRead, CancellationToken cancellationToken)
            {
                //
                // We allow up to 3 attempts to read our record from the logical log.
                // - First could be that the current buffer is right at the end and so the first read
                //   will return zero since there is no data in the buffer
                // - Second could be the read from the logger fails due to a transient truncation of the shared log
                //   See TFS 5044108.
                // - Third attempt should succeed, if not then there is a bug and so we assert
                //
                const int zeroBytesReadLimit = 3;
                var zeroBytesReadCount = 0;

                Buffer readBuffer = readContext._ReadBuffer;
                long readLocation = readContext._ReadLocation;

                bool startNextRead = false;
                bool isNextRead;
                long readOffset;
                InternalReadAsyncResults results;
                results.TotalDone = 0;
                results.Context = readContext;

                if ((results.Context._ReadLocation < this._HeadTruncationPoint) || (results.Context._ReadLocation >= this._NextWritePosition))
                {
                    AppTrace.TraceSource.WriteWarning(
                        TraceType,
                        "InternalReadAsync: Read in nonexistent space location {0} HeadTruncation {1} NextWritePosition {2}",
                        readLocation,
                        this._HeadTruncationPoint,
                        this._NextWritePosition
                        );
                    return results; // attempt to start a read in non-existing space
                }

                var todo = (int) Math.Min(this._NextWritePosition - results.Context._ReadLocation, count); // NOTE: Can go negative when read cursor is beyond EOS

                while (todo > 0)
                {
                    isNextRead = false;
                    if (results.Context._ReadBuffer == null)
                    {
                        PhysicalLogStreamReadResults r;

                        // TODO: Remove this when stabilized
                        var sw = new Stopwatch();

                        if ((this._InterfaceVersion == 0) || (bytesToRead == 0))
                        {
                            // TODO: Remove this when stabilized
                            AppTrace.TraceSource.WriteInfo(
                                TraceType,
                                "InternalReadAsync: legacy read {0} at {1}",
                                count,
                                results.Context._ReadLocation);
                            sw.Start();
                            try
                            {
                                r = await this._UnderlyingStream.ReadContainingAsync((ulong) (results.Context._ReadLocation + 1), cancellationToken).ConfigureAwait(false);
                            }
                            catch (Exception ex)
                            {
                                AppTrace.TraceSource.WriteWarning(
                                    TraceType,
                                    "InternalReadAsync: legacy read failed {0} at {1} {2}",
                                    count,
                                    results.Context._ReadLocation,
                                    ex.ToString());
                                throw;
                            }
                            readOffset = results.Context._ReadLocation;
                        }
                        else
                        {
                            try
                            {
                                // TODO: Remove this when stabilized
                                AppTrace.TraceSource.WriteInfo(
                                    TraceType,
                                    "InternalReadAsync: MultiRecordRead {0} {1} at {2}",
                                    count,
                                    bytesToRead,
                                    results.Context._ReadLocation);

                                sw.Start();
                                try
                                {
                                    //
                                    // See if the read can be satisfied by the _NextReadBuffer
                                    //
                                    ReadTask readTask = null;
                                    long offsetToRead = results.Context._ReadLocation;

                                    if (results.Context._NextReadTask != null) 
                                    {
                                        if (results.Context._NextReadTask.IsValid() && (results.Context._NextReadTask.IsInRange(offsetToRead)))
                                        {
                                            readTask = results.Context._NextReadTask;
                                        } else {
                                            //
                                            // The readahead is not valid since it was either invalidated by a truncate or the read is out 
                                            // of range. Abandon the read and clean it up before moving forward.
                                            //
                                            AppTrace.TraceSource.WriteInfo(
                                                TraceType,
                                                "InternalReadAsync: abandon read ahead {0} {1}",
                                                results.Context._NextReadTask._Offset,
                                                results.Context._NextReadTask._Length
                                                );

                                            // CONSIDER: Why do we need to await here ? Can't we do this on another thread somehow ?
                                            try
                                            {
                                                r = await GetReadResults(results.Context._NextReadTask).ConfigureAwait(false);
                                            }
                                            catch (Exception ex)
                                            {
                                                //
                                                // We never care if a next read fails
                                                //
                                                AppTrace.TraceSource.WriteInfo(
                                                        TraceType,
                                                        "InternalReadAsync: background read task exception {0} {1} {2}",
                                                        results.Context._NextReadTask._Offset,
                                                        results.Context._NextReadTask._Length,
                                                        ex.ToString()
                                                        );
                                            }
                                            results.Context._NextReadTask = null;
                                        }
                                        results.Context._NextReadTask = null;
                                    }
                                    
                                    if (readTask == null)
                                    {
                                        //
                                        // Since the next read doesn't have data we need then we need to do the read right now
                                        //
                                        readTask = StartBackgroundRead(offsetToRead, bytesToRead);
                                    }
                                    else
                                    {
                                        isNextRead = true;
                                    }

                                    Stopwatch swRead = new Stopwatch();
                                    swRead.Start();
                                    r = await GetReadResults(readTask).ConfigureAwait(false);
                                    swRead.Stop();
                                    readOffset = readTask.GetOffset();

                                    startNextRead = true;
                                }
                                catch (Exception ex)
                                {
                                    if (!isNextRead)
                                    {
                                        //
                                        // If this was a foreground read then we want to pass the exception back to the caller as
                                        // the read was invalid.
                                        //
                                        AppTrace.TraceSource.WriteWarning(
                                            TraceType,
                                            "InternalReadAsync: MultiRecord read failed {0} {1} at {2} {3}",
                                            count,
                                            bytesToRead,
                                            results.Context._ReadLocation,
                                            ex.ToString());
                                        throw;
                                    }
                                    else
                                    {
                                        //
                                        // Otherwise if this was next read then we don't care if it fails, we will just try again
                                        //
                                        AppTrace.TraceSource.WriteInfo(
                                            TraceType,
                                            "InternalReadAsync: MultiRecord read failed {0} {1} at {2} {3}",
                                            count,
                                            bytesToRead,
                                            results.Context._ReadLocation,
                                            ex.ToString());
                                        continue;     // while (todo > 0);
                                    }
                                }
                            }
                            catch (System.Fabric.FabricException ex)
                            {
                                if (ex.InnerException is System.Runtime.InteropServices.COMException)
                                {
                                    AppTrace.TraceSource.WriteWarning(
                                        TraceType,
                                        "InternalReadAsync: ComInteropException, assuming downlevel driver {0}",
                                        ex.ToString()
                                        );
                                    this._InterfaceVersion = 0;
                                    continue;  // while(todo > 0)
                                }

                                throw;
                            }
                        }
                        sw.Stop();
                        AppTrace.TraceSource.WriteInfo(
                            TraceType,
                            "InternalReadAsync: Read took {0} ticks for {1} {2} bytes at {3}",
                            sw.ElapsedTicks,
                            count,
                            bytesToRead,
                            results.Context._ReadLocation);

                        results.Context._ReadBuffer = Buffer.CreateReadBuffer(
                            this._BlockMetadataSize,
                            readOffset,
                            r.ResultingMetadata,
                            r.ResultingIoPageAlignedBuffer, 
                            TraceType);

                        uint leftToRead = results.Context._ReadBuffer.GetSizeLeftToRead();

                        if ((isNextRead) && 
                            ((results.Context._ReadLocation < readOffset) ||
                             (results.Context._ReadLocation >= (readOffset + leftToRead))))
                        {
                            //
                            // Ensure that our read ahead actually contains the data we want. It is possible that
                            // the read ahead will return a shorter record than we asked for and although we think 
                            // it might be in the read ahead, it actually is not
                            //
                            results.Context._ReadBuffer.Dispose();
                            results.Context._ReadBuffer = null;
                            AppTrace.TraceSource.WriteWarning(
                                TraceType,
                                "InternalReadAsync: read ahead is a short record {0} {1} {2}",
                                readOffset,
                                leftToRead,
                                results.Context._ReadLocation
                                );
                            continue;  // while(todo > 0)
                        }

                        if (startNextRead)
                        {
                            //
                            // Prime to read following record
                            //
                            long offsetToNextRead = readOffset + results.Context._ReadBuffer.GetSizeLeftToRead();
                            results.Context._NextReadTask = StartBackgroundRead(offsetToNextRead, bytesToRead);
                        }
                    }

                    int done = (int)results.Context._ReadBuffer.Get(ref buffer, offset, (uint)todo);

                    if (done == 0)
                    {
                        zeroBytesReadCount++;
                        if (zeroBytesReadCount == zeroBytesReadLimit)
                        {
                            var s = string.Format(
                                    CultureInfo.InvariantCulture,
                                    "InternalReadAsync: MultiRecordRead too many retries {0} {1} {2} at {3}",
                                    count,
                                    bytesToRead,
                                    todo,
                                    results.Context._ReadLocation);
                                AppTrace.TraceSource.WriteInfo(TraceType, s);

                             throw new IOException(s);
                        }
                    }
                    else
                    {
                        zeroBytesReadCount = 0;
                    }

                    todo -= done;
                    offset += done;
                    results.Context._ReadLocation += done;
                    results.TotalDone += done;

                    if (todo > 0)
                    {
                        results.Context._ReadBuffer.Dispose();
                        results.Context._ReadBuffer = null;
                    }
                }

                return results;
            }

            void StartOperation()
            {
                Interlocked.Increment(ref this._OutstandingOperations);

                if (! this._AllowNewOperations)
                {
                    EndOperation();
                    throw new FabricObjectClosedException();
                }
            }

            void EndOperation()
            {
                long outstandingOperations = Interlocked.Decrement(ref this._OutstandingOperations);
                if (! this._AllowNewOperations)
                {
                    if (outstandingOperations == 0)
                    {
                        this._AllOperationsDrained.TrySetResult(2);
                    }
                }
            }
            async Task WaitForAllOperationsToDrain()
            {
                //
                // Indicate that no new operations should be started
                //
                this._AllowNewOperations = false;

                //
                // If there are no outstanding operations then set the TCS to indicate this
                //
                long outstandingOperations = Interlocked.Read(ref this._OutstandingOperations);
                if (outstandingOperations == 0)
                {
                    this._AllOperationsDrained.TrySetResult(0);
                }

                //
                // Wait for TCS to indicate all operations drained
                //
                await this._AllOperationsDrained.Task.ConfigureAwait(false);
            }           

            //* ILogicalLog implementation - See Interfaces.cs
            async Task
                ILogicalLog.CloseAsync(CancellationToken cancellationToken)
            {
                GC.SuppressFinalize(this);

                if (this.IsOpen)
                {
                    //
                    // Wait for all outstanding reads to complete
                    //
                    bool drainingReadTasks = true;
                    while (drainingReadTasks)
                    {
                        ReadTask task;

                        lock (_ReadTasks)
                        {
                            if (_ReadTasks.Count == 0)
                            {
                                drainingReadTasks = false;
                                break;
                            }
                            else
                            {
                                task = _ReadTasks[0];
                                _ReadTasks.Remove(task);
                            }
                        }

                        PhysicalLogStreamReadResults r;

                        try
                        {
                            r = await GetReadResults(task).ConfigureAwait(false);

                        } catch(Exception ex) {
                            AppTrace.TraceSource.WriteInfo(
                                    TraceType,
                                    "InternalReadAsync: background read task exception {0} {1} {2}",
                                    task._Offset,
                                    task._Length,
                                    ex.ToString()
                                    );
                        }
                    }
                    
                    if (Interlocked.CompareExchange(ref this._IsOpen, 0, 1) == 1)
                    {
                        await WaitForAllOperationsToDrain().ConfigureAwait(false);

                        await OnLogicalLogClose(this, cancellationToken).ConfigureAwait(false);
                    }
                }
            }

            void
                ILogicalLog.Abort()
            {
                // Trigger close but don't wait
                ((ILogicalLog) this).CloseAsync(CancellationToken.None);
            }

            bool
                ILogicalLog.IsFunctional
            {
                get
                {
                    if (!this.IsOpen)
                        return false;

                    return this._UnderlyingStream.IsFunctional();
                }
            }

            long
                ILogicalLog.Length
            {
                get { return (this._NextWritePosition - this._HeadTruncationPoint) - 1; }
            }

            long
                ILogicalLog.WritePosition
            {
                get { return this._NextWritePosition; }
            }

            long
                ILogicalLog.ReadPosition
            {
                get { return this._ReadContext._ReadLocation; }
            }

            long
                ILogicalLog.HeadTruncationPosition
            {
                get { return this._HeadTruncationPoint; }
            }

            long
                ILogicalLog.MaximumBlockSize
            {
                //
                // Maximum size that can be written is the Maximum IO buffer size plus the leftover room
                // in the metadata header
                //
                get { return (this._MaxBlockSize - this._RecordOverhead); }
            }

            uint
                ILogicalLog.MetadataBlockHeaderSize
            {
                get { return this._BlockMetadataSize; }
            }

            Stream
                ILogicalLog.CreateReadStream(int SequentialAccessReadSize)
            {
                LogicalLogStream stream = new LogicalLogStream(this, this._InterfaceVersion, SequentialAccessReadSize);
                AddLogicalLogStream(stream);
                return (stream);
            }

            async Task<int>
                ILogicalLog.ReadAsync(byte[] buffer, int offset, int count, int bytesToRead, CancellationToken cancellationToken)
            {
                InternalReadAsyncResults results;

                StartOperation();
                try
                {
                    results = await this.InternalReadAsync(
                        this._ReadContext,
                        buffer,
                        offset,
                        count,
                        bytesToRead,
                        cancellationToken).ConfigureAwait(false);

                    this._ReadContext = results.Context;
                }
                finally
                {
                    EndOperation();
                }
                return results.TotalDone;
            }

            long
                ILogicalLog.SeekForRead(long offset, SeekOrigin origin)
            {
                long newReadLocation = -1;

                switch (origin)
                {
                    case SeekOrigin.Begin:
                        newReadLocation = offset;
                        break;

                    case SeekOrigin.End:
                        newReadLocation = this._NextWritePosition + offset;
                        break;

                    case SeekOrigin.Current:
                        newReadLocation = this._ReadContext._ReadLocation + offset;
                        break;
                }

                // Check read buffer (if there is one) to see if we can reposition within that buffer
                // 
                if (this._ReadContext._ReadBuffer != null)
                {
                    if (this._ReadContext._ReadBuffer.Intersects(newReadLocation))
                    {
                        this._ReadContext._ReadBuffer.SetPosition(newReadLocation - this._ReadContext._ReadBuffer.BasePosition);
                    }
                    else
                    {
                        // TODO: Work out Reuse() for read approach
                        this._ReadContext._ReadBuffer.Dispose();
                        this._ReadContext._ReadBuffer = null;
                    }
                }

                this._ReadContext._ReadLocation = newReadLocation;
                return this._ReadContext._ReadLocation;
            }

            async Task
                ILogicalLog.AppendAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
            {
                StartOperation();
                try
                {
                    var originalCount = count;

                    while (count > 0)
                    {
                        int done = (int)_WriteBuffer.Put(ref buffer, offset, (uint)count);

                        count -= done;
                        offset += done;
                        this._NextWritePosition += done;

                        if (count > 0)
                        {
                            //
                            // No need to make a barrier here as if the buffer is just completely full then count == 0 and
                            // this code path isn't executed. Flush will happen by replicator just after this.
                            //
                            await this.InternalFlushAsync(false, cancellationToken).ConfigureAwait(false);
                        }
                    }
                }
                finally
                {
                    EndOperation();
                }
            }

#if MockRandomWriteException
    //
    // How often to throw a write exception
    //
            int throwWriteExceptionInterval = 100;
            int throwWriteExceptionCounter = 0;
#endif 

            public async Task DelayBeforeFlush(CancellationToken cancellationToken)
            {
                if (this.logWriteFaultInjectionParameters.Test_LogDelayRatio <= 0) return;
                if (this.randomGenerator.NextDouble() < this.logWriteFaultInjectionParameters.Test_LogDelayRatio)
                {
                    var flushDelay = TimeSpan.FromMilliseconds(this.randomGenerator.Next(this.logWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds, this.logWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds));
                    AppTrace.TraceSource.WriteInfo(TraceType, string.Format("Delaying {0} before Write in KtlLogicalLog", flushDelay));
                    await Task.Delay(flushDelay).ConfigureAwait(false);

                    if (this.randomGenerator.NextDouble() < this.logWriteFaultInjectionParameters.Test_LogDelayProcessExitRatio)
                    {
                        AppTrace.TraceSource.WriteInfo(TraceType, "Exiting process due to fault injection");
                        NativeMethods.ExitProcess();
                    }
                }
            }

            private async Task
                InternalFlushAsync(bool IsBarrier, CancellationToken cancellationToken)
            {
                int flushInProgress = Interlocked.CompareExchange(ref this._InternalFlushInProgress, 1, 0);

                if (flushInProgress == 0)
                {
                    ReleaseAssert.AssertIf(this._WriteBuffer.IsSealed, "FlushAsync: Attempted reuse of buffer");

                    NativeLog.IKIoBuffer mdBuffer;
                    uint mdBufferSize;
                    NativeLog.IKIoBuffer pageAlignedBuffer;
                    long sizeOfUserDataSealed;
                    long asnOfRecord;
                    long opOfRecord;

                    await DelayBeforeFlush(CancellationToken.None).ConfigureAwait(false);

                    this._WriteBuffer.SealForWrite(
                        this._HeadTruncationPoint,
                        IsBarrier,
                        out mdBuffer,
                        out mdBufferSize,
                        out pageAlignedBuffer,
                        out sizeOfUserDataSealed,
                        out asnOfRecord,
                        out opOfRecord);

                    if (sizeOfUserDataSealed > 0)
                    {
                        this._NextOperationNumber++;

#if MockRandomWriteException
    // NOTE: this assumes that InternalFlushAsync is called in a single threaded manner. 
                    if (++throwWriteExceptionCounter == throwWriteExceptionInterval)
                    {
                        throwWriteExceptionCounter = 0;
                        AppTrace.TraceSource.WriteInfo(TraceType,
                                "InternalFlushAsync: throw write exception after {0} count", throwWriteExceptionInterval
                        );
                        throw new ?????
                    }
#endif

                        {
                            await this._UnderlyingStream.WriteAsync(
                                (ulong)asnOfRecord,
                                (ulong)opOfRecord,
                                mdBufferSize,
                                mdBuffer,
                                pageAlignedBuffer,
                                cancellationToken).ConfigureAwait(false);
                        }
                    }

                    if (this._WriteBuffer != null)
                    {
                        this._WriteBuffer.Dispose();
                        this._WriteBuffer = null;
                    }

                    // TODO: Come up with a better reuse approach; unseal/reuse if sizeOfUserDataSealed == 0
                    // Switch to new output buffer home at next location in stream (asn) and op (version) space 
                    this._WriteBuffer = Buffer.CreateWriteBuffer(
                        this._BlockMetadataSize,
                        this._MaxBlockSize,
                        this._NextWritePosition,
                        this._NextOperationNumber,
                        this._Id);

                    ReleaseAssert.AssertIf(this._WriteBuffer.IsSealed, "FlushAsync: New buffer is sealed !");

                    this._InternalFlushInProgress = 0;
                }
                else
                {
                    AppTrace.TraceSource.WriteWarning(TraceType, "InternalFlushAsync ignored since flush is in progress");
                }
            }

            async Task
                ILogicalLog.FlushAsync(CancellationToken cancellationToken)
            {
                StartOperation();
                try
                {
                    await this.InternalFlushAsync(
                        false,
                        cancellationToken).ConfigureAwait(false);
                }
                finally
                {
                    EndOperation();
                }
            }

            async Task
                ILogicalLog.FlushWithMarkerAsync(CancellationToken cancellationToken)
            {
                StartOperation();
                try
                {
                    await this.InternalFlushAsync(
                        true,
                        // Flush with a barrier
                        cancellationToken).ConfigureAwait(false);
                }
                finally
                {
                    EndOperation();
                }
            }


            void
                ILogicalLog.TruncateHead(long streamOffset)
            {
                StartOperation();
                try
                {
                    AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "TruncateHead: _HeadTruncationPoint {0}, _NextWritePosition {1}, streamOffset {2}",
                        this._HeadTruncationPoint,
                        this._NextWritePosition,
                        streamOffset);

                    if (this._HeadTruncationPoint < streamOffset)
                    {
                        this._UnderlyingStream.Truncate((ulong)streamOffset + 1, (ulong)streamOffset + 1);
                        this._HeadTruncationPoint = streamOffset;
                    }
                }
                finally
                {
                    EndOperation();
                }
            }


            async Task
                ILogicalLog.TruncateTail(long streamOffset, CancellationToken cancellationToken)
            {
                StartOperation();
                try
                {
                    AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "TruncateTail: Enter _HeadTruncationPoint {0}, _NextWritePosition {1}, streamOffset {2}",
                        this._HeadTruncationPoint,
                        this._NextWritePosition,
                        streamOffset);

                    if ((streamOffset >= this._NextWritePosition) || (streamOffset < 0))
                    {
                        throw new ArgumentOutOfRangeException(
                            "Offset " + streamOffset.ToString() + " above NextWritePosition " + this._NextWritePosition.ToString());
                    }

                    if (streamOffset <= this._HeadTruncationPoint)
                    {
                        throw new ArgumentOutOfRangeException(
                            "Offset " + streamOffset.ToString() + " below HeadTruncationPoint  " + this._HeadTruncationPoint.ToString());
                    }

                    await this.InternalFlushAsync(false, cancellationToken).ConfigureAwait(false);

                    var nullWriteBuffer = Buffer.CreateWriteBuffer(this._BlockMetadataSize, this._MaxBlockSize, streamOffset, this._NextOperationNumber, this._Id);
                    this._NextOperationNumber++;

                    NativeLog.IKIoBuffer mdBuffer;
                    uint mdBufferSize;
                    NativeLog.IKIoBuffer pageAlignedBuffer;
                    long sizeOfUserDataSealed;
                    long asnOfRecord;
                    long opOfRecord;

                    nullWriteBuffer.SealForWrite(
                        this._HeadTruncationPoint,
                        true,
                        // TruncateTail records must have barrier
                        out mdBuffer,
                        out mdBufferSize,
                        out pageAlignedBuffer,
                        out sizeOfUserDataSealed,
                        out asnOfRecord,
                        out opOfRecord);

                    await this._UnderlyingStream.WriteAsync(
                        (ulong)asnOfRecord,
                        (ulong)opOfRecord,
                        mdBufferSize,
                        mdBuffer,
                        pageAlignedBuffer,
                        cancellationToken).ConfigureAwait(false);

                    this._NextWritePosition = streamOffset;

                    //
                    // Rebuild a new write buffer at the correct opNumber and asn
                    //
                    if (this._WriteBuffer != null)
                    {
                        this._WriteBuffer.Dispose();
                        this._WriteBuffer = null;
                    }

                    this._WriteBuffer = Buffer.CreateWriteBuffer(
                        this._BlockMetadataSize,
                        this._MaxBlockSize,
                        this._NextWritePosition,
                        this._NextOperationNumber,
                        this._Id);

                    if (this._ReadContext._ReadBuffer != null)
                    {
                        this._ReadContext._ReadBuffer.Dispose();
                        this._ReadContext._ReadBuffer = null;
                    }

                    InvalidateAllReads();
                }
                finally
                {
                    EndOperation();
                }
            }

            Task
                ILogicalLog.WaitCapacityNotificationAsync(uint percentOfSpaceUsed, CancellationToken cancellationToken)
            {
                return this._UnderlyingStream.WaitForNotificationAsync(
                    NativeLog.KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE.LogSpaceUtilization,
                    percentOfSpaceUsed,
                    cancellationToken);
            }

            async Task
                ILogicalLog.ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken cancellationToken)
            {
                StartOperation();
                try
                {
                    var results = await this._UnderlyingStream.IoCtlAsync(
                        (uint)KLogicalLogInformation.LogicalLogControlCodes.WriteOnlyToDedicatedLog,
                        null,
                        cancellationToken).ConfigureAwait(false);

                }
                finally
                {
                    EndOperation();
                }
            }

            async Task
                ILogicalLog.ConfigureWritesToSharedAndDedicatedLogAsync(CancellationToken cancellationToken)
            {
                StartOperation();
                try
                {
                    var results = await this._UnderlyingStream.IoCtlAsync(
                        (uint)KLogicalLogInformation.LogicalLogControlCodes.WriteToSharedAndDedicatedLog,
                        null,
                        cancellationToken).ConfigureAwait(false);
                }
                finally
                {
                    EndOperation();
                }
            }


            /// <summary>
            /// Retrieve the current log stream usage information
            /// </summary>
            /// <returns>KLogicalLogInformation.CurrentLogUsageInformation</returns>
            private async Task<KLogicalLogInformation.CurrentLogUsageInformation>
                QueryLogStreamUsageInfo(CancellationToken cancellationToken)
            {
                KLogicalLogInformation.CurrentLogUsageInformation currentState;
                try
                {
                    var results = await this._UnderlyingStream.IoCtlAsync(
                        (uint)KLogicalLogInformation.LogicalLogControlCodes.QueryCurrentLogUsageInformation,
                        null,
                        cancellationToken).ConfigureAwait(false);

                    UInt32 size;
                    results.OutBuffer.QuerySize(out size);

                    unsafe
                    {
                        ReleaseAssert.AssertIfNot(
                            size == sizeof(KLogicalLogInformation.CurrentLogUsageInformation),
                            "Interface Fault");

                        void* r;
                        results.OutBuffer.GetBuffer(out r);
                        currentState = *(KLogicalLogInformation.CurrentLogUsageInformation*)r;
                    }
                }
                finally
                {
                    EndOperation();
                }
                return currentState;
            }

            async Task<uint>
                ILogicalLog.QueryLogUsageAsync(CancellationToken cancellationToken)
            {
                KLogicalLogInformation.CurrentLogUsageInformation info;

                info = await this.QueryLogStreamUsageInfo(cancellationToken).ConfigureAwait(false);
                return (info.PercentageLogUsage);
            }

            Task
                ILogicalLog.WaitBufferFullNotificationAsync(CancellationToken cancellationToken)
            {
                // TODO: Once WF Replicator folks are engaged - nail down actual behavior
                throw new NotImplementedException();
            }

            public void 
                SetSequentialAccessReadSize(Stream LogStream, int SequentialAccessReadSize)
            {
                ReleaseAssert.AssertIfNot(LogStream is LogicalLogStream, "Expect LogStream to be a LogicalLogStream");

                var logicalLogStream = (LogicalLogStream) LogStream;

                logicalLogStream.SetSequentialAccessReadSize(SequentialAccessReadSize);
            }


            //* Read-only Stream abstraction implementation
            private class LogicalLogStream : Stream
            {
                private readonly LogicalLog _Parent;
                private ReadContext _ReadContext;
                private int _SequentialAccessReadSize;
                private UInt16 _InterfaceVersion;


                public LogicalLogStream(LogicalLog parent, UInt16 InterfaceVersion, int SequentialAccessReadSize)
                {
                    this._Parent = parent;
                    this._ReadContext = new ReadContext();
                    this._ReadContext._ReadLocation = 0;
                    this._ReadContext._ReadBuffer = null;
                    this._InterfaceVersion = InterfaceVersion;

                    if (this._InterfaceVersion >= 1)
                    {
                        this._SequentialAccessReadSize = SequentialAccessReadSize;
                    }
                    else
                    {
                        this._SequentialAccessReadSize = 0;
                    }
                }

                public void 
                    InvalidateReadAhead()
                {
                    this._ReadContext._ReadBuffer = null;
                }

                public void 
                    SetSequentialAccessReadSize(int SequentialAccessReadSize)
                {
                    if (this._InterfaceVersion >= 1)
                    {
                        //
                        // Can only use sequential mode when interface is version 1 or higher
                        //
                        this._SequentialAccessReadSize = SequentialAccessReadSize;
                    }
                }

                protected override void 
                    Dispose(bool disposing)
                {
                    if (disposing)
                    {
                        this._Parent.RemoveLogicalLogStream((LogicalLogStream)this);

                        if (this._ReadContext._ReadBuffer != null)
                        {
                            this._ReadContext._ReadBuffer.Dispose();
                            this._ReadContext._ReadBuffer = null;
                        }
                    }

                    base.Dispose(disposing);
                }


                public override long Length
                {
                    get { return ((ILogicalLog) this._Parent).WritePosition; }
                }

                public override bool CanRead
                {
                    get { return true; }
                }

                public override bool CanSeek
                {
                    get { return true; }
                }

                public override bool CanWrite
                {
                    get { return false; }
                }

                public override void Flush()
                {
                    this._ReadContext._ReadBuffer = null;
                }

                public override long Position
                {
                    get { return this._ReadContext._ReadLocation; }
                    set { this.Seek(value, SeekOrigin.Begin); }
                }

                public override int 
                    Read(byte[] buffer, int offset, int count)
                {
                    var bytesToRead = this._SequentialAccessReadSize;
                    var results = this._Parent.InternalReadAsync(
                        this._ReadContext,
                        buffer,
                        offset,
                        count,
                        bytesToRead,
                        CancellationToken.None);

                    results.Wait();
                    this._ReadContext = results.Result.Context;

                    return results.Result.TotalDone;
                }

                public override async Task<int> 
                    ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
                {
                    var bytesToRead = this._SequentialAccessReadSize;

                    var results = this._Parent.InternalReadAsync(
                        this._ReadContext,
                        buffer,
                        offset,
                        count,
                        bytesToRead,
                        cancellationToken);

                    if (results.IsCompleted == false)
                    {
                        await results.ConfigureAwait(false);
                    }

                    this._ReadContext = results.Result.Context;

                    return results.Result.TotalDone;
                }

                public override long 
                    Seek(long offset, SeekOrigin origin)
                {
                    long newReadLocation = -1;

                    switch (origin)
                    {
                        case SeekOrigin.Begin:
                            newReadLocation = offset;
                            break;

                        case SeekOrigin.End:
                            newReadLocation = ((ILogicalLog) this._Parent).WritePosition + offset;
                            break;

                        case SeekOrigin.Current:
                            newReadLocation = this._ReadContext._ReadLocation + offset;
                            break;
                    }

                    // Check read buffer (if there is one) to see if we can reposition within that buffer
                    // 
                    if (this._ReadContext._ReadBuffer != null)
                    {
                        this._ReadContext._ReadBuffer.Dispose();
                        this._ReadContext._ReadBuffer = null;
                    }

                    if (this._ReadContext._NextReadTask != null)
                    {
                        this._ReadContext._NextReadTask.InvalidateRead();
                    }

                    this._ReadContext._ReadLocation = newReadLocation;
                    return this._ReadContext._ReadLocation;
                }

                public override void 
                    SetLength(long value)
                {
                    throw new NotImplementedException();
                }

                public override void 
                    Write(byte[] buffer, int offset, int count)
                {
                    throw new NotImplementedException();
                }
            }
        }
    }
}