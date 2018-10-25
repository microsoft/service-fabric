// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;
using namespace Data::StateManager;
using namespace Data::Utilities;
using namespace StateManagerTests;

SerializableMetadata::CSPtr TestCheckpointFileReadWrite::ReadMetadata(
    __in PartitionedReplicaId const & traceId,
    __in BinaryReader & reader,
    __in KAllocator & allocator)
{
    SerializableMetadata::CSPtr serializableMetadataCSPtr = nullptr;
    NTSTATUS status = CheckpointFileAsyncEnumerator::ReadMetadata(traceId, reader, allocator, serializableMetadataCSPtr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    return serializableMetadataCSPtr;
}

ULONG32 TestCheckpointFileReadWrite::WriteMetadata(
    __in PartitionedReplicaId const & traceId,
    __in BinaryWriter & writer,
    __in SerializableMetadata const & metadata,
    __in SerializationMode::Enum serailizationMode)
{
    return CheckpointFile::WriteMetadata(traceId, writer, metadata, serailizationMode);
}