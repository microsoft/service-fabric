// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log.Interop
{
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
//* Type mappings
    using KTL_LOG_ASN = System.UInt64;
    using BOOLEAN = System.SByte;
    using HRESULT = System.UInt32;
    using KTL_LOG_STREAM_ID = System.Guid;
    using KTL_LOG_STREAM_TYPE_ID = System.Guid;
    using KTL_LOG_ID = System.Guid;
    using KTL_DISK_ID = System.Guid;
    using EnumToken = System.UIntPtr;


    // Async Task result types for various Stream operations
    internal struct PhysicalLogStreamAsnRangeInfo
    {
        public KTL_LOG_ASN LowestAsn;
        public KTL_LOG_ASN HighestAsn;
        public KTL_LOG_ASN LogTruncationAsn;
    }

    internal struct PhysicalLogStreamReadResults
    {
        public KTL_LOG_ASN ResultingAsn;
        public UInt32 ResultingMetadataSize;
        public UInt64 Version;
        public NativeLog.IKIoBuffer ResultingMetadata;
        public NativeLog.IKIoBuffer ResultingIoPageAlignedBuffer;
    }

    internal struct PhysicalLogStreamQueryRecordResults
    {
        public UInt64 Version;
        public NativeLog.KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION Disposition;
        public UInt32 RecordSize;
        public UInt64 DebugInfo1;
    }

    internal struct IoctlResults
    {
        public UInt32 Result;
        public NativeLog.IKBuffer OutBuffer;
    }


    //** Copy of KLogicalLogInformation in KLogicalLog.h
    internal static class KLogicalLogInformation
    {
        //
        // This is the fixed size of the metadata buffer used for any
        // writes
        //
        public static Int32 FixedMetadataSize = 0x1000;

        //
        // This method returns the LogStreamType that is associated
        // with the winfab logical log
        //
        public static Guid GetLogicalLogStreamType()
        {
            // {99847EB8-B2CA-47c5-8919-62D16F3694A8}
            return new Guid(0x99847eb8, 0xb2ca, 0x47c5, 0x89, 0x19, 0x62, 0xd1, 0x6f, 0x36, 0x94, 0xa8);
        }

        //
        // This struct preceeds every stream block written and describes
        // the block of data. It is expected to be placed on an ULONGLONG
        // (8 byte boundary) and padded out to a ULONGLONG boundary. Any
        // data for the record immediately follows the stream block header
        // as it is padded to 8 bytes.
        //
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct StreamBlockHeader
        {
            public const UInt64 Sig = 0xabcdef0000fedcba;

            public UInt64 Signature;

            public Guid StreamId;

            //
            // This is the stream offset to which the block is written
            //
            public Int64 StreamOffsetPlusOne;

            //
            // This is a CRC64 for the stream block header only. In
            // computing this the HeaderCRC64 field is 0.
            //
            public UInt64 HeaderCRC64;

            //
            // This is a CRC64 for the data portion of the record. It is
            // computed from the data portion of the record that
            // immediately follows the stream block header for DataSize
            // bytes.
            //
            public UInt64 DataCRC64;

            //
            // This is the highest operation id or version associated with
            // this record block
            //
            public Int64 HighestOperationId;

            // 
            // Value of the HeadTruncation Point when block was written
            //
            public Int64 HeadTruncationPoint;

            //
            // This is the number of bytes of data for this record block.
            // Note that DataSize may not correspond to filling every byte
            // of the metadata and data blocks. In fact DataSize may be
            // zero in which case it means that there is no data at this
            // stream offset.
            //
            public UInt32 DataSize;

            //
            // Reserved, pad to 8 bytes
            //
            public UInt32 Reserved;
        }

        //
        // This header is part of the metadata that is written to a block
        // log store and follows the core logger and ktl shim metadata
        // header.
        //
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct MetadataBlockHeader
        {
            //
            // This is the offset from the beginning of the metadata block.
            // If the value is less than FixedMetadataSize then the stream
            // header is contained within the metadata. If larger than
            // FixedMetadataSize then the stream header is in the IoData
            // block
            //
            public UInt32 OffsetToStreamHeader;
            public UInt32 Flags;
        };

        public enum MetadatBlockHeaderFlags : uint
        {
            //
            // Indicates that the record written is the last record of a logical log record
            //
            IsEndOfLogicalRecord = (uint) 0x00000001
        }

        //
        // The logical log stream type plugin supports the following
        // controls that are sent using LogStream->StartIoctl()
        //

        //
        // ControlCode = QueryLogicalLogTailAsnAndHighestOperationCount
        // InBuffer = null
        // OutBuffer = struct LogicalLogTailAsnAndHighestOperation
        //
        public enum LogicalLogControlCodes : uint
        {
            QueryLogicalLogTailAsnAndHighestOperationControl = (uint) 1,
            WriteOnlyToDedicatedLog = (uint) 2,
            WriteToSharedAndDedicatedLog = (uint) 3,
            QueryCurrentWriteInformation = (uint) 4,
            QueryDriverBuildInformation = (uint) 5,
            SetWriteThrottleThreshold = (uint) 6,
#if DBG
            DelayDedicatedWrites = (uint)7,
            DelaySharedWrites = (uint)8,
#endif
            QueryCurrentLogUsageInformation = (uint) 9,
            
            EnableCoalescingWrites = (uint)10,
            DisableCoalescingWrites = (uint)11,
            SetSharedTruncationAsn = (uint)12,

            QueryLogicalLogReadInformationControl = (uint)13,
            Last = (uint) 13
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct LogicalLogTailAsnAndHighestOperation
        {
            public UInt64 HighestOperationCount;
            public UInt64 TailAsn;
            public UInt32 MaximumBlockSize;
            public KLogicalLogInformation.StreamBlockHeader RecoveredLogicalLogHeader;
        };

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct LogicalLogReadInformation
        {
            public UInt32 MaximumReadRecordSize;
        };


        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct DriverBuildInformation
        {
            public UInt32 BuildNumber;
            public BOOLEAN IsFreeBuild;
        };

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct CurrentLogUsageInformation
        {
            public UInt32 PercentageLogUsage;
        };
    }

    /// <summary>
    /// Each IPhysicalLogStream represents an underlying KtlLogStream instance - itself representing a logical 
    /// sequence of log records. A client can use an KtlLogStream object to write and read log records. It can also 
    /// use it to truncate a log stream. Log streams can be physically multiplexed into a single log container, 
    /// but they are logically separate.
    /// </summary>
    internal interface IPhysicalLogStream
    {
        /// <summary>
        /// Determine if the underlying stream and container are in a functional state
        /// </summary>
        /// <returns>false if the underlying stream or container is in a faulted state</returns>
        bool IsFunctional();

        /// <summary>
        /// Close the underlying KtlLogStream instance
        /// </summary>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>Task peforming the open</returns>
        Task CloseAsync(CancellationToken Token);

        /// <summary>
        /// Return the Unique ID assigned to the stream when it was created.
        /// </summary>
        Guid QueryLogStreamId();

        /// <summary>
        /// Return the byte offset at which user definied metadata may be written into the
        /// supplied IKIoBuffer to the WriteAsync method.
        /// </summary>
        /// <returns>Offset to user metadata</returns>
        UInt32 QueryReservedMetadataSize();

        /// <summary>
        /// This method asynchronously writes a new log record.
        /// </summary>
        /// <param name="Asn">Supplies the Asn (Application-assigned Sequence Number) of the record.</param>
        /// <param name="Version">Supplies the version number of the record. If a record with the same Asn
        /// already exists, the outcome depends on the Version value. If the existing record
        /// has a lower version, it will be overwritten. Otherwise, the new write will fail.</param>
        /// <param name="MetatdataSize">Supplies the length of the optional meta data block of the record.</param>
        /// <param name="MetadataBuffer">Supplies an optional meta data buffer block of the record. If specified 
        /// then the meta data buffer must be at least as large as the value specified by QueryReservedMetaDataSize() 
        /// and the application metadata placed after the log stream meta data header.</param>
        /// <param name="IoPageAlignedBuffer">Supplies an optional IO page-aligned buffer part of the record.</param>
        /// <param name="Token">Used to cancel the close operation</param>
        Task WriteAsync(
            KTL_LOG_ASN Asn,
            UInt64 Version,
            UInt32 MetatdataSize,
            NativeLog.IKIoBuffer MetadataBuffer,
            NativeLog.IKIoBuffer IoPageAlignedBuffer,
            CancellationToken Token);

        /// <summary>
        /// Read a record from a log stream
        /// </summary>
        /// <param name="Asn">Supplies the Asn (Application-assigned Sequence Number) of the record read.</param>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>PhysicalLogStreamReadResults</returns>
        Task<PhysicalLogStreamReadResults> ReadAsync(
            KTL_LOG_ASN Asn,
            CancellationToken Token);

        /// <summary>
        /// Read the physical record from a log stream that contains the given ASN (stream offset) (it's latest version)
        /// </summary>
        /// <param name="Asn">Supplies the Asn (Application-assigned Sequence Number) of the record read.</param>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>PhysicalLogStreamReadResults</returns>
        Task<PhysicalLogStreamReadResults> ReadContainingAsync(
            KTL_LOG_ASN Asn,
            CancellationToken Token);

        /// <summary>
        /// Read the physical record from a log stream that begins withthe given ASN (stream offset) (it's latest version) 
        /// and then data from subsequent records until all data requested has been read or end of stream.
        /// </summary>
        /// <param name="Asn">Supplies the Asn (Application-assigned Sequence Number) of the record read.</param>
        /// <param name="BytesToRead">Supplies the total number of bytes to read from the stream</param>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>PhysicalLogStreamReadResults</returns>
        Task<PhysicalLogStreamReadResults> MultiRecordReadAsync(
            KTL_LOG_ASN Asn,
            UInt32 BytesToRead,
            CancellationToken Token);

        /// <summary>
        /// This method requests asynchronous truncation of a log stream. Truncation will occur in the background,
        /// potentially after the operation completes.
        ///
        /// If there is enough log space, the caller would like to keep all recordswhose Asn's are higher than 
        /// PreferredTruncationPoint. If there is not enough space, any record with an Asn strictly less than or 
        /// equal to TruncationPoint can be truncated.
        /// </summary>
        /// <param name="TruncationPoint">Supplies the highest Asn that can be truncated.</param>
        /// <param name="PreferredTruncationPoint">Supplies the preferred log truncation point. PreferredTruncationPoint is 
        /// normally lower than TruncationPoint.</param>
        void Truncate(KTL_LOG_ASN TruncationPoint, KTL_LOG_ASN PreferredTruncationPoint);

        /// <summary>
        /// This method asynchronously queries the record ranges present in a current stream.          
        /// </summary>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>PhysicalLogStreamAsnRangeInfo</returns>
        Task<PhysicalLogStreamAsnRangeInfo> QueryRecordRangeAsync(CancellationToken Token);

        /// <summary>
        /// This method asynchronously queries the metadata information of a specfic log record.
        /// For the current version, record meta data is assumed to be small enoughto fit into memory. 
        /// </summary>
        /// <param name="Asn">Id of the record in question.</param>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>PhysicalLogStreamQueryRecordResults</returns>
        Task<PhysicalLogStreamQueryRecordResults> QueryRecordAsync(
            KTL_LOG_ASN Asn,
            CancellationToken Token);

        /// <summary>
        /// Query the metadata for a supplied range of log records.
        /// </summary>
        /// <param name="LowestAsn">First possible in the result set.</param>
        /// <param name="HighestAsn">Last possible in the result set.</param>
        /// <param name="ResultArray">Results are appended to this provided array. The array is logically of
        /// type KPHYSICAL_LOG_STREAM_RecordMetadata</param>
        /// <param name="Token">Used to cancel the close operation</param>
        Task QueryRecordsAsync(
            KTL_LOG_ASN LowestAsn,
            KTL_LOG_ASN HighestAsn,
            NativeLog.IKArray ResultArray,
            CancellationToken Token);

        /// <summary>
        /// This method starts an async which completes when a specific stream threshold 
        /// has been reached. The async operation is completed once when the threshold is 
        /// reached.
        ///
        /// Note that a stream will not be destructed properly if there is an AsyncStreamNotificationContext 
        /// pending and the stream was not first closed.
        /// </summary>
        /// <param name="ThresholdTypeEnum">Supported ThresholdTypeEnum Values:
        /// 
        ///     LogSpaceUtilization - this specifies that when usageof log space for the 
        ///     stream crosses a value that the operation is completed. ThresholdValue specifies the
        ///     percentage of log space used that triggers completion. Threshold value must be an integer
        ///     between 0 and 99.</param>
        /// <param name="ThresholdValue">0-99</param>
        /// <param name="Token">Used to cancel the close operation</param>
        Task WaitForNotificationAsync(
            NativeLog.KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE ThresholdTypeEnum,
            UInt64 ThresholdValue,
            CancellationToken Token);

        /// <summary>
        /// Issues an Ioctl to the dedicated logical log plug-in and get results back
        /// </summary>
        /// <param name="ControlCode">Transparent value</param>
        /// <param name="InBuffer">Transparent buffer passed to plug-in</param>
        /// <param name="Token">Used to cancel the close operation</param>
        Task<IoctlResults> IoCtlAsync(
            UInt32 ControlCode,
            NativeLog.IKBuffer InBuffer,
            CancellationToken Token);
    }


    //* KtlLogStream managed wrapper implementation
    internal unsafe class PhysicalLogStream : IPhysicalLogStream
    {
        private NativeLog.IKPhysicalLogStream _NativeStream;
        private NativeLog.IKPhysicalLogStream2 _NativeStream2;

        public
            PhysicalLogStream(NativeLog.IKPhysicalLogStream NativeStream)
        {
            this._NativeStream = NativeStream;
            if (this._NativeStream is NativeLog.IKPhysicalLogStream2)
            {
                this._NativeStream2 = this._NativeStream as NativeLog.IKPhysicalLogStream2;
            }
            else
            {
                this._NativeStream2 = null;
            }
        }

        //* IsFunctional
        public bool
            IsFunctional()
        {
            return this._NativeStream.IsFunctional() == (UInt32) 0;
        }

        //* Close
        public Task
            CloseAsync(CancellationToken Token)
        {
            Task t;

            t = Utility.WrapNativeAsyncInvokeInMTA(this.CloseBeginWrapper, this.CloseEndWrapper, Token, "NativeLog.CloseStream");

            return (t);
        }

        private NativeCommon.IFabricAsyncOperationContext
            CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream.BeginClose(Callback, out context);

            return context;
        }

        private void
            CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this._NativeStream.EndClose(context);
        }

        //* QueryLogStreamId
        public Guid
            QueryLogStreamId()
        {
            KTL_LOG_STREAM_ID id;

            this._NativeStream.QueryLogStreamId(out id);
            return id;
        }

        //* QueryReservedMetadataSize
        public UInt32
            QueryReservedMetadataSize()
        {
            UInt32 size;

            this._NativeStream.QueryReservedMetadataSize(out size);
            return size;
        }


        //* QueryRecordRange
        public Task<PhysicalLogStreamAsnRangeInfo>
            QueryRecordRangeAsync(CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<PhysicalLogStreamAsnRangeInfo>(
                (Callback) => this.QueryRecordRangeBeginWrapper(Callback),
                (Context) => this.QueryRecordRangeEndWrapper(Context),
                Token,
                "NativeLog.QueryRecordRange");
        }

        private NativeCommon.IFabricAsyncOperationContext
            QueryRecordRangeBeginWrapper(
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream.BeginQueryRecordRange(Callback, out context);
            return context;
        }

        private PhysicalLogStreamAsnRangeInfo
            QueryRecordRangeEndWrapper(
            NativeCommon.IFabricAsyncOperationContext Context)
        {
            PhysicalLogStreamAsnRangeInfo result;

            this._NativeStream.EndQueryRecordRange(
                Context,
                out result.LowestAsn,
                out result.HighestAsn,
                out result.LogTruncationAsn);

            return result;
        }


        //* Write
        public Task
            WriteAsync(
            KTL_LOG_ASN Asn,
            UInt64 Version,
            UInt32 MetatdataSize,
            NativeLog.IKIoBuffer MetadataBuffer,
            NativeLog.IKIoBuffer IoPageAlignedBuffer,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.WriteBeginWrapper(Asn, Version, MetatdataSize, MetadataBuffer, IoPageAlignedBuffer, Callback),
                (Context) => this.WriteEndWrapper(Context),
                Token,
                "NativeLog.WriteStream");
        }

        private NativeCommon.IFabricAsyncOperationContext
            WriteBeginWrapper(
            KTL_LOG_ASN Asn,
            UInt64 Version,
            UInt32 MetatdataSize,
            NativeLog.IKIoBuffer MetadataBuffer,
            NativeLog.IKIoBuffer IoPageAlignedBuffer,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            // CONSIDER: Does native code need to AddRef MetadataBuffer and IoPageAlignedBuffer ?

            this._NativeStream.BeginWrite(Asn, Version, MetatdataSize, MetadataBuffer, IoPageAlignedBuffer, Callback, out context);
            return context;
        }

        private void
            WriteEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeStream.EndWrite(Context);
        }

        //* Read
        public Task<PhysicalLogStreamReadResults>
            ReadAsync(
            KTL_LOG_ASN Asn,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<PhysicalLogStreamReadResults>(
                (Callback) => this.ReadBeginWrapper(Asn, Callback),
                (Context) => this.ReadEndWrapper(Context),
                Token,
                "NativeLog.ReadStream");
        }

        private NativeCommon.IFabricAsyncOperationContext
            ReadBeginWrapper(
            KTL_LOG_ASN Asn,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream.BeginRead(Asn, Callback, out context);
            return context;
        }

        private PhysicalLogStreamReadResults
            ReadEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            var results = new PhysicalLogStreamReadResults();

            this._NativeStream.EndRead(
                Context,
                out results.Version,
                out results.ResultingMetadataSize,
                out results.ResultingMetadata,
                out results.ResultingIoPageAlignedBuffer);

            return results;
        }

        public Task<PhysicalLogStreamReadResults>
            ReadContainingAsync(
            KTL_LOG_ASN Asn,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<PhysicalLogStreamReadResults>(
                (Callback) => this.ReadContainingBeginWrapper(Asn, Callback),
                (Context) => this.ReadContainingEndWrapper(Context),
                Token,
                "NativeLog.ReadContaining");
        }

        private NativeCommon.IFabricAsyncOperationContext
            ReadContainingBeginWrapper(
            KTL_LOG_ASN Asn,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream.BeginReadContaining(Asn, Callback, out context);
            return context;
        }

        private PhysicalLogStreamReadResults
            ReadContainingEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            var results = new PhysicalLogStreamReadResults();

            this._NativeStream.EndReadContaining(
                Context,
                out results.ResultingAsn,
                out results.Version,
                out results.ResultingMetadataSize,
                out results.ResultingMetadata,
                out results.ResultingIoPageAlignedBuffer);

            return results;
        }

        public Task<PhysicalLogStreamReadResults>
            MultiRecordReadAsync(
            KTL_LOG_ASN Asn,
            UInt32 BytesToRead,
            CancellationToken Token)
        {
            if (this._NativeStream2 != null)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PhysicalLogStreamReadResults>(
                    (Callback) => this.MultiRecordReadBeginWrapper(Asn, BytesToRead, Callback),
                    (Context) => this.MultiRecordReadEndWrapper(Context),
                    Token,
                    "NativeLog.MultiRecordRead");
            }
            else
            {
                //
                // Throw same exception that would be thrown if driver doesn't have the api
                //
                var innerException = new System.Runtime.InteropServices.COMException();
                throw new System.Fabric.FabricException("", innerException);
            }
        }

        private NativeCommon.IFabricAsyncOperationContext
            MultiRecordReadBeginWrapper(
            KTL_LOG_ASN Asn,
            UInt32 BytesToRead,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream2.BeginMultiRecordRead(Asn, BytesToRead, Callback, out context);
            return context;
        }

        private PhysicalLogStreamReadResults
            MultiRecordReadEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            var results = new PhysicalLogStreamReadResults();

            results.ResultingAsn = 0;
            results.Version = 0;
            results.ResultingMetadataSize = 0;

            this._NativeStream2.EndMultiRecordRead(
                Context,
                out results.ResultingMetadata,
                out results.ResultingIoPageAlignedBuffer);

            return results;
        }


        //* QueryRecord
        public Task<PhysicalLogStreamQueryRecordResults>
            QueryRecordAsync(
            KTL_LOG_ASN Asn,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<PhysicalLogStreamQueryRecordResults>(
                (Callback) => this.QueryRecordBeginWrapper(Asn, Callback),
                (Context) => this.QueryRecordEndWrapper(Context),
                Token,
                "NativeLog.QueryRecord");
        }

        private NativeCommon.IFabricAsyncOperationContext
            QueryRecordBeginWrapper(
            KTL_LOG_ASN Asn,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream.BeginQueryRecord(Asn, Callback, out context);
            return context;
        }

        private PhysicalLogStreamQueryRecordResults
            QueryRecordEndWrapper(
            NativeCommon.IFabricAsyncOperationContext Context)
        {
            var results = new PhysicalLogStreamQueryRecordResults();

            this._NativeStream.EndQueryRecord(
                Context,
                out results.Version,
                out results.Disposition,
                out results.RecordSize,
                out results.DebugInfo1);

            return results;
        }


        //* QueryRecords
        public Task
            QueryRecordsAsync(
            KTL_LOG_ASN LowestAsn,
            KTL_LOG_ASN HighestAsn,
            NativeLog.IKArray ResultArray,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.QueryRecordsBeginWrapper(LowestAsn, HighestAsn, ResultArray, Callback),
                (Context) => this.QueryRecordsEndWrapper(Context),
                Token,
                "NativeLog.QueryRecords");
        }

        private NativeCommon.IFabricAsyncOperationContext
            QueryRecordsBeginWrapper(
            KTL_LOG_ASN LowestAsn,
            KTL_LOG_ASN HighestAsn,
            NativeLog.IKArray ResultArray,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            // CONSIDER: Does native code need to AddRef ResultArray ?

            this._NativeStream.BeginQueryRecords(LowestAsn, HighestAsn, ResultArray, Callback, out context);
            return context;
        }

        private void
            QueryRecordsEndWrapper(
            NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeStream.EndQueryRecords(Context);
        }


        //* Truncate
        public void
            Truncate(KTL_LOG_ASN TruncationPoint, KTL_LOG_ASN PreferredTruncationPoint)
        {
            this._NativeStream.Truncate(TruncationPoint, PreferredTruncationPoint);
        }


        //* WaitForNotification
        public Task
            WaitForNotificationAsync(
            NativeLog.KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE NotificationType,
            UInt64 NotificationValue,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.WaitForNotificationBeginWrapper(NotificationType, NotificationValue, Callback),
                (Context) => this.WaitForNotificationEndWrapper(Context),
                Token,
                "NativeLog.WaitForNotification");
        }

        private NativeCommon.IFabricAsyncOperationContext
            WaitForNotificationBeginWrapper(
            NativeLog.KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE NotificationType,
            UInt64 NotificationValue,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeStream.BeginWaitForNotification(NotificationType, NotificationValue, Callback, out context);
            return context;
        }

        private void
            WaitForNotificationEndWrapper(
            NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeStream.EndWaitForNotification(Context);
        }


        //* Ioctl
        public Task<IoctlResults>
            IoCtlAsync(
            UInt32 ControlCode,
            NativeLog.IKBuffer InBuffer,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IoctlResults>(
                (Callback) => this.IoCtlAsyncBeginWrapper(ControlCode, InBuffer, Callback),
                (Context) => this.IoCtlAsyncEndWrapper(Context),
                Token,
                "NativeLog.WaitForNotification");
        }

        private NativeCommon.IFabricAsyncOperationContext
            IoCtlAsyncBeginWrapper(
            UInt32 ControlCode,
            NativeLog.IKBuffer InBuffer,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            // CONSIDER: does native code need to AddRef InBuffer ?

            this._NativeStream.BeginIoctl(ControlCode, InBuffer, Callback, out context);
            return context;
        }

        private IoctlResults
            IoCtlAsyncEndWrapper(
            NativeCommon.IFabricAsyncOperationContext Context)
        {
            IoctlResults results;

            this._NativeStream.EndIoctl(Context, out results.Result, out results.OutBuffer);
            return results;
        }
    }
}