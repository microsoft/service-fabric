// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#pragma once

namespace Data
{
   namespace TStore
   {
      class PropertyChunkMetadata
      {
      public:
         PropertyChunkMetadata(__in ULONG32 blockSize);
         ~PropertyChunkMetadata();

         static const ULONG32 Size = sizeof(ULONG32) + 4;

         __declspec(property(get = get_BlockSize)) ULONG32 BlockSize;
         ULONG32 get_BlockSize() const
         {
            return blockSize_;
         }

         static PropertyChunkMetadata Read(__in BinaryReader& reader);
         void Write(__in BinaryWriter& writer);

      private:
         ULONG32 blockSize_;
      };
   }
}
