// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        // todo: cleanup comments, casing, formatting, etc.
        /// <summary>
        /// A logical log represents a sequence of logged bytes via simple streaming semantics. An application can 
        /// use an ILogicalLog handle instance to write and read log records - format defined entirely by that user. 
        /// ILogicalLogs are also used to truncate a log stream - on either end - Head or Tail. New bytes are Appended 
        /// (written) only at the Tail and the oldest bytes are at the Head.
        ///
        /// Logical Logs may be physically multiplexed into a single log container (aka physical log), but they are 
        /// logically separate with their own 63-bit address space.
        /// </summary>
        interface ILogicalLog 
            : public Utilities::IDisposable
        {
            K_SHARED_INTERFACE(ILogicalLog)

        public:

            /// <summary>
            /// Close access to a current logical log and release any associated resources
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     
            /// </summary>
            /// <param name="cancellationToken">Used to cancel the CloseAsync operation</param>
            /// <returns></returns>
            virtual ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) = 0;

            /// <summary>
            /// Abort (rude close) the logical log use synchronously - will occur automatically through GC if CloseAsync is not called
            /// </summary>
            virtual void Abort() = 0;

            //
            // Determine if a current ILogicalLog and underlying resources are functional and available
            //
            __declspec(property(get = GetIsFunctional)) BOOLEAN IsFunctional;
            virtual BOOLEAN GetIsFunctional() = 0;

            /// <summary>
            /// Size of log stream's contents - only non-truncated space
            /// </summary>
            __declspec(property(get = GetLength)) LONGLONG Length;
            virtual LONGLONG GetLength() const = 0;

            /// <summary>
            /// Return the next stream-space address to be written into
            /// </summary>
            __declspec(property(get = GetWritePosition)) LONGLONG WritePosition;
            virtual LONGLONG GetWritePosition() const = 0;

            /// <summary>
            /// Return the next stream-space to be read from
            /// </summary>
            __declspec(property(get = GetReadPosition)) LONGLONG ReadPosition;
            virtual LONGLONG GetReadPosition() const = 0;

            /// <summary>
            /// Return the current Head truncation stream-space location - no information logically exists at and 
            /// below this point in the logical log
            /// </summary>
            __declspec(property(get = GetHeadTruncationPosition)) LONGLONG HeadTruncationPosition;
            virtual LONGLONG GetHeadTruncationPosition() const = 0;

            /// <summary>
            /// Return the maximum block size available for a physical record
            /// </summary>
            __declspec(property(get = GetMaximumBlockSize)) LONGLONG MaximumBlockSize;
            virtual LONGLONG GetMaximumBlockSize() const = 0;

            /// <summary>
            /// Return the size of the header used in the metadata block
            /// </summary>
            __declspec(property(get = GetMetadataBlockHeaderSize)) ULONG MetadataBlockHeaderSize;
            virtual ULONG GetMetadataBlockHeaderSize() const = 0;

            ///
            /// The current size of the occupied region of the log (bytes). Total disk-usage might exceed this if disk 
            /// space is pre-reserved (e.g. dedicated direct + nonsparse dedicated)>
            ///
            __declspec(property(get = GetSize)) ULONGLONG Size;
            virtual ULONGLONG GetSize() const = 0;

            ///
            /// The space remaining in the log (bytes). If exhausted, writes will fail.
            ///
            __declspec(property(get = GetSpaceRemaining)) ULONGLONG SpaceRemaining;
            virtual ULONGLONG GetSpaceRemaining() const = 0;

            /// <summary>
            /// Return an abstract Stream reference for this ILogicalLog optimized for sequential or random reads
            /// </summary>
            /// <param name="SequentialAccessReadSize">Number of bytes to read and cache on each read request for sequential access. Zero for random access with no read ahead.</param>
            virtual NTSTATUS CreateReadStream(__out ILogicalLogReadStream::SPtr& stream, __in LONG sequentialAccessReadSize) = 0;

            /// <summary>
            /// Set the read size for a sequential access stream
            /// </summary>
            /// <param name="LogStream">Sequential access LogicalLogStream on which to set the size</param>
            /// <param name="SequentialAccessReadSize">Number of bytes to read and cache on each read request. Zero for random access with no read ahead.</param>
            virtual VOID SetSequentialAccessReadSize(__in ILogicalLogReadStream& logStream, __in LONG sequentialAccessReadSize) = 0;

            /// <summary>
            ///  Read a sequence of bytes from the current logical log, advancing the ReadPosition within the stream by the 
            ///  number of bytes read
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     ArgumentOutOfRangeException
            ///     InvalidDataException
            ///     
            /// </summary>
            /// <param name="buffer">buffer to receive the bytes read</param>
            /// <param name="offset">offset into buffer to start the transfer</param>
            /// <param name="count">length of desired read</param>
            /// <param name="bytesToRead">number of bytes to read from log</param>
            /// <param name="cancellationToken">Used to cancel the ReadAsync operation</param>
            /// <returns>The number of bytes actually read - a zero indicates end-of-stream (WritePosition)</returns>
            virtual ktl::Awaitable<NTSTATUS> ReadAsync(
                __out LONG& bytesRead,
                __in KBuffer& buffer,
                __in ULONG offset,
                __in ULONG count,
                __in ULONG bytesToRead,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Set the ReadPosition within the current ILogicalLog
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     ArgumentOutOfRangeException
            /// 
            /// </summary>
            /// <param name="offset">Offset to the new ReadPosition</param>
            /// <param name="origin">Starting position: current, start, or end</param>
            /// <returns>New ReadLocation</returns>
            virtual NTSTATUS SeekForRead(__in LONGLONG offset, __in Common::SeekOrigin::Enum origin) = 0;

            /// <summary>
            /// Write a sequence of bytes to the current stream at WriteLocation, advancing the position within 
            /// this stream by the number of bytes written
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     
            /// </summary>
            /// <param name="buffer">source buffer for the bytes to be written</param>
            /// <param name="offset">offset into the source buffer at which the write starts</param>
            /// <param name="count">Size of the write</param>
            /// <param name="cancellationToken">Used to cancel the AppendAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const & buffer,
                __in LONG offsetIntoBuffer,
                __in ULONG count,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Cause any new buffered data from AppendAsync() to be written to the underlying device
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     
            /// </summary>
            /// <param name="cancellationToken">Used to cancel the FlushAsync</param>
            virtual ktl::Awaitable<NTSTATUS> FlushAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Cause any new buffered data from AppendAsync() to be written to the underlying device aLONGLONG with the
            /// barrier flag marking the end of a logical log record
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     
            /// </summary>
            /// <param name="cancellationToken">Used to cancel the FlushWithMarkerAsync</param>
            virtual ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Trigger the background truncation of the stream-space [0, StreamOffset]
            /// </summary>
            /// <param name="StreamOffset">New HeadTruncationPosition</param>
            virtual ktl::Awaitable<NTSTATUS> TruncateHead(__in LONGLONG StreamOffset) = 0;

            /// <summary>
            /// Truncation the stream-space [StreamOffset, WritePosition] - repositions WritePosition
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     ArgumentOutOfRangeException
            ///     
            /// </summary>
            /// <param name="StreamOffset">New WritePosition</param>
            /// <param name="cancellationToken">Used to cancel the TruncateTail</param>
            virtual ktl::Awaitable<NTSTATUS> TruncateTail(__in LONGLONG StreamOffset, __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Asynchronously wait for a given percentage of the logical log's stream-space to be used
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="percentOfSpaceUsed">Percentage full trigger point</param>
            /// <param name="cancellationToken">Used to cancel the WaitCapacityNotificationAsync</param>
            virtual ktl::Awaitable<NTSTATUS> WaitCapacityNotificationAsync(__in ULONG PercentOfSpaceUsed, __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Asynchronously wait for the write buffer of a current logical log to come close to full
            /// 
            /// Known Exceptions:
            /// 
            ///     NotImplementedException
            ///     
            /// </summary>
            /// <param name="cancellationToken">Used to cancel the WaitBufferFullNotificationAsync</param>
            virtual ktl::Awaitable<NTSTATUS> WaitBufferFullNotificationAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Asynchronously configure logical log to use dedicated log only
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            virtual ktl::Awaitable<NTSTATUS> ConfigureWritesToOnlyDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Asynchronously configure logical log to use both shared and dedicated log
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            virtual ktl::Awaitable<NTSTATUS> ConfigureWritesToSharedAndDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Asynchronously query the percentage usage of the logical log
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            virtual ktl::Awaitable<NTSTATUS> QueryLogUsageAsync(__out ULONG& PercentUsage, __in ktl::CancellationToken const & cancellationToken) = 0;
        };
    }
}
