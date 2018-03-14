// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        // Member variable Offset_ and Size_ must be bigger or euqal than 0
        // An SST file is made up multiple blocks.BlockHandle is a pointer to a block.
        // It contains two values, offset and size. Offset presents where the block begins and Size is the block size.
        class BlockHandle : 
            public KObject<BlockHandle>, 
            public KShared<BlockHandle>
        {

            K_FORCE_SHARED(BlockHandle)

        public:

            static NTSTATUS Create(
                __in ULONG64 Offset,
                __in ULONG64 Size,
                __in KAllocator & allocator,
                __out BlockHandle::SPtr& result) noexcept;

            static ULONG32 SerializedSize();

            static BlockHandle::SPtr Read(
                __in BinaryReader & binaryReader,
                __in KAllocator & allocator);

            BlockHandle(
                __in ULONG64 Offset,
                __in ULONG64 Size);

            __declspec(property(get = get_Offset)) ULONG64 Offset;
            ULONG64 get_Offset() const { return Offset_; }

            __declspec(property(get = get_Size)) ULONG64 Size;
            ULONG64 get_Size() const { return Size_; }

            ULONG64 EndOffset() const;

            void Write(__in BinaryWriter & binaryWriter);

        private:
            // Gets the offset of this block within the stream.
            ULONG64 const Offset_;

            // Gets the size of this block within the stream.
            ULONG64 const Size_;
        };
    }
}
