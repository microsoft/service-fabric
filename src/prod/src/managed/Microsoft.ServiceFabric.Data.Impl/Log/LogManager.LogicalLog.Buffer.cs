// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log
{
    using System.Fabric.Common;
    using System.Runtime.CompilerServices;
    using System.Fabric.Data.Log.Interop;
    using System.Globalization;
    using System.Threading;

    /// <summary>
    /// LogManager.LogicalLog.Buffer implementation
    /// </summary>
    public static partial class LogManager
    {
        internal partial class LogicalLog : ILogicalLog
        {
            internal class Buffer : IDisposable
            {
                private readonly NativeLog.IKIoBuffer _MetadataKIoBuffer;
                private readonly NativeLog.IKIoBuffer _PageAlignedKIoBuffer;
                private readonly NativeLog.IKIoBuffer _PageAlignedKIoBufferView; // Used to do actual I/O - mapped to a subset of _OutputPageAlignedBuffer
                private readonly NativeLog.IKIoBuffer _CombinedKIoBuffer; // Combines _OutputMetadataBuffer & _OutputPageAlignedBuffer
                private readonly uint _MetadataSize;

                private KIoBufferStream _CombinedBufferStream; // Logical stream over _OutputMetadataBuffer & _OutputPageAlignedBuffer
                private uint _OffsetToData;
                private unsafe KLogicalLogInformation.StreamBlockHeader* _StreamBlockHeader;
                private unsafe KLogicalLogInformation.MetadataBlockHeader* _MetadataBlockHeader;
                private UInt32 _AllocationReportedToGC;

                // Properties
                public bool IsSealed
                {
                    get
                    {
                        unsafe
                        {
                            return this._StreamBlockHeader->HeaderCRC64 != 0;
                        }
                    }
                }

                public long BasePosition
                {
                    get
                    {
                        unsafe
                        {
                            return this._StreamBlockHeader->StreamOffsetPlusOne - 1;
                        }
                    }
                }

                public long SizeWritten
                {
                    get { return this._CombinedBufferStream.GetPosition() - this._OffsetToData; }
                }

                /// <summary>
                /// Return the number of bytes left to read in a KIoBufferStream
                /// </summary>
                public uint GetSizeLeftToRead()
                {
                    return (this._CombinedBufferStream.GetLength() - this._CombinedBufferStream.GetPosition());
                }

                public NativeLog.IKIoBuffer MetadataBuffer
                {
                    get { return this._MetadataKIoBuffer; }
                }

                public NativeLog.IKIoBuffer PageAlignedBuffer
                {
                    get { return this._PageAlignedKIoBuffer; }
                }

                public uint NumberOfBytes
                {
                    get { return this._CombinedBufferStream.GetLength(); }
                }

                private const int MetadataBlockSize = 4096;

                //* Batching filter for GC.*MemoryPressure API
                private static long s_AccumulatedDeltaGCSize = 0;
                private const long s_MaxAccumulatedDeltaGCSize = 100*1024*1024; // 100MB threshold

                private static void TryApplyGCMemoryPressureChange()
                {
                    var snappedValue = s_AccumulatedDeltaGCSize;
                    if (Math.Abs(snappedValue) >= s_MaxAccumulatedDeltaGCSize)
                    {
                        if (Interlocked.CompareExchange(ref s_AccumulatedDeltaGCSize, 0, snappedValue) == snappedValue)
                        {
                            // This thread must signal the change in load
                            if (snappedValue > 0)
                            {
                                GC.AddMemoryPressure(snappedValue);
                            }
                            else
                            {
                                GC.RemoveMemoryPressure(Math.Abs(snappedValue));
                            }
                        }
                    }
                }

                private static void AddGCMemoryPressure(uint delta)
                {
                    Interlocked.Add(ref s_AccumulatedDeltaGCSize, delta);
                    TryApplyGCMemoryPressureChange();
                }

                private static void RemoveGCMemoryPressure(uint delta)
                {
                    Interlocked.Add(ref s_AccumulatedDeltaGCSize, (-1L*(long) delta));
                    TryApplyGCMemoryPressureChange();
                }

                // static factories
                public static Buffer
                    CreateWriteBuffer(uint blockMetadataSize, uint maxBlockSize, long streamLocation, long opNumber, Guid StreamId)
                {
                    return new Buffer(blockMetadataSize, maxBlockSize, streamLocation, opNumber, StreamId);
                }

                public static Buffer
                    CreateReadBuffer(
                    uint blockMetadataSize,
                    long startingStreamPosition,
                    NativeLog.IKIoBuffer metadataBuffer,
                    NativeLog.IKIoBuffer pageAlignedKIoBuffer,
                    string traceType)
                {
                    return new Buffer(blockMetadataSize, startingStreamPosition, metadataBuffer, pageAlignedKIoBuffer, traceType);
                }


                private
                    Buffer(uint blockMetadataSize, uint maxBlockSize, long streamLocation, long opNumber, Guid StreamId)
                {
                    this._AllocationReportedToGC = maxBlockSize + (uint) KLogicalLogInformation.FixedMetadataSize;
                    AddGCMemoryPressure(this._AllocationReportedToGC);

                    this._MetadataSize = blockMetadataSize;
                    NativeLog.CreateSimpleKIoBuffer((uint) KLogicalLogInformation.FixedMetadataSize, out this._MetadataKIoBuffer);

                    //
                    // Allocate the write KIoBuffer to be the full block size requested by the caller despite the fact that some
                    // data will live in the metadata portion. Consider the fact that when the block is completely full, there will be some
                    // amount of data in the metadata and then the data portion will be full as well except for a gap at the end of the block
                    // which is the size of the data in the metadata portion. When rounding up we will need this last block despite it not being
                    // completely full.
                    //
                    NativeLog.CreateSimpleKIoBuffer((uint) (maxBlockSize), out this._PageAlignedKIoBuffer);

                    NativeLog.CreateEmptyKIoBuffer(out this._CombinedKIoBuffer);
                    this._CombinedKIoBuffer.AddIoBufferReference(this._MetadataKIoBuffer, 0, (uint) KLogicalLogInformation.FixedMetadataSize);
                    this._CombinedKIoBuffer.AddIoBufferReference(
                        this._PageAlignedKIoBuffer,
                        0,
                        (uint) (maxBlockSize - KLogicalLogInformation.FixedMetadataSize));

                    this._CombinedBufferStream = new KIoBufferStream(this._CombinedKIoBuffer, (uint) blockMetadataSize);

                    NativeLog.CreateEmptyKIoBuffer(out this._PageAlignedKIoBufferView);

                    unsafe
                    {
                        KLogicalLogInformation.MetadataBlockHeader* mdHdr = null;

                        mdHdr = (KLogicalLogInformation.MetadataBlockHeader*) this._CombinedBufferStream.GetBufferPointer();
                        this._MetadataBlockHeader = mdHdr;
                        ReleaseAssert.AssertIfNot(
                            this._CombinedBufferStream.Skip((uint) sizeof(KLogicalLogInformation.MetadataBlockHeader)),
                            "Unexpected Skip failure");

                        this._StreamBlockHeader = (KLogicalLogInformation.StreamBlockHeader*) this._CombinedBufferStream.GetBufferPointer();
                        mdHdr->OffsetToStreamHeader = this._CombinedBufferStream.GetPosition();

                        // Position the stream at 1st byte of user record
                        ReleaseAssert.AssertIfNot(
                            this._CombinedBufferStream.Skip((uint) sizeof(KLogicalLogInformation.StreamBlockHeader)),
                            "Unexpected Skip failure");


                        this._StreamBlockHeader->Signature = KLogicalLogInformation.StreamBlockHeader.Sig;
                        this._StreamBlockHeader->StreamOffsetPlusOne = streamLocation + 1;
                        this._StreamBlockHeader->HighestOperationId = opNumber;
                        this._StreamBlockHeader->StreamId = StreamId;

                        this._StreamBlockHeader->HeaderCRC64 = 0;
                        this._StreamBlockHeader->DataCRC64 = 0;
                        this._StreamBlockHeader->DataSize = 0;
                        this._StreamBlockHeader->Reserved = 0;

                        this._OffsetToData = mdHdr->OffsetToStreamHeader + (uint) sizeof(KLogicalLogInformation.StreamBlockHeader);
                    }
                }

                private
                    Buffer(
                    uint blockMetadataSize,
                    long startingStreamPosition,
                    NativeLog.IKIoBuffer metadataBuffer,
                    NativeLog.IKIoBuffer pageAlignedKIoBuffer,
                    string traceType)
                {
                    this._MetadataSize = blockMetadataSize;
                    this._MetadataKIoBuffer = metadataBuffer;
                    this._PageAlignedKIoBuffer = pageAlignedKIoBuffer;
                    unsafe
                    {
                        this._StreamBlockHeader = null;
                    }
                    this._PageAlignedKIoBufferView = null;
                    this._OffsetToData = uint.MaxValue;

                    NativeLog.CreateEmptyKIoBuffer(out this._CombinedKIoBuffer);
                    this.OpenForRead(startingStreamPosition, traceType);
                }

                ~Buffer()
                {
                    this.Dispose(false);
                }

                public void
                    Dispose()
                {
                    this.Dispose(true);
                }

                private void
                    Dispose(bool IsDisposing)
                {
                    if (IsDisposing)
                    {
                        GC.SuppressFinalize(this);
                        if (this._MetadataKIoBuffer != null)
                        {
                            this._MetadataKIoBuffer.Clear();
                        }

                        if (this._PageAlignedKIoBuffer != null)
                        {
                            this._PageAlignedKIoBuffer.Clear();
                        }

                        if (this._PageAlignedKIoBufferView != null)
                        {
                            this._PageAlignedKIoBufferView.Clear();
                        }

                        if (this._CombinedKIoBuffer != null)
                        {
                            this._CombinedKIoBuffer.Clear();
                        }

                        this._CombinedBufferStream.Reset();

                        unsafe
                        {
                            this._StreamBlockHeader = null;
                            this._MetadataBlockHeader = null;
                        }
                    }

                    if (this._AllocationReportedToGC > 0)
                    {
                        RemoveGCMemoryPressure(this._AllocationReportedToGC);
                        this._AllocationReportedToGC = 0;
                    }
                }

                /// <summary>
                /// Predicate that determines if a given stream-space interval intersects with the current buffer contents. This is 
                /// only valid for WriteBuffer and not ReadBuffer
                /// </summary>
                public bool
                    Intersects(long streamOffset, int size = 1)
                {
                    if (size <= 0)
                        throw new ArgumentOutOfRangeException();

                    if (!this.IsSealed)
                    {
                        long sizeOfUserDataInBuffer = this._CombinedBufferStream.GetPosition() - this._OffsetToData;
                        long bufferBaseStreamOffset;

                        unsafe
                        {
                            bufferBaseStreamOffset = this._StreamBlockHeader->StreamOffsetPlusOne - 1;
                        }

                        // If not outside completely, it must be at least partially inside
                        return !((streamOffset >= (bufferBaseStreamOffset + sizeOfUserDataInBuffer) ||
                                  ((streamOffset + size) <= bufferBaseStreamOffset)));
                    }

                    return false;
                }

                /// <summary>
                /// Position the buffer "cursor" for next get()/Put()
                /// </summary>
                public void
                    SetPosition(long bufferOffset)
                {
                    if ((bufferOffset > uint.MaxValue) || (bufferOffset < 0))
                        throw new ArgumentOutOfRangeException();

                    var requestedOffset = (uint) bufferOffset;
                    if (requestedOffset >= (this._CombinedBufferStream.GetLength() - this._OffsetToData))
                        throw new ArgumentOutOfRangeException();

                    ReleaseAssert.AssertIfNot(
                        this._CombinedBufferStream.PositionTo(this._OffsetToData + requestedOffset),
                        "Internal size or offset inconsistency");
                }

                /// <summary>
                /// Write bytes into the current buffer at it's "cursor"
                /// </summary>
                /// <returns>number of bytes written - limited by buffer size</returns>
                public unsafe uint
                    Put(byte* toWrite, uint size)
                {
                    var todo = Math.Min(size, this._CombinedBufferStream.GetLength() - this._CombinedBufferStream.GetPosition());
                    if (todo > 0)
                    {
                        this._CombinedBufferStream.Put(toWrite, todo);
                    }

                    return todo;
                }

                /// <summary>
                /// Write bytes into the current buffer at it's "cursor" from a byte[]
                /// </summary>
                /// <returns>number of bytes written - limited by buffer size</returns>
                [MethodImpl(MethodImplOptions.NoInlining)]
                public unsafe uint
                Put(ref byte[] buffer, int offset, uint count)
                {
                    uint done;

                    fixed (byte* outPtr = &buffer[offset])
                    {
                        done = Put(outPtr, count);
                    }

                    return (done);
                }

                /// <summary>
                /// Read bytes from the current buffer starting at its "cursor"
                /// </summary>
                /// <returns>Actual number of bytes read - limited by content size</returns>
                public unsafe uint
                    Get(byte* dest, uint size)
                {
                    var todo = Math.Min(size, this._CombinedBufferStream.GetLength() - this._CombinedBufferStream.GetPosition());
                    if (todo > 0)
                    {
                        this._CombinedBufferStream.Pull(dest, todo);
                    }

                    return todo;
                }

                /// <summary>
                /// Read bytes from the current buffer starting at its "cursor" into bytes array
                /// </summary>
                /// <returns>Actual number of bytes read - limited by content size</returns>
                [MethodImpl(MethodImplOptions.NoInlining)]
                public unsafe uint Get(ref byte[] buffer, int offset, uint size)
                {
                    uint done;
                    fixed (byte* outPtr = &buffer[offset])
                    {
                        done = Get(outPtr, size);
                    }
                    return (done);
                }

                /// <summary>
                /// Prepare the current buffer to be written
                /// </summary>
                public void
                    SealForWrite(
                    long currentHeadTruncationPoint,
                    bool IsBarrier,
                    out NativeLog.IKIoBuffer metadataBuffer,
                    out uint metadataSize,
                    out NativeLog.IKIoBuffer pageAlignedBuffer,
                    out long userSizeOfStreamData,
                    out long AsnOfRecord,
                    out long OpOfRecord)
                {
                    unsafe
                    {
                        uint trimSize = 0;
                        uint dataResidingOutsideMetadata = 0;
                        UInt64 dataCrc = 0;

                        this._MetadataBlockHeader->Flags = IsBarrier ? (uint) KLogicalLogInformation.MetadatBlockHeaderFlags.IsEndOfLogicalRecord : 0;

                        this._StreamBlockHeader->DataSize = this._CombinedBufferStream.GetPosition() - this._OffsetToData;
                        this._StreamBlockHeader->HeadTruncationPoint = currentHeadTruncationPoint;
                        this._PageAlignedKIoBufferView.Clear();

                        // Compute CRC64 for record data
                        if ((this._OffsetToData < KLogicalLogInformation.FixedMetadataSize) &&
                            ((this._OffsetToData + this._StreamBlockHeader->DataSize) <= KLogicalLogInformation.FixedMetadataSize))
                        {
                            // no data outside the metadata buffer 
                            metadataSize = this._OffsetToData + this._StreamBlockHeader->DataSize;
                        }
                        else
                        {
                            //
                            // Compute how much of the metadata is being used. It should be the entire 4K block minus the
                            // space reserved by the physical logger
                            //
                            metadataSize = (uint) KLogicalLogInformation.FixedMetadataSize - this._MetadataSize;
                            ReleaseAssert.AssertIfNot(this._CombinedBufferStream.PositionTo(this._OffsetToData), "Unexpected PositionTo failure");

                            KIoBufferStream.InterationCallback LocalHashFunc = ((
                                byte* ioBufferFragment,
                                ref UInt32 fragmentSize) =>
                            {
                                dataCrc = NativeLog.KCrc64(ioBufferFragment, fragmentSize, dataCrc);
                                return 0;
                            });

                            ReleaseAssert.AssertIfNot(
                                this._CombinedBufferStream.Iterate(LocalHashFunc, this._StreamBlockHeader->DataSize) == 0,
                                "Unexpected Iterate failure");

                            //
                            // Compute the number of data blocks that are needed to hold the data that is within the payload 
                            // data buffer. This excludes the payload data in the metadata portion
                            //
                            ReleaseAssert.AssertIf(this._OffsetToData > MetadataBlockSize, "Unexpected size for _OffsetToData");
                            dataResidingOutsideMetadata = this._StreamBlockHeader->DataSize - (MetadataBlockSize - this._OffsetToData);
                            trimSize = ((((dataResidingOutsideMetadata) + (MetadataBlockSize - 1))/MetadataBlockSize)*MetadataBlockSize);
                            this._PageAlignedKIoBufferView.AddIoBufferReference(
                                this._PageAlignedKIoBuffer,
                                0,
                                (uint) trimSize);
                        }
                        this._StreamBlockHeader->DataCRC64 = dataCrc;

                        // Now compute block header CRC
                        this._StreamBlockHeader->HeaderCRC64 = NativeLog.KCrc64(
                            this._StreamBlockHeader,
                            (uint) sizeof(KLogicalLogInformation.StreamBlockHeader),
                            0);

                        // Build results
                        metadataBuffer = this._MetadataKIoBuffer;
                        pageAlignedBuffer = this._PageAlignedKIoBufferView;
                        userSizeOfStreamData = this._StreamBlockHeader->DataSize;
                        AsnOfRecord = this._StreamBlockHeader->StreamOffsetPlusOne;
                        OpOfRecord = this._StreamBlockHeader->HighestOperationId;
                    }
                }

                /// <summary>
                /// Validate and prepare the current buffer for read (Get()) operations
                /// </summary>
                /// <param name="streamPosition">Buffer contents base stream-space location</param>
                /// <param name="traceType">The traceType to use for tracing.</param>
                private void
                    OpenForRead(long streamPosition, string traceType)
                {
                    // Combine the metadata and page-align IoBuffers
                    uint mdBufferSize;
                    uint paBufferSize;

                    this._MetadataKIoBuffer.QuerySize(out mdBufferSize);
                    this._PageAlignedKIoBuffer.QuerySize(out paBufferSize);

                    this._AllocationReportedToGC = mdBufferSize + paBufferSize;
                    AddGCMemoryPressure(this._AllocationReportedToGC);

                    this._CombinedKIoBuffer.Clear();
                    this._CombinedKIoBuffer.AddIoBufferReference(this._MetadataKIoBuffer, 0, mdBufferSize);
                    this._CombinedKIoBuffer.AddIoBufferReference(this._PageAlignedKIoBuffer, 0, paBufferSize);

                    this._CombinedBufferStream.Reuse(this._CombinedKIoBuffer, this._MetadataSize);

                    // Parse the log stream physical record
                    unsafe
                    {
                        KLogicalLogInformation.MetadataBlockHeader* mdHdr = null;

                        mdHdr = (KLogicalLogInformation.MetadataBlockHeader*) this._CombinedBufferStream.GetBufferPointer();

                        ReleaseAssert.AssertIfNot(
                            this._CombinedBufferStream.PositionTo(mdHdr->OffsetToStreamHeader),
                            "Unexpected PositionTo failure");

                        this._StreamBlockHeader = (KLogicalLogInformation.StreamBlockHeader*) this._CombinedBufferStream.GetBufferPointer();

                        // Position the stream at 1st byte of user record
                        ReleaseAssert.AssertIfNot(
                            this._CombinedBufferStream.Skip((uint) sizeof(KLogicalLogInformation.StreamBlockHeader)),
                            "Unexpected Skip failure");

                        this._OffsetToData = mdHdr->OffsetToStreamHeader + (uint) sizeof(KLogicalLogInformation.StreamBlockHeader);

#if false
    // TODO: Start: DebugOnly
                        UInt64      savedCrc = _StreamBlockHeader->HeaderCRC64;
                        _StreamBlockHeader->HeaderCRC64 = 0;
                        if (NativeLog.KCrc64(_StreamBlockHeader, (uint)sizeof(KLogicalLogInformation.StreamBlockHeader), 0) != savedCrc)
                            throw new InvalidDataException();

                        _StreamBlockHeader->HeaderCRC64 = 0;    // NOTE: Leave HeaderCRC64 0 - this is the unsealed condition

                        if ((_OffsetToData + _StreamBlockHeader->DataSize) > mdBufferSize)
                        {
                            savedCrc = 0;
                            KIoBufferStream.InterationCallback LocalHashFunc = ((
                                byte* ioBufferFragment,
                                ref UInt32 fragmentSize) =>
                            {
                                savedCrc = NativeLog.KCrc64(ioBufferFragment, fragmentSize, savedCrc);
                                return 0;
                            });

                            ReleaseAssert.AssertIfNot(
                                _CombinedBufferStream.Iterate(LocalHashFunc, _StreamBlockHeader->DataSize) == 0,
                                "Unexpected Iterate failure");

                            if (_StreamBlockHeader->DataCRC64 != savedCrc)
                                throw new InvalidDataException();
                        }
                        // TODO: END: DebugOnly
#endif

                        // Compute position and limits of the KIoBufferStream
                        System.Diagnostics.Debug.Assert(this._StreamBlockHeader->StreamOffsetPlusOne > 0);
                        var recordOffsetToDesiredPosition = streamPosition - (this._StreamBlockHeader->StreamOffsetPlusOne - 1);

                        if ((recordOffsetToDesiredPosition < 0) || (recordOffsetToDesiredPosition >= this._StreamBlockHeader->DataSize))
                        {
                            var s =
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    "OpenForRead: streamPosition {0}, StreamOffsetPlusOne {1}, DataSize {2}, recordOffsetToDesiredPosition {3}, HeadTruncationPoint {4}, HighestOperationId {5} ",
                                    streamPosition,
                                    this._StreamBlockHeader->StreamOffsetPlusOne,
                                    this._StreamBlockHeader->DataSize,
                                    recordOffsetToDesiredPosition,
                                    this._StreamBlockHeader->HeadTruncationPoint,
                                    this._StreamBlockHeader->HighestOperationId);

                            AppTrace.TraceSource.WriteInfo(
                                traceType,
                                s
                                );
                        }

                        System.Diagnostics.Debug.Assert(recordOffsetToDesiredPosition <= uint.MaxValue);
                        this._CombinedBufferStream.Reuse(
                            this._CombinedKIoBuffer,
                            (uint) recordOffsetToDesiredPosition + this._OffsetToData,
                            this._OffsetToData + this._StreamBlockHeader->DataSize);
                    }
                }
            }
        }
    }
}