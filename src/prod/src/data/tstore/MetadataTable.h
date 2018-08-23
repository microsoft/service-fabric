// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data 
{
    namespace TStore
    {
        // todo: incomplete class, more work required.
        class MetadataTable : 
            public KObject<MetadataTable>,
            public KShared<MetadataTable>
        {
            K_FORCE_SHARED(MetadataTable)

        public:
            typedef KDelegate<ULONG(const ULONG32 & key)> HashFunctionType;

            static bool Test_IsCheckpointLSNExpected;

            static NTSTATUS
                Create(
                    __in KAllocator& allocator,
                    __out MetadataTable::SPtr& result,
                    __in LONG64 checkpointLSN = -1);

            __declspec(property(get = get_CheckpointLSN, put = set_CheckpointLSN)) LONG64 CheckpointLSN;
            LONG64 get_CheckpointLSN() const
            {
                return checkpointLSN_;
            }

            void set_CheckpointLSN(LONG64 value)
            {
                if (Test_IsCheckpointLSNExpected)
                {
                    ASSERT_IFNOT(value >= 0, "CheckpointLSN must be positive.");
                }

                checkpointLSN_ = value;
            }

            __declspec(property(get = get_MetadataFileSize, put = set_MetadataFileSize)) ULONG64 MetadataFileSize;
            ULONG64 get_MetadataFileSize() const
            {
                return metadataFileSize_;
            }

            void set_MetadataFileSize(__in ULONG64 value)
            {
                metadataFileSize_ = value;
            }

            __declspec(property(get = get_Table, put = set_Table)) IDictionary<ULONG32, FileMetadata::SPtr>::SPtr Table;
            IDictionary<ULONG32, FileMetadata::SPtr>::SPtr get_Table()
            {
                return tableSPtr_;
            }

            void set_Table(Dictionary<ULONG32, FileMetadata::SPtr>& value)
            {
               tableSPtr_ = &value;
            }

            ktl::Awaitable<void> CloseAsync();
            void AddReference();
            bool TryAddReference();
            ktl::Awaitable<void> ReleaseReferenceAsync();
            void TestMarkAsClosed();

        private:

           MetadataTable(
              __in ULONG32 size,
              __in LONG64 checkpointLSN,
              __in HashFunctionType func);

            LONG64 checkpointLSN_;
            bool isClosed_;
            LONG64 referenceCount_;
            ULONG64 metadataFileSize_;

            IDictionary<ULONG32, FileMetadata::SPtr>::SPtr tableSPtr_;
        };
    }
}
