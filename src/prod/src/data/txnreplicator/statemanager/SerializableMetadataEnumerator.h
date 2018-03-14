// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class SerializableMetadataEnumerator:
            public IEnumerator<SerializableMetadata::CSPtr>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(SerializableMetadataEnumerator)

        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KBuffer const & readBuffer,
                __in KAllocator & allocator,
                __out SPtr & result);

            SerializableMetadata::CSPtr Current() override;

            bool MoveNext() override;

        private:
            NOFAIL SerializableMetadataEnumerator(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KBuffer const & readBuffer);

        private:
            Utilities::BinaryReader itemReader_;
            const ULONG endPos_;
        };
    }
}
