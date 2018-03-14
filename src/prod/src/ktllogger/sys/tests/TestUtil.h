// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


NTSTATUS WriteRecord(
    KtlLogStream::SPtr& logStream,
    ULONGLONG version,
    KtlLogAsn recordAsn,
    ULONG DataBufferBlocks = 8,
    ULONG DataBufferBlockSize = 16 * 0x100,
    ULONGLONG ReservationToUse = 0,
    KIoBuffer::SPtr MetadataBuffer = nullptr,
    ULONG metadataIoSize = 0,
    ULONGLONG* WriteLatencyInMs = NULL
    );

NTSTATUS WriteRecord(
    KtlLogStream::SPtr& logStream,
    ULONGLONG version,
    KtlLogAsn recordAsn,
    KIoBuffer::SPtr IoBuffer,
    ULONGLONG ReservationToUse,
    KIoBuffer::SPtr MetadataBuffer,
    ULONG metadataIoSize,
    ULONGLONG* WriteLatencyInMs = NULL
    );


NTSTATUS FindDiskIdForDriveLetter(
    UCHAR& DriveLetter,
    GUID& DiskIdGuid
    );

NTSTATUS CompareKIoBuffersA(
    __in KIoBuffer& IoBufferA,
    __in KIoBuffer& IoBufferB
);


NTSTATUS PrepareRecordForWrite(
    __in KtlLogStream::SPtr& logStream,
    __in ULONGLONG version,
    __in KtlLogAsn recordAsn,
    __in ULONG myMetadataSize, // = 0x100;
    __in ULONG dataBufferBlocks, // 8
    __in ULONG dataBufferBlockSize, // 16* 0x1000
    __out KIoBuffer::SPtr& dataIoBuffer,
    __out KIoBuffer::SPtr& metadataIoBuffer 
    );

NTSTATUS TruncateStream(
    KtlLogStream::SPtr& logStream,
    KtlLogAsn recordAsn
);

NTSTATUS CreateAndFillIoBuffer(
    UCHAR Entropy,
    ULONG BufferSize,
    KIoBuffer::SPtr& IoBuffer
);

NTSTATUS CompareKIoBuffers(
    KIoBuffer& IoBufferA,
    KIoBuffer& IoBufferB,
    ULONG HeaderBytesToSkip
);


NTSTATUS ReadContainingRecord(
    KtlLogStream::SPtr logStream,    
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    );

NTSTATUS ReadNextRecord(
    KtlLogStream::SPtr logStream,    
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    );

NTSTATUS ReadPreviousRecord(
    KtlLogStream::SPtr logStream,    
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    );

NTSTATUS ReadExactRecord(
    KtlLogStream::SPtr logStream,    
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    );

NTSTATUS ValidateRecordData(
    ULONGLONG StreamOffsetExpected,
    ULONG DataSizeExpected,
    PUCHAR DataExpected,
    ULONGLONG VersionExpected,
    KtlLogStreamId& StreamIdExpected,
    ULONG metadataSizeRead,
    KIoBuffer::SPtr metadataIoBufferRead,
    KIoBuffer::SPtr dataIoBufferRead,
    ULONG reservedMetadataSize                          
    );

typedef VOID (*WackStreamBlockHeader)(KLogicalLogInformation::StreamBlockHeader* streamBlockHeader);

NTSTATUS SetupLLRecord(
    KtlLogStream::SPtr logStream,
    KIoBuffer::SPtr MetadataIoBuffer,
    KIoBuffer::SPtr& newMetadataBuffer,
    KIoBuffer::SPtr DataIoBuffer,                            
    KIoBuffer::SPtr& dataIoBufferRef,
    ULONGLONG version,
    ULONGLONG asn,
    PUCHAR data,
    ULONG dataSize,
    ULONG offsetToStreamBlockHeader,
    BOOLEAN IsBarrierRecord = TRUE,
    WackStreamBlockHeader WackIt = [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader){ UNREFERENCED_PARAMETER(streamBlockHeader);}
    );

NTSTATUS SetupRecord(
    KtlLogStream::SPtr logStream,
    KIoBuffer::SPtr& MetadataIoBuffer,
    KIoBuffer::SPtr& DataIoBuffer,                            
    ULONGLONG version,
    ULONGLONG asn,
    PUCHAR data,
    ULONG dataSize,
    ULONG offsetToStreamBlockHeader,
    BOOLEAN IsBarrierRecord,
    ULONGLONG* WriteLatencyInMs,
    WackStreamBlockHeader WackIt
    );
	
NTSTATUS SetupAndWriteRecord(
    KtlLogStream::SPtr logStream,
    KIoBuffer::SPtr MetadataIoBuffer,
    KIoBuffer::SPtr DataIoBuffer,                            
    ULONGLONG version,
    ULONGLONG asn,
    PUCHAR data,
    ULONG dataSize,
    ULONG offsetToStreamBlockHeader,
    BOOLEAN IsBarrierRecord = TRUE,
    ULONGLONG* WriteLatencyInMs = NULL,
    WackStreamBlockHeader WackIt = [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader){ UNREFERENCED_PARAMETER(streamBlockHeader);}
    );

VOID VerifyTailAndVersion(
    KtlLogStream::SPtr LogStream,
    ULONGLONG ExpectedVersion,
    ULONGLONG ExpectedAsn,
    KLogicalLogInformation::StreamBlockHeader* ExpectedHeader = NULL
    );

NTSTATUS GetStreamTruncationPoint(
    KtlLogStream::SPtr logStream,
    KtlLogAsn& truncationAsn
    );

VOID DeleteAllContainersOnDisk(
    KGuid diskId
    );

VOID DeleteAllContainersOnDisk(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    );
