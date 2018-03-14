// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class TestCheckpointFileReadWrite
        {
        public:
            static SerializableMetadata::CSPtr ReadMetadata(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryReader & reader,
                __in KAllocator & allocator)
            {
                return CheckpointFileAsyncEnumerator::ReadMetadata(traceId, reader, allocator);
            }

            static ULONG32 WriteMetadata(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryWriter & writer, 
                __in SerializableMetadata const & metadata,
                __in SerializationMode::Enum serailizationMode)
            {
                return CheckpointFile::WriteMetadata(traceId, writer, metadata, serailizationMode);
            }
        };
    }
}
