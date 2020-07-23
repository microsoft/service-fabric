// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// Handy macros
//
class KLoggerUtilities
{
    public:
    static inline ULONG RoundUpTo4K(__in ULONG size)
    {
        return(((size) + 0xFFF) &(~0xFFF));
    }
    

    static BOOLEAN IsOn4KBoundary(__in ULONG x)
    {
        return(  (RoundUpTo4K(x)) == (x) );
    }

    template<typename T> static KActivityId ActivityIdFromStreamId(__in T s)
    {
        return((KActivityId)(s.Get().Data1));
    }   
};

class KLogicalLogInformation
{
    public:

    //
    // This is the fixed size of the metadata buffer used for any
    // writes
    //
    static const ULONG FixedMetadataSize = 0x1000;
    
    //
    // This method returns the LogStreamType that is associated
    // with the SF logical log
    //
    static RvdLogStreamType const&    GetLogicalLogStreamType()
    {
        // {99847EB8-B2CA-47c5-8919-62D16F3694A8}
        static GUID logicalLogLogStreamTypeGuid = 
                 { 0x99847eb8, 0xb2ca, 0x47c5, { 0x89, 0x19, 0x62, 0xd1, 0x6f, 0x36, 0x94, 0xa8 } };
        return *(reinterpret_cast<RvdLogStreamType const*>(&logicalLogLogStreamTypeGuid));
    }        
    
    //
    // This struct precedes every stream block written and describes
    // the block of data. It is expected to be placed on an ULONGLONG
    // (8 byte boundary) and padded out to a ULONGLONG boundary. Any
    // data for the record immediately follows the stream block header
    // as it is padded to 8 bytes.
    //
    struct StreamBlockHeader
    {
        static const ULONGLONG Sig = 0xabcdef0000fedcba;
        
        ULONGLONG Signature;
        
        KtlLogStreamId StreamId;

        //
        // This is the stream offset to which the block is written
        //
        ULONGLONG StreamOffset;
        
        //
        // This is a CRC64 for the stream block header only. In
        // computing this the HeaderCRC64 field is 0.
        //
        ULONGLONG HeaderCRC64;

        //
        // This is a CRC64 for the data portion of the record. It is
        // computed from the data portion of the record that
        // immediately follows the stream block header for DataSize
        // bytes.
        //
        ULONGLONG DataCRC64;

        //
        // This is the highest operation id or version associated with
        // this record block
        //
        ULONGLONG HighestOperationId;

        // 
        // Value of the HeadTruncation Point when block was written
        //
        ULONGLONG HeadTruncationPoint;      // NOTE: Transparent to KTL Logger

        //
        // This is the number of bytes of data for this record block.
        // Note that DataSize may not correspond to filling every byte
        // of the metadata and data blocks. In fact DataSize may be
        // zero in which case it means that there is no data at this
        // stream offset.
        //
        ULONG DataSize;

        //
        // Reserved, pad to 8 bytes
        //
        ULONG Reserved;
    };

    //
    // This computes a CRC for the stream block header. It should be
    // called after the DataCRC64 is computed.
    //
    static ULONGLONG ComputeStreamBlockHeaderCRC(
        __in KLogicalLogInformation::StreamBlockHeader* StreamBlockHdr
        )
    {
        StreamBlockHdr->HeaderCRC64 = 0;
        ULONGLONG crc64 = KChecksum::Crc64(StreamBlockHdr, sizeof(KLogicalLogInformation::StreamBlockHeader), 0);
        return(crc64);
    }

    static ULONGLONG ComputeDataBlockCRC(
        __in KLogicalLogInformation::StreamBlockHeader* StreamBlockHdr,
        __in KIoBuffer& IoData,
        __in ULONG IoDataOffset
        )
    {
        ULONG totalDataSize = StreamBlockHdr->DataSize;
        ULONGLONG crc64 = 0;

        if ((IoDataOffset < KLogicalLogInformation::FixedMetadataSize) &&
            (IoDataOffset + totalDataSize) <= KLogicalLogInformation::FixedMetadataSize)
        {
            //
            // If all data is contained within metadata block then no need
            // to compute a data CRC
            //
            return(0);
        }

        
        PUCHAR dataPtr;
        ULONG dataSize;

        if (IoDataOffset < KLogicalLogInformation::FixedMetadataSize)
        {
            //
            // First block to calculate is part of the metadata
            // block
            //
            dataPtr = (PUCHAR)StreamBlockHdr + sizeof(KLogicalLogInformation::StreamBlockHeader);
            dataSize = FixedMetadataSize - IoDataOffset;

            if (dataSize > totalDataSize)
            {
                dataSize = totalDataSize;
            }
            totalDataSize -= dataSize;

            crc64 = KChecksum::Crc64(dataPtr, dataSize, crc64);

            IoDataOffset = KLogicalLogInformation::FixedMetadataSize;
        }

        //
        // Now compute CRC for the data in the IoBuffer
        //
        KIoBufferElement* ioBufferElement;
        for (ioBufferElement = IoData.First();
             (ioBufferElement != NULL) && (totalDataSize > 0);
             ioBufferElement = IoData.Next(*ioBufferElement))
        {
            IoDataOffset = IoDataOffset - KLogicalLogInformation::FixedMetadataSize;
            dataPtr = (PUCHAR)ioBufferElement->GetBuffer() + IoDataOffset;
            dataSize = ioBufferElement->QuerySize() - IoDataOffset;

            if (dataSize > totalDataSize)
            {
                dataSize = totalDataSize;
            }
            totalDataSize -= dataSize;

            crc64 = KChecksum::Crc64(dataPtr, dataSize, crc64);

            IoDataOffset = KLogicalLogInformation::FixedMetadataSize;
        }
        
        return(crc64);
    }

    //
    // This header is part of the metadata that is written to a block
    // log store and follows the core logger and ktl shim metadata
    // header.
    //
    struct MetadataBlockHeader
    {
        //
        // This is the offset from the beginning of the metadata block.
        // If the value is less than FixedMetadataSize then the stream
        // header is contained within the metadata. If larger than
        // FixedMetadataSize then the stream header is in the IoData
        // block
        //
        ULONG OffsetToStreamHeader;

        //
        // These are a set of flags that describe the physical record written
        //

        //
        // Indicates that the record written is the last record of a logical log record
        //
        static const ULONG IsEndOfLogicalRecord = 0x00000001;

        //
        // Flags are used when the logical log writes from user mode
        // are sent to the driver and indicated that the record marker
        // is at the end of the record being written. Within the driver the
        // flag gets translated to RecordMarkerOffsetPlusOne which has
        // the actual offset of where the record marker is located. The
        // driver does this since multiple records written get
        // coalesced into a single larger record.
        //
        union
        {
            ULONG Flags;

            //
            // If this is the last record of a logical log then this is the offset into the
            // record that indicates the marked location.
            //
            ULONG RecordMarkerOffsetPlusOne;
        };
    };

    //
    // FindLogicalLogHeaders is a utility routine for parsing a
    // physical log record into the various headers and data pointers.
    // It will also do validation on the record to verify that the
    // headers are well formed.
    //
    // MetadataBuffer is a pointer to the record metadata
    //
    // IoBuffer is a pointer to the data Io buffer for the record
    //
    // ReservedMetadataSize is the size of any metadata that the
    //     physical logger reserves ahead of any metadata available to the
    //     user of the log. Typically this would be logStream->QueryReservedMetadataSize()
    //
    // MetadataHeader returns with a pointer to the MetadataBlockHeader
    //
    // StreamBlockHeader returns with a pointer to the
    //     StreamBlockHeader. Note that it may be in the metadata or io
    //     data buffers.
    //
    // DataSize returns with the size of the valid data for the record
    //
    // DataOffset returns with an offset pointing to the beginning of
    //     the data. If the offset is less than FixedMetadataSize then the
    //     beginning of the data is in the metadata buffer. If the offset
    //     is larger than FixedMetadataSize then the beginning of the data
    //     is in the io buffer. Note that data may cross from metadata
    //     buffer into data buffer.
    //
    static NTSTATUS FindLogicalLogHeaders(
        __in PUCHAR MetadataBuffer,
        __in KIoBuffer& IoBuffer,
        __in ULONG ReservedMetadataSize,
        __out KLogicalLogInformation::MetadataBlockHeader*& MetadataHeader,
        __out KLogicalLogInformation::StreamBlockHeader*& StreamBlockHeader,
        __out ULONG& DataSize,                             
        __out ULONG& DataOffset
    )
    {
        NTSTATUS status = STATUS_SUCCESS;       
        PUCHAR p = MetadataBuffer;
        ULONG totalDataSpace;

        StreamBlockHeader = NULL;
        
        MetadataHeader = (KLogicalLogInformation::MetadataBlockHeader*)(p + ReservedMetadataSize);

        if (MetadataHeader->OffsetToStreamHeader < KLogicalLogInformation::FixedMetadataSize)
        {
            if (! ( (MetadataHeader->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader)) <=
                        KLogicalLogInformation::FixedMetadataSize))
            {
                KDbgErrorWData(0, "KLogicalLogInformation::FindLogicalLogHeaders MetadataHeader",
                               STATUS_INVALID_PARAMETER,
                               MetadataHeader->OffsetToStreamHeader,
                               KLogicalLogInformation::FixedMetadataSize,
                               sizeof(KLogicalLogInformation::StreamBlockHeader),
                               0);
                return(STATUS_INVALID_PARAMETER);
            }

            StreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)(p + MetadataHeader->OffsetToStreamHeader);
        } else {
            if (! ( (MetadataHeader->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader) -
                         KLogicalLogInformation::FixedMetadataSize) <= IoBuffer.First()->QuerySize()) )
            {
                KDbgErrorWData(0, "KLogicalLogInformation::FindLogicalLogHeaders MetadataHeader2",
                               STATUS_INVALID_PARAMETER,
                               MetadataHeader->OffsetToStreamHeader,
                               KLogicalLogInformation::FixedMetadataSize,
                               sizeof(KLogicalLogInformation::StreamBlockHeader),
                               IoBuffer.First()->QuerySize());
                return(STATUS_INVALID_PARAMETER);
            }

            p = (PUCHAR)IoBuffer.First()->GetBuffer();     
            StreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)(p +
                                                    (MetadataHeader->OffsetToStreamHeader - KLogicalLogInformation::FixedMetadataSize));
        }

        DataSize = StreamBlockHeader->DataSize;
        DataOffset = MetadataHeader->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);

        //
        // Ensure there is enough room left in buffer for all declared
        // data
        //
        totalDataSpace = (KLogicalLogInformation::FixedMetadataSize + IoBuffer.QuerySize()) -
                         (MetadataHeader->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader));
        if (DataSize > totalDataSpace)
        {
            KDbgErrorWData(0, "KLogicalLogInformation::FindLogicalLogHeaders SpaceForData",
                           STATUS_INVALID_PARAMETER,
                           MetadataHeader->OffsetToStreamHeader,
                           DataSize,
                           totalDataSpace,
                           IoBuffer.QuerySize());
            return(STATUS_INVALID_PARAMETER);
        }
        
        return(status);
    }

    static NTSTATUS FindLogicalLogHeaders(
        __in KBuffer::SPtr MetadataBuffer,
        __in KIoBuffer& IoBuffer,
        __in ULONG ReservedMetadataSize,
        __out KLogicalLogInformation::MetadataBlockHeader*& MetadataHeader,
        __out KLogicalLogInformation::StreamBlockHeader*& StreamBlockHeader,
        __out ULONG& DataSize,                             
        __out ULONG& DataOffset
    )
    {
        NTSTATUS status;
        
        status = FindLogicalLogHeaders((PUCHAR)MetadataBuffer->GetBuffer(),
                              IoBuffer,
                              ReservedMetadataSize,
                              MetadataHeader,
                              StreamBlockHeader,
                              DataSize,
                              DataOffset);

        return(status);
    }

    static NTSTATUS FindLogicalLogHeadersWithCoreLoggerOffset(
        __in KBuffer::SPtr MetadataBuffer,
        __in ULONG CoreLoggerOffset,
        __in KIoBuffer& IoBuffer,
        __in ULONG ReservedMetadataSize,
        __out KLogicalLogInformation::MetadataBlockHeader*& MetadataHeader,
        __out KLogicalLogInformation::StreamBlockHeader*& StreamBlockHeader,
        __out ULONG& DataSize,                             
        __out ULONG& DataOffset
    )
    {
        NTSTATUS status;
        
        status = FindLogicalLogHeaders((PUCHAR)(MetadataBuffer->GetBuffer()) - CoreLoggerOffset,
                              IoBuffer,
                              ReservedMetadataSize + CoreLoggerOffset,
                              MetadataHeader,
                              StreamBlockHeader,
                              DataSize,
                              DataOffset);

        return(status);
    }


    static NTSTATUS FindLogicalLogHeadersWithCoreLoggerOffset(
        __in PUCHAR MetadataBuffer,
        __in ULONG CoreLoggerOffset,
        __in KIoBuffer& IoBuffer,
        __in ULONG ReservedMetadataSize,
        __out KLogicalLogInformation::MetadataBlockHeader*& MetadataHeader,
        __out KLogicalLogInformation::StreamBlockHeader*& StreamBlockHeader,
        __out ULONG& DataSize,                             
        __out ULONG& DataOffset
    )
    {
        NTSTATUS status;
        
        status = FindLogicalLogHeaders(MetadataBuffer - CoreLoggerOffset,
                              IoBuffer,
                              ReservedMetadataSize + CoreLoggerOffset,
                              MetadataHeader,
                              StreamBlockHeader,
                              DataSize,
                              DataOffset);

        return(status);
    }


    //
    // The logical log stream type plugin supports the following
    // controls that are sent using LogStream->StartIoctl()
    //
    //
    // ControlCode = QueryLogicalLogTailAsnAndHighestOperationCount
    //     InBuffer = NULL
    //     OutBuffer = struct LogicalLogTailAsnAndHighestOperation
    //
    //     returns the tail asn and other info useful for recovery
    //
    // ControlCode = WriteOnlyToDedicatedLog
    //     InBuffer = NULL
    //     OutBuffer = NULL
    //
    //     configured all writes to only to dedicated log
    //
    // ControlCode = WriteToSharedAndDedicatedLog
    //     InBuffer = NULL
    //     OutBuffer = NULL
    //
    //     configured all writes to both shared and dedicated log
    //
    // ControlCode = QueryCurrentWriteInformation
    //     InBuffer = NULL
    //     OutBuffer = CurrentWriteInformation
    //
    //     return information about the latest completed writes
    //
    // ControlCode = QueryCurrentBuildInformation
    //     InBuffer = NULL
    //     OutBuffer = CurrentBuildInformation
    //
    //     return information about the build version and type
    //
    // ControlCode = DelaySharedWrites
    //    InBuffer = NULL
    //    OutBuffer = NULL
    //
    //    Configures a delay when writing to shared log
    //
    // ControlCode = DelayDedicatedWrites
    //    InBuffer = NULL
    //    OutBuffer = NULL
    //
    //    Configures a delay when writing to dedicated log
    //
    // ControlCode = SetWriteThrottleThreshold
    //    InBuffer = WriteThrottleThreshold with new values
    //    OutBuffer = WriteThrottleThreshold with previous values
    //
    //    Set the write threshold for throttling for the current stream
    //
    // ControlCode = QueryCurrentLogUsage
    //     InBuffer = NULL
    //     OutBuffer = CurrentLogUsageInformation
    //
    //     return information about the build version and type
    //
    // ControlCode = SetSharedTruncationAsn
    //    InBuffer = SharedTruncationAsnStruct with new values
    //    OutBuffer = SharedTruncationAsnStruct with previous values
    //
    //    Set the shared truncating ASN for the current stream
    //
    // ControlCode = QueryLogicalLogReadInformation
    //     InBuffer = NULL
    //     OutBuffer = struct LogicalLogReadInformation
    //
    //     returns information used for reading the log
    //
    // ControlCode = QueryAcceleratedFlushMode
    //     InBuffer = NULL
    //     OutBuffer = struct AcceleratedFlushMode
    //
    //     returns information about accelerated flush mode
    //
    enum
    {
        QueryLogicalLogTailAsnAndHighestOperationControl = 1,
        WriteOnlyToDedicatedLog = 2,
        WriteToSharedAndDedicatedLog = 3,
        QueryCurrentWriteInformation = 4,
        QueryCurrentBuildInformation = 5,
        SetWriteThrottleThreshold = 6,
#if DBG
        DelayDedicatedWrites = 7,
        DelaySharedWrites = 8,
#endif
        QueryCurrentLogUsageInformation = 9,
        EnableCoalescingWrites = 10,
        DisableCoalescingWrites = 11,
        SetSharedTruncationAsn = 12,

        QueryLogicalLogReadInformation = 13,

        QueryTelemetryStatistics = 14,
                                         
        QueryLogSizeAndSpaceRemaining = 15,
        
        QueryAcceleratedFlushMode = 16,

        Last = 16
    } LogicalLogControlCodes;

    struct TelemetryStatistics
    {
        LONGLONG DedicatedWriteBytesOutstanding;
        LONGLONG DedicatedWriteBytesOutstandingThreshold;
        LONGLONG ApplicationBytesWritten;
        LONGLONG SharedBytesWritten;
        LONGLONG DedicatedBytesWritten;
    };
    
    struct LogicalLogTailAsnAndHighestOperation
    {
        ULONGLONG HighestOperationCount;
        ULONGLONG TailAsn;
        ULONG     MaximumBlockSize;
        KLogicalLogInformation::StreamBlockHeader RecoveredLogicalLogHeader;
    };
    
    struct CurrentWriteInformation
    {
        ULONGLONG MaxAsnWrittenToBothLogs;
    };

    struct CurrentBuildInformation
    {
        ULONG BuildNumber;
        BOOLEAN IsFreeBuild;
    };

    struct WriteThrottleThreshold
    {
        //
        // This is the maximum number of bytes that can be outstanding before
        // throttling occurs. 
        //
        LONGLONG MaximumOutstandingBytes;
    };
    static const LONGLONG WriteThrottleThresholdMinimum = 16 * (1024 * 1024);
    static const LONGLONG WriteThrottleThresholdNoLimit = -1;

    struct CurrentLogUsageInformation
    {
        ULONG PercentageLogUsage;
    };

    struct SharedTruncationAsnStruct
    {
        //
        // This is the current shared truncation ASN
        //
        RvdLogAsn SharedTruncationAsn;
    };

    struct LogicalLogReadInformation
    {
        ULONG     MaximumReadRecordSize;
    };
    
    struct LogSizeAndSpaceRemaining
    {
        ULONGLONG LogSize;
        ULONGLONG SpaceRemaining;
    };

    struct AcceleratedFlushMode
    {
        BOOLEAN IsAccelerateInActiveMode;
    };
        
};


            
class LLRecordObject : public KObject<LLRecordObject>
{
    public:
        LLRecordObject();
        ~LLRecordObject();

        VOID Clear();

        NTSTATUS Initialize(
            __in ULONG HeaderSkipOffset,                            
            __in ULONG CoreLoggerOverhead,
            __in PVOID MetadataBuffer,
            __in ULONG MetadataBufferSize,
            __in_opt KIoBuffer* IoBuffer
        );

        NTSTATUS Initialize(
            __in ULONG HeaderSkipOffset,                            
            __in ULONG CoreLoggerOverhead,
            __in KBuffer& MetadataBuffer,
            __in_opt KIoBuffer* IoBuffer
            );
        
        NTSTATUS Initialize(
            __in ULONG HeaderSkipOffset,                            
            __in ULONG CoreLoggerOverhead,
            __in KIoBuffer& IoBuffer
            );

        NTSTATUS Initialize(
            __in ULONG HeaderSkipOffset,                            
            __in ULONG CoreLoggerOverhead,
            __in KIoBuffer& MetadataIoBuffer,
            __in KIoBuffer& IoBuffer
            );

        NTSTATUS InitializeExisting(
            __in BOOLEAN IgnoreDataSize
        );
        
        VOID InitializeSourceForCopy();
        VOID InitializeDestinationForCopy();
        
        NTSTATUS ValidateRecord();

        VOID UpdateValidationInformation();

        VOID UpdateHeaderInformation(
            __in ULONGLONG StreamOffset,
            __in ULONGLONG Version,
            __in ULONG DataSize,
            __in ULONGLONG HeadTruncationPoint,
            __in ULONG Flags
            );

        VOID UpdateHeaderInformation(
            __in ULONGLONG Version,
            __in ULONGLONG HeadTruncationPoint,
            __in ULONG Flags
            );

        VOID UpdateHeaderInformation(
            __in ULONGLONG StreamOffset,
            __in ULONGLONG Version,
            __in ULONGLONG HeadTruncationPoint
            );

        BOOLEAN CopyFrom(
            __inout LLRecordObject& Source
            );

        BOOLEAN CopyFrom(
            __in PUCHAR& Source,
            __in ULONG& SourceOffset,
            __in ULONG& NumberOfBytes
            );

        BOOLEAN CopyFrom(
            __in KIoBuffer& Source,
            __in ULONG& SourceOffset,
            __in ULONG& NumberOfBytes
            );

        VOID InitializeHeadersForNewRecord(
            __in KtlLogStreamId StreamId,
            __in ULONGLONG StreamOffset,
            __in ULONGLONG Version,
            __in ULONGLONG HeadTruncationPoint = 0,
            __in ULONG Flags = 0
            );

        VOID TrimEndOfRecord(
            __in ULONG NewDataSize,
            __in ULONGLONG Version,
            __in ULONGLONG HeadTruncationPoint = 0,
            __in ULONG Flags = 0
            );

        NTSTATUS ExtendIoBuffer(
            __in KIoBuffer& IoBuffer,
            __in ULONG IoBufferOffset,
            __in ULONG SizeToExtend
        );

        NTSTATUS TrimIoBufferToSizeNeeded();
        
        //
        // Source or Destination Accessors
        //
        BOOLEAN IsInitialized()
        {
            return(_MetadataBuffer != NULL);
        }
        
        ULONG GetRecordMarkerPlusOne()
        {
            return(_MetadataHeader->RecordMarkerOffsetPlusOne);
        }

        ULONGLONG GetStreamOffset()
        {
            return(_StreamBlockHeader->StreamOffset);
        }

        ULONGLONG GetVersion()
        {
            return(_StreamBlockHeader->HighestOperationId);
        }

        KIoBuffer* GetIoBuffer()
        {
            return(_IoBuffer.RawPtr());
        }

        PUCHAR GetMetadataBuffer()
        {
            return(_MetadataBuffer);
        }

        ULONG GetMetadataBufferSize()
        {
            return(_MetadataBufferSize);
        }

        VOID SetRecordMarkerPlusOne(
            __in ULONG RecordMarkerPlusOne
        )
        {
            _MetadataHeader->RecordMarkerOffsetPlusOne = RecordMarkerPlusOne;
        }
        
        ULONG GetDataSize(
            )
        {
            return(_DataSize);
        }

        void SetDataSize(__in ULONG DataSize)
        {
            _DataSize = DataSize;
            _StreamBlockHeader->DataSize = _DataSize;           
        }

        ULONG GetDataOffset(
            )
        {
            return(_DataOffset);
        }

        KLogicalLogInformation::StreamBlockHeader* GetStreamBlockHeader(
            )
        {
            return(_StreamBlockHeader);
        }

        KLogicalLogInformation::MetadataBlockHeader* GetMetadataBlockHeader(
            )
        {
            return(_MetadataHeader);
        }

        ULONG GetTrimmedIoBufferSize()
        {
            return(KLoggerUtilities::RoundUpTo4K(_IoBuffer->QuerySize() - _IoBufferSizeLeft));
        }

        ULONG GetCoreLoggerOverhead()
        {
            return(_CoreLoggerOverhead);
        }

        void SetCoreLoggerOverhead(__in ULONG CoreLoggerOverhead)
        {
            _CoreLoggerOverhead = CoreLoggerOverhead;
        }

        ULONG GetHeaderSkipOffset()
        {
            return(_HeaderSkipOffset);
        }

        //
        // Source accessors
        //
        ULONG GetDataConsumed()
        {
            return(_DataConsumed);
        }
        
        ULONG GetDataLeftInSource()
        {
            return(_DataSizeInRecordMetadata + _DataSizeInRecordIoBuffer);
        }

        //
        // Destination Accessors
        //
        ULONG GetSpaceLeftInDestination()
        {
            return(_MetadataBufferSizeLeft + _IoBufferSizeLeft);
        }

        ULONG GetIoBufferSpaceLeftInDestination()
        {
            return(_IoBufferSizeLeft);
        }
        
        ULONG GetMetadataSpaceLeftInDestination()
        {
            return(_MetadataBufferSizeLeft);
        }

        ULONG GetIoBufferOffsetInDestination()
        {
            return(_IoBufferOffset);
        }       
        

    private:
        ULONG _HeaderSkipOffset;
        ULONG _CoreLoggerOverhead;
        ULONG _TotalOverhead;
        PUCHAR _MetadataBuffer;
        ULONG _MetadataBufferSize;
        KIoBuffer::SPtr _IoBuffer;

        KLogicalLogInformation::MetadataBlockHeader* _MetadataHeader;
        KLogicalLogInformation::StreamBlockHeader* _StreamBlockHeader;
        ULONG _DataSize;
        ULONG _DataOffset;

        //
        // Used for destination of copy
        //
        PUCHAR _MetadataBufferPtr;
        ULONG _MetadataBufferSizeLeft;
        ULONG _IoBufferOffset;
        ULONG _IoBufferSizeLeft;
        
        //
        // Used for source of copy
        //
        ULONG _DataSizeInRecordMetadata;
        ULONG _DataOffsetInRecordMetadata;
        ULONG _DataSizeInRecordIoBuffer;
        ULONG _DataOffsetInRecordIoBuffer;
        
        ULONG _DataConsumed;
};

            


class KtlLogVerify
{
    public:
        //
        // Record verification support
        //
        static const ULONGLONG VerificationDataSignature = 0xabbadaba;

        struct KtlVerificationData
        {
            ULONGLONG Signature;
            ULONGLONG DataCrc64;
        };

        struct KtlMetadataHeader
        {
            KtlVerificationData VerificationData;
            ULONG ActualMetadataSize;
        };

    public:
        static VOID
        ComputeCrc64ForIoBuffer(
            __in const KIoBuffer::SPtr& IoBuffer,
            __in ULONG NumberBytesToChecksum,
            __inout ULONGLONG& Crc64
            );

        static VOID
        ComputeRecordVerification(
            __out KtlLogVerify::KtlVerificationData& VerificationData,
            __in KtlLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in ULONG MetaDataLength,
            __in const KIoBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer
            );
        
        static NTSTATUS
        VerifyRecordCallback(
            __in_bcount(MetaDataBufferSize) UCHAR const *const MetaDataBuffer,
            __in ULONG MetaDataBufferSize,
            __in const KIoBuffer::SPtr& IoBuffer
            );                      
};

