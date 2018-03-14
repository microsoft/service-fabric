// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        // CheckpointFileBlocks stores the sizes of blocks of state provider metadata records.
        // It used to improve sequential reading.
        // Member RecordBlockSizes will record the size of each metadata, and the length stands for the count of metadata
        class CheckpointFileBlocks : 
            public KObject<CheckpointFileBlocks>,
            public KShared<CheckpointFileBlocks>
        {
            K_FORCE_SHARED(CheckpointFileBlocks)

        public:
            static NTSTATUS Create(
                __in KSharedArray<ULONG32> & recordBlockSizes,
                __in KAllocator & allocator,
                __out SPtr& result) noexcept;

            static SPtr Read(
                __in Utilities::BinaryReader & reader, 
                __in Utilities::BlockHandle const & handle, 
                __in KAllocator & allocator);

        public:
            __declspec(property(get = get_RecordBlockSizes, put = put_RecordBlockSizes)) KSharedArray<ULONG32>::SPtr RecordBlockSizes;
            KSharedArray<ULONG32>::SPtr get_RecordBlockSizes() const;

        public:
            void Write(__in Utilities::BinaryWriter & writer);

        private:
            static KSharedArray<ULONG32>::SPtr ReadArray(
                __in Utilities::BinaryReader & reader,
                __in KAllocator & allocator);

            static void WriteArray(
                __in Utilities::BinaryWriter & writer, 
                __in KArray<ULONG32> const & array);

        private: // Constructor
            CheckpointFileBlocks(__in KSharedArray<ULONG32> & recordBlockSizes) noexcept;

        private: // Member
            const KSharedArray<ULONG32>::SPtr recordBlockSizes_;
        };
    }
}
