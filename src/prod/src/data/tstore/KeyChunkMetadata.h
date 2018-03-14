// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        //
        // Metadata about the key block written at the begining of the block.
        //
        class KeyChunkMetadata
        {

        public:

            KeyChunkMetadata(__in ULONG32 blockSize);

            //
            // int for size field and 4 bytes of reserved space.
            //
            static const ULONG32 Size = sizeof(ULONG32) + 4;

            __declspec(property(get = get_BlockSize)) ULONG32 BlockSize;
            ULONG32 get_BlockSize() const
            {
                return blockSize_;
            }

            static KeyChunkMetadata Read(__in BinaryReader& reader);

            void Write(__in BinaryWriter& writer);

        private:
            ULONG32 blockSize_;

        };
    }
}
