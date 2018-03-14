// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        //
        // The copy metadata.
        //
        class CopyHeader final
            : public KObject<CopyHeader>
            , public KShared<CopyHeader>
        {
            K_FORCE_SHARED(CopyHeader)

        public:
            static CopyHeader::SPtr Create(
                __in ULONG32 version,
                __in CopyStage::Enum copyStage,
                __in LONG64 primaryReplicaId,
                __in KAllocator & allocator);
            
            static CopyHeader::SPtr Deserialize(
                __in Utilities::OperationData const & operationData,
                __in KAllocator & allocator);
            
            static KBuffer::SPtr Serialize(
                __in CopyHeader const & copyHeader,
                __in KAllocator & allocator);

            __declspec(property(get = get_Version)) ULONG32 Version;
            ULONG32 get_Version() const
            {
                return version_;
            }

            __declspec(property(get = get_CopyStage)) CopyStage::Enum CopyStageValue;
            CopyStage::Enum get_CopyStage() const
            {
                return copyStage_;
            }

            __declspec(property(get = get_PrimaryReplicaId)) LONG64 PrimaryReplicaId;
            LONG64 get_PrimaryReplicaId() const
            {
                return primaryReplicaId_;
            }

        private:
            // Initializes a new instance of the CopyHeader class.
            CopyHeader(
                __in ULONG32 version,
                __in CopyStage::Enum copyStage,
                __in LONG64 primaryReplicaId);

            // The version.
            ULONG32 version_;

            // The copy stage.
            CopyStage::Enum copyStage_;

            // The primary replica id.
            LONG64 primaryReplicaId_;
        };
    }
}
