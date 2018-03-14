// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

CopyMetadata::CopyMetadata(
    __in ULONG32 copyStateMetadataVersion,
    __in ProgressVector & progressVector,
    __in Epoch & startingEpoch,
    __in LONG64 startingLogicalSequenceNumber,
    __in LONG64 checkpointLsn,
    __in LONG64 uptoLsn,
    __in LONG64 highestStateProviderCopiedLsn)
    : copyStateMetadataVersion_(copyStateMetadataVersion)
    , progressVector_(&progressVector)
    , startingEpoch_(startingEpoch)
    , startingLogicalSequenceNumber_(startingLogicalSequenceNumber)
    , checkpointLsn_(checkpointLsn)
    , uptoLsn_(uptoLsn)
    , highestStateProviderCopiedLsn_(highestStateProviderCopiedLsn)
{
}

CopyMetadata::~CopyMetadata()
{
}

CopyMetadata::SPtr CopyMetadata::Create(
    __in ULONG32 copyStateMetadataVersion,
    __in ProgressVector & progressVector,
    __in Epoch & startingEpoch,
    __in LONG64 startingLogicalSequenceNumber,
    __in LONG64 checkpointLsn,
    __in LONG64 uptoLsn,
    __in LONG64 highestStateProviderCopiedLsn,
    __in KAllocator & allocator)
{
    CopyMetadata * pointer = _new(COPYMETADATA_TAG, allocator)CopyMetadata(
        copyStateMetadataVersion, 
        progressVector, 
        startingEpoch, 
        startingLogicalSequenceNumber, 
        checkpointLsn, 
        uptoLsn, 
        highestStateProviderCopiedLsn);

    ASSERT_IFNOT(
        pointer != nullptr,
        "Failed to create CopyMetadata");

    return CopyMetadata::SPtr(pointer);
}

CopyMetadata::SPtr CopyMetadata::Deserialize(
    __in OperationData const & operationData,
    __in KAllocator & allocator)
{
    ProgressVector::SPtr copiedProgressVector = ProgressVector::Create(allocator);

    KBuffer::CSPtr buffer = operationData[0];
    BinaryReader br(*buffer, allocator);

    ULONG32 copyStateMetadataVersion;
    LONG64 startingLogicalSequenceNumber;
    LONG64 checkpointLsn;
    LONG64 uptoLsn;
    LONG64 highestStateProviderCopiedLsn;

    LONG64 epochDataLossVersion;
    LONG64 epochConfigurationVersion;

    br.Read(copyStateMetadataVersion);
    copiedProgressVector->Read(br, false);
    
    br.Read(epochDataLossVersion);
    br.Read(epochConfigurationVersion);
    Epoch startingEpoch(epochDataLossVersion, epochConfigurationVersion);

    br.Read(startingLogicalSequenceNumber);
    br.Read(checkpointLsn);
    br.Read(uptoLsn);
    br.Read(highestStateProviderCopiedLsn);

    ASSERT_IFNOT(
        copyStateMetadataVersion != 1 || buffer->QuerySize() == br.Position,
        "Invalid copy metadata during deserialization, version: {0} buffer size: {1} br.Position: {2}", 
        copyStateMetadataVersion,
        buffer->QuerySize(),
        br.Position);

    return Create(
        copyStateMetadataVersion, 
        *copiedProgressVector, 
        startingEpoch, 
        startingLogicalSequenceNumber, 
        checkpointLsn, 
        uptoLsn, 
        highestStateProviderCopiedLsn, 
        allocator);
}

KBuffer::SPtr CopyMetadata::Serialize(
    __in CopyMetadata const & copyMetadata,
    __in KAllocator & allocator)
{
    BinaryWriter bw(allocator);

    bw.Write(copyMetadata.copyStateMetadataVersion_);
    copyMetadata.progressVector_->Write(bw);
    bw.Write(copyMetadata.startingEpoch_.DataLossVersion);
    bw.Write(copyMetadata.startingEpoch_.ConfigurationVersion);
    bw.Write(copyMetadata.startingLogicalSequenceNumber_);
    bw.Write(copyMetadata.checkpointLsn_);
    bw.Write(copyMetadata.uptoLsn_);
    bw.Write(copyMetadata.highestStateProviderCopiedLsn_);

    KBuffer::SPtr buffer;
    NTSTATUS status = KBuffer::Create(bw.Position, buffer, allocator);

    ASSERT_IFNOT(
        status == STATUS_SUCCESS,
        "CopyMetadata::Serialize failed to allocate KBuffer {0:x}",
        status);

    buffer = bw.GetBuffer(0);

    return buffer;
}
