// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include <ktl.h>
#include <ktrace.h>
#include "minmax.h"

#include "KtlPhysicalLog.h"
#include "../inc/ktllogger.h"
#include "../inc/KLogicalLog.h"
#include "KtlLogMarshal.h"

//
// LLRecordObject
//
LLRecordObject::LLRecordObject(
    )
{
    Clear();
}

LLRecordObject::~LLRecordObject()
{
}

VOID LLRecordObject::Clear()
{
    _HeaderSkipOffset = 0;
    _CoreLoggerOverhead = 0;
    _TotalOverhead = 0;
    _MetadataBuffer = NULL;
    _MetadataBufferSize = 0;
    _IoBuffer = nullptr;
    _MetadataHeader = NULL;
    _StreamBlockHeader = NULL;
    _DataSize = 0;
    _DataOffset = 0;
    _MetadataBufferPtr = NULL;
    _MetadataBufferSizeLeft = 0;
    _IoBufferOffset = 0;
    _IoBufferSizeLeft = 0;
    _DataSizeInRecordMetadata = 0;
    _DataOffsetInRecordMetadata = 0;
    _DataSizeInRecordIoBuffer = 0;
    _DataOffsetInRecordIoBuffer = 0;
    _DataConsumed = 0;
}

NTSTATUS LLRecordObject::Initialize(
    __in ULONG HeaderSkipOffset,                            
    __in ULONG CoreLoggerOverhead,
    __in PVOID MetadataBuffer,
    __in ULONG MetadataBufferSize,
    __in_opt KIoBuffer* IoBuffer
    )
{
    _MetadataBuffer = (PUCHAR)MetadataBuffer;
    _MetadataBufferSize = MetadataBufferSize;
    _IoBuffer = IoBuffer;
    _CoreLoggerOverhead = CoreLoggerOverhead;
    _HeaderSkipOffset = HeaderSkipOffset;
    _TotalOverhead = (CoreLoggerOverhead + sizeof(KtlLogVerify::KtlMetadataHeader) + 7) & ~7;
    return(STATUS_SUCCESS);
}

NTSTATUS LLRecordObject::Initialize(
    __in ULONG HeaderSkipOffset,                            
    __in ULONG CoreLoggerOverhead,
    __in KBuffer& MetadataBuffer,
    __in_opt KIoBuffer* IoBuffer
    )
{
    return(Initialize(HeaderSkipOffset, CoreLoggerOverhead, (PUCHAR)MetadataBuffer.GetBuffer(), MetadataBuffer.QuerySize(), IoBuffer));
}

NTSTATUS LLRecordObject::Initialize(
    __in ULONG HeaderSkipOffset,                            
    __in ULONG CoreLoggerOverhead,
    __in KIoBuffer& IoBuffer
    )
{
    NTSTATUS status;
    KIoBuffer::SPtr metadataIoBuffer;
    KIoBuffer::SPtr dataIoBuffer;

    status = KIoBuffer::CreateEmpty(metadataIoBuffer, IoBuffer.GetThisAllocator(), IoBuffer.GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    status = metadataIoBuffer->AddIoBufferReference(IoBuffer,
                                                    0,
                                                    KLogicalLogInformation::FixedMetadataSize);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = KIoBuffer::CreateEmpty(dataIoBuffer, IoBuffer.GetThisAllocator(), IoBuffer.GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    status = dataIoBuffer->AddIoBufferReference(IoBuffer,
                                                KLogicalLogInformation::FixedMetadataSize,
                                                IoBuffer.QuerySize() - KLogicalLogInformation::FixedMetadataSize);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = Initialize(HeaderSkipOffset, CoreLoggerOverhead, *metadataIoBuffer, *dataIoBuffer);
    
    return(status);
}

NTSTATUS LLRecordObject::Initialize(
    __in ULONG HeaderSkipOffset,                            
    __in ULONG CoreLoggerOverhead,
    __in KIoBuffer& MetadataIoBuffer,
    __in KIoBuffer& IoBuffer
    )
{
    NTSTATUS status;

    status = Initialize(HeaderSkipOffset,
                        CoreLoggerOverhead,
                        (PVOID)MetadataIoBuffer.First()->GetBuffer(),
                        KLogicalLogInformation::FixedMetadataSize,
                        &IoBuffer);

    return(status);
}

NTSTATUS LLRecordObject::InitializeExisting(
    __in BOOLEAN IgnoreDataSize
    )
{
    NTSTATUS status;

    KInvariant(! IgnoreDataSize);
    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(_MetadataBuffer,
                                                                               _CoreLoggerOverhead,
                                                                               *_IoBuffer,
                                                                               _TotalOverhead - _CoreLoggerOverhead,
                                                                               _MetadataHeader,
                                                                               _StreamBlockHeader,
                                                                               _DataSize,
                                                                               _DataOffset);
    
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    return(STATUS_SUCCESS);
}

NTSTATUS LLRecordObject::ValidateRecord(
    )
{
    NTSTATUS status;
    
    status = InitializeExisting(FALSE);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    status = KtlLogVerify::VerifyRecordCallback(_MetadataBuffer+_CoreLoggerOverhead, _MetadataBufferSize-_CoreLoggerOverhead, _IoBuffer);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    InitializeSourceForCopy();
    InitializeDestinationForCopy(); 
    
    return(STATUS_SUCCESS);
}

VOID LLRecordObject::UpdateValidationInformation(
    )
{
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader =
        (KtlLogVerify::KtlMetadataHeader*)(_MetadataBuffer + _CoreLoggerOverhead);

    metaDataHeader->ActualMetadataSize = _MetadataBufferSize;

    //
    // Add verification information to the metadata header
    //
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            _StreamBlockHeader->StreamOffset,
                                            _StreamBlockHeader->HighestOperationId,
                                            _MetadataBufferSize,
                                            nullptr,                // Not used by function
                                            _IoBuffer);

#if DBG
    _StreamBlockHeader->DataCRC64 = KLogicalLogInformation::ComputeDataBlockCRC(_StreamBlockHeader,
                                                                                *_IoBuffer,
                                                                                 _DataOffset);
    _StreamBlockHeader->HeaderCRC64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(_StreamBlockHeader);
#endif
}

VOID LLRecordObject::UpdateHeaderInformation(
    __in ULONGLONG StreamOffset,
    __in ULONGLONG Version,
    __in ULONGLONG HeadTruncationPoint
    )
{
    _StreamBlockHeader->StreamOffset = StreamOffset;
    _StreamBlockHeader->HeadTruncationPoint = HeadTruncationPoint;
    _StreamBlockHeader->HighestOperationId = Version;
}

VOID LLRecordObject::UpdateHeaderInformation(
    __in ULONGLONG StreamOffset,
    __in ULONGLONG Version,
    __in ULONG DataSize,
    __in ULONGLONG HeadTruncationPoint,
    __in ULONG Flags
    )
{
    _MetadataHeader->Flags = Flags;
    _StreamBlockHeader->StreamOffset = StreamOffset;
    _StreamBlockHeader->HeadTruncationPoint = HeadTruncationPoint;
    _StreamBlockHeader->HighestOperationId = Version;
    _StreamBlockHeader->DataSize = DataSize;

    _DataSize = DataSize;

    UpdateValidationInformation();
}

VOID LLRecordObject::UpdateHeaderInformation(
    __in ULONGLONG Version,
    __in ULONGLONG HeadTruncationPoint,
    __in ULONG Flags
    )
{
    _MetadataHeader->Flags = Flags;
    _StreamBlockHeader->HeadTruncationPoint = HeadTruncationPoint;
    _StreamBlockHeader->HighestOperationId = Version;
    _StreamBlockHeader->DataSize = _DataSize;

    UpdateValidationInformation();
}

VOID LLRecordObject::InitializeHeadersForNewRecord(
    __in KtlLogStreamId StreamId,
    __in ULONGLONG StreamOffset,
    __in ULONGLONG Version,
    __in ULONGLONG HeadTruncationPoint,
    __in ULONG Flags
    )
{
    NTSTATUS status;
    
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;

    metadataBlockHeader = (KLogicalLogInformation::MetadataBlockHeader*)(_MetadataBuffer + _TotalOverhead);
    ULONG offsetToStreamBlockHeader = _TotalOverhead + sizeof(KLogicalLogInformation::MetadataBlockHeader);
    metadataBlockHeader->OffsetToStreamHeader = offsetToStreamBlockHeader + _HeaderSkipOffset;
    metadataBlockHeader->Flags = Flags;

    streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)(_MetadataBuffer + offsetToStreamBlockHeader);
    RtlZeroMemory(streamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));

    streamBlockHeader->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
    streamBlockHeader->StreamId = StreamId;
    streamBlockHeader->StreamOffset = StreamOffset;
    streamBlockHeader->HighestOperationId = Version;
    streamBlockHeader->DataSize = 0;
    streamBlockHeader->HeadTruncationPoint = HeadTruncationPoint;

    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(_MetadataBuffer, _HeaderSkipOffset, *_IoBuffer, _TotalOverhead,
                                                           _MetadataHeader, _StreamBlockHeader, _DataSize, _DataOffset);
    KInvariant(NT_SUCCESS(status));

    InitializeSourceForCopy();
    InitializeDestinationForCopy(); 
}

VOID LLRecordObject::InitializeDestinationForCopy()
{
    ULONG numberBytesInMetadata = KLogicalLogInformation::FixedMetadataSize - _DataOffset;

    if (_DataSize < numberBytesInMetadata)
    {
        _MetadataBufferPtr = _MetadataBuffer + (_DataOffset - _HeaderSkipOffset) + _DataSize;
        _MetadataBufferSizeLeft = numberBytesInMetadata - _DataSize;
        _IoBufferOffset = 0;
        _IoBufferSizeLeft = _IoBuffer->QuerySize();
    } else {
        ULONG numberBytesInIoBuffer = _DataSize - numberBytesInMetadata;
        _MetadataBufferPtr = _MetadataBuffer + numberBytesInMetadata;
        _MetadataBufferSizeLeft = 0;
        _IoBufferOffset = numberBytesInIoBuffer;
        _IoBufferSizeLeft = _IoBuffer->QuerySize() - numberBytesInIoBuffer;
    }
}

VOID LLRecordObject::InitializeSourceForCopy()
{
    _DataConsumed = 0;
    _DataSizeInRecordMetadata = KLogicalLogInformation::FixedMetadataSize - _DataOffset;
    _DataOffsetInRecordMetadata = _DataOffset - _HeaderSkipOffset;
    _DataOffsetInRecordIoBuffer = 0;

    if (_DataSize <= _DataSizeInRecordMetadata)
    {
        _DataSizeInRecordMetadata = _DataSize;
        _DataSizeInRecordIoBuffer = 0;
    }
    else
    {
        _DataSizeInRecordIoBuffer = _DataSize - _DataSizeInRecordMetadata;
    }
}

NTSTATUS LLRecordObject::TrimIoBufferToSizeNeeded()
{
    NTSTATUS status;
	KIoBuffer::SPtr trimmedIoBuffer;
    ULONG trimmedIoBufferSize = GetTrimmedIoBufferSize();

    _StreamBlockHeader->DataSize = _DataSize;

    if (trimmedIoBufferSize < _IoBuffer->QuerySize())
    {
        //
        // If there is extra space at the end of the io buffer then we
        // need to trim it back
        //
		status = KIoBuffer::CreateEmpty(trimmedIoBuffer, _IoBuffer->GetThisAllocator(), _IoBuffer->GetThisAllocationTag());
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        if (trimmedIoBufferSize > 0)
        {
			status = trimmedIoBuffer->AddIoBufferReference(*_IoBuffer, 0, trimmedIoBufferSize, _IoBuffer->GetThisAllocationTag());
            if (!NT_SUCCESS(status))
            {
                return(status);
            }
        }

        _IoBuffer = trimmedIoBuffer;
    }
    return(STATUS_SUCCESS);
}

VOID LLRecordObject::TrimEndOfRecord(
    __in ULONG NewDataSize,
    __in ULONGLONG Version,
    __in ULONGLONG HeadTruncationPoint,
    __in ULONG Flags
    )
{
    UpdateHeaderInformation(_StreamBlockHeader->StreamOffset, Version, NewDataSize, HeadTruncationPoint, Flags);
    InitializeDestinationForCopy(); 
}

BOOLEAN LLRecordObject::CopyFrom(
    __inout LLRecordObject& Source
    )
{    
    //
    // Copy data from the source into this buffer
    //
    ULONG bytesToCopy;
    PUCHAR recordDataPtr;
    
    if (_MetadataBufferSizeLeft > 0)
    {
        //
        // If there is room to write into the output metadata, start there with the record metadata
        //
        bytesToCopy = _MetadataBufferSizeLeft;
        if (Source._DataSizeInRecordMetadata < bytesToCopy)
        {
            bytesToCopy = Source._DataSizeInRecordMetadata;
        }

        recordDataPtr = (PUCHAR)(Source._MetadataBuffer) + Source._DataOffsetInRecordMetadata;
        KMemCpySafe(_MetadataBufferPtr, _MetadataBufferSizeLeft, recordDataPtr, bytesToCopy);

        _DataSize += bytesToCopy;
        _MetadataBufferPtr += bytesToCopy;
        _MetadataBufferSizeLeft -= bytesToCopy;

        Source._DataOffsetInRecordMetadata += bytesToCopy;
        Source._DataSizeInRecordMetadata -= bytesToCopy;
        Source._DataConsumed += bytesToCopy;
    }

    if ((_MetadataBufferSizeLeft > 0) && (Source._DataSizeInRecordIoBuffer > 0))
    {
        //
        // If there is still room to write into the output metadata, try to fill with IoBuffer
        //
        bytesToCopy = _MetadataBufferSizeLeft;
        if (Source._DataSizeInRecordIoBuffer < bytesToCopy)
        {
            bytesToCopy = Source._DataSizeInRecordIoBuffer;
        }

        KIoBufferStream recordDataStream(*Source._IoBuffer, Source._DataOffsetInRecordIoBuffer);
        recordDataStream.Pull(_MetadataBufferPtr, bytesToCopy);

        _DataSize += bytesToCopy;
        _MetadataBufferPtr += bytesToCopy;
        _MetadataBufferSizeLeft -= bytesToCopy;

        Source._DataOffsetInRecordIoBuffer += bytesToCopy;
        Source._DataSizeInRecordIoBuffer -= bytesToCopy;
        Source._DataConsumed += bytesToCopy;
    }

    if (Source._DataSizeInRecordMetadata > 0)
    {
        //
        // If there is still data in the record metadata then move into IoBuffer
        //
        bytesToCopy = _IoBufferSizeLeft;
        if (Source._DataSizeInRecordMetadata < bytesToCopy)
        {
            bytesToCopy = Source._DataSizeInRecordMetadata;
        }

        if (bytesToCopy > 0)
        {
            ULONG zero = 0;
            ULONG bytesToCopyCopy = bytesToCopy;
            recordDataPtr = (PUCHAR)(Source._MetadataBuffer) + (Source._DataOffsetInRecordMetadata);
            CopyFrom(recordDataPtr, zero, bytesToCopyCopy);

            // _IoBuffer* updated in CopyFrom

            Source._DataOffsetInRecordMetadata += bytesToCopy;
            Source._DataSizeInRecordMetadata -= bytesToCopy;
            Source._DataConsumed += bytesToCopy;
        }
    }

    if (Source._DataSizeInRecordIoBuffer > 0)
    {
        //
        // If there is data in the IoBuffer then move it
        //
        bytesToCopy = _IoBufferSizeLeft;
        if (Source._DataSizeInRecordIoBuffer < bytesToCopy)
        {
            bytesToCopy = Source._DataSizeInRecordIoBuffer;
        }

        ULONG bytesToCopyCopy = bytesToCopy;
        ULONG dataOffsetInRecordIoBuffer = Source._DataOffsetInRecordIoBuffer;
        CopyFrom(*Source._IoBuffer, dataOffsetInRecordIoBuffer, bytesToCopyCopy);

        // _IoBuffer* updated in CopyFrom

        Source._DataSizeInRecordIoBuffer -= bytesToCopy;
        Source._DataOffsetInRecordIoBuffer += bytesToCopy;
        Source._DataConsumed += bytesToCopy;
    }

    return((_MetadataBufferSizeLeft + _IoBufferSizeLeft) == 0);
}


BOOLEAN LLRecordObject::CopyFrom(
    __in PUCHAR& Source,
    __in ULONG& SourceOffset,
    __in ULONG& NumberOfBytes
    )
{
    //
    // Copy data from the source into this buffer
    //
    ULONG bytesToCopy;
    PUCHAR recordDataPtr;

    Source = Source + SourceOffset;

    if (_MetadataBufferSizeLeft > 0)
    {
        //
        // If there is room to write into the output metadata, start there with the record metadata
        //
        bytesToCopy = _MetadataBufferSizeLeft;
        if (NumberOfBytes < bytesToCopy)
        {
            bytesToCopy = NumberOfBytes;
        }

        recordDataPtr = Source;
        KMemCpySafe(_MetadataBufferPtr, _MetadataBufferSizeLeft, recordDataPtr, bytesToCopy);

        _DataSize += bytesToCopy;
        _MetadataBufferPtr += bytesToCopy;
        _MetadataBufferSizeLeft -= bytesToCopy;

        NumberOfBytes -= bytesToCopy;
        Source += bytesToCopy;
    }

    if (NumberOfBytes > 0)
    {
        //
        // If there is still data in the record metadata then move into IoBuffer
        //
        bytesToCopy = _IoBufferSizeLeft;
        if (NumberOfBytes < bytesToCopy)
        {
            bytesToCopy = NumberOfBytes;
        }

        if (bytesToCopy > 0)
        {
            KIoBufferStream::CopyTo(*_IoBuffer, _IoBufferOffset, bytesToCopy, Source);

            _DataSize += bytesToCopy;
            _IoBufferOffset += bytesToCopy;
            _IoBufferSizeLeft -= bytesToCopy;

            NumberOfBytes -= bytesToCopy;
            Source += bytesToCopy;
        }
    }

    Source -= SourceOffset;
    return((_MetadataBufferSizeLeft + _IoBufferSizeLeft) == 0);
}

BOOLEAN LLRecordObject::CopyFrom(
    __in KIoBuffer& Source,
    __in ULONG& SourceOffset,
    __in ULONG& NumberOfBytes
    )
{
    ULONG numberElements = Source.QueryNumberOfIoBufferElements();
    ULONG elementsOffsetLow = 0;
    ULONG elementsOffsetHigh = 0;
    KIoBufferElement::SPtr element;
    BOOLEAN isDestinationFull = FALSE;

    element = Source.First();
    for (ULONG i = 0; (i < numberElements) && (NumberOfBytes > 0) && (! isDestinationFull); i++)
    {

        elementsOffsetHigh = elementsOffsetLow + element->QuerySize();
        if ((SourceOffset >= elementsOffsetLow) && (SourceOffset < elementsOffsetHigh))
        {
            ULONG elementLength = elementsOffsetHigh - elementsOffsetLow;
            ULONG bytesToCopy, bytesCopied;
            PUCHAR p;

            p = (PUCHAR)(element->GetBuffer()) + (SourceOffset - elementsOffsetLow);
            bytesToCopy = NumberOfBytes;
            if (bytesToCopy > elementLength)
            {
                bytesToCopy = elementLength;
            }

            ULONG zero = 0;
            bytesCopied = bytesToCopy;
            isDestinationFull = CopyFrom(p, zero, bytesCopied);

            bytesCopied = bytesToCopy - bytesCopied;
            SourceOffset += bytesCopied;
            NumberOfBytes -= bytesCopied;
        }

        elementsOffsetLow = elementsOffsetHigh;
        element = Source.Next(*element);
    }

    return(isDestinationFull);
}


NTSTATUS LLRecordObject::ExtendIoBuffer(
    __in KIoBuffer& IoBuffer,
    __in ULONG IoBufferOffset,
    __in ULONG SizeToExtend
)
{
    NTSTATUS status;

    KInvariant(KLoggerUtilities::IsOn4KBoundary(IoBufferOffset));
    KInvariant(KLoggerUtilities::IsOn4KBoundary(SizeToExtend));
    KInvariant((IoBufferOffset + SizeToExtend) <= IoBuffer.QuerySize());


    status = _IoBuffer->AddIoBufferReference(IoBuffer, IoBufferOffset, SizeToExtend);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    _IoBufferSizeLeft += SizeToExtend;
    return(status);
}

//
// Verification computation and callback
//
VOID
KtlLogVerify::ComputeCrc64ForIoBuffer(
    __in const KIoBuffer::SPtr& IoBuffer,
    __in ULONG NumberBytesToChecksum,
    __inout ULONGLONG& Crc64
    )
{
    KIoBufferElement::SPtr ioBufferElement;
    ULONG bytesLeft = NumberBytesToChecksum;
    
    for (ioBufferElement = IoBuffer->First();
         ((ioBufferElement) && (bytesLeft > 0));
         ioBufferElement = IoBuffer->Next(*ioBufferElement))
    {
        ULONG bytesToChecksum = MIN(ioBufferElement->QuerySize(), bytesLeft);
        bytesLeft -= bytesToChecksum;
        
        Crc64 = KChecksum::Crc64(ioBufferElement->GetBuffer(),
                                 bytesToChecksum,
                                 Crc64);
    }
    
    KInvariant(bytesLeft == 0);
}

VOID
KtlLogVerify::ComputeRecordVerification(
    __out KtlLogVerify::KtlVerificationData& VerificationData,
    __in KtlLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataLength,
    __in const KIoBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer
    )
{
    UNREFERENCED_PARAMETER(RecordAsn);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(MetaDataLength);
    UNREFERENCED_PARAMETER(MetaDataBuffer);
    
    RtlZeroMemory(&VerificationData, sizeof(VerificationData));
    VerificationData.Signature = KtlLogVerify::VerificationDataSignature;
    ComputeCrc64ForIoBuffer(IoBuffer,
                            IoBuffer->QuerySize(),
                            VerificationData.DataCrc64);    
}

NTSTATUS
KtlLogVerify::VerifyRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const MetaDataBuffer,
    __in ULONG MetaDataBufferSize,
    __in const KIoBuffer::SPtr& IoBuffer
)
{
    NTSTATUS status;
    
    if (MetaDataBufferSize < sizeof(KtlLogVerify::KtlMetadataHeader))
    {
        //
        // Metadata buffer is too small so it must be wrong
        //
        status = K_STATUS_LOG_STRUCTURE_FAULT;
        return(status);
    }

    const struct KtlLogVerify::KtlMetadataHeader* metaDataHeader = (struct KtlLogVerify::KtlMetadataHeader*)MetaDataBuffer;   
    const struct KtlLogVerify::KtlVerificationData* verificationData = &metaDataHeader->VerificationData;
    
    if (verificationData->Signature != KtlLogVerify::VerificationDataSignature)
    {
        //
        // Signature doesn't match
        //
        status = K_STATUS_LOG_STRUCTURE_FAULT;
        return(status);
    }

    ULONGLONG crc64 = 0;
    ComputeCrc64ForIoBuffer(IoBuffer,
                            IoBuffer->QuerySize(),
                            crc64);

    if (crc64 != verificationData->DataCrc64)
    {
        //
        // CRC doesn't match !
        //
        status = K_STATUS_LOG_STRUCTURE_FAULT;
        return(status);     
    }
    
    return(STATUS_SUCCESS);
}
