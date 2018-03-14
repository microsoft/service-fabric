// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

CopyHeader::CopyHeader(
    __in ULONG32 version,
    __in CopyStage::Enum copyStage,
    __in LONG64 primaryReplicaId)
    : version_(version)
    , copyStage_(copyStage)
    , primaryReplicaId_(primaryReplicaId)
{
}

CopyHeader::~CopyHeader()
{
}

CopyHeader::SPtr CopyHeader::Create(
    __in ULONG32 version,
    __in CopyStage::Enum copyStage,
    __in LONG64 primaryReplicaId,
    __in KAllocator & allocator)
{
    CopyHeader* pointer = _new(COPYHEADER_TAG, allocator)CopyHeader(
        version, 
        copyStage, 
        primaryReplicaId);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return CopyHeader::SPtr(pointer);
}

CopyHeader::SPtr CopyHeader::Deserialize(
    __in OperationData const & operationData,
    __in KAllocator & allocator)
{
    KBuffer::CSPtr buffer = operationData[0];
    BinaryReader br(*buffer, allocator);

    ULONG32 copyMetadataVersion;
    LONG32 copyStageLong;
    LONG64 copyPrimaryReplicaId;
    CopyStage::Enum copyStage;

    br.Read(copyMetadataVersion);

    br.Read(copyStageLong);
    copyStage = static_cast<CopyStage::Enum>(copyStageLong);

    br.Read(copyPrimaryReplicaId);

    ASSERT_IFNOT(
        copyMetadataVersion != 1 || buffer->QuerySize() == br.Position,
        "Invalid copy header during deserialization, version:{0}", 
        copyMetadataVersion);

    return Create(copyMetadataVersion, copyStage, copyPrimaryReplicaId, allocator);
}

KBuffer::SPtr CopyHeader::Serialize(
    __in CopyHeader const & copyHeader,
    __in KAllocator & allocator)
{
    BinaryWriter bw(allocator);

    bw.Write(copyHeader.version_);
    bw.Write(copyHeader.copyStage_);
    bw.Write(copyHeader.primaryReplicaId_);

    KBuffer::SPtr buffer;
    NTSTATUS status = KBuffer::Create(bw.Position, buffer, allocator);

    ASSERT_IFNOT(
        status == STATUS_SUCCESS,
        "CopyHeader::Serialize failed to allocate KBuffer with status {0:x}",
        status);

    buffer = bw.GetBuffer(0);

    return buffer;
}
